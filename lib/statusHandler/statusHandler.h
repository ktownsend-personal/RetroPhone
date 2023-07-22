#ifndef status_h
#define status_h

#include "Arduino.h"
#include "progressModes.h"
#include "FastLED.h"

void rainbowFade();
void blink();
typedef void (*func)();

template <int PIN>
class statusHandler {
  public:
    statusHandler(int framesPerSecond);
    void show(modes mode);
    void run();

  private:
    int fps;
    func visualization;
};

#define NUM_LEDS 1
CRGB leds[NUM_LEDS];

template <int PIN>
statusHandler<PIN>::statusHandler(int framesPerSecond){
  fps = framesPerSecond;
  pinMode(PIN, OUTPUT);
  FastLED.addLeds<WS2811, PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  leds[0] = CRGB::Black;
}

template <int PIN>
void statusHandler<PIN>::show(modes mode){
  visualization = NULL;
  switch(mode){
    case call_idle:
      leds[0] = CRGB::Black;
      break;
    case call_ready:
      leds[0] = CRGB::Green;
      break;
    case call_incoming:
      leds[0] = CRGB::Blue;
      break;
    case call_fail:
      leds[0] = CRGB::Red;
      break;
    case call_busy:
      leds[0] = CRGB::Yellow;
      break;
    case call_connected:
      visualization = rainbowFade;
      break;
    default:
      leds[0] = CRGB::Beige;
      visualization = blink;
      break;
  }
  FastLED.setBrightness(25);
  FastLED.show();
}

template <int PIN>
void statusHandler<PIN>::run(){

  // abort if no visualization
  if(visualization == NULL) return;

  //manage frame rate
  static unsigned long nextShow;
  if(millis() < nextShow) return;
  nextShow = millis() + (1000/fps);

  // do the thing
  visualization();
  FastLED.show();  
}

void blink(){
  static bool toggle;
  EVERY_N_MILLISECONDS(500){
    toggle = !toggle;
    static CRGB color;
    if(leds[0] != CRGB::Black) color = leds[0];
    leds[0] = toggle ? CRGB::Black : color;
  }
}

void rainbowFade(){
  EVERY_N_MILLISECONDS(20){
    static int i = 0;
    if(i > 255) i = 0;
    leds[0] = ColorFromPalette(RainbowColors_p, i++, 255, LINEARBLEND);
  }
}

#endif