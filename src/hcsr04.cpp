#include "hcsr04.h"

double getDistance(double temperature)
{
	int dist = -1;
	unsigned long start = -1;
	unsigned long end = -1;
	unsigned long timeout = -1;
	unsigned long delaytime = -1;
	// Make sure the trigger pin is low
	digitalWrite(DISTTRIG, LOW);
	delayMicroseconds(2);
	// trigger
	digitalWrite(DISTTRIG, HIGH);
	delayMicroseconds(10);
	timeout = micros() + 250UL;
	digitalWrite(DISTTRIG, LOW);
	// get time and wait
	while (digitalRead(DISTECHO) != HIGH && micros() < timeout) {
		delayMicroseconds(1);
	}
	start = micros();
	timeout = start + 23200UL;
	delayMicroseconds(50);
	while (digitalRead(DISTECHO) != LOW && micros() < timeout) {
		delayMicroseconds(1);
	}
	end = micros(); 
	if(end > timeout)
	{
		delaytime = -1;
		return delaytime;
	} else {
		delaytime = end - start;
	}
	//delaytime = pulseIn(DISTECHO, HIGH);

	double speedOfSoundInCmPerMs = 0.03313 + 0.0000606 * temperature; // Cair ≈ (331.3 + 0.606 ⋅ ϑ) m/s
	double distanceCm = delaytime / 2.0 * speedOfSoundInCmPerMs;
	return distanceCm;
}
