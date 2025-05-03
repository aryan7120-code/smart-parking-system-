// Define specifics for Blynk cloud platform (choose one)
#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"       // Optional for Blynk IoT 2.0
#define BLYNK_TEMPLATE_NAME "YOUR_TEMPLATE_NAME" // Optional for Blynk IoT 2.0
#define BLYNK_DEVICE_NAME "Smart Parking Spot 1" // Give your device a name

#define BLYNK_PRINT Serial // Enables serial prints for debugging Blynk connection

// Include Libraries
#include <ESP8266WiFi.h>      // For ESP8266 WiFi connection
#include <BlynkSimpleEsp8266.h> // For Blynk communication on ESP8266
#include <Servo.h>            // For Servo motor control

// --- Configuration ---
// Blynk Credentials
char auth[] = "YOUR_BLYNK_AUTH_TOKEN"; // Replace with your Blynk Auth Token

// Wi-Fi Credentials
char ssid[] = "YOUR_WIFI_SSID";     // Replace with your Wi-Fi network name
char pass[] = "YOUR_WIFI_PASSWORD"; // Replace with your Wi-Fi password

// Hardware Pin Definitions
#define IR_SENSOR_PIN   D2  // Digital pin connected to IR sensor OUT
#define SERVO_PIN       D1  // PWM Digital pin connected to Servo Signal

// Servo Configuration
#define GATE_CLOSED_ANGLE 0   // Angle for the servo when gate is closed
#define GATE_OPEN_ANGLE   90  // Angle for the servo when gate is open

// IR Sensor Logic State (Adjust if your sensor is different)
#define IR_DETECTED_STATE LOW  // Sensor output state when car is detected (Occupied)
#define IR_EMPTY_STATE    HIGH // Sensor output state when spot is empty

// Blynk Virtual Pin Definitions
#define VPIN_PARKING_STATUS V0 // Labeled Value Display widget for status text
#define VPIN_GATE_CONTROL   V1 // Button widget to control the gate
#define VPIN_STATUS_LED     V2 // LED widget for visual status

// --- Global Variables ---
Servo gateServo;                 // Create servo object to control a servo
BlynkTimer timer;                // Blynk timer for periodic tasks
bool isSpotOccupied = false;     // Current state of the parking spot
bool previousSpotState = false;  // Previous state to detect changes for alerts

// --- Setup Function ---
void setup() {
  Serial.begin(115200);          // Start serial communication for debugging
  Serial.println("\nSmart Parking System Booting Up...");

  // Configure Hardware Pins
  pinMode(IR_SENSOR_PIN, INPUT); // Set IR sensor pin as input
  gateServo.attach(SERVO_PIN);  // Attach the servo on SERVO_PIN
  gateServo.write(GATE_CLOSED_ANGLE); // Start with the gate closed
  Serial.println("Hardware Initialized.");

  // Connect to Wi-Fi and Blynk
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Use BLYNK_TEMPLATE_ID and BLYNK_TEMPLATE_NAME if using Blynk IoT 2.0
  Blynk.begin(auth, ssid, pass);
  // For specific server (optional):
  // Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);
  // Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,100), 8080);

  Serial.println("Connecting to Blynk...");
  while (Blynk.connected() == false) {
    // Wait until connected
  }
  Serial.println("Blynk Connected!");

  // Set up a timer to call checkParkingStatus function every 1 second (1000 ms)
  timer.setInterval(1000L, checkParkingStatus);

  // Optional: Update Blynk with initial state on connection
  Blynk.virtualWrite(VPIN_GATE_CONTROL, 0); // Ensure button shows gate closed initially
  updateBlynkUI(isSpotOccupied);          // Send initial parking status
}

// --- Main Loop ---
void loop() {
  Blynk.run(); // Process Blynk tasks (communication with server, widget updates)
  timer.run(); // Run tasks scheduled by the Blynk timer (like checking sensor)
  // You can add other non-blocking code here if needed
}

// --- Functions ---

// Function called by the timer to check parking status
void checkParkingStatus() {
  int sensorValue = digitalRead(IR_SENSOR_PIN);
  //Serial.print("IR Sensor Value: "); Serial.println(sensorValue); // Uncomment for debugging

  // Determine current state based on sensor reading
  isSpotOccupied = (sensorValue == IR_DETECTED_STATE);

  // Check if the parking state has changed
  if (isSpotOccupied != previousSpotState) {
    Serial.print("Parking Status Changed: ");
    Serial.println(isSpotOccupied ? "Occupied" : "Empty");

    // Update the status on Blynk
    updateBlynkUI(isSpotOccupied);

    // --- Alert Logic: Send notification when spot becomes EMPTY ---
    if (!isSpotOccupied) { // If the spot is NOW empty (was occupied before)
      Serial.println("ALERT: Parking spot is now EMPTY!");
      Blynk.notify("Parking Spot is now Empty!"); // Send push notification
    }

    // Update the previous state tracker
    previousSpotState = isSpotOccupied;
  }
}

// Function to update Blynk widgets based on parking status
void updateBlynkUI(bool occupied) {
  if (occupied) {
    Blynk.virtualWrite(VPIN_PARKING_STATUS, "Occupied");
    Blynk.virtualWrite(VPIN_STATUS_LED, 255); // Turn LED widget ON (e.g., Red)
  } else {
    Blynk.virtualWrite(VPIN_PARKING_STATUS, "Empty");
    Blynk.virtualWrite(VPIN_STATUS_LED, 0);   // Turn LED widget OFF (e.g., Green/Off)
  }
}

// Function called when Blynk App button (V1) is pressed
BLYNK_WRITE(VPIN_GATE_CONTROL) {
  int buttonValue = param.asInt(); // Get value from button (0 or 1)

  if (buttonValue == 1) {
    // Open the gate
    gateServo.write(GATE_OPEN_ANGLE);
    Serial.println("Gate Opened via Blynk");
    Blynk.virtualWrite(VPIN_GATE_CONTROL, 1); // Keep button state ON
  } else {
    // Close the gate
    gateServo.write(GATE_CLOSED_ANGLE);
    Serial.println("Gate Closed via Blynk");
    Blynk.virtualWrite(VPIN_GATE_CONTROL, 0); // Keep button state OFF
  }
}

// Optional: Function called when device connects to Blynk server
BLYNK_CONNECTED() {
  Serial.println("Reconnected to Blynk Server.");
  // Request server to sync the state of the gate control button
  Blynk.syncVirtual(VPIN_GATE_CONTROL);
  // Update status immediately on reconnect
  updateBlynkUI(isSpotOccupied);
}
