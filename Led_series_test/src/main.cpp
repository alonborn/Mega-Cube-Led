#include <Arduino.h>
#include <FastLED.h>
#include <math.h>

// =====================================================
// Configuration
// =====================================================
#define DATA_PIN_RIGHT  6   // חצי ימני: x=2,3
#define DATA_PIN_LEFT   7   // חצי שמאלי: x=0,1

#define LED_TYPE        WS2811
#define COLOR_ORDER     GRB
#define BRIGHTNESS      255

#define SIZE_X          4
#define SIZE_Y          4
#define SIZE_Z          16

#define LEDS_PER_HALF   (2 * SIZE_Y * SIZE_Z)   // 128
#define NUM_LEDS        (SIZE_X * SIZE_Y * SIZE_Z) // 256

#define MODE            13

CRGB leds[NUM_LEDS]; 


// =====================================================
// Helpers
// =====================================================
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
    for (uint16_t i = 0; i < NUM_LEDS; i++)
    {
        leds[i].fadeToBlackBy(amount);
    }
}

int wrapIndex(int idx)
{
    while (idx < 0) idx += NUM_LEDS;
    while (idx >= NUM_LEDS) idx -= NUM_LEDS;
    return idx;
}

static inline uint8_t clampAdd(uint8_t v, int16_t add)
{
    int16_t x = (int16_t)v + add;
    if (x < 0) x = 0;
    if (x > 255) x = 255;
    return (uint8_t)x;
}

// =====================================================
// 3D Mapping
// =====================================================
// Coordinates:
// x = 0..3  (left -> right)
// y = 0..3  (front -> back)
// z = 0..15 (bottom -> top)
//
// LEFT half  (pin LEFT)  = x 0..1
// RIGHT half (pin RIGHT) = x 2..3
//
// Each half is mapped as a serpentine 2x4 base:
// localX = 0..1, y = 0..3
//
// Cell order inside one half:
// y=0: x 0 -> 1
// y=1: x 1 -> 0
// y=2: x 0 -> 1
// y=3: x 1 -> 0
//
// Vertical direction also serpentine by column index:
// even pillar index:  bottom -> top
// odd pillar index:   top -> bottom
// =====================================================
static inline uint16_t idxFromXYZ(uint8_t x, uint8_t y, uint8_t z)
{
    if (x >= SIZE_X || y >= SIZE_Y || z >= SIZE_Z)
    {
        return 0;
    }

    // Determine which half
    const bool isRightHalf = (x >= 2);

    const uint8_t localX = isRightHalf ? (x - 2) : x;   // 0..1 inside the half
    const uint16_t halfBase = isRightHalf ? 0 : LEDS_PER_HALF;
    // note:
    // leds[0..127]   -> RIGHT half on pin 6
    // leds[128..255] -> LEFT half on pin 7

    uint8_t pillarInHalf;
    if ((y & 1) == 0)
    {
        // even row in base: 0 -> 1
        pillarInHalf = y * 2 + localX;
    }
    else
    {
        // odd row in base: 1 -> 0
        pillarInHalf = y * 2 + (1 - localX);
    }

    const uint16_t pillarBase = (uint16_t)pillarInHalf * SIZE_Z;

    if ((pillarInHalf & 1) == 0)
    {
        // even pillar: bottom -> top
        return halfBase + pillarBase + z;
    }
    else
    {
        // odd pillar: top -> bottom
        return halfBase + pillarBase + (SIZE_Z - 1 - z);
    }
}

void setLedAt(uint8_t x, uint8_t y, uint8_t z, const CRGB& color)
{
    if (x >= SIZE_X || y >= SIZE_Y || z >= SIZE_Z) return;
    leds[idxFromXYZ(x, y, z)] = color;
}

void addLedAt(uint8_t x, uint8_t y, uint8_t z, const CRGB& color)
{
    if (x >= SIZE_X || y >= SIZE_Y || z >= SIZE_Z) return;
    leds[idxFromXYZ(x, y, z)] += color;
}

// =====================================================
// Effects 1-7: chain-based over all 256 LEDs
// =====================================================
void chaseDot(uint8_t rounds, uint8_t hue, uint16_t stepMs)
{
    allOff();

    for (uint8_t r = 0; r < rounds; r++)
    {
        for (uint16_t i = 0; i < NUM_LEDS; i++)
        {
            fadeAll(80);
            leds[i] = CHSV(hue, 255, 255);
            showDelay(stepMs);
        }
    }

    allOff();
}

