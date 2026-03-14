#include <Arduino.h>
#include <Servo.h>
#include <Bounce2.h>

// ================== CONFIG ==================
// MODE 1: toggle servos open / close
// MODE 2: solenoids only (twist once)
// MODE 3: close -> twist N times -> FULL open (drop) -> wait -> PARTIAL open (ready)
// MODE 4: just PARTIAL open (ready)
// MODE 5: 1st click close -> 2nd click twist N times -> 3rd click FULL open -> 4th click PARTIAL open -> repeat
// MODE 6: like MODE 5, but after FULL open it waits 1s and returns to IN automatically (no extra button push)
#define MODE 6

// Twist cycles (MODE 3/5/6)
#define SOL_CYCLES 3

// Set to 1 if MOSFET is active-LOW
#define SOLENOID_ACTIVE_LOW 0
// ===========================================

Servo servo1;
Servo servo2;
Bounce2::Button button;

// ---- Pins ----
static const uint8_t SERVO1_PIN  = 9;
static const uint8_t SERVO2_PIN  = 10;
static const uint8_t BUTTON_PIN  = 3;

static const uint8_t SOL1_PIN = 7;
static const uint8_t SOL2_PIN = 8;

// ---- Servo angles ----
// FULL_OPEN: completely open so LED can drop
// IN: "ready" position (partially closed) to hold next LED
// OUT: clamp/closed
static const uint8_t SERVO1_FULL_OPEN = 76;
static const uint8_t SERVO2_FULL_OPEN = 76;

static const uint8_t SERVO1_IN  = 88;
static const uint8_t SERVO1_OUT = 92;

static const uint8_t SERVO2_IN  = 89;
static const uint8_t SERVO2_OUT = 95;

// ---- Timing ----
static const uint16_t SERVO_SETTLE_MS = 1400;
static const uint16_t CLAMP_HOLD_MS   = 120;

static const uint16_t SOLENOID_ON_MS  = 300;
static const uint16_t BETWEEN_SOL_MS  = 80;
static const uint16_t BETWEEN_PUSH_MS = 500;

static const uint16_t DROP_WAIT_MS    = 1500; // used in MODE 3

// MODE 6 extra timing: auto-return from FULL_OPEN to IN
static const uint16_t MODE6_AUTO_RETURN_MS = 200;

// ---- Solenoid polarity ----
#if SOLENOID_ACTIVE_LOW
static const uint8_t SOL_ON  = LOW;
static const uint8_t SOL_OFF = HIGH;
#else
static const uint8_t SOL_ON  = HIGH;
static const uint8_t SOL_OFF = LOW;
#endif

// ---- State ----
static bool servosClosed = false;     // used in MODE 1
static uint8_t mode5_step = 0;        // 0..2 (and reused for mode 6)

void attachServos() {
	servo1.attach(SERVO1_PIN);
	servo2.attach(SERVO2_PIN);
}

void setServos(uint8_t s1, uint8_t s2) {
	servo1.write(s1);
	servo2.write(s2);
}

void moveServosTo(uint8_t s1, uint8_t s2) {
	setServos(s1, s2);
	delay(SERVO_SETTLE_MS);
}

void moveServosIn() {
	moveServosTo(SERVO1_IN, SERVO2_IN);
	servosClosed = false;
}

void moveServosOut() {
	moveServosTo(SERVO1_OUT, SERVO2_OUT);
	servosClosed = true;
}

void moveServosFullOpen() {
	moveServosTo(SERVO1_FULL_OPEN, SERVO2_FULL_OPEN);
	servosClosed = false;
}

void toggleServos() {
	if (servosClosed) {
		moveServosIn();
	} else {
		moveServosOut();
	}
}

void initSolenoids() {
	pinMode(SOL1_PIN, OUTPUT);
	pinMode(SOL2_PIN, OUTPUT);

	digitalWrite(SOL1_PIN, SOL_OFF);
	digitalWrite(SOL2_PIN, SOL_OFF);
}

