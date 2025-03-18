/*
  WiFiAccessPoint.ino creates a WiFi access point and listens for UDP packets.
  The LED will blink only when data is received.
*/

#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiUdp.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // Set the GPIO pin where you connected your test LED
#endif

// Set these to your desired credentials.
const char *ssid = "better-uber";
const char *password = "expressif-lied-to-me";

WiFiUDP udp;
unsigned int localUdpPort = 8080;  // local port to listen on
char incomingPacket[512];          // buffer for incoming packets

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);  // Start with LED off

  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  // You can remove the password parameter if you want the AP to be open.
  // a valid password must have more than 7 characters
  if (!WiFi.softAP(ssid, password)) {
    Serial.println("Soft AP creation failed.");
    while (1);
  }
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  udp.begin(localUdpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", myIP.toString().c_str(), localUdpPort);
  
  // Blink LED once to indicate successful setup
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    // Flash the LED to indicate data received
    digitalWrite(LED_BUILTIN, HIGH);
    
    // receive incoming UDP packets
    memset(incomingPacket, 0, sizeof(incomingPacket)); // Clear buffer first
    int len = udp.read(incomingPacket, sizeof(incomingPacket) - 1);
    if (len > 0) {
      incomingPacket[len] = 0; // Ensure null termination
      Serial.printf("Received packet of size %d from %s:%d\n", packetSize, udp.remoteIP().toString().c_str(), udp.remotePort());
      Serial.printf("Message: %s\n", incomingPacket);
    }
    
    // Turn LED off after processing packet
    delay(100);  // Keep LED on briefly for visibility
    digitalWrite(LED_BUILTIN, LOW);
  }
}