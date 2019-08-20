/*---------------------KRENOVATOR---------------------
  SMART DISTANCE & ALERT using Adafruit MQTT

  Demonstrates to aware of the distance using the
  ultrasonic sensor which can measure a distance 
  in centimeter where the LED is responded when
  the threshold is reached

  Get the code at github:
  https://github.com/MZulsyahmi/smart-distance-and-alert-using-mqtt.git
  
  by M.Zulsyahmi @krenovator
  September 2018
 */

/*the library that needed to be included in the codes*/
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET LED_BUILTIN
Adafruit_SSD1306 display(OLED_RESET);

#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#if (SSD1306_LCDHEIGHT != 64)   //to declare the pixel of OLED
#error("Height incorrect, please fix Adafruit_SSD1306.h!");   //if got an error, the codes need to be changed
#endif

/*initialize all the pin*/
int trigPin = D4; 
int echoPin = D3; 

int ledPinR = D8;     // LED red
int ledPinY = D7;     // LED yellow
int ledPinG = D6;     // LED green

int buzzer = D5;      // buzzer

long duration;
int distance;
int distances[128];

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "YourNetworkName"
#define WLAN_PASS       "NetworkPassword"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "YourUsername"
#define AIO_KEY         "Password"

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish Measurement = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/YourFeedsName");

// Setup a feed called 'onoff' for subscribing to changes.
//Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff");

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void setup() {
  pinMode(trigPin, OUTPUT);   //trigger pin as a receiver
  pinMode(echoPin, INPUT);    //echo pin as a transmitter
  
  pinMode(ledPinR, OUTPUT);  
  pinMode(ledPinY, OUTPUT);  
  pinMode(ledPinG, OUTPUT); 

  pinMode(buzzer, OUTPUT);    //buzzer
  
  display.clearDisplay();

  Serial.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)

  Serial.println(F("Adafruit MQTT demo"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
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

  // Setup MQTT subscription for onoff feed.
  //mqtt.subscribe(&onoffbutton);
}

uint32_t x=0;

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here

  // Now we can publish stuff!
  Serial.print(F("\nSending photocell val "));
  Serial.print(x);
  Serial.print("...");
  
/******************************** The controlled coding ********************************/

  digitalWrite(trigPin, LOW); //to set the trigger as off
  delayMicroseconds(5);

  digitalWrite(trigPin, HIGH); //test to emits
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW); //set back to off

  duration = pulseIn(echoPin, HIGH); //emits the ultrasonic
  distance = (duration/2)/29.1; //formula for the distance in ultrasonic in cm

  Serial.print("\nDistance: ");
  Serial.print(distance);

  display.clearDisplay();

  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.print(distance);
  display.print(" cm");

  delay(10);

/*condition of threshold*/
  if(distance <= 10){ //less than 10cm to give a WARNING
    Serial.print("  Warning!");   //warning will pop up
    digitalWrite(ledPinR, HIGH);  //led will lights up
    digitalWrite(ledPinY, HIGH);  //led will lights up
    digitalWrite(buzzer,HIGH);    // buzzer will turn ON
    delay(100);
  }

  else if (distance > 10 && distance <=20){ //between 10cm to 20cm to give a caution sign
    Serial.print("  Careful!"); //clear sign pop up
    digitalWrite(ledPinR, LOW); //led red turns off
    digitalWrite(ledPinY, HIGH); //led yellow turns ON
    delay(100);
  }

  else if (distance > 20){ //greater than 20cm to give a clear sign
    Serial.print("  Clear"); //clear sign pop up
    digitalWrite(ledPinR, LOW);   //led red turns off
    digitalWrite(ledPinY, LOW);   //led yellow turns off
    digitalWrite(ledPinG, HIGH);  //led green turns ON
    digitalWrite(buzzer,LOW);     // buzzer will turn OFF
    delay(100);
  }
  
/******************************** Condition to publish ********************************/
  if (!Measurement.publish(distance)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

  display.display();
  delay(2000);

  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  /*
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
  */
}

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
