const net = require("net");
const fs = require("fs");
const uPlot = require("uplot");
const mdnsResolver = require("mdns-resolver");

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
      {
        stroke: "green",
      },
    ],
  },
  [],
  document.getElementById("data-chart")
);

document.getElementById("reload").addEventListener("click", () => {
  location.reload();
});

document
  .getElementById("zero")
  .addEventListener("click", () => client.write("zero\n"));

document
  .getElementById("reset")
  .addEventListener("click", () => client.write("reset\n"));

document.getElementById("clear-data").addEventListener("click", () => {
  scaleData = [];
  client.write("clear\n");
});

document.getElementById("pause").addEventListener("click", () => {
  paused = !paused;
  document.getElementById("pause").innerText = paused ? "Unpause" : "Pause";
});

document
  .getElementById("pause-mcu")
  .addEventListener("click", () => client.write("pause\n"));

//from https://stackoverflow.com/questions/11257062/converting-json-object-to-csv-format-in-javascript
function arrayToCSV(arr) {
  const array = [Object.keys(arr[0])].concat(arr);

  return array
    .map((it) => {
      return Object.values(it).toString();
    })
    .join("\n");
}

document.getElementById("dump").addEventListener("click", () => {
  let csvData = arrayToCSV(scaleData);

  fs.writeFile("frontend-data-dump.csv", csvData, "utf8", function (err) {
    if (err) {
      console.error("CSV Log Error");
      return console.error(err);
    }

    console.log("CSV file has been saved.");
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

let deviceIp = "0.0.0.0";
const client = new net.Socket();

mdnsResolver
  .resolve4("ESPTestStand.local")
  .then((ip) => {
    deviceIp = ip;
    document.getElementById(
      "mdns-status"
    ).innerText = `Device found. IP: ${deviceIp}`;
    client.connect(81, deviceIp, () => {
      console.log(`Connected to ${deviceIp}`);
      document.getElementById(
        "mdns-status"
      ).innerText = `Connected to ${deviceIp}`;
    });
  })
  .catch((err) => {
    document.getElementById("mdns-status").innerText = "Unable to find device";
    console.error(err);
  });

client.on("error", (err) => {
  console.log(`Network Error: ${err}`);
  client.end();
});

client.on("data", (data) => {
  data = data.toString().split("\n").slice(0, -1);
  if (data.length > 1) {
    if (data.length < 20) console.warn("Slightly Bad Data < 20");
    else console.warn("Slightly Bad Data >= 20");
  }
  data.forEach((line) => {
    line_unprocessed = line;
    line = line.split(",");

    let mcuPaused = line[0] === "1";
    let counter = parseInt(line[1]);
    let timestamp = parseInt(line[2]);
    let scaleValueRaw = parseInt(line[3]);
    let scaleValueCalibrated =
      scaleValueRaw / window.localStorage.getItem("scale-calibration");

    let prevData = scaleData.slice(-1)[0];

    if (
      line_unprocessed === "" ||
      line.length !== 4 ||
      (prevData && counter < prevData.counter) ||
      (prevData && timestamp < prevData.timestamp)
    ) {
      console.error("Bad Data");
      console.error(line_unprocessed);
      return; // Discard bad data
    }

    if (prevData && counter - prevData.counter > 1 && !paused)
      console.error(
        `Data Loss Detected: ${counter - prevData.counter} lines lost`
      );

    let lastNValues = scaleData
      .slice(-90)
      .map((data) => data.scaleValueCalibrated);
    let scaleValueCalibratedAvg =
      lastNValues.reduce((a, b) => a + b, 0) /
      (lastNValues.length < 1 ? 1 : lastNValues.length);

    if (!paused) {
      scaleData.push({
        counter: counter,
        timestamp: timestamp,
        scaleValueRaw: scaleValueRaw,
        scaleValueCalibrated: scaleValueCalibrated,
        scaleValueCalibratedAvg: scaleValueCalibratedAvg,
      });
    }

    document.getElementById("timestamp").innerText = timestamp;
    if (prevData)
      document.getElementById("freq").innerText =
        1000 / (timestamp - prevData.timestamp);
    document.getElementById("scale-raw").innerText = scaleValueRaw;
    document.getElementById("scale-cal").innerText =
      Math.round(scaleValueCalibrated * 1000) / 1000;
    document.getElementById("scale-cal-avg").innerText =
      Math.round(scaleValueCalibratedAvg * 1000) / 1000;
    document.getElementById("pause-mcu").innerText = mcuPaused
      ? "Unpause MCU"
      : "Pause MCU";
  });

  let graphLength = parseFloat(document.getElementById("graph-length").value);

  chart.setData([
    scaleData.slice(-graphLength).map((data) => data.timestamp),
    scaleData.slice(-graphLength).map((data) => data.scaleValueCalibrated),
    scaleData.slice(-graphLength).map((data) => data.scaleValueCalibratedAvg),
  ]);
});
