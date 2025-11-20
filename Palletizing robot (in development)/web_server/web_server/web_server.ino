#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <SPI.h>

#define SD_CS 5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18

const char* ssid = "ESP32_Web";
const char* password = "12345678";

WebServer server(80);

String getContentType(const String& path) {
  if (path.endsWith(".html")) return "text/html";
  if (path.endsWith(".css")) return "text/css";
  if (path.endsWith(".js")) return "application/javascript";
  return "text/plain";
}

void handleFileRequest() {
  String path = server.uri();
  if (path == "/") path = "/index.html";
  String contentType = getContentType(path);

  if (!SD.exists(path)) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  File file = SD.open(path, FILE_READ);
  server.streamFile(file, contentType);
  file.close();
}

void handleButton() {
  Serial.println("Кнопка натиснута — повідомлення від браузера!");
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  if (!SD.begin(SD_CS)) {
    Serial.println("SD init failed!");
    while (true);
  }
  Serial.println("SD ready.");

  WiFi.softAP(ssid, password);
  Serial.print("WiFi AP IP: ");
  Serial.println(WiFi.softAPIP());

  server.onNotFound(handleFileRequest);
  server.on("/button", HTTP_POST, handleButton);

  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  server.handleClient();
}
