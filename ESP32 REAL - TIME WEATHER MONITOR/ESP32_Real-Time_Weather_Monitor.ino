#include <WiFi.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";

WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  dht.begin();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("/data") != -1) {
      float temperature = dht.readTemperature();
      float humidity = dht.readHumidity();

      String json = "{\"temperature\":" + String(temperature, 1) +
                    ",\"humidity\":" + String(humidity, 1) + "}";

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.println(json);
    }
    else {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html; charset=UTF-8");
      client.println("Connection: close");
      client.println();
      client.println(R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 Live Weather</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    body { font-family: Arial; text-align: center; padding: 20px; }
    .card { background: #f9f9f9; padding: 20px; display: inline-block; border-radius: 10px; box-shadow: 0 0 10px #ccc; }
    canvas { max-width: 95vw; }
    .btn { padding: 10px 20px; margin: 10px; font-size: 16px; cursor: pointer; }
    #timestamp { font-size: 14px; color: #555; margin-top: 10px; }
  </style>
</head>
<body>
  <h2>&#x2600;&#xFE0F; ESP32 Live Weather Station</h2>
  <div class="card">
    <p>&#x1F321; Temperature: <span id="temp">--</span> °C</p>
    <p>&#x1F4A7; Humidity: <span id="hum">--</span> %</p>
    <p id="timestamp">Last updated: --</p>
    <button class="btn" onclick="toggleRefresh()">&#x1F501; Toggle Auto-Refresh</button>
    <button class="btn" onclick="downloadCSV()">&#x1F4E5; Download CSV</button>
  </div>
  <canvas id="chart" height="150"></canvas>

  <script>
    const tempEl = document.getElementById("temp");
    const humEl = document.getElementById("hum");
    const timeEl = document.getElementById("timestamp");

    let autoRefresh = true;
    let csvData = [["Time", "Temperature", "Humidity"]];

    const ctx = document.getElementById("chart").getContext("2d");
    const chart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: [],
        datasets: [
          { label: 'Temperature (°C)', data: [], borderColor: 'red', fill: false },
          { label: 'Humidity (%)', data: [], borderColor: 'blue', fill: false }
        ]
      },
      options: {
        animation: false,
        scales: { x: { display: false }, y: { beginAtZero: true } }
      }
    });

    function fetchData() {
      if (!autoRefresh) return;
      fetch("/data")
        .then(res => res.json())
        .then(data => {
          const now = new Date().toLocaleTimeString();
          chart.data.labels.push(now);
          chart.data.datasets[0].data.push(data.temperature);
          chart.data.datasets[1].data.push(data.humidity);

          if (chart.data.labels.length > 30) {
            chart.data.labels.shift();
            chart.data.datasets[0].data.shift();
            chart.data.datasets[1].data.shift();
          }

          chart.update();
          tempEl.textContent = data.temperature;
          humEl.textContent = data.humidity;
          timeEl.textContent = "Last updated: " + now;

          csvData.push([now, data.temperature, data.humidity]);
        });
    }

    setInterval(fetchData, 2000);

    function toggleRefresh() {
      autoRefresh = !autoRefresh;
      alert("Auto-refresh is now " + (autoRefresh ? "ON" : "OFF"));
    }

    function downloadCSV() {
      let csvContent = "data:text/csv;charset=utf-8," +
        csvData.map(e => e.join(",")).join("\n");
      const encodedUri = encodeURI(csvContent);
      const link = document.createElement("a");
      link.setAttribute("href", encodedUri);
      link.setAttribute("download", "weather_data.csv");
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
    }
  </script>
</body>
</html>
      )rawliteral");
    }
  }
}
