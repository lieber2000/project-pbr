#include "Arduino.h"
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include "TinyGPS++.h"

int green = 2;
int red = 3;
int yellow = 4;
int stopDistanceCm = 25;
int slowDistanceCm = 50;
int readFrequency = 300;
boolean usingInterrupt = false;
void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy

Adafruit_GPS GPS(&Serial1);
#define GPSECHO  true // debug?

TinyGPSPlus gps;

class Ultrasonic {
	public:
		Ultrasonic(int pin);
    void DistanceMeasure(void);
		long microsecondsToCentimeters(void);
		long microsecondsToInches(void);
	private:
		int _pin; //pin number of Arduino that is connected with SIG pin of Ultrasonic Ranger.
    long duration; // the Pulse time received;
};

Ultrasonic::Ultrasonic(int pin) {
	_pin = pin;
}

/*Begin the detection and get the pulse back signal*/
void Ultrasonic::DistanceMeasure(void)
{
  pinMode(_pin, OUTPUT);
	digitalWrite(_pin, LOW);
	delayMicroseconds(2);
	digitalWrite(_pin, HIGH);
	delayMicroseconds(5);
	digitalWrite(_pin,LOW);
	pinMode(_pin,INPUT);
	duration = pulseIn(_pin,HIGH);
}

/*The measured distance from the range 0 to 400 Centimeters*/
long Ultrasonic::microsecondsToCentimeters(void) {
	return duration/29/2;
}

Ultrasonic ultrasonic(7);
void setup() {
	pinMode(green, OUTPUT);
	pinMode(red, OUTPUT);
	pinMode(yellow, OUTPUT);
	Serial.begin(9600);
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);    // Set the update rate: 1 Hz
  useInterrupt(true);
  delay(1000);
}

// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
}

void useInterrupt(boolean v) {
  if (v) {
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  }
}

uint32_t timer = millis();

void loop() {
	if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
      return;  // we can fail to parse a sentence in which case we should just wait for another
  }

  // if millis() or timer wraps around, we'll just reset it
  if (timer > millis())  timer = millis();
  if (millis() - timer > 1000) {
		if (GPS.fix) {
			printGpsInfo();
		} else {
		  Serial.print("No Fix");
		}
	}

	long RangeInCentimeters;
	ultrasonic.DistanceMeasure(); // get the current signal time;
	RangeInCentimeters = ultrasonic.microsecondsToCentimeters();
	printRangeInfo(RangeInCentimeters);
	Serial.println();

	delay(readFrequency);
}

void updateSpeedIndicators(int pin) {
	analogWrite(green, 0);
	analogWrite(yellow, 0);
	analogWrite(red, 0);
	analogWrite(pin, 50);
}

void printRangeInfo(int rangeCm) {
	Serial.print(", ");
	Serial.print(rangeCm);
	Serial.print("cm, ");
	if (rangeCm < stopDistanceCm) {
		updateSpeedIndicators(red);
		Serial.print("STOP!");
	} else if (rangeCm < slowDistanceCm) {
		updateSpeedIndicators(yellow);
		Serial.print("SLOW...");
	} else {
		updateSpeedIndicators(green);
		Serial.print("BALLS TO THE WALL!");
	}
}

void printGpsInfo() {
	Serial.print(GPS.latitude, 4);
	Serial.print(GPS.lat);
	Serial.print(", ");
	Serial.print(GPS.longitude, 4);
	Serial.print(GPS.lon);
	Serial.print(", ");
	Serial.print(GPS.speed);
	Serial.print(" knots,  ");
	Serial.print(GPS.angle);
	Serial.print(" degrees");
}