void sparkleField(uint16_t durationMs, uint8_t sparksPerFrame, uint16_t frameMs)
{
    allOff();
    const uint32_t start = millis();

    while (millis() - start < durationMs)
    {
        fadeAll(40);

        for (uint8_t s = 0; s < sparksPerFrame; s++)
        {
            const uint16_t idx = random16(NUM_LEDS);
            leds[idx] += CHSV(random8(), 200, 255);
        }

        showDelay(frameMs);
    }

    allOff();
}

void rocketAndExplosion()
{
    allOff();

    const uint8_t rocketHue = 160;

    for (int pos = 0; pos < NUM_LEDS; pos++)
    {
        fadeAll(60);
        leds[pos] = CHSV(rocketHue, 255, 255);

        const int tail = pos - 1;
        if (tail >= 0)
        {
            leds[tail] += CHSV(0, 255, 80);
        }

        showDelay(18);
    }

    const int center = random16(NUM_LEDS);
    const uint8_t baseHue = random8();

    for (int radius = 0; radius <= NUM_LEDS / 2; radius++)
    {
        fadeAll(25);

        const int a = wrapIndex(center - radius);
        const int b = wrapIndex(center + radius);

        leds[a] += CHSV(baseHue + radius * 8, 255, 255);
        leds[b] += CHSV(baseHue + radius * 8 + 20, 255, 255);

        showDelay(18);
    }

    for (uint8_t k = 0; k < 20; k++)
    {
        fadeAll(25);
        showDelay(25);
    }

    allOff();
}

void showIndexTest(uint16_t stepMs)
{
    for (uint16_t i = 0; i < NUM_LEDS; i++)
    {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        leds[i] = CRGB::Red;
        FastLED.show();
        delay(stepMs);
    }
}

