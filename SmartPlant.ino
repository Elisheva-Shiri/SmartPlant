//we need to rescaling the temp
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "DHT.h"
#include <ESP32Servo.h>

/****************************** WiFi Access Point *********************************************/
#define WLAN_SSID       "iPhone"
#define WLAN_PASS       "0509903439"

/****************************** Adafruit.io defines  ********************************************/
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "DSEPD"
#define AIO_KEY         "aio_zeeF068xFovhd8fJ1jsiNsIDy9aX"
//need to change for prersonal Adafruit keyInfo

/************************** Adafruit IO stuff *********************************8******************/
unsigned long _lastUploadTimestamp = 0;
int _lastUploadedVal = -1;
const unsigned long MAX_ELAPSED_TIME_BETWEEN_UPLOADS_MS = 10000; // max upload rate on free tier is 30/minute 
const unsigned long MIN_ELAPSED_TIME_BETWEEN_UPLOADS_MS = 2000; // don't set this lower than 2000 (2 secs) given upload rate limit

/****************************** Global State (Don't change this!) ******************************/
// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;
// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** PUBLISH Feeds *********************************************************/
// Setup feeds called for publishing.
Adafruit_MQTT_Publish humidity_level = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");
Adafruit_MQTT_Publish temperature_level = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");

Adafruit_MQTT_Publish moisture_level_cloud = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/moisture");
Adafruit_MQTT_Publish water_level_cloud = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/water");

/****************************** SUBSCRIBE Feeds *********************************************************/
// Setup feeds called for Subscribe.
Adafruit_MQTT_Subscribe humidity_back = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/humidity");
Adafruit_MQTT_Subscribe temperature_back = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/temperature");

Adafruit_MQTT_Subscribe Button1_back = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/button1");
Adafruit_MQTT_Subscribe Button2_back = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/button2");
Adafruit_MQTT_Subscribe Button3_back = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/button3");

Adafruit_MQTT_Subscribe buttons[3] = {Button1_back, Button2_back, Button3_back};

/****************************** DHT22 & defines ****************************************************/
#define DHTPIN 18
#define DHTTYPE DHT22
int tempVal;    // temperature sensor raw readings
float volts;    // variable for storing voltage 
float temp_read_C;     // actual temperature variable
DHT dht(DHTPIN, DHTTYPE);

float x=0;  //max temp

/****************************** Pump WaterLevel Moisture defines ****************************************************/
const int relayPump = 15;
const int moisture = 34;
const int tankLevel = 35; //sensor pin used

int LED_R = 21 ; //water level sensor // off dc in case mousture is wet
int LED_B = 22; // dc motor

const int MOISTURE_fix = 1000 ; //the value after the LED goes on
int water_level = 0 ;
int moisture_level = 0;

/****************************** RGB & buttons defines ****************************************************/
int redPin = 9, greenPin = 10, bluePin = 5;
int button1_index = 0, button2_index = 1, button3_index = 2, button = -1;

/****************************** LED & SERVO ****************************************************/
Servo servo;
const int ldrPin = A0, LED = 2;
//In the setup servo declare as pin 13



/****************************** Sketch Code **************************************************/
/****************************** Placement setup functions ******************************************/
void setup_WIFI();
void setup_dht();  //temperature & humidity
void setup_pump_water_moisture();
void setup_RGB_button();
void setup_LED_servo();

/****************************** Placement loop functions ******************************************/
void MQTT_connect();
void loop_dht();
void loop_pump_water_moisture();
void loop_RGB_button();
void loop_LED_servo();

/****************************** setup *******************************************************/
void setup() {
  Serial.begin(9600);
    Serial.println(F("Adafruit MQTT demo"));
  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  setup_WIFI();
  
  setup_dht();
  setup_pump_water_moisture(); 
  setup_RGB_button();
  setup_LED_servo();
  
}

void loop() {
  MQTT_connect();   // Ensure the connection to the MQTT server is alive
  static int attempt = 0;
  Serial.print(F("Attempt number: "));
  Serial.println(attempt);
  
  loop_dht();
  loop_pump_water_moisture();
  loop_RGB_button();
  loop_LED_servo();
  
  attempt++;
  delay (1000);
}

/****************************** setup function **********************************************/
/******************************** setup WIFI ************************************************/
void setup_WIFI() {
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());  
  Serial.println("setup_WIFI end");
}
/****************************** setup tempreture % humidity *******************************/
void setup_dht() {
  Serial.println("setup_dht start");
  dht.begin();
//  delay(10);
  Serial.println("setup_dht end");
}

/****************************** setup Pump WaterLevel Moisture *******************************/
void setup_pump_water_moisture() {
  Serial.begin(9600);
  Serial.println("setup_pump_water_moisture start");
  pinMode(moisture, INPUT);
  pinMode(relayPump, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_B, OUTPUT);
  Serial.println("setup_pump_water_moisture end");
}

