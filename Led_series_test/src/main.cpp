#include <Arduino.h>
#include <FastLED.h>

// ----------------- Configuration -----------------
#define DATA_PIN    6
#define NUM_LEDS    128
#define LED_TYPE    WS2812B    // PL9823 compatible
#define COLOR_ORDER GRB
#define BRIGHTNESS  80


/*
==================== MODES ====================

1 - Chase Dot
    נקודה בודדת רצה לאורך כל השרשרת.
    טוב לבדיקה שהסדר והכיווניות של הלדים נכונים.

2 - Sparkle Field
    ניצוצות אקראיים על כל השרשרת (אפקט זיקוקים).

3 - Rocket & Explosion
    "טיל" רץ לאורך השרשרת ואז מתפוצץ באפקט רדיאלי.

4 - Index Test
    כל לד נדלק באדום אחד-אחד לפי אינדקס.
    טוב לבדיקה ידנית של מיקום.

6 - Single LED Hue Sweep
    מיועד לבדיקה של לד בודד.
    עובר בהדרגה על כל הצבעים (Hue 0-255).

7 - Blink All
    כל הלדים נדלקים ונכבים בקצב של 1Hz (פעם בשנייה).
    טוב לבדיקה מהירה של אספקת מתח ויציבות כללית.

8 - Two Columns Mirror Pairs (Serpentine)
    יש 2 טורים של 16 (סה"כ 32) מחוברים בזיגזג:
    טור 1 עולה (1..16), טור 2 יורד (32..17).
    מדליק זוגות באותו גובה: 1&32, 2&31, ... 16&17, ואז חוזר אחורה.
    הצבע מתחלף בהדרגה.

================================================
*/

#define MODE 10   // <- בחר מצב: 1..7

CRGB leds[NUM_LEDS];

// ----------------- Helpers -----------------
void showDelay(uint16_t ms)
{
    FastLED.show();
    delay(ms);
}

void allOff()
{
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
}

void fadeAll(uint8_t amount)
{
    for (int i = 0; i < NUM_LEDS; i++)
        leds[i].fadeToBlackBy(amount);
}

int wrapIndex(int idx)
{
    while (idx < 0) idx += NUM_LEDS;
    while (idx >= NUM_LEDS) idx -= NUM_LEDS;
    return idx;
}

static inline uint16_t idxFromColRow(uint8_t col, uint8_t row, uint8_t rows)
{
    // row: 0=למטה, rows-1=למעלה
    // col 0 עולה, col 1 יורד, col 2 עולה, col 3 יורד ... (סרפנטינה)
    const uint16_t base = (uint16_t)col * rows;

    if ((col & 1) == 0) {
        // עמודה זוגית: עולה
        return base + row;                 // 0-based
    } else {
        // עמודה אי-זוגית: יורדת
        return base + (rows - 1 - row);    // 0-based
    }
}

void mode10_PlasmaTunnel(uint16_t stepMs)
{
	const uint8_t ROWS = 16;
	const uint8_t COLS = 8;
	const uint16_t TOTAL = ROWS * COLS;

	if (NUM_LEDS < TOTAL)
	{
		allOff();
		delay(500);
		return;
	}

	static uint16_t t = 0;

	for (uint8_t row = 0; row < ROWS; row++)
	{
		for (uint8_t col = 0; col < COLS; col++)
		{
			// מרחק מהמרכז (ליצירת אפקט מנהרה)
			float centerDist = abs((int)col - 1.5f);

			// גל פלזמה אנכי
			float wave1 = sin((row * 0.5f) + (t * 0.15f));

			// גל עומק מהמרכז החוצה
			float wave2 = cos((centerDist * 2.0f) - (t * 0.1f));

			// שילוב
			float combined = wave1 + wave2;

			// נרמול
			uint8_t brightness = (uint8_t)((combined + 2.0f) * 63.0f); // 0-255

			// גוון משתנה לפי שורה + זמן
			uint8_t hue = (uint8_t)(t * 2 + row * 8 + col * 10);

			uint16_t idx = idxFromColRow(col, row, ROWS);
			leds[idx] = CHSV(hue, 255, brightness);
		}
	}

	FastLED.show();
	delay(stepMs);
	t++;
}



