#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Ticker.h> //watch dog
#include <DHTesp.h> // https://github.com/beegee-tokyo/DHTesp
#include <Hash.h> // required for mqtt client
#include <ESP8266MQTTClient.h> // https://github.com/tuanpmt/ESP8266MQTTClient

#include "../wiring.h"
#include "../secrets.h"

#include "hcsr04.h"

// reset watchdog
Ticker tickerOSWatch;
static unsigned long last_loop;
void ICACHE_RAM_ATTR osWatch(void) 
{
	unsigned long t = millis();
	unsigned long last_run = abs(t - last_loop);
	if(last_run >= (OSWATCH_RESET_TIME_MS)) 
	{
		// save the hit here to eeprom or to rtc memory if needed
		Serial.println("watchdog restarting");
		ESP.restart();  // normal reboot 
		//ESP.reset();  // hard reset
	} else {
		Serial.println("watchdog check passed");
	}
}



// temp sensor
DHTesp dht;
double lastTemperature = 25.0d;
// relaystate
int relay1state = 0;

ESP8266WebServer server(80);   //instantiate server at port 80 (http port)

// MQTT handlers


void setup()
{
	Serial.begin(115200);
	Serial.println("waking up");
	last_loop = millis();
	tickerOSWatch.attach_ms((OSWATCH_RESET_TIME_MS / 3), osWatch);
	
	// init temp
	dht.setup(DHTTEMPIN, DHTesp::DHT11);
	// setup dist pins
	pinMode(DISTTRIG, OUTPUT);
	pinMode(DISTECHO, INPUT);
	pinMode(RELAY1, OUTPUT);
	pinMode(RELAY2, OUTPUT);
	
	pinMode(LLED, OUTPUT);
	digitalWrite(LLED, HIGH);
	WiFi.begin(WIFI_SID, WIFI_PASSWD);
	Serial.println("waiting for connection");
	while (WiFi.status() != WL_CONNECTED)
	{
		digitalWrite(LLED, LOW);
		delay(200);
		digitalWrite(LLED, HIGH);
		delay(300);
	}
	Serial.print("connected as ");
	Serial.println(WiFi.localIP().toString());
	// setup api functions
	server.on("/", []()
		{
			//Report the current status of the device
			server.send(200, "text/plain", "hellow");
		}
	);
	server.on("/temp", []()
		{
			// Print temperature sensor details.
			float humidity = dht.getHumidity();
			float temperature = dht.getTemperature();
			char body[1024];
			sprintf(body,  "<html> <head>   <title>ESP8266 Page</title> <meta name='viewport' content='width=device-width, initial-scale=1.0'>  <style>     h1 {text-align:center; }     td {font-size: 50%; padding-top: 30px;}     .temp {font-size:150%; color: #FF0000;}     .press {font-size:150%; color: #00FF00;}     .hum {font-size:150%; color: #0000FF;}   </style> </head>  <body>    <h1>ESP8266 Sensor Page</h1>    <div id='div1'>        <table>           <tr>            <td>Temperature</td><td class='temp'>%.2f</td>          </tr>          <tr>   <td>Humidity</td><td class='hum'>%.2f</td>  </tr> </div> </body>  </html>", temperature, humidity);
			server.send(200, "text/html", body);
			lastTemperature = (double) temperature;
		}
	);
	
	server.on("/dist", []()
		{
			// Get the time the ultrasonic pulses took from the sensor to the blocking object
			//int hitTime = ultrasonicSensor.getHitTime();
			// Calculate the approximate distance in centimeters (as seen in extras/HC-SR04.txt)
			//int distanceInCm = hitTime / 29;
			//double* distance = HCSR04.measureDistanceCm();
			double dist = getDist(lastTemperature);
			char body[1024];
			sprintf(body,  "<html> <head>   <title>ESP8266 Page</title> <meta name='viewport' content='width=device-width, initial-scale=1.0'>  <style>     h1 {text-align:center; }     td {font-size: 50%; padding-top: 30px;}     .temp {font-size:150%; color: #FF0000;}     .press {font-size:150%; color: #00FF00;}     .hum {font-size:150%; color: #0000FF;}   </style> </head>  <body>    <h1>ESP8266 Sensor Page</h1>    <div id='div1'>        <table>           <tr><td>Distance in cm</td><td class='temp'>%.2f</td>  </tr> </div> </body>  </html>", dist);
			server.send(200, "text/html", body);
		}
	);
	
	server.begin();
}

void loop(void)
{
	last_loop = millis(); // checkin with watchdog
	digitalWrite(LLED, LOW);
	server.handleClient();
	//digitalWrite(RELAY1, !digitalRead(RELAY1));
	//digitalWrite(RELAY2, !digitalRead(RELAY1));
	digitalWrite(LLED, HIGH);
	delay(10000);
}