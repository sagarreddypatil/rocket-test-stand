const net = require("net");
const uPlot = require("uplot");

const scaleData = [];
const chart = new uPlot(
  {
    title: "Force vs Time(ms)",
    id: "chart",
    scales: {
      x: {
        time: false,
      },
    },
    width: 800,
    height: 250,
    series: [
      {},
      {
        stroke: "red",
      },
    ],
  },
  [],
  document.getElementById("data-chart")
);

const client = new net.Socket();
client.connect(80, "192.168.1.16", () => {
  console.log("Connected");
});

client.on("data", (data) => {
  data = data.toString().split("\n").slice(0, -1);
  data.forEach((line) => {
    let badData = line === "";
    line = line.split(",");

    let timestamp = parseInt(line[0]);
    let scaleValueRaw = parseInt(line[1]);

    if (
      badData ||
      line.length !== 2 ||
      (scaleData.length > 0 && timestamp < scaleData.slice(-1)[0].timestamp)
    ) {
      console.error("Bad Data");
      return; // Discard bad data
    }
    scaleData.push({
      timestamp: timestamp,
      scaleValueRaw: scaleValueRaw,
    });

    document.getElementById("timestamp").innerText = timestamp;
    document.getElementById("scale-raw").innerText = scaleValueRaw;
  });

  chart.setData([
    scaleData.slice(-500).map((data) => data.timestamp),
    scaleData.slice(-500).map((data) => data.scaleValueRaw),
  ]);
});
