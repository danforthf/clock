#include <stdio.h>
#include <Time.h>
#include <Wire.h>
#include <DS1307RTC.h>

#define H_BUTTON
#define M_BUTTON
#define CLOCK_SET
#define DATE_SET
#define H_DIAL
#define M_DIAL

#define H_PRESSED 2
#define M_PRESSED 1
#define HM_PRESSED 3

#define TIME_REQUEST  7    // ASCII bell character requests a time sync message
#define DEBOUNCE_DELAY 100

#define SET_TIME -1
#define SET_DATE 1
#define SET_MINUTES 1
#define SET_HOURS 0

// globals
int lastButtonReading = 0;
int buttonReading;
int buttonState;
int switchState = 0;
long lastPress;


void setup() {

	#ifdef SERIAL
	Serial.begin(9600);
	#endif

	
	// set time provider depending on platform
	#ifdef TEENSY3
	setSyncProvider(getTeensy3Time);
	#endif
	#ifdef UNO
	setSyncProvider(RTC.get);
	#endif

	// button inputs
	pinMode(H_BUTTON, INPUT);
	pinMode(M_BUTTON, INPUT);
	pinMode(REVERSE, INPUT);

	// switch inputs
	pinMode(CLOCK_SET, INPUT);
	pinMode(DATE_SET, INPUT);

	// output dials
	pinMode(H_DIAL, OUTPUT);
	pinMode(M_DIAL, OUTPUT);

}

void loop() {

	displayTime();

	switchState = checkSwitch();
	if (switchState == SET_TIME) userSetTime();
	else if (switchState == SET_DATE) userSetDate();


	buttonReading = checkButtons();
	if (buttonReading != lastButtonReading) lastPress = millis();
	else if (millis() - lastPress > DEBOUNCE_DELAY) {
		
		if (buttonReading == H_PRESSED) while (checkButtons() == H_PRESSED) displayDate();
		else if (buttonReading == M_PRESSED) while (checkButtons() == M_PRESSED) displayTemp();
	}

}

void displayTime(){
	analogWrite(H_DIAL, hourFormat12() * 255 / 11);
	analogWrite(M_DIAL, minute() * 255 / 59);

	#ifdef SERIAL
	Serial.print("Time: ");
	if (hourFormat12() < 10) Serial.print("0");
    Serial.print(hourFormat12());
    Serial.print(":");
    Serial.print(minute());
    Serial.println(isAM() ? "AM" : "PM");
    #endif
}

void displayDate(){
	analogWrite(H_DIAL, month() * 255 / 11);
	analogWrite(M_DIAL, day() * 255 / 59);

	#ifdef SERIAL
	Serial.print("Date: ");
	Serial.print(year());
	Serial.print("-");
	Serial.print(month());
	Serial.print("-");
	Serial.println(day());
	#endif
}

int checkSwitch(){
	if (digitalRead(CLOCK_SET)) return SET_TIME;
	else if (digitalRead(DATE_SET)) return SET_DATE;
	else return 0;
}

int checkButtons(){
	return (digitalRead(H_BUTTON) << 1) + digitalRead(M_BUTTON);
}
void userSetTime(){
	int setupButtonState;
	int setupLastPress;
	int setupLastButtonState;
	int setSelection = SET_HOURS;
	Serial.println("Setting time...");

	// time setting loop, exit when switched to different position
	while(checkSwitch() == SET_TIME) {
		// get button state
		setupButtonState = checkButtons();
		// debounce check
		if (setupButtonState != setupLastButtonState) setupLastPress = millis();
		else if (millis() - setupLastPress > DEBOUNCE_DELAY) {
			// minutes and hours set seperately
			/* Setting by hours
			 * note that incrementing by minutes past hours bounds changes the hour
			 * example: going down from 12:00 goes to 11:59, not 12:59
			 */
			if (setSelection == SET_HOURS) {
				// hour(left) button decreases time
				if (setupButtonState == H_PRESSED) {
					adjustTime(-SECS_PER_HOUR);
					displayTime();
					sleep(200);
				// minutes (right) button increases time
				} else if (setupButtonState == M_PRESSED){
					adjustTime(SECS_PER_HOUR);
					displayTime();
					sleep(200);
				// switch modes if both buttons pressed
				} else if (setupButtonState == HM_PRESSED){
					setSelection = SET_MINUTES;
					signalDial(M_DIAL);
					displayTime();
					sleep(100);
				}
			// same as above but changing time by minutes
			} else if (setSelection == SET_MINUTES){
				if (setupButtonState == H_PRESSED) {
					adjustTime(-SECS_PER_MIN);
					displayTime();
					sleep(200);
				} else if (setupButtonState == M_PRESSED){
					adjustTime(SECS_PER_MIN);
					displayTime();
					sleep(200);
				} else if (setupButtonState == HM_PRESSED){
					setSelection = SET_HOURS;
					signalDial(H_DIAL);
					displayTime();
					sleep(100);
				}
			}
			
		}
	}
}

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

void signalDial(int pin){
	int i;
	for (i = 0; i < 3; i++) {
		analogWrite(pin, 0);
		delay(100);
		analogWrite(pin, 255);		
		delay(100);
	}
	return;
}
