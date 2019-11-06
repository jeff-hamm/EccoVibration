
#include "arduino.h"

//#define HAS_POTS
// Vibration sensor 1
		const int VIBRATION_PIN1 = 26;
// Vibration sensor 2
		const int VIBRATION_PIN2 = 13;
// Relay pin
		const int OUTPUT_PIN = 12;

#ifdef HAS_POTS
		// Sensitivity pot
        const int TRIGGER_SENSITIVITY_PIN = 16;
		// Duration pot
        const int DURATION_PIN = 20;
		void setupPot(int pin) {
			pinMode(pin, INPUT);
			pinMode(pin+1, OUTPUT);
			pinMode(pin-1, OUTPUT);
			digitalWrite(pin+1, HIGH);
			digitalWrite(pin-1, LOW);
		}
#else
		// number of seconds the relay is held open after a vibration
		const int COUNTDOWN_SECONDS = 10;

		// number of vibration triggers needed in VIBRATION_WINDOW_MS milliseconds to trigger the relay
		const long VIBRATION_SENSITIVITY_THRESHOLD = 1;

		
#endif

		// constants
		const uint32_t SECONDS_PER_TICK = 1000;
        const int VIBRATION_WINDOW_MS = 1000;

		// global state
        uint32_t countDownTime = 0;
        int vibrationSum = 0;
		long currentThreshold = 0;


        void setup()
        {
			delay(500);
            Serial.begin(9600);
            Serial.write("Hello");
			pinMode(VIBRATION_PIN1, INPUT_PULLUP);
			pinMode(VIBRATION_PIN2, INPUT_PULLUP);
#ifdef HAS_POTS
			setupPot(TRIGGER_SENSITIVITY_PIN);
            setupPot(DURATION_PIN);
#endif
            pinMode(OUTPUT_PIN, OUTPUT);
			digitalWrite(OUTPUT_PIN, LOW);
        }

        void loop()
        {
            uint32_t delta = elapsedTime();
			if (isVibrating(delta))
                beginCountDown();
            else
                updateCountDown(delta);

        }


		uint32_t countdownDurationSeconds = COUNTDOWN_SECONDS;
        uint32_t elapsedTime()
        {
			static uint32_t loopTime = 0;
			static uint32_t displaySeconds = 0;
			static bool lastBlink = LOW;
            uint32_t delta = millis() - loopTime;
            loopTime += delta;
            uint32_t newSeconds = loopTime / SECONDS_PER_TICK;
            if (displaySeconds != newSeconds)
            {
//				digitalWrite(LED_PIN, lastBlink != lastBlink);
				getVibrationThreshold();
				updateDuration();
				if (countDownTime > 0)
                    Serial.printf("Countdown: %i...\n", countDownTime/SECONDS_PER_TICK);
                else
                    Serial.printf("No vibrations - Sum: %i vs Threshold %i with timer %i seconds\n", vibrationSum, currentThreshold, countdownDurationSeconds);
                displaySeconds = newSeconds;
            }

            return delta;
        }

        
        bool isVibrating(uint32_t tdelta)
        {
	        static uint8_t vibrations[VIBRATION_WINDOW_MS];
			static bool lastValue = false; 
			static int vibrationIndex = 0;
            // clear/reuse values from time delta, but, 
            // increment the count in the current index if
            // no clock change.
            for (int i = 0; i < (int)tdelta; i++)
            {
				vibrationIndex++;
                vibrationIndex = vibrationIndex % VIBRATION_WINDOW_MS;
                // subtract off the old value and reset this index
                vibrationSum -= vibrations[vibrationIndex];
                vibrations[vibrationIndex] = 0;
            }

            // If we've manually reset the counter, we'll get negative values for a bit
            if (vibrationSum < 0)
                vibrationSum = 0;

			// check both sensors
            bool value = digitalRead(VIBRATION_PIN1) &&  digitalRead(VIBRATION_PIN2);
			
            bool r = false;
            // falling edge
            if (lastValue && !value)
            {
                vibrationSum++;
                vibrations[vibrationIndex]++;
                long threshold = getVibrationThreshold();
                if (vibrationSum > threshold)
                {
                    Serial.printf("Vibration!  Sum: %i Threshold: %i\n", vibrationSum,threshold);
                    // reset counter
                    vibrationSum = 0;
                    r = true;
                }
            }
            lastValue = value;
            return r;
        }

        long getVibrationThreshold()
        {
#ifdef HAS_POTS
            int val = analogRead(TRIGGER_SENSITIVITY_PIN/);
            long newThreshold = map(val, 0, 1023, MIN_VIBRATION_THRESHOLD,
                MAX_VIBRATION_THRESHOLD);
            if (newThreshold != currentThreshold)
            {
                currentThreshold = newThreshold;
                Serial.printf("New vibration threshold %i\n", currentThreshold);
            }

			return newThreshold;
#else
						return currentThreshold = VIBRATION_SENSITIVITY_THRESHOLD;
#endif
        }

        const int MIN_COUNTDOWN_SECONDS = 10;
        const int MAX_COUNTDOWN_SECONDS = 60 * 60 * 24;

        
        void beginCountDown()
        {
            Serial.printf("Starting countdown for %i seconds\n", countdownDurationSeconds);
            countDownTime = countdownDurationSeconds * SECONDS_PER_TICK;
            digitalWrite(OUTPUT_PIN, true);
        }

        void updateCountDown(uint32_t delta)
        {
            if (countDownTime > delta)
                countDownTime -= delta;
            else
                countDownTime = 0;

            digitalWrite(OUTPUT_PIN, countDownTime > 0);
        }

		uint32_t updateDuration() {
#ifdef HAS_POTS
			int val = analogRead(DURATION_PIN);
			Serial.println(val);
            // Possibly a problem, but, make it do something without a pot
            uint32_t seconds  = map(val,0,1023,MIN_COUNTDOWN_SECONDS,MAX_COUNTDOWN_SECONDS);
			if(seconds != countdownDurationSeconds) {
				Serial.printf("Updated countdown timer to %i seconds", countdownDurationSeconds);
				countdownDurationSeconds = seconds;
			}
			return countdownDurationSeconds;
#else
			return countdownDurationSeconds = COUNTDOWN_SECONDS;
#endif // HAS_POTS

		}

 
