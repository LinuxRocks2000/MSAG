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
                ret.push(view.getUint8(i));
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
    }
};


class ProtocolConnection {
    constructor(url) {
        this.socket = new WebSocket(url);
        this.manifest = {};
        this.opened = false;
        this.onopen = () => { };
        this.socket.addEventListener("open", () => {
            this.onopen();
        });/*
        this.socket.addEventListener("message", (evt) => {
            evt.data.arrayBuffer().then(d => {
                var view = new Uint8Array(d);
                var c = new Cursor(view);
                var opcode = c.popByte();
                var params = []; // todo: preallocate somehow
                while (!c.empty()) {
                    var type = String.fromCharCode(c.popByte());
                    if (type == 's') {
                        var len = c.popByte();
                        if (len == 255) {
                            len = 0;
                            for (var x = 0; x < 4; x++) {
                                len *= 256;
                                len += c.popByte();
                            }
                        }
                        var str = "";
                        for (var i = 0; i < len; i++) {
                            str += String.fromCharCode(c.popByte());
                        }
                        params.push(str);
                    }
                    else if (type == 'U') {
                        var r = 0;
                        for (var i = 0; i < 8; i++) {
                            r += c.popByte() * (256 ** i);
                        }
                        params.push(r);
                    }
                    else if (type == 'I') {
                        var data = [];
                        for (var i = 0; i < 8; i++) { // TODO: make this less inefficient
                            data.push(c.popByte());
                        }
                        var r = data[7];
                        if (r > 127) {
                            r -= 256;
                        }
                        for (var i = 6; i >= 0; i--) {
                            r *= 256;
                            r += data[i];
                        }
                        params.push(r);
                    }
                    else if (type == 'F') {
                        var buf = new ArrayBuffer(8);
                        var view = new DataView(buf);
                        for (var i = 0; i < 8; i++) {
                            view.setUint8(8 - i - 1, c.popByte());
                        }
                        console.log(view.getFloat64(0));
                    }
                }
                console.log(params);
            });
        });*/
    }

    async loadManifest(manifestURL) {
        this.manifest = await (await fetch(manifestURL)).json();
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
                    console.log(frame);
                    this.socket.send(new Uint8Array(frame));
                }
            }
        }
    }
}

var p = new ProtocolConnection('ws://localhost:3001/game');
p.loadManifest('manifest.json').then(() => {
    console.log("manifest up: time to do other stuff!");
    var initSender = p.sendHandle("incoming", "Init"); // TODO: fix the protocol names (change from outgoing and incoming to s2c and c2s)
    initSender(45);
});