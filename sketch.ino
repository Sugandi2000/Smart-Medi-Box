///////////////////////////
// Libraries and Constants
///////////////////////////

// Including required libraries
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <DHTesp.h>
#include <WiFi.h>

// OLED screen settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define OLED_SDA 21
#define OLED_SCL 22

// Pin definitions
#define BUZZER 5
#define LED_GREEN 15
#define LED_RED 2

#define CANCEL_BTN 34
#define OK_BTN 32
#define UP_BTN 35
#define DOWN_BTN 33
#define DHTPIN 12

// NTP Server and time offsets
#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET_DST 0

// Initial UTC offset (in seconds) which is relevent to the local zone
int utc_offset = 19800;

// Musical notes frequencies
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C, D, E, F, G, A, B, C_H};
int n_notes = 8;

///////////////////////////
// Menu and Global Variables
///////////////////////////

// Menu modes
int current_mode = 0;
int max_modes = 5;
String modes[] = {"Set Time", "Set Alarm 1", "Set Alarm 2", "Set Alarm 3", "Disable Alarms"};

// Objects declaration
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;

// Global variables for time and alarms
int year = 0;
int month = 0;
int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;

// Alarm functionality variables
bool alarm_enabled = true;
int n_alarms = 3;
int alarm_hours[] = {10, 10, 10}; // Default alarm hours
int alarm_minutes[] = {30, 32, 55}; // Default alarm minutes
bool alarm_triggered[] = {false, false, false}; // Flag to indicate if an alarm has been triggered

///////////////////////////
// Setup Function
///////////////////////////

void setup() {
  // Set pin modes for peripherals
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(CANCEL_BTN, INPUT);
  pinMode(OK_BTN, INPUT);
  pinMode(UP_BTN, INPUT);
  pinMode(DOWN_BTN, INPUT);

  dhtSensor.setup(DHTPIN, DHTesp::DHT22);

  // Initialize OLED display
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Loop indefinitely if allocation fails
  }
  display.display();
  delay(500);

  // Connect to WiFi network
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WIFI", 0, 0, 2);
  }
  display.clearDisplay();
  print_line("Connected to WIFI", 0, 0, 2);
  delay(500);

  // Get time from NTP server
  configTime(utc_offset, UTC_OFFSET_DST, NTP_SERVER);

  // Display welcome message
  display.clearDisplay();
  print_line("Welcome", 15, 20, 2);
  delay(1000);
  display.clearDisplay();
  print_line("to the", 20, 20, 2);
  delay(1000);
  display.clearDisplay();
  print_line("MediBox", 15, 20, 2);
  delay(1000);
  display.clearDisplay();
}

///////////////////////////
// Loop Function
///////////////////////////

void loop() {
  delay(10); // Delay for stability

  // Update time and check for alarms
  update_time_with_check_alarm();

  // Check if the menu button is pressed
  if (digitalRead(OK_BTN) == LOW) {
    delay(500);
    go_to_menu();
  }

  // Monitor the temperature and humidity
  check_temp();
}

///////////////////////////
// Print Text Function
///////////////////////////

void print_line(String text, int column, int row, int text_size) {
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.println(text);
  display.display();
}

///////////////////////////
// Print Date and Time Function
///////////////////////////

// Print current date and time on the OLED display
void print_time_now(void) {
  // Print date in format: DD/MM/YYYY
  print_line(String(days) + "/" + String(month) + "/" + String(year), 40, 0, 1);

  // Print alarm indicator
  display.clearDisplay();
  if (alarm_enabled) {
    print_line("AL", 0, 0, 1);
  }

  // Print hours
  if (hours < 10) {
    print_line("0" + String(hours), 5, 20, 3); // Add leading zero for only single digit hours
  } else {
    print_line(String(hours), 10, 20, 3);
  }
  print_line(": ", 40, 20, 3);

  if (minutes < 10) {
    print_line("0" + String(minutes), 50, 20, 3); // Add leading zero for only single digit minutes
  } else {
    print_line(String(minutes), 50, 20, 3);
  }
  print_line(":", 80, 27, 2);

  if (seconds < 10) {
    print_line("0" + String(seconds), 88, 27, 2); // Add leading zero for only single digit seconds
  } else {
    print_line(String(seconds), 90, 27, 2);
  }
}

///////////////////////////
// Update Time Function
///////////////////////////

