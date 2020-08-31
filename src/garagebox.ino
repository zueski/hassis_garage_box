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

// store last values
static unsigned long last_sensor;
double lastTemperature = 0.0;
double lastHumidity = 0;
double lastDistance = 0.0;
int lastCarPresent = 0;

ESP8266WebServer server(80);   //instantiate server at port 80 (http port)

// 
void handleTempSensor()
{
	float humidity = dht.getHumidity();
	float temperature = dht.getTemperature();
	lastTemperature = (double) temperature;
	lastHumidity = (double) humidity;
	Serial.print("Temp: ");
	Serial.println(temperature);
	Serial.print("Humid: ");
	Serial.println(humidity);
	// call MQTT publish
}

void handleDistSensor()
{
	double distance = getDistance(lastTemperature);
	int carPresent = (distance < CAR_PRESENT_CM_THREASHOLD) ? 1 : 0;
	if(carPresent != lastCarPresent)
	{
		// call MQTT with new state
	}
	lastCarPresent = carPresent;
}
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
			char body[1024];
			sprintf(body,  "<html><head><title>Garage Status Page</title></head><body>"
							"<h1>Garage Status Page</h1><div id='div1'><table>"
							"<tr><td>Temperature</td><td class='temp'>%.1f</td></tr>"
							"<tr><td>Humidity</td><td class='hum'>%.1f</td></tr>"
							"<tr><td>Car present</td><td class='hum'>%i</td></tr>"
							"</table></div></body></html>", lastTemperature, lastHumidity, lastCarPresent);
			server.send(200, "text/html", body);
		}
	);
		
	server.begin();
}

void loop(void)
{
	// checkin with watchdog
	last_loop = millis(); 
	if(last_sensor + 30000L < last_loop)
	{
		// turn on the activity light
		digitalWrite(LLED, LOW); 
		last_sensor = last_loop;
		handleTempSensor();
		handleDistSensor();
		// turn off the activity light
		digitalWrite(LLED, HIGH);
	}
	server.handleClient();
	delay(500);
}