void pulseSolenoid(uint8_t pin, uint16_t on_ms) {
	digitalWrite(pin, SOL_ON);
	delay(on_ms);
	digitalWrite(pin, SOL_OFF);
}

void twistOnce() {
	pulseSolenoid(SOL1_PIN, SOLENOID_ON_MS);
	delay(BETWEEN_SOL_MS);
	pulseSolenoid(SOL2_PIN, SOLENOID_ON_MS);
}

void twistCycles(uint8_t cycles) {
	for (uint8_t i = 0; i < cycles; ++i) {
		twistOnce();
		if (i + 1 < cycles) {
			delay(BETWEEN_PUSH_MS);
		}
	}
}

// ---------- Mode behaviors ----------

void modeSolenoidsOnly() {
	twistOnce();
}

void modeClampTwistReleaseWithDrop() {
	moveServosOut();
	delay(CLAMP_HOLD_MS);

	twistCycles((uint8_t)SOL_CYCLES);

	moveServosFullOpen();
	delay(DROP_WAIT_MS);

	moveServosIn();
}

void modeOpenOnly() {
	moveServosIn();
}

void mode5StepAdvance() {
	// MODE 5 sequence:
	// start state: IN (ready)
	// step 0: close (OUT) - clamp LED
	// step 1: twist N cycles, then FULL_OPEN (drop)
	// step 2: back to IN (ready) [requires another click]
	switch (mode5_step) {
		case 0:
			moveServosOut();
			delay(CLAMP_HOLD_MS);
			mode5_step = 1;
			break;

		case 1:
			twistCycles((uint8_t)SOL_CYCLES);
			delay(CLAMP_HOLD_MS);

			moveServosFullOpen();     // release / drop
			mode5_step = 2;
			break;

		case 2:
		default:
			moveServosIn();           // ready for next LED insert
			mode5_step = 0;
			break;
	}
}

void mode6StepAdvance() {
	// MODE 6: same as MODE 5, except step 2 happens automatically after FULL_OPEN (wait 1s, then IN)
	// start state: IN (ready)
	// step 0: click -> OUT (clamp)
	// step 1: click -> twist N cycles, FULL_OPEN, wait 1s, then IN automatically, reset to step 0
	switch (mode5_step) {
		case 0:
			moveServosOut();
			delay(CLAMP_HOLD_MS);
			mode5_step = 1;
			break;

		case 1:
		default:
			twistCycles((uint8_t)SOL_CYCLES);
			delay(CLAMP_HOLD_MS);

			moveServosFullOpen();              // release / drop
			delay(MODE6_AUTO_RETURN_MS);       // auto wait
			moveServosIn();                    // back to "ready" automatically

			mode5_step = 0;
			break;
	}
}

void initButton() {
	pinMode(BUTTON_PIN, INPUT_PULLUP);

	button.attach(BUTTON_PIN);
	button.interval(20);
	button.setPressedState(LOW);
}

void setup() {
	attachServos();
	initButton();

#if (MODE == 2) || (MODE == 3) || (MODE == 5) || (MODE == 6)
	initSolenoids();
#endif

	// Safe start
#if (MODE == 1) || (MODE == 3) || (MODE == 4) || (MODE == 5) || (MODE == 6)
	moveServosIn(); // start at "ready" (partial open)
#endif

#if (MODE == 5) || (MODE == 6)
	mode5_step = 0; // first click will close
#endif
}

void loop() {
	button.update();

	if (!button.fell()) {
		return;
	}

#if MODE == 1
	toggleServos();
#elif MODE == 2
	modeSolenoidsOnly();
#elif MODE == 3
	modeClampTwistReleaseWithDrop();
#elif MODE == 4
	modeOpenOnly();
#elif MODE == 5
	mode5StepAdvance();
#elif MODE == 6
	mode6StepAdvance();
#else
	#error "Invalid MODE. Use MODE 1, 2, 3, 4, 5, or 6."
#endif
}