/****************************** setup RGB % button *******************************/
void setup_RGB_button() {
  Serial.println("setup_RGB_button start");
  for (button = 0; button < 3; button++)
    mqtt.subscribe(&buttons[button]);   //  subscribe buttons state
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  
  Serial.println("setup_RGB_button end");
}

/****************************** setup LED & servo *******************************/
void setup_LED_servo() {
  Serial.println("setup_LED_servo start");
  pinMode(ldrPin, INPUT);
  servo.attach(13);
  servo.write(13);
  pinMode(LED,OUTPUT);
  
  Serial.println("setup_LED_servo end");
}



/****************************** loop function ***********************************************/
/************Function to connect and reconnect as necessary to the MQTT server***************/
void MQTT_connect() {
  int8_t ret;
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }
  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}

/************************Function to send and  humidity & tempreture*************************/
void loop_dht() {
  // Read humidity
  float humidity_read = dht.readHumidity();
  
  // Read temperature as Celsius and convers to degrees C
  float temp_read = dht.readTemperature();
  volts = temp_read/1023.0;             // normalize by the maximum temperature raw reading range
  temp_read_C = (volts - 0.5) * (-100) ;         //calculate temperature celsius from voltage as per the equation found on the sensor spec sheet.
  
  //print temperature and humidity
  Serial.print(F("\nTemperature is: "));
  Serial.print(temp_read_C);
  Serial.println (" degrees C");       // In the same line print degrees C
  Serial.print(F("\nHumidity: "));
  Serial.println(humidity_read);

//  publish temperature;
  temperature_level.publish(temp_read_C);
  humidity_level.publish(humidity_read);

//  delay(10000);
}

/************************Function Pump WaterLevel Moisture*************************/
void loop_pump_water_moisture() {

  moisture_level = analogRead(moisture);
  Serial.print("Moisture Sensor Value:");
  Serial.println(analogRead(moisture_level)); 
  water_level = analogRead(tankLevel);
  Serial.print("Water level is : ");
  Serial.println(water_level);
  moisture_level_cloud.publish(moisture_level);
  water_level_cloud.publish(water_level);
  
  if (water_level <= 100){
  Serial.println("Tank: Empty");
  digitalWrite(LED_R,HIGH);
  digitalWrite(LED_B,LOW);
  }

  else {
    if (water_level>100 && water_level<=200){
    Serial.println("Water level: Low");
    digitalWrite(LED_R , LOW);
    digitalWrite(LED_B , HIGH);
    }
    else if (water_level>200 && water_level<=250){
    Serial.println("Water level: Medium");
    Serial.print("Water level = ");
    Serial.println(water_level);
    digitalWrite(LED_R , LOW);
    digitalWrite(LED_B , HIGH);
    }
    else {
    Serial.println("Water level: High");
    digitalWrite(LED_R , LOW);
    digitalWrite(LED_B , HIGH);
    }
  
    if (moisture_level < MOISTURE_fix) {
    digitalWrite(relayPump, HIGH);
    Serial.println("Current not Flowing");
//    delay(5000);
    }
    
    else {
    digitalWrite(relayPump, LOW);
    Serial.println("Current Flowing");
    }
  }
//  delay(4000);
}

/************************Function to send and  RGB_button*************************/
void loop_RGB_button() {
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    
      int button_index = -1;

      if (subscription == &buttons[button1_index]) {
          button_index = 0;
      }
      else if (subscription == &buttons[button2_index]) {
          button_index = 1;
      }
      else if (subscription == &buttons[button3_index]) {
          button_index = 2;
      }

      
      uint16_t inputVal = atoi((char *)buttons[button_index].lastread);  // convert to a number


      if (button_index == 0) {
        digitalWrite(redPin, HIGH);
        digitalWrite(greenPin,LOW);
        digitalWrite(bluePin, LOW);
      }
      else if (button_index == 1) {
          digitalWrite(redPin, LOW);
          digitalWrite(greenPin,LOW);
          digitalWrite(bluePin, HIGH);
      }
      else if (button_index == 2) {
          digitalWrite(redPin, LOW);
          digitalWrite(greenPin,HIGH);
          digitalWrite(bluePin, LOW);
      }
      else {
        digitalWrite(redPin, LOW);
        digitalWrite(greenPin,LOW);
        digitalWrite(bluePin, LOW);
      }
      }
}

/************************Function to chack the sunlight's shine*************************/
void loop_LED_servo() {
  int ldrStatus = analogRead(ldrPin);
  if (ldrStatus>1000) {
//    servo.attach(13);
    servo.write(0);
//    servo.detach();
    digitalWrite(LED,LOW);
    delay(300);
    Serial.println("sunLight high");
  }
  else if(ldrStatus >300 && ldrStatus<=1000){
//    servo.attach(13);
    servo.write(90);
//    servo.detach();
    digitalWrite(LED,LOW);
    delay(300);
    Serial.println("sunLight middle");
  }
   else {
//    servo.attach(13);
    servo.write(90);
//    servo.detach();
    digitalWrite(LED,HIGH);
    delay(300);
    Serial.println("sunLight low");
  
  }

}