void mirrorPairsBounce(uint16_t stepMs)
{
    const uint8_t ROWS = 16;
    const uint8_t COLS = 4;
    const uint16_t TOTAL = (uint16_t)ROWS * COLS;

    if (NUM_LEDS < TOTAL) {
        allOff();
        delay(500);
        return;
    }

    static int row = 0;     // 0..ROWS-1
    static int dir = 1;     // +1 / -1
    static uint8_t hue = 0;

    fill_solid(leds, NUM_LEDS, CRGB::Black);

    // מדליק זוגות מראה לכל "שכבה" פנימה
    for (uint8_t c = 0; c < (COLS / 2); ++c) {
        const uint8_t leftCol  = c;
        const uint8_t rightCol = (COLS - 1 - c);

        const uint16_t li = idxFromColRow(leftCol,  (uint8_t)row, ROWS);
        const uint16_t ri = idxFromColRow(rightCol, (uint8_t)row, ROWS);

        // אפשר לשחק עם ההיסט כדי שכל זוג יקבל צבע קצת אחר
        const uint8_t pairHue = hue + (uint8_t)(c * 25);

        leds[li] = CHSV(pairHue, 255, 255);
        leds[ri] = CHSV(pairHue + 10, 255, 255);
    }

    FastLED.show();
    delay(stepMs);

    hue += 3;

    row += dir;
    if (row >= (ROWS - 1)) { row = ROWS - 1; dir = -1; }
    if (row <= 0)          { row = 0;        dir = +1; }
}
// ----------------- Effects -----------------

// 1) Confirms order + chain: a single bright dot runs across all LEDs
void chaseDot(uint8_t rounds, uint8_t hue, uint16_t stepMs)
{
    allOff();
    for (uint8_t r = 0; r < rounds; r++)
    {
        for (int i = 0; i < NUM_LEDS; i++)
        {
            fadeAll(80);
            // leds[i] = CHSV(hue, 255, 255);
            leds[i] = CRGB::White;
            showDelay(stepMs);
        }
    }
    allOff();
}

// 2) Sparkles across the whole strip (like fireworks embers)
void sparkleField(uint16_t durationMs, uint8_t sparksPerFrame, uint16_t frameMs)
{
    allOff();
    uint32_t start = millis();

    while (millis() - start < durationMs)
    {
        fadeAll(40);

        for (uint8_t s = 0; s < sparksPerFrame; s++)
        {
            int idx = random16(NUM_LEDS);
            leds[idx] += CHSV(random8(), 200, 255);
        }

        showDelay(frameMs);
    }
    allOff();
}

// 3) A "rocket" travels, then "explodes" from a random center
void rocketAndExplosion()
{
    allOff();

    // Rocket travel
    int pos = 0;
    uint8_t rocketHue = 160; // blue-ish
    for (pos = 0; pos < NUM_LEDS; pos++)
    {
        fadeAll(60);
        leds[pos] = CHSV(rocketHue, 255, 255);

        // small "fuse" behind the rocket
        int tail = pos - 1;
        if (tail >= 0) leds[tail] += CHSV(0, 255, 80);

        showDelay(35);
    }

    // Explosion center
    int center = random16(NUM_LEDS);
    uint8_t baseHue = random8();

    // Explosion pulses: expand outward from center
    for (int radius = 0; radius <= NUM_LEDS / 2; radius++)
    {
        fadeAll(25);

        int a = wrapIndex(center - radius);
        int b = wrapIndex(center + radius);

        leds[a] += CHSV(baseHue + radius * 10, 255, 255);
        leds[b] += CHSV(baseHue + radius * 10 + 20, 255, 255);

        showDelay(40);
    }

    // Fade out
    for (int k = 0; k < 25; k++)
    {
        fadeAll(25);
        showDelay(30);
    }

    allOff();
}

// 4) Index test: each LED turns red one-by-one
void showIndexTest(uint16_t stepMs)
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        leds[i] = CRGB::Red;
        FastLED.show();
        delay(stepMs);
    }
}

// 6) Single LED health test: assume only ONE LED exists, sweep smoothly through all hues
void singleLedHueSweep(uint8_t ledIndex, uint16_t stepMs, uint8_t cycles)
{
    allOff();
    ledIndex = (ledIndex < NUM_LEDS) ? ledIndex : 0;

    for (uint8_t c = 0; c < cycles; c++)
    {
        for (uint16_t h = 0; h <= 255; h++)
        {
            fill_solid(leds, NUM_LEDS, CRGB::Black);
            leds[ledIndex] = CHSV((uint8_t)h, 255, 255);
            showDelay(stepMs);
        }
    }

    allOff();
}

// 7) Blink all LEDs on/off at 1 Hz (once per second)
void blinkAll(uint16_t seconds, CRGB color)
{
    for (uint16_t s = 0; s < seconds; s++)
    {
        fill_solid(leds, NUM_LEDS, color);
        FastLED.show();
        delay(500);

        fill_solid(leds, NUM_LEDS, CRGB::Black);
        FastLED.show();
        delay(500);
    }
    allOff();
}

static inline uint8_t clampAdd(uint8_t v, int16_t add)
{
	int16_t x = (int16_t)v + add;
	if (x < 0)   x = 0;
	if (x > 255) x = 255;
	return (uint8_t)x;
}

