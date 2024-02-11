let ws = new WebSocket("ws://localhost:3001/game");
ws.addEventListener("open", () => {
    console.log("OPENED");
    ws.send("HI");
});
ws.addEventListener("close", () => {
    console.log("CLOSED");
});
ws.addEventListener("message", (evt) => {
    console.log(evt);
});
ws.addEventListener("error", (evt) => {
    console.log("ERROR");
});