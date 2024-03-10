// main code for MSAG's official client
// depends on MSAG's protocol.js implementation - MMOSG's protocol has a different manifest format and calling conventions.
// the current state of this is "dev/mess" - I didn't want to write game boilerplate yet, so this really just exists to get the gameserver working.
// all of it will be replaced by an improved program once the gameserver lives.


var spaceW = 0;
var spaceH = 0;

var cx = 0; // viewport position, relative to screen center
var cy = 0; // these will eventually be controlled by the Player.

var playerObjId = -1;


var typeInfo = undefined;

fetch("types.json").then(data => {
    data.json().then(json => {
        typeInfo = json;
        console.log(typeInfo);
    });
});


var objects = {}; // "master" list with ID-based indexing
var ground = []; // smaller "specialized" lists with standard array indexing, containing references to the objects.
var players = [];


var keys = {};

window.addEventListener("keydown", (evt) => {
    keys[evt.key] = true;
});

window.addEventListener("keyup", (evt) => {
    keys[evt.key] = false;
});


function rand32() { // Cryptographically INSECURE random uint32. 
    // generates two random 16-bit ints and shifts one up to the MSB
    // since integers are always signed in JavaScript
    var msb = Math.floor(Math.random() * 65536);
    var lsb = Math.floor(Math.random() * 65536);
    return msb << 16 + lsb;
}


var canvas = document.getElementById("main");
var ctx = canvas.getContext("2d");


function tileDraw(image, tileW, tileH, x, y, width, height) {
    // TODO: efficiency (don't draw unnecessary tiles)
    for (var x = 0; x < width / tileW; x++) {
        for (var y = 0; y < height / tileH; y++) {
            ctx.drawImage(image, x * tileW, y * tileH);
        }
    }
}


var inputUpdate = undefined;


function gameloop() {
    ctx.fillStyle = "black";
    ctx.fillRect(0, 0, window.innerWidth, window.innerHeight);
    ctx.translate(window.innerWidth / 2 - cx, window.innerHeight / 2 - cy);
    ctx.fillStyle = "lightgrey";
    ctx.fillRect(0, 0, spaceW, spaceH);
    ground.forEach(groundPiece => {
        if (groundPiece.type == typeInfo.GROUND.SOIL) {
            tileDraw(document.getElementById("soilTile"), 100, 100, groundPiece.x, groundPiece.y, groundPiece.width, groundPiece.height);
        }
    });
    players.forEach(player => {
        ctx.fillStyle = "green";
        ctx.fillRect(player.x, player.y, 50, 50);
        if (player.id == playerObjId) {
            ctx.fillStyle = "blue";
            ctx.fillRect(player.x + 12.5, player.y + 12.5, 25, 25);
            cx = player.x;
            cy = player.y;
        }
    }); // TODO: 1000D made some nice art for players, need to swap that in! (hint hint, dear 1000D)
    inputUpdate(keys["ArrowUp"], keys["ArrowDown"], keys["ArrowLeft"], keys["ArrowRight"]);
    ctx.translate(-window.innerWidth / 2 + cx, -window.innerHeight / 2 + cy);
    requestAnimationFrame(gameloop);
}


function onResize() {
    canvas.width = window.innerWidth;
    canvas.height = window.innerHeight;
}

onResize()
window.addEventListener("resize", onResize);


var p = new ProtocolConnection('ws://localhost:3001/game', () => {
    console.log("socket connected");
    p.loadManifest('manifest.json').then(() => {
        console.log("manifest up: now we're going to send our init message and set up event listeners!");
        var initSender = p.sendHandle("incoming", "Init"); // TODO: fix the protocol names (change from outgoing and incoming to s2c and c2s)
        var createRoom = p.sendHandle("incoming", "RoomCreate");
        var joinRoom = p.sendHandle("incoming", "RoomJoin");
        var connect = p.sendHandle("incoming", "RoomConnect");
        inputUpdate = p.sendHandle("incoming", "InputUpdate");
        initSender();
        p.onMessage("outgoing", "Welcome", () => {
            console.log("The server has kindly welcomed us!");
        });
        p.onMessage("outgoing", "SpaceSet", params => {
            spaceH = params.spaceHeight;
            spaceW = params.spaceWidth;
        });
        p.onMessage("outgoing", "RoomCreated", (params) => {
            if (!localStorage.ownedRooms) {
                localStorage.ownedRooms = "{}";
            }
            var ownedRooms = JSON.parse(localStorage.ownedRooms);
            ownedRooms[params.roomid] = params.creator;
            localStorage.ownedRooms = JSON.stringify(ownedRooms); // TODO: make this nicer with a Proxy or something (or a set of helper functions)
            alert("Room successfully created! The room code is " + params.roomid + ".");
        }); // the RoomCreated command is only sent to the actual creator, so we know the creator id is us
        p.onMessage("outgoing", "GroundSet", params => {
            // SetGround MAY NEVER BE CALLED MORE THAN ONCE PER GROUND PIECE IN ONE SESSION!
            objects[params.id] = params;
            ground.push(objects[params.id]);
        });
        p.onMessage("outgoing", "PlayerSet", params => {
            objects[params.id] = params;
            players.push(objects[params.id]);
        });
        p.onMessage("outgoing", "IdSet", (params) => {
            playerObjId = params.objectID;
        });
        p.onMessage("outgoing", "Move", (params) => {
            objects[params.id].x = params.x;
            objects[params.id].y = params.y;
        });
        document.getElementById("create-room").onclick = () => {
            var roomOwnerKey = rand32();
            createRoom(document.getElementById("newroomname").value, document.getElementById("newroommapname").value, roomOwnerKey);
        };
        document.getElementById("join-room").onclick = () => {
            var pID = rand32();
            alert("player id: " + pID);
            console.log("player id: " + pID);
            joinRoom(document.getElementById("handle"), pID, document.getElementById("joinid"));
        };
        document.getElementById("connect").onclick = () => {
            connect(document.getElementById("connectID").value);
            document.getElementById("main").style.display = "";
            gameloop();
        };
    });
});