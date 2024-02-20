// Include necessary libraries
#include <Ethernet.h> // Ethernet library for connecting to the network
#include "ThingSpeak.h" // ThingSpeak library for IoT projects
#include "DHT.h" // DHT library for temperature and humidity sensor

// Define sensor type and pin
#define DHTTYPE DHT11 // DHT11 type for temperature and humidity sensor
#define DHTPIN 5 // Pin number where the DHT11 sensor is connected

// Define relay pins
#define rel_1 3 // Pin for the pump relay
#define rel_2 2 // Pin for the light relay

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Initialize variables
float responseValue = 0; // Placeholder for storing response values from ThingSpeak
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x40, 0x4F}; // MAC address for the Ethernet shield

// Static IP configuration in case DHCP fails
IPAddress ip(192, 168, 1, 150); // Static IP address for the Arduino
IPAddress myDns(192, 168, 1, 1); // DNS (usually the gateway address)

// ThingSpeak channel settings
unsigned long mySensorsChannelNumber = 1356424; // ThingSpeak channel for sensors
const char * myWriteAPIKey_Sensors = "6659XMNQ6Y2G7H9Y"; // Write API key for sensor channel

unsigned long myCommandsChannelNumber = 1331903; // ThingSpeak channel for commands
const char * myWriteAPIKey_Commands = "IFOBVPJQ1EXQ1LZ8"; // Write API key for commands channel

// Variables to store sensor readings
float number1 = 0.0; // Placeholder for temperature
float number2 = 0.0; // Placeholder for humidity
// Additional placeholders for more sensor readings (if needed)
float number3 = 0.0;
float number4 = 0.0;
float number5 = 0.0;
float number6 = 0.0;

// Timing variables for updating and reading from ThingSpeak
unsigned long lastWriteTime = 0; // Last time data was written to ThingSpeak
unsigned long lastReadTime = 0; // Last time data was read from ThingSpeak

// Variables for automatic watering system
int start_pump = 0; // Flag to control pump activation
unsigned long start_timer = 0; // Timer for pump operation

// Soil moisture sensor calibration values
float airValue_1 = 729.0; // Air value (dry soil)
float waterValue_1 = 0.0; // Water value (sensor submerged in water)

// Setup function runs once at startup
void setup() {
  dht.begin(); // Initialize DHT sensor
  delay(1000); // Wait for sensor initialization
  Ethernet.init(10); // Initialize Ethernet with the specified CS pin
  Serial.begin(115200); // Start serial communication at 115200 baud rate
  
  // Attempt to start Ethernet using DHCP
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield not found. Can't run without hardware. :(");
      while (true) delay(1); // Infinite loop if no Ethernet hardware is found
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    Ethernet.begin(mac, ip); // Try to configure using static IP
  } else {
    Serial.print("DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }
  delay(1000); // Give Ethernet shield a moment to initialize
  ThingSpeak.begin(client); // Initialize ThingSpeak

  // Setup relay pins as outputs
  pinMode(rel_1, OUTPUT); // Pump relay
  pinMode(rel_2, OUTPUT); // Light relay
}

// Function to read and calculate soil moisture
float getMoisture() {
  float soilMoistureValue = analogRead(A0); // Read moisture value from analog pin A0
  Serial.println("===============Moisture 1");
  Serial.println("Moisture 1 analog=" + String(soilMoistureValue));

  // Adjust moisture value based on calibration
  if (soilMoistureValue < waterValue_1) soilMoistureValue = waterValue_1;
  else if (soilMoistureValue > airValue_1) soilMoistureValue = airValue_1;
  Serial.println("Moisture 1 analog with limits=" + String(soilMoistureValue));

  // Convert to percentage
  soilMoistureValue = (airValue_1 - soilMoistureValue) / (airValue_1 - waterValue_1) * 100;
  Serial.println("Moisture 1 final value%=" + String(soilMoistureValue));
  return soilMoistureValue;
}

// Main loop function runs repeatedly
void loop() {
  // Update ThingSpeak with sensor values every 20 seconds
  if ((millis() - lastWriteTime) > 20000) {
    Serial.println("Read sensors values...");
    float h = dht.readHumidity(); // Read humidity
    float t = dht.readTemperature(); // Read temperature
    lastWriteTime = millis(); // Update last write time
    
    if (isnan(t) || isnan(h)) {
      Serial.println("Failed to read from DHT sensor");
    } else {
      Serial.println("Temp=" + String(t) + " *C");
      Serial.println("Humidity=" + String(h) + " %");
      number1 = t; // Assign temperature to number1
      number2 = h; // Assign humidity to number2
      number4 = getMoisture(); // Read and assign soil moisture to number4

      // Check soil moisture and control pump accordingly
      if (number4 > 10.0) wet_dry = 0; // Soil is wet
      else if (number4 < 10.0) {
        wet_dry = 1; // Soil is dry
        start_pump = 1; // Start pump
        start_timer = millis(); // Start timer for pump
      }
      
      // Write sensor values to ThingSpeak
      ThingSpeak.setField(1, number1);
      ThingSpeak.setField(2, number2);
      ThingSpeak.setField(3, responseValue); // Response from commands channel
      ThingSpeak.setField(4, number4); // Soil moisture
      // Additional fields can be set here
      
      int x = ThingSpeak.writeFields(mySensorsChannelNumber, myWriteAPIKey_Sensors);
      if (x == 200) {
        Serial.println("Channel update successful.");
      } else {
        Serial.println("Problem updating channel. HTTP error code " + String(x));
      }
    }
  }

  // Read commands from ThingSpeak every 15 seconds and control devices accordingly
  if ((millis() - lastReadTime) > 15000) {
    lastReadTime = millis();
    float value = ThingSpeak.readFloatField(myCommandsChannelNumber, 1);
    // Additional code to read commands and control devices
  }

  // Control devices based on manual commands and automatic sensor readings
  // Additional control logic can be added here
}
