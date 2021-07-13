var net = require("net");

var client = new net.Socket();
client.connect(80, "192.168.1.16", () => {
  console.log("Connected");
});

client.on("data", (data) => {
  console.log("Received: " + data);
});
