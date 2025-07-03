// Libraries
#include <LiquidCrystal_I2C.h>  // For LCD display
#include <SoftwareSerial.h>      // For GSM module
#include "WiFi.h"                // For ESP32 WiFi
#include <HTTPClient.h>          // For WhatsApp notifications

// Pin Definitions
#define GAS_SENSOR_PIN     34    // MQ2/MQ5 sensor
#define FLAME_SENSOR_PIN   35    // Flame sensor
#define GREEN_LED_PIN      32    // Normal status LED
#define RED_LED_PIN        33    // Alert LED
#define BUZZER_PIN        25    // Buzzer
#define FAN_RELAY_PIN     26    // Cooling fan control
#define GAS_RELAY_PIN     27    // Gas regulator control

// GSM Module pins
#define GSM_TX_PIN        16
#define GSM_RX_PIN        17

// Thresholds
#define GAS_THRESHOLD     1500  // Adjust based on sensor calibration
#define FLAME_THRESHOLD   500   // Adjust based on sensor calibration

// Initialize objects
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C address might vary
SoftwareSerial gsmModule(GSM_TX_PIN, GSM_RX_PIN);

// User contact details
const char* PHONE_NUMBER = "+1234567890";  // Replace with actual number
const char* WHATSAPP_API_KEY = "your_api_key";  // Replace with actual API key

// System states
enum SystemState {
  NORMAL,
  GAS_LEAK,
  FIRE_DETECTED
};

SystemState currentState = NORMAL;
bool alertSent = false;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  gsmModule.begin(9600);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  
  // Configure pins
  pinMode(GAS_SENSOR_PIN, INPUT);
  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(FAN_RELAY_PIN, OUTPUT);
  pinMode(GAS_RELAY_PIN, OUTPUT);
  
  // Initial state - everything off
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(FAN_RELAY_PIN, LOW);
  digitalWrite(GAS_RELAY_PIN, LOW);
  
  // Initialize GSM module
  initGSM();
  
  // Show startup message
  displayMessage("System Starting", "Please Wait...");
  delay(2000);
}

void loop() {
  // Read sensor values
  int gasValue = analogRead(GAS_SENSOR_PIN);
  int flameValue = analogRead(FLAME_SENSOR_PIN);
  
  // Determine system state
  SystemState newState = determineState(gasValue, flameValue);
  
  // Handle state change
  if (newState != currentState) {
    currentState = newState;
    handleStateChange();
  }
  
  // Update system based on current state
  updateSystem();
  
  delay(100);  // Small delay to prevent excessive polling
}

SystemState determineState(int gasValue, int flameValue) {
  if (flameValue < FLAME_THRESHOLD) {
    return FIRE_DETECTED;
  }
  if (gasValue > GAS_THRESHOLD) {
    return GAS_LEAK;
  }
  return NORMAL;
}

void handleStateChange() {
  alertSent = false;  // Reset alert flag on state change
  
  switch (currentState) {
    case NORMAL:
      setNormalState();
      break;
    case GAS_LEAK:
      setGasLeakState();
      break;
    case FIRE_DETECTED:
      setFireState();
      break;
  }
}

void updateSystem() {
  switch (currentState) {
    case NORMAL:
      digitalWrite(GREEN_LED_PIN, HIGH);
      digitalWrite(RED_LED_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(FAN_RELAY_PIN, LOW);
      digitalWrite(GAS_RELAY_PIN, LOW);
      if (!alertSent) {
        displayMessage("All Systems", "Normal");
      }
      break;
      
    case GAS_LEAK:
    case FIRE_DETECTED:
      // Blink red LED
      digitalWrite(RED_LED_PIN, (millis() % 1000) < 500);
      digitalWrite(GREEN_LED_PIN, LOW);
      
      // Activate safety measures
      digitalWrite(BUZZER_PIN, HIGH);
      digitalWrite(FAN_RELAY_PIN, HIGH);
      digitalWrite(GAS_RELAY_PIN, HIGH);
      
      // Send alerts if not already sent
      if (!alertSent) {
        sendAlerts();
        alertSent = true;
      }
      break;
  }
}

void setNormalState() {
  displayMessage("All Systems", "Normal");
}

void setGasLeakState() {
  displayMessage("Gas Leak!", "Action Taken");
}

void setFireState() {
  displayMessage("Fire Detected!", "Action Taken");
}

void displayMessage(const char* line1, const char* line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void initGSM() {
  // Initialize GSM module
  gsmModule.println("AT");
  delay(1000);
  gsmModule.println("AT+CMGF=1"); // Set SMS text mode
  delay(1000);
}

void sendAlerts() {
  // Send SMS
  sendSMS();
  
  // Make phone call
  makeCall();
  
  // Send WhatsApp message
  sendWhatsApp();
}

void sendSMS() {
  gsmModule.println("AT+CMGS=\"" + String(PHONE_NUMBER) + "\"");
  delay(1000);
  
  String message = currentState == FIRE_DETECTED ? 
    "ALERT: Fire detected! Emergency measures activated." :
    "ALERT: Gas leak detected! Emergency measures activated.";
    
  gsmModule.println(message);
  delay(100);
  gsmModule.write(26); // CTRL+Z to send SMS
  delay(1000);
}

void makeCall() {
  gsmModule.println("ATD" + String(PHONE_NUMBER) + ";");
  delay(20000); // Wait for 20 seconds
  gsmModule.println("ATH"); // Hang up
}

void sendWhatsApp() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String message = currentState == FIRE_DETECTED ? 
      "ALERT: Fire detected! Emergency measures activated." :
      "ALERT: Gas leak detected! Emergency measures activated.";
      
    // Replace with actual WhatsApp API endpoint and implementation
    String url = "https://api.whatsapp.com/send?phone=" + String(PHONE_NUMBER) + "&text=" + message;
    http.begin(url);
    http.addHeader("Authorization", String("Bearer ") + WHATSAPP_API_KEY);
    http.POST("");
    http.end();
  }
}