void singleLedHueSweep(uint16_t ledIndex, uint16_t stepMs, uint8_t cycles)
{
    allOff();

    if (ledIndex >= NUM_LEDS)
    {
        ledIndex = 0;
    }

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

void blinkAll(uint16_t seconds, const CRGB& color)
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

// =====================================================
// 3D effects
// =====================================================

// 8) Horizontal planes moving up/down
void mode8_PlaneBounce(uint16_t stepMs)
{
    static int z = 0;
    static int dir = 1;
    static uint8_t hue = 0;

    fill_solid(leds, NUM_LEDS, CRGB::Black);

    for (uint8_t x = 0; x < SIZE_X; x++)
    {
        for (uint8_t y = 0; y < SIZE_Y; y++)
        {
            setLedAt(x, y, (uint8_t)z, CHSV(hue + x * 10 + y * 15, 255, 255));
        }
    }

    FastLED.show();
    delay(stepMs);

    hue += 3;
    z += dir;

    if (z >= (SIZE_Z - 1))
    {
        z = SIZE_Z - 1;
        dir = -1;
    }
    if (z <= 0)
    {
        z = 0;
        dir = +1;
    }
}

// 9) Expanding cube wave from center
void mode9_CenterPulse3D(uint16_t stepMs)
{
    static uint16_t t = 0;

    fill_solid(leds, NUM_LEDS, CRGB::Black);

    const float cx = 1.5f;
    const float cy = 1.5f;
    const float cz = 7.5f;

    for (uint8_t x = 0; x < SIZE_X; x++)
    {
        for (uint8_t y = 0; y < SIZE_Y; y++)
        {
            for (uint8_t z = 0; z < SIZE_Z; z++)
            {
                const float dx = x - cx;
                const float dy = y - cy;
                const float dz = (z - cz) * 0.35f;

                const float dist = sqrtf(dx * dx + dy * dy + dz * dz);

                const float wave = sinf(dist * 2.2f - t * 0.18f);
                int brightness = (int)((wave + 1.0f) * 120.0f);

                if (brightness < 0) brightness = 0;
                if (brightness > 255) brightness = 255;

                const uint8_t hue = (uint8_t)(t * 2 + z * 6 + (x + y) * 10);

                setLedAt(x, y, z, CHSV(hue, 255, (uint8_t)brightness));
            }
        }
    }

    FastLED.show();
    delay(stepMs);
    t++;
}

// 10) Really "wow": 3D helix / vortex
void mode10_Vortex3D(uint16_t stepMs)
{
    static uint16_t t = 0;

    fill_solid(leds, NUM_LEDS, CRGB::Black);

    const float cx = 1.5f;
    const float cy = 1.5f;

    for (uint8_t z = 0; z < SIZE_Z; z++)
    {
        const float angle = (t * 0.15f) + (z * 0.45f);

        const float px1 = cx + 1.2f * cosf(angle);
        const float py1 = cy + 1.2f * sinf(angle);

        const float px2 = cx + 1.2f * cosf(angle + 3.14159f);
        const float py2 = cy + 1.2f * sinf(angle + 3.14159f);

        for (uint8_t x = 0; x < SIZE_X; x++)
        {
            for (uint8_t y = 0; y < SIZE_Y; y++)
            {
                const float dx1 = x - px1;
                const float dy1 = y - py1;
                const float d1 = sqrtf(dx1 * dx1 + dy1 * dy1);

                const float dx2 = x - px2;
                const float dy2 = y - py2;
                const float d2 = sqrtf(dx2 * dx2 + dy2 * dy2);

                int b1 = (int)(255.0f - d1 * 180.0f);
                int b2 = (int)(255.0f - d2 * 180.0f);

                if (b1 < 0) b1 = 0;
                if (b1 > 255) b1 = 255;
                if (b2 < 0) b2 = 0;
                if (b2 > 255) b2 = 255;

                CRGB c = CHSV((uint8_t)(t * 2 + z * 8), 255, (uint8_t)b1);
                c += CHSV((uint8_t)(t * 2 + z * 8 + 128), 255, (uint8_t)b2);

                leds[idxFromXYZ(x, y, z)] = c;
            }
        }
    }

    FastLED.show();
    delay(stepMs);
    t++;
}

// =====================================================
// Optional debug: test by XYZ
// =====================================================
void mode11_TestXYZ(uint16_t stepMs)
{
    for (uint8_t z = 0; z < SIZE_Z; z++)
    {
        for (uint8_t y = 0; y < SIZE_Y; y++)
        {
            for (uint8_t x = 0; x < SIZE_X; x++)
            {
                fill_solid(leds, NUM_LEDS, CRGB::Black);
                setLedAt(x, y, z, CRGB::Red);
                FastLED.show();
                delay(stepMs);
            }
        }
    }
}

// 13) Simple strip-order test: light one LED at a time in raw index order
void mode13_SimpleRowTest(uint16_t stepMs)
{
    static uint16_t index = 0;

    fill_solid(leds, NUM_LEDS, CRGB::Black);
    leds[index] = CRGB::White;
    FastLED.show();
    delay(stepMs);

    index++;
    if (index >= NUM_LEDS)
    {
        index = 0;
    }
}

// =====================================================
// Arduino
// =====================================================
void setup()
{
    delay(200);

    // Right half first: leds[0..127]
    FastLED.addLeds<LED_TYPE, DATA_PIN_RIGHT, COLOR_ORDER>(leds, 0, LEDS_PER_HALF);

    // Left half second: leds[128..255]
    FastLED.addLeds<LED_TYPE, DATA_PIN_LEFT, COLOR_ORDER>(leds, LEDS_PER_HALF, LEDS_PER_HALF);

    FastLED.setBrightness(BRIGHTNESS);

    random16_add_entropy(analogRead(A0));
    allOff();
}

void loop()
{
    switch (MODE)
    {
        case 1:
            chaseDot(2, 0, 18);
            break;

        case 2:
            sparkleField(1500, 5, 25);
            break;

        case 3:
            rocketAndExplosion();
            break;

        case 4:
            showIndexTest(80);
            break;

        case 6:
            singleLedHueSweep(0, 10, 3);
            break;

        case 7:
            blinkAll(10, CRGB::White);
            break;

        case 8:
            mode8_PlaneBounce(60);
            break;

        case 9:
            mode9_CenterPulse3D(20);
            break;

        case 10:
            mode10_Vortex3D(30);
            break;

        case 11:
            mode11_TestXYZ(120);
            break;

        case 12:
            fill_solid(leds, NUM_LEDS, CRGB::White);
            FastLED.show();
            break;

        case 13:
            mode13_SimpleRowTest(120);
            break;

        default:
            allOff();
            delay(500);
            break;
    }

    delay(10);
}
