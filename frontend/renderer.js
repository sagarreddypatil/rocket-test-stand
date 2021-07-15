const net = require("net");
const fs = require("fs");
const uPlot = require("uplot");

let paused = false;

let scaleData = [];
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

document
  .getElementById("zero")
  .addEventListener("click", () => client.write("zero"));

document
  .getElementById("reset")
  .addEventListener("click", () => client.write("reset"));

document
  .getElementById("clear-data")
  .addEventListener("click", () => (scaleData = []));

document.getElementById("pause").addEventListener("click", () => {
  paused = !paused;
  document.getElementById("pause").innerText = paused ? "Unpause" : "Pause";
});

document.getElementById("dump").addEventListener("click", () => {
  let jsonData = JSON.stringify(scaleData);

  fs.writeFile("data-dump.json", jsonData, "utf8", function (err) {
    if (err) {
      console.err("JSON Log Error");
      return console.err(err);
    }

    console.log("JSON file has been saved.");
  });
});

document.getElementById("calibrate").addEventListener("click", () => {
  document.getElementById("calibration-info").innerText = "Calibrating...";
  let startIdx = scaleData.length - 1;
  setTimeout(() => {
    let sliced = scaleData.slice(startIdx);
    let avg =
      sliced.map((n) => n.scaleValueRaw).reduce((a, b) => a + b) /
      sliced.length;

    let calScale = parseFloat(document.getElementById("cal-scale").value);

    window.localStorage.setItem("scale-calibration", avg / calScale);
    document.getElementById("calibration-info").innerText =
      "Calibration Finished";
  }, 1000);
});

document.getElementById("esp-ip").addEventListener("change", () => {
  let newIp = document.getElementById("esp-ip").value;

  client.end();
  client.connect(80, document.getElementById("esp-ip").value, () => {
    console.log(`Connected to ${newIp}`);
  });
});

const client = new net.Socket();
client.connect(80, document.getElementById("esp-ip").value, () => {
  console.log("Connected");
});

client.on("data", (data) => {
  data = data.toString().split("\n").slice(0, -1);
  if (data.length > 1) console.warn("Slightly Bad Data");
  data.forEach((line) => {
    let badData = line === "";
    line = line.split(",");

    let counter = parseInt(line[0]);
    let timestamp = parseInt(line[1]);
    let scaleValueRaw = parseInt(line[2]);
    let scaleValueCalibrated =
      scaleValueRaw / window.localStorage.getItem("scale-calibration");

    let prevData = scaleData.slice(-1)[0];

    if (
      badData ||
      line.length !== 3 ||
      (prevData && counter < prevData.counter) ||
      (prevData && timestamp < prevData.timestamp)
    ) {
      console.error("Bad Data");
      return; // Discard bad data
    }

    if (prevData && counter - prevData.counter > 1 && !paused)
      console.warn(
        `Data Loss Detected - Counter Difference: ${counter - prevData.counter}`
      );

    if (!paused)
      scaleData.push({
        counter: counter,
        timestamp: timestamp,
        scaleValueRaw: scaleValueRaw,
        scaleValueCalibrated: scaleValueCalibrated,
      });

    document.getElementById("timestamp").innerText = timestamp;
    document.getElementById("scale-raw").innerText = scaleValueRaw;
    document.getElementById("scale-cal").innerText =
      Math.round(scaleValueCalibrated * 1000) / 1000;
  });

  client.on("error", (err) => {
    console.log(`Network Error: ${err}`);
    client.end();
  });

  let graphLength = parseFloat(document.getElementById("graph-length").value);

  chart.setData([
    scaleData.slice(-graphLength).map((data) => data.timestamp),
    scaleData.slice(-graphLength).map((data) => data.scaleValueCalibrated),
  ]);
});
