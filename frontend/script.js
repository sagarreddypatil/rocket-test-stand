let scaleReading = 0;
let forceReading = 0;
let timestamp = 0;
let chart = new uPlot(
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

window.data = [];

let gateway = `ws://192.168.1.6/ws`;
let websocket;
window.addEventListener("load", () => {
  document
    .getElementById("zero")
    .addEventListener("click", () => websocket.send("zero"));
  document
    .getElementById("calibrate")
    .addEventListener("click", () =>
      window.localStorage.setItem(
        "cal-divisor",
        scaleReading / parseFloat(document.getElementById("cal-scale").value)
      )
    );
  document
    .getElementById("clear")
    .addEventListener("click", () => (window.data = []));
  document.getElementById("dump").addEventListener("click", () => {
    window.localStorage.setItem("data-dump", JSON.stringify(window.data));
    window.data = [];
  });
  document.getElementById("dl-dump").addEventListener("click", () => {
    dump = JSON.parse(window.localStorage.getItem("data-dump"));
    CSV = "timestamp,Raw Scale Value,Force Value\n";
    dump.forEach(
      (data) =>
        (CSV += `${data.timestamp},${data.scaleReading},${data.forceReading}\n`)
    );
    download("data.csv", CSV);
  });

  initWebSocket();
});

function initWebSocket() {
  console.log("Trying to open a WebSocket connection...");
  websocket = new WebSocket(gateway);
  websocket.onopen = () => console.log("Connection opened");
  websocket.onclose = () => alert("Connection closed");
  websocket.onmessage = onMessage;
}

function onMessage(event) {
  [timestamp, scaleReading] = event.data.split(",").map((a) => parseInt(a));
  forceReading = scaleReading / window.localStorage.getItem("cal-divisor");

  window.data.push({
    timestamp: timestamp,
    scaleReading: scaleReading,
    forceReading: forceReading,
  });

  let graphPrev = document.getElementById("graph-prev").value;

  chart.setData([
    window.data.slice(-graphPrev).map((data) => data.timestamp),
    window.data.slice(-graphPrev).map((data) => data.forceReading),
  ]);

  document.getElementById("scale").innerText = scaleReading;
  document.getElementById("scale-cal").innerText = forceReading.toFixed(2);
  document.getElementById("timestamp").innerText = timestamp;
  document.getElementById("freq").innerText =
    1000 / (timestamp - window.data.slice(-2)[0].timestamp);
}

function download(filename, text) {
  var element = document.createElement("a");
  element.setAttribute(
    "href",
    "data:text/plain;charset=utf-8," + encodeURIComponent(text)
  );
  element.setAttribute("download", filename);

  element.style.display = "none";
  document.body.appendChild(element);

  element.click();

  document.body.removeChild(element);
}