void mode9_MirrorPulseWave(uint16_t stepMs)
{
	const uint8_t ROWS = 16;
	const uint8_t COLS = 4;
	const uint16_t TOTAL = (uint16_t)ROWS * COLS;

	if (NUM_LEDS < TOTAL) {
		allOff();
		delay(500);
		return;
	}

	// מצב אנימציה
	static uint16_t t = 0;      // זמן
	static int8_t dir = 1;      // כיוון גל עולה/יורד
	static int8_t head = 0;     // שורה מובילה

	// עדכון תנועה
	head += dir;
	if (head >= (int8_t)(ROWS - 1)) { head = ROWS - 1; dir = -1; }
	if (head <= 0)                 { head = 0;        dir = +1; }

	// בסיס: כבה
	fill_solid(leds, NUM_LEDS, CRGB::Black);

	// "פעימה" מהמרכז החוצה: centerPair = בין הטורים 1 ו-2
	// radius גדל/קטן עם הזמן (מחזורי)
	const uint8_t maxR = (COLS / 2);      // עבור COLS=4 => 2 רמות: r=0,1
	const uint8_t radius = (uint8_t)((t / 6) % (maxR * 2)); // 0..3
	const uint8_t r = (radius < maxR) ? radius : (uint8_t)((maxR * 2 - 1) - radius); // 0,1,1,0...

	// צבע זורם לאורך הציר האנכי
	const uint8_t baseHue = (uint8_t)(t / 2);

	for (uint8_t row = 0; row < ROWS; ++row) {

		// מרחק מה"ראש" (יוצר זנב)
		int8_t d = (int8_t)row - head;
		if (d < 0) d = -d;

		// בהירות לזנב: קרוב ל-head בהיר, רחוק חשוך
		// (0->255, 1->180, 2->120, 3->70, 4+->30)
		uint8_t v = 30;
		if      (d == 0) v = 255;
		else if (d == 1) v = 180;
		else if (d == 2) v = 120;
		else if (d == 3) v = 70;

		// צבע משתנה לפי גובה (שורה) וזמן
		uint8_t hue = baseHue + row * 6;

		// ציור כל 4 הטורים עם אפקט "פעימה" מהמרכז החוצה
		for (uint8_t col = 0; col < COLS; ++col) {

			// מרחק מהמרכז (ב-4 טורים: col 1 ו-2 הם מרכז)
			uint8_t distFromCenter = (col < 2) ? (1 - col) : (col - 2); // col0->1, col1->0, col2->0, col3->1

			// אם dist תואם ל-r (רדיוס הפעימה) -> תן בוסט בהירות/סטורציה
			uint8_t vv = v;
			uint8_t sat = 255;

			if (distFromCenter == r) {
				vv = clampAdd(vv, 70);
				sat = 255;
			} else {
				// ריכוך הרקע
				vv = (uint8_t)((uint16_t)vv * 75 / 100);
				sat = 220;
			}

			// זוגות מראה מקבלים גוון מעט שונה -> נראה "עמוק"
			if (col == 0 || col == 3) hue += 10;

			const uint16_t i = idxFromColRow(col, row, ROWS);
			leds[i] = CHSV(hue, sat, vv);
		}

		// היילייט מיוחד לזוגות מראה בדיוק בשורת ה-head (ניצוץ חד)
		if (row == (uint8_t)head) {
			const uint16_t i0 = idxFromColRow(0, row, ROWS);
			const uint16_t i3 = idxFromColRow(3, row, ROWS);
			const uint16_t i1 = idxFromColRow(1, row, ROWS);
			const uint16_t i2 = idxFromColRow(2, row, ROWS);

			leds[i0] += CHSV(baseHue + 40, 40, 120);
			leds[i3] += CHSV(baseHue + 40, 40, 120);
			leds[i1] += CHSV(baseHue + 10, 40, 80);
			leds[i2] += CHSV(baseHue + 10, 40, 80);
		}
	}

	FastLED.show();
	delay(stepMs);
	t++;
}

// ----------------- Arduino -----------------
void setup()
{
    delay(200);
    FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);

    random16_add_entropy(analogRead(A0)); // leave A0 floating if possible
    allOff();
}

void loop()
{
    switch (MODE)
    {
        case 1:
            chaseDot(2, 0, 30);
            break;

        case 2:
            sparkleField(1500, 3, 25);
            break;

        case 3:
            rocketAndExplosion();
            break;

        case 4:
            showIndexTest(500);
            break;

        case 6:
            singleLedHueSweep(/*ledIndex=*/0, /*stepMs=*/12, /*cycles=*/4);
            break;

        case 7:
            blinkAll(/*seconds=*/2000, /*color=*/CRGB::White);
            break;
        case 8:
            mirrorPairsBounce(/*stepMs=*/5);
            break;
        case 9:
            mode9_MirrorPulseWave(/*stepMs=*/20);
            break;
        case 10:
            mode10_PlasmaTunnel(/*stepMs=*/20);
            break;
        default:
            allOff();
            delay(500);
            break;
    }

    delay(10);
}