// Update the system time variables with current time obtained from NTP server
void update_time() {
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  // Define character arrays to store time information
  char date_str[8];
  char month_str[8];
  char year_str[20];
  char hour_str[28];
  char min_str[8];
  char sec_str[8];

  // Format time information into strings
  strftime(month_str, 20, "%m %d %Y", &timeinfo);
  strftime(date_str, 20, "%d", &timeinfo);
  strftime(year_str, 20, "%Y", &timeinfo);
  strftime(sec_str, 8, "%S", &timeinfo);
  strftime(hour_str, 8, "%H", &timeinfo);
  strftime(min_str, 8, "%M", &timeinfo);

  // Convert formatted strings into integers
  days = atoi(date_str);
  month = atoi(month_str);
  year = atoi(year_str);
  hours = atoi(hour_str);
  minutes = atoi(min_str);
  seconds = atoi(sec_str);

}

///////////////////////////
// Alarm Functions
///////////////////////////

// Activate alarm to signal medicine time
void ring_alarm() {
  display.clearDisplay();
  print_line("MEDICINE TIME!", 0, 0, 2);
  digitalWrite(LED_GREEN, HIGH);

  bool break_happened = false;

  // Ring the buzzer until canceled or alarm stops
  while (break_happened == false && digitalRead(CANCEL_BTN) == HIGH) {
    for (int i = 0; i < n_notes; i++) {
      if (digitalRead(CANCEL_BTN) == LOW) { // Check if alarm is canceled
        delay(200);
        break_happened = true; // Exit the loop if alarm is canceled
        break;
      }
      tone(BUZZER, notes[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }
  }

  digitalWrite(LED_GREEN, LOW);
  display.clearDisplay();
}

// Check alarms and update time
void update_time_with_check_alarm() {
  update_time();        // Update system time
  print_time_now();     // Print current time

  // Check if any alarms are triggered
  if (alarm_enabled == true) {
    for (int i = 0; i < n_alarms; i++) {
      if (alarm_triggered[i] == false && alarm_hours[i] == hours && alarm_minutes[i] == minutes) {
        ring_alarm();                         // Trigger the alarm
        alarm_triggered[i] = true;            // Set alarm as triggered
      }
    }
  }
}

///////////////////////////
// Button Press Handling
///////////////////////////

// Wait for a button press and return the pressed button
int wait_for_button_press() {
  while (true) {
    if (digitalRead(UP_BTN) == LOW) {
      delay(200);
      return UP_BTN;
    }
    else if (digitalRead(DOWN_BTN) == LOW) {
      delay(200);
      return DOWN_BTN;
    }
    else if (digitalRead(CANCEL_BTN) == LOW) {
      delay(200);
      return CANCEL_BTN;
    }
    else if (digitalRead(OK_BTN) == LOW) {
      delay(200);
      return OK_BTN;
    }
    update_time(); // Update system time while waiting for button press
  }
}

//////////////////////////////////
// Menu Navigation Functionality
//////////////////////////////////

// Function to navigate through the menu
void go_to_menu() {
  while (digitalRead(CANCEL_BTN) == HIGH) { // Keep looping until CANCEL button is pressed
    display.clearDisplay();
    print_line("Menu", 50, 0, 1);
    print_line("->" + modes[(current_mode)],0 , 9, 1.5); // Display current menu option with arrow
     // Display the next four menu options
    print_line(modes[(current_mode + 1) % 5], 0, 24, 1);
    print_line(modes[(current_mode + 2) % 5], 0, 34, 1);
    print_line(modes[(current_mode + 3) % 5], 0, 44, 1);
    print_line(modes[(current_mode + 4) % 5], 0, 54, 1);

    int pressed = wait_for_button_press();

    if (pressed == UP_BTN) {
      delay(200);
      current_mode += 1; // Move to the next menu option
      current_mode = current_mode % max_modes; // Wrap around if reaching the end of menu options
    }
    else if (pressed == DOWN_BTN) {
      delay(200);
      current_mode -= 1; // Move to the previous menu option
      if (current_mode < 0) {
        current_mode = max_modes - 1; // Wrap around if reaching the beginning of menu options
      }
    }
    else if (pressed == OK_BTN) {
      delay(200);
      run_mode(current_mode); // Execute the selected menu option
    }
    else if (pressed == CANCEL_BTN) {
      delay(200);
      break; // Exit the menu
    }
  }
}

////////////////////////
// Time Setting Function
////////////////////////

// Function to set the time
void set_time() {
  // Set hour
  int temp_hour = 0;
  int temp_UTC = 0;
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == UP_BTN) {
      delay(200);
      temp_hour += 1;
      if (temp_hour > 14) { // Wrap around to -12 if hour exceeds 14 to adhere to UTC time zones
        temp_hour = -12;
      }
    }
    else if (pressed == DOWN_BTN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < -12) { // Wrap around to 14 if hour is less than -12 to adhere to UTC time zones
        temp_hour = 14;
      }
    }
    else if (pressed == OK_BTN) {
      delay(200);
      temp_UTC = (temp_hour * 3600); // Calculate hours in seconds for the UTC offset
      break;
    }
    else if (pressed == CANCEL_BTN) {
      delay(200);
      break;
    }
  }

  // Set minutes
  int temp_minute = 0;
  while (true) {
    display.clearDisplay();
    print_line("Enter", 0, 0, 2);
    print_line("minute: " + String(temp_minute), 0, 13, 2);

    int pressed = wait_for_button_press();
    if (pressed == UP_BTN) {
      delay(200);
      temp_minute += 15;
      temp_minute = temp_minute % 60; // Wrap around to 0 if minutes exceed 59
    }
    else if (pressed == DOWN_BTN) {
      delay(200);
      temp_minute -= 15;
      if (temp_minute < 0) { // Wrap around to 45 if minutes go below 0
        temp_minute = 45;
      }
    }
    else if (pressed == OK_BTN) { // Update UTC offset and synchronize time
      delay(200);
      temp_UTC += (temp_minute * 60); // Add minutes to UTC offset in seconds
      utc_offset = temp_UTC; // Update global UTC offset
      configTime(utc_offset, UTC_OFFSET_DST, NTP_SERVER); // Synchronize time with NTP server using updated offset  
      break;
    }
    else if (pressed == CANCEL_BTN) {
      delay(200);
      break;
    }
  }
  display.clearDisplay();
  // Display confirmation message
  print_line("TIME IS   SET!!!", 0, 0, 2);
  delay(1000);
  print_time_now(); // Print updated time
}

