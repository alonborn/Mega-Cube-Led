#include <Arduino.h>
#include <FastLED.h>

// ----------------- Configuration -----------------
#define DATA_PIN    6          // Change if needed
#define NUM_LEDS    2
#define LED_TYPE    WS2812B    // PL9823 compatible
#define COLOR_ORDER GRB
#define BRIGHTNESS  80

CRGB leds[NUM_LEDS];

// ----------------- Helpers -----------------
void allOff()
{
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
}

void quickFlash(uint8_t index, const CRGB& color)
{
    leds[index] = color;
    FastLED.show();
    delay(80);
    leds[index] = CRGB::Black;
    FastLED.show();
}

// ----------------- Effects -----------------
void colorChase()
{
    leds[0] = CRGB::Red;
    leds[1] = CRGB::Black;
    FastLED.show();
    delay(300);

    leds[0] = CRGB::Black;
    leds[1] = CRGB::Green;
    FastLED.show();
    delay(300);

    leds[0] = CRGB::Blue;
    leds[1] = CRGB::Blue;
    FastLED.show();
    delay(300);

    allOff();
}

void sparkleBurst()
{
    for (int i = 0; i < 6; i++)
    {
        leds[0] = CHSV(random8(), 255, 255);
        leds[1] = CHSV(random8(), 255, 255);
        FastLED.show();
        delay(100);

        allOff();
        delay(50);
    }
}

void rocketAndPop()
{
    // Rocket rise (LED 0)
    for (uint8_t b = 0; b < 255; b += 15)
    {
        leds[0] = CHSV(160, 255, b);   // blue rocket
        leds[1] = CHSV(0, 255, b / 6); // dim red fuse
        FastLED.show();
        delay(20);
    }

    // Explosion
    for (int i = 0; i < 12; i++)
    {
        leds[0] = CHSV(random8(), 255, 255);
        leds[1] = CHSV(random8(), 255, 255);
        FastLED.show();
        delay(40);
    }

    // Fade out
    for (int f = 255; f > 0; f -= 15)
    {
        leds[0].nscale8_video(f);
        leds[1].nscale8_video(f);
        FastLED.show();
        delay(25);
    }

    allOff();
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
    colorChase();      // verifies data order
    sparkleBurst();    // random sparkles
    rocketAndPop();    // fireworks effect
    delay(500);
}
