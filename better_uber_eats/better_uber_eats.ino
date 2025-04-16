/*
  WiFiAccessPoint.ino creates a WiFi access point, enables bidirectional UDP communication,
  and calculates/displays data transfer rates in kB per second in both directions.
*/

#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiUdp.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // Set the GPIO pin where you connected your test LED
#endif


#define RX_PIN 44
#define TX_PIN 43
#define BAUD 115200


// Set these to your desired credentials.
const char *ssid = "better-uber";
const char *password = "expressif-lied-to-me";

// Constants for message parsing
const int MAX_TAG_SIZE = 96;
const int MAX_MESSAGE_SIZE = 4000;
const char NEWLINE = '\n';


WiFiUDP udp;
unsigned int localUdpPort = 8080;  // local port to listen on
char incomingPacket[4096];         // buffer for incoming packets
uint8_t outgoingPacket[1400];      // buffer for outgoing packets

// Remote device address (will be set on first packet received)
IPAddress remoteIP;
uint16_t remotePort = 0;

// // Variables for receive data rate calculation
// unsigned long totalBytesReceived = 0;
// unsigned long lastReceiveReportTime = 0;
// unsigned long reportInterval = 1000;  // Report data rate every 1 second
// float maxReceiveDataRate = 0;

// Variables for send data rate calculation
// unsigned long totalBytesSent = 0;
// unsigned long lastSendReportTime = 0;
// float maxSendDataRate = 0;
bool isSending = false;

// Function to handle different types of messages based on tags
void handleMessage(const String& tag, const String& message) {
    if (tag == "CV") {
        // Handle computer vision messages
        // Example: Parse x,y coordinates
        float x, y;
        int width, height;
        sscanf(message.c_str(), "%f,%f,%d,%d", &y, &x, &height, &width);
        Serial.printf("Received CV coordinates: x=%f, y=%f\n - Max Coordinates: width=%d, height=%d\n", x, y, width, height);
        Serial1.printf("<%d,%d>", round(x), width);
        // Add your CV handling code here
    }
    // Add more tag handlers here
    else {
        Serial.printf("Unknown tag received: %s\n", tag.c_str());
        Serial.printf("Unknown tag received - message: %s\n", message.c_str());
    }
}


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

  // // Initialize timing for data rate calculation
  // lastReceiveReportTime = millis();
  // lastSendReportTime = millis();
  
  // // Fill outgoing packet with arbitrary data
  // for (int i = 0; i < sizeof(outgoingPacket); i++) {
  //   outgoingPacket[i] = i % 256;
  // }

  Serial1.begin(BAUD,SERIAL_8N1,RX_PIN,TX_PIN); //this for tm4c

}

void processIncomingPacket(int packetSize) {
    if (packetSize > sizeof(incomingPacket)) {
        Serial.printf("Packet too large: %d bytes\n", packetSize);
        // Clear the packet
        while (udp.available()) {
            udp.read();
        }
        return;
    }

    // Read packet into buffer
    int length = udp.read(incomingPacket, sizeof(incomingPacket));
    if (length > 0) {
        // Null terminate the buffer
        incomingPacket[length] = 0;
        
        // Find first newline (end of tag)
        char* tagEnd = strchr(incomingPacket, NEWLINE);
        if (!tagEnd) {
            Serial.println("No tag found in packet");
            return;
        }
        
        // Calculate tag length
        int tagLength = tagEnd - incomingPacket;
        if (tagLength > MAX_TAG_SIZE) {
            Serial.println("Tag too long");
            return;
        }
        
        // Extract tag
        String tag = String(incomingPacket).substring(0, tagLength);
        
        // Find message start (after tag newline)
        char* messageStart = tagEnd + 1;
        // Find message end
        char* messageEnd = strchr(messageStart, NEWLINE);
        if (!messageEnd) {
            Serial.println("No message terminator found");
            return;
        }
        
        // Calculate message length
        int messageLength = messageEnd - messageStart;
        if (messageLength > MAX_MESSAGE_SIZE) {
            Serial.println("Message too long");
            return;
        }
        
        // Extract message
        String message = String(messageStart).substring(0, messageLength);
        
        // Handle the message based on its tag
        handleMessage(tag, message);
    }
}

