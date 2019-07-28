//DEEPSLEEP ZAVEDENY, PREPOJTE GPIO16 (D0) s RST PINOM
// V OPACNOM PRIPADE NEBUDE SKETCH POUZITELNY, ESP SA NEPREBUDI WAKE VOVYODOM
#include <ESP8266WiFi.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define ONE_WIRE_BUS 0 //D3 DS18b20 pripojit na tuto zbernicu
#define BME280_ADRESA (0x76)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Adafruit_BME280 bme;
const char* ssid = "meno_wifi";
const char* password = "heslo_wifi";
const char* host = "www.arduino.php5.sk";
const int httpPort = 80;
WiFiClient client;
void setup() {
  sensors.begin(); //aktivujem senzory na OneWire zbernici
  if (!bme.begin(BME280_ADRESA)) {
    Serial.println("BME280 senzor nenalezen, zkontrolujte zapojeni!");
    while (1);
  }
  Serial.begin(115200); //rychlost seriovej linky
  Serial.println();
  Serial.println("pripajam na ");
  Serial.println(ssid);
  WiFi.begin(ssid, password); //pripoj sa na wifi siet s heslom
  while (WiFi.status() != WL_CONNECTED) { //pokial sa nepripojime na wifi opakuj pripajanie a spustaj funkcie pre ovladanie v offline rezime
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Wifi pripojene s IP:");
  Serial.println(WiFi.localIP());
  odosli_teploty();
  skontroluj_reset();
  ESP.deepSleep(30e7);
}


void odosli_teploty() {
  sensors.requestTemperatures();
  delay(500);
  client.stop();
  if (client.connect(host, httpPort)) {
    String teplota1 = String(sensors.getTempCByIndex(0));
    String teplota2 = String(sensors.getTempCByIndex(1));
    float t3 = bme.readTemperature();
    String vlhkost = String(bme.readHumidity());
    float tlak = bme.readPressure() / 100.0F;
    float nadmorska_vyska = bme.readAltitude(1013.25);
    String teplota3 = String(t3);
    float tlak_hladina_mora = tlak / pow(1 - ((0.0065 * nadmorska_vyska) / (t3 + (0.0065 * nadmorska_vyska) + 273.15)), 5.257);
    String url = "/meteostanicav2/system/nodemcu/add.php?teplota1=" + teplota1 + "&teplota2=" + teplota2 + "&teplota3=" + teplota3 + "&tlak=" + tlak_hladina_mora + "&vlhkost=" + vlhkost;
    client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "User-Agent: NodeMCU\r\n" + "Connection: close\r\n\r\n");
    Serial.println("Hodnoty do databazy uspesne odoslane");
  } else if (!client.connect(host, httpPort)) {
    Serial.println("Nepodarilo sa odoslat hodnoty - chyba siete");
  }
}

void skontroluj_reset() {
  client.stop();
  if (client.connect(host, httpPort)) {
    String url = "/meteostanicav2/system/resetdosky.txt";
    client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "User-Agent: NodeMCU\r\n" + "Connection: close\r\n\r\n");
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
    }
    String line = client.readStringUntil('\n');
    if (line == "Cakam na potvrdenie restartu") {
      client.stop();
      if (client.connect(host, httpPort)) {
        String link = "/meteostanicav2/system/nodemcu/potvrdreset.php";
        client.print(String("GET ") + link + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "User-Agent: NodeMCU\r\n" + "Connection: close\r\n\r\n");
        Serial.println("Reset potvrdeny - vykonany");
        delay(500);
        ESP.restart();
      } else if (!client.connect(host, httpPort)) {
        Serial.println("Neuspesne odoslanie informacie o resete");
      }
    }
  } else if (!client.connect(host, httpPort)) {
    Serial.println("Neuspesne pripojenie pre stav resetu");
  }
}
void loop() {

}
