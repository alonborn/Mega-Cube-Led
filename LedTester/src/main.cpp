#include <Arduino.h>
#include <FastLED.h>

// ----------------- Configuration -----------------
#define DATA_PIN     6
#define NUM_LEDS     1      // מניחים שיש רק לד אחד
#define LED_TYPE     WS2812B
#define COLOR_ORDER  GRB
#define BRIGHTNESS   80

CRGB leds[NUM_LEDS];

// ----------------- Helpers -----------------
void initLeds()
{
    FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);
}

void setLedHue(uint8_t hue)
{
    leds[0] = CHSV(hue, 255, 255);
    FastLED.show();
}

void clearLed()
{
    leds[0] = CRGB::Black;
    FastLED.show();
}

// ----------------- Mode 6 -----------------
void singleLedHueSweep(uint16_t stepMs)
{
    for (uint16_t h = 0; h <= 255; h++)
    {
        setLedHue((uint8_t)h);
        delay(stepMs);
    }
}

// ----------------- Arduino -----------------
void setup()
{
    delay(200);
    initLeds();
    clearLed();
}

void loop()
{
    singleLedHueSweep(12);   // שנה מהירות כאן אם רוצים
}