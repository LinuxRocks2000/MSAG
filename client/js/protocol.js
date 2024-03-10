// protocol code for MSAG
// by Tyler Clarke

class Cursor {
    constructor(data) { // any indexable data
        this.data = data;
        this.index = 0;
    }

    popByte() {
        this.index++;
        return this.data[this.index - 1];
    }

    empty() {
        return this.index >= this.data.length;
    }
}


const lexicon = {
    "uint32_t": {
        encode(data) {
            var buf = new ArrayBuffer(4);
            var view = new DataView(buf);
            view.setUint32(0, data);
            var ret = [];
            for (var i = 0; i < 4; i++) {
                ret.push(view.getUint8(3 - i));
            }
            return ret;
        },
        decode(data) {
            var ret = 0;
            for (var i = 0; i < 4; i++) {
                ret += data.popByte() * (256 ** i);
            }
            return ret;
        }
    },
    "bool": {
        encode(data) {
            return [data ? 1 : 0];
        },
        decode(data) {
            return data.popByte() == 0 ? false : true;
        }
    },
    "float32_t": {
        encode(data) {
            var buf = new ArrayBuffer(4);
            var view = new DataView(buf);
            view.setFloat32(0, data);
            var ret = [];
            for (var i = 0; i < 4; i++) {
                ret.push(view.getUint8(3 - i));
            }
            return ret;
        },
        decode(data) {
            var buf = new ArrayBuffer(4);
            var view = new DataView(buf);
            for (var i = 0; i < 4; i++) {
                view.setUint8(3 - i, data.popByte());
            }
            return view.getFloat32(0);
        }
    },
    "string": {
        encode(string) {
            var data = new TextEncoder("utf-8").encode(string);
            var ret = [];
            if (data.length >= 255) {
                ret.push(255);
                for (var i = 0; i < 4; i++) { // TODO: make sure this actually encodes the right size
                    ret.push(Math.round(data.length / (256 ** i)) % 256);
                }
            }
            else {
                ret.push(data.length);
            }
            for (var i = 0; i < data.length; i++) {
                ret.push(data[i]);
            }
            return ret;
        },
        decode(data) {
            var len = data.popByte();
            if (len == 255) {
                len = 0;
                for (var i = 0; i < 4; i++) {
                    len += data.popByte() * (256 ** i);
                }
            }
            var ret = new Uint8Array(len);
            for (var i = 0; i < len; i++) {
                ret[i] = data.popByte();
            }
            let decoder = new TextDecoder("utf-8");
            return decoder.decode(ret);
        }
    }
};


class ProtocolConnection {
    constructor(url, onopen) {
        this.socket = new WebSocket(url);
        this.manifest = {};
        this.listeningProtocols = [];
        this.opened = false;
        this.socket.addEventListener("open", () => {
            onopen();
        });
        this.socket.addEventListener("message", (evt) => {
            evt.data.arrayBuffer().then(d => {
                var view = new Uint8Array(d);
                var c = new Cursor(view);
                var opcode = c.popByte();
                this.listeningProtocols.forEach(protocol => {
                    this.manifest[protocol].formats.forEach(format => {
                        if (format.opcode == opcode) {
                            if (format.handlers) {
                                // NOTE: since there's no cursor rewind, this won't work if handlers are defined across protocols. This might cause problems later.
                                // TODO: cursor rewinds and/or sane(r) protocol handling.
                                var params = {}; // this is a JS object, it mirrors the argument definitions in the manifest
                                format.arguments.forEach(arg => {
                                    params[arg.name] = lexicon[arg.type].decode(c);
                                });
                                format.handlers.forEach(handler => {
                                    handler(params);
                                });
                            }
                        }
                    });
                });
            });
        });
    }

    async loadManifest(manifestURL) {
        this.manifest = await (await fetch(manifestURL, {cache: "no-store"})).json();
    }

    sendHandle(protocol, name) {
        for (var i = 0; i < this.manifest[protocol].formats.length; i++) {
            var f = this.manifest[protocol].formats[i];
            if (f.className == name) {
                return (...frameArgs) => {
                    var frame = [f.opcode];
                    f.arguments.forEach((arg, i) => {
                        frame.push(...lexicon[arg.type].encode(frameArgs[i]));
                    });
                    this.socket.send(new Uint8Array(frame));
                }
            }
        }
    }

    onMessage(protocol, name, handler) {
        for (var i = 0; i < this.manifest[protocol].formats.length; i++) {
            if (this.manifest[protocol].formats[i].className == name) {
                if (!this.manifest[protocol].formats[i].handlers) {
                    this.manifest[protocol].formats[i].handlers = [];
                }
                this.manifest[protocol].formats[i].handlers.push(handler);
            }
        }
        if (this.listeningProtocols.indexOf(protocol) == -1) {
            this.listeningProtocols.push(protocol);
        }
    }
}