// Set alarm time
void set_alarm(int alarm) {
  int temp_hour = alarm_hours[alarm];

  // Set alarm hour
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == UP_BTN) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24; // Ensure hour stays within range 0-23
    }
    else if (pressed == DOWN_BTN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0) {
        temp_hour = 23; // If hour goes below 0, set to 23
      }
    }
    else if (pressed == OK_BTN) {
      delay(200);
      alarm_hours[alarm] = temp_hour; // Set alarm hour
      break;
    }
    else if (pressed == CANCEL_BTN) {
      delay(200);
      break;
    }
  }

  // Set alarm minutes
  int temp_minute = alarm_minutes[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter     minute: " + String(temp_minute), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == UP_BTN) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60; // Ensure minutes stays within range 0-59
    }
    else if (pressed == DOWN_BTN) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0) {
        temp_minute = 59; // If minutes go below 0, set to 59
      }
    }
    else if (pressed == OK_BTN) {
      delay(200);
      alarm_minutes[alarm] = temp_minute;          // Set alarm minutes
      alarm_triggered[alarm] = false;             // Reset alarm trigger status
      alarm_enabled = true;                       // Enable alarm
      break;
    }
    else if (pressed == CANCEL_BTN) {
      delay(200);
      break;
    }
  }
  display.clearDisplay();
  print_line("ALARM IS  SET!!!", 0, 0, 2);
  delay(1000); // One second delay for readability
}

//////////////////////
// Menu Execution Function
//////////////////////

void run_mode(int mode) {
  if (mode == 0) { // If mode is 0, set time
    set_time();
  }

  else if (mode == 1 || mode == 2 || mode == 3) { // If mode is 1, 2, or 3, set alarm
    set_alarm(mode - 1); // Adjusting mode for array indexing
  }

  else if (mode == 4) { // If mode is 4, toggle alarm disable
    if (alarm_enabled == true) {
      display.clearDisplay();
      print_line("ALARM IS  DISABLED!!", 0, 0, 2);
      delay(1000);
    } else {
      display.clearDisplay();
      print_line("ALARM IS  ALREADY   DISABLED!!", 0, 0, 2);
      delay(1000);
    }
    alarm_enabled = false; // Disable the alarm

  }
}

//////////////////////
// Temperature and Humidity Monitoring Function
//////////////////////

void check_temp() {
  // Obtain temperature and humidity data from the sensor
  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  // Check if temperature is above 32°C
  if (data.temperature >= 32) {
    digitalWrite(LED_RED, HIGH);
    print_line("TEMPERATURE IS HIGH", 0, 48, 1);
    delay(200); // Delay for stability
  }
  // Check if temperature is below 26°C
  else if (data.temperature <= 26) {
    digitalWrite(LED_RED, HIGH);
    print_line("TEMPERATURE IS LOW", 0, 48, 1);
    delay(200);
  }
  else {
    digitalWrite(LED_RED, LOW);
  }
  // Check if humidity is above 80%
  if (data.humidity >= 80) {
    digitalWrite(LED_RED, HIGH);
    print_line("HUMIDITY IS HIGH", 0, 57, 1);
    delay(200);
  }
  // Check if humidity is below 60%
  else if (data.humidity <= 60) {
    digitalWrite(LED_RED, HIGH);
    print_line("HUMIDITY IS LOW", 0, 57, 1);
    delay(200);
  }
  else {
    digitalWrite(LED_RED, LOW);
  }
}