void loop() {
  //unsigned long currentTime = millis();
  
  // Check for incoming packets
  int packetSize = udp.parsePacket();
  if (packetSize) {
    // Turn on LED during data reception
    digitalWrite(LED_BUILTIN, HIGH);
    
    // Capture the sender's IP and port if we don't have it yet
    if (remotePort == 0) {
      remoteIP = udp.remoteIP();
      remotePort = udp.remotePort();
      Serial.printf("Client connected from IP: %s, Port: %u\n", remoteIP.toString().c_str(), remotePort);
      isSending = true;  // Start sending data once we know where to send
    }


    // Always log the sender's information for debugging
    IPAddress currentIP = udp.remoteIP();
    uint16_t currentPort = udp.remotePort();
    Serial.printf("Received packet from IP: %s, Port: %u, Size: %d\n", 
                currentIP.toString().c_str(), currentPort, packetSize);
    
    // Update connection info even if it changes
    remoteIP = currentIP;
    remotePort = currentPort;
    isSending = true;
    // Process the incoming packet
    processIncomingPacket(packetSize);
    
    // // Receive incoming UDP packets
    // int len = udp.read(incomingPacket, sizeof(incomingPacket) - 1);
    // if (len > 0) {
    //   // Count the bytes received
    //   totalBytesReceived += len;
    // }
    
    // Turn LED off after processing packet
    digitalWrite(LED_BUILTIN, LOW);
  }
  
  //not testing, no need to send data
  /*
  // Send data if we have a client
  if (isSending && remotePort != 0) {
    // Send a packet
    udp.beginPacket(remoteIP, remotePort);
    udp.write(outgoingPacket, sizeof(outgoingPacket));
    udp.endPacket();
    
    // Update send statistics
    totalBytesSent += sizeof(outgoingPacket);
  }

  // Calculate and report receive data transfer rate every second
  if (currentTime - lastReceiveReportTime >= reportInterval) {
    float elapsedSeconds = (currentTime - lastReceiveReportTime) / 1000.0;
    float receiveDataRate = totalBytesReceived / elapsedSeconds / 1024.0;  // Convert to kB/s
    
    if (receiveDataRate > maxReceiveDataRate) {
      maxReceiveDataRate = receiveDataRate;
    }
    
    Serial.printf("RECEIVE - Data received in last %.1f seconds: %lu bytes\n", elapsedSeconds, totalBytesReceived);
    Serial.printf("RECEIVE - Current data rate: %.2f kB/s\n", receiveDataRate);
    Serial.printf("RECEIVE - Max data rate: %.2f kB/s\n", maxReceiveDataRate);
    
    // Reset counters for next interval
    totalBytesReceived = 0;
    lastReceiveReportTime = currentTime;
  }

  // Calculate and report send data transfer rate every second
  if (isSending && (currentTime - lastSendReportTime >= reportInterval)) {
    float elapsedSeconds = (currentTime - lastSendReportTime) / 1000.0;
    float sendDataRate = totalBytesSent / elapsedSeconds / 1024.0;  // Convert to kB/s
    
    if (sendDataRate > maxSendDataRate) {
      maxSendDataRate = sendDataRate;
    }
    
    Serial.printf("SEND - Data sent in last %.1f seconds: %lu bytes\n", elapsedSeconds, totalBytesSent);
    Serial.printf("SEND - Current data rate: %.2f kB/s\n", sendDataRate);
    Serial.printf("SEND - Max data rate: %.2f kB/s\n", maxSendDataRate);
    Serial.println();
    
    // Reset counters for next interval
    totalBytesSent = 0;
    lastSendReportTime = currentTime;
    
  }
  */
}