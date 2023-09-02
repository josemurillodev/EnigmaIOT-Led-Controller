#include "LedStripConfig.h"
#include "palettes.h"

// TODO: convert blink to boolean to update value
// TODO: fix ripple, fire, noise, wizard

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    2
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB
#define NUM_LEDS    100 // 300
#define BRIGHTNESS  255

#define qsubd(x, b)  ((x>b)?b:0)
#define qsuba(x, b)  ((x>b)?x-b:0)

#define maxsteps 16

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))  // Definition for the array of routines to display.

// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 55, suggested range 20-100 pio system prune --dry-run
#define COOLING  80

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120

CRGB leds[NUM_LEDS];
CLEDController *ledcontroller;
 
uint32_t framedelay = 24;

// DECLARE_GRADIENT_PALETTE( Muri_p);
// DEFINE_GRADIENT_PALETTE(Muri_p) {
//     0,   2,  1,  1,
//    53,  18,  1,  0,
//   104,  69, 29,  1,
//   153, 167,135, 10,
//   255,  46, 56,  4};

static CRGBPalette16 paletteArray[6] = { OceanColors_p, CloudColors_p, LavaColors_p, ForestColors_p, PartyColors_p, HeatColors_p };

LedStripConfig::LedStripConfig() {
}

void LedStripConfig::initLedStrip() {
  // FastLED.setMaxPowerInMilliWatts(50000);
  FastLED.setMaxPowerInVoltsAndMilliamps(12, 2000);
  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  // FastLED.setTemperature(Tungsten40W);

  ledcontroller = &FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);

  // set_max_power_in_volts_and_milliamps(5, 2000);
  ledcontroller->init();
  ledcontroller->setCorrection(TypicalLEDStrip);
  ledcontroller->setDither(BRIGHTNESS < 255);
  _started = true;
  ledcontroller->showLeds(255);
  ledcontroller->setLeds(leds, _leds);
}

void LedStripConfig::writeRgb(uint8_t r, uint8_t g, uint8_t b) {
  CRGB color = CRGB(r, g, b);
  fill_solid(ledcontroller->leds(), _leds, color);
}

void LedStripConfig::writeHsv(double h, double s, double v) {
  ColorConverter::HsvToRgb(h, s, v, rgb_r, rgb_g, rgb_b);
  writeRgb(rgb_r, rgb_g, rgb_b);
}

void LedStripConfig::setRgb(uint8_t r, uint8_t g, uint8_t b) {
  if (ledMode != LS_SOLID && ledMode != LS_BLINK) setStatus(LS_SOLID);
  rgb_r = r;
  rgb_g = g;
  rgb_b = b;
  ColorConverter::RgbToHsv(rgb_r, rgb_g, rgb_b, hue, saturation, value);
}

void LedStripConfig::setHsv(double h, double s, double v) {
  if (ledMode != LS_SOLID) setStatus(LS_SOLID);
  hue = h;
  saturation = s;
  value = v;
  ColorConverter::HsvToRgb(h, s, v, rgb_r, rgb_g, rgb_b);
}

// void LedStripConfig::updateHsv(double h, double s, double v) {
//   if (ledMode != LS_SOLID) setStatus(LS_SOLID);
//   hue = h;
//   saturation = s;
//   value = v;
//   ColorConverter::HsvToRgb(hue, saturation, value, rgb_r, rgb_g, rgb_b);
//   writeRgb(rgb_r, rgb_g, rgb_b);
// }

double LedStripConfig::getCurrentStep(float multiplier) {
  uint16_t msperbeat = 1000.0 / ((bpm * multiplier) / 60.0);
  int degrees = map(_globaltime % msperbeat, 0, msperbeat, 0, 360);
  double radians = (degrees * 71) / 4068.0;
  // // double val = 1.0 + 1 * sin(radians);
  return sin(radians);
}

void LedStripConfig::setReverse(bool rev) {
  reverse = rev;
}

void LedStripConfig::setLeds(uint16_t count) {
  if (count <= NUM_LEDS) {
    _leds = count;
    if (_started) {
      ledcontroller->showLeds(0);
      ledcontroller->setLeds(leds, _leds);
    }
  }
}

uint16_t LedStripConfig::getLeds() {
  return _leds;
}

void LedStripConfig::update(time_t time) {
  _globaltime = time;
	static time_t lastSensorData;
	static const time_t SENSOR_PERIOD = 25;
  // byte tcp[72];
  // memcpy_P(tcp, (byte*)pgm_read_dword(&(gGradientPalettes[0])), 72);
	if (_globaltime - lastSensorData > SENSOR_PERIOD) {
		lastSensorData = _globaltime;
    _deltams = _globaltime - _prevms;
    _prevms = _globaltime;
    // Serial.println(ledMode);
    if (!isOn) {
      writeRgb(0, 0, 0);
      ledcontroller->showLeds(value * 255);
      return;
    }
    if (ledMode == LS_SOLID) {
      // double v = saturation < 0.5 && value > 0.5 ? 0.5 : value;
      writeHsv(hue, saturation, value);
    }
    // else if (ledMode == LS_LOADING) {
    //   int val = 128.0 + 128 * getCurrentStep();
    //   writeRgb(val, 0, val);
    // } else if (ledMode == LS_SUCCESS) {
    //   int val = 128.0 + 128 * getCurrentStep();
    //   writeRgb(0, val, 0);
    // } else if (ledMode == LS_INFO) {
    //   int val = 128.0 + 128 * getCurrentStep();
    //   writeRgb(0, 0, val);
    // } else if (ledMode == LS_ERROR) {
    //   int val = 128.0 + 128 * getCurrentStep();
    //   writeRgb(val, 0, 0);
    // }
    else if (ledMode == LS_BLINK) {
      double beat = ((getCurrentStep() + 1.0) * (0.8) / (2.0)) + 0.1;
      writeHsv(hue, saturation, beat * value);
    }
    else if (ledMode == LS_PALETTE) {
      // fill_palette(leds, _leds, 0, 2, gGradientPalettes[ledpalette], 255, LINEARBLEND);
      for(int i = 0; i < _leds; i++){
        uint8_t index = map(reverse ? _leds - i : i, 0, _leds, 5, 250);
        leds[i] = ColorFromPalette(gGradientPalettes[ledpalette], index);
      }
    }
    else if (ledMode == LS_FLOW) { flow(); }
    else if (ledMode == LS_RAINBOW) { rainbow(); }
    else if (ledMode == LS_GRADIENT) { gradient(); }
    else if (ledMode == LS_SPARKLES) { sparkles(); }
    else if (ledMode == LS_WAVE) { waveAnim(); }
    else if (ledMode == LS_HEARTHBEAT) { heartBeat(ledpalette); }
    else if (ledMode == LS_CONFETTI) { confetti(ledpalette); }
    else if (ledMode == LS_NOISE) { noise(ledpalette); }
    else if (ledMode == LS_RIPPLE) { ripple(ledpalette); }
    // else if (ledMode == LS_FIRE) { fire(ledpalette); }
    else if (ledMode == LS_PLASMA) { plasma(ledpalette); }
    // else if (ledMode == LS_PRIDE) { pride(); }
    // else if (ledMode == LS_CYLON) { cylon(); }
    else if (ledMode == LS_DISCOBALL) { discoBall(); }
    // else if (ledMode == LS_WIZARD) { wizard(); }
    // else if (ledMode == LS_CHESS) { chess(); }
    // else if (ledMode == LS_FLASH) { flash(ledpalette); }
    // else if (ledMode == LS_PACIFICA) { pacifica(); }

    if ((ledMode == LS_SUCCESS
      || ledMode == LS_INFO
      || ledMode == LS_ERROR)
      && _globaltime - _lastUpdated > 3000) {
      //  TODO: Add last status state
      setStatus(_prevStatus);
    }

    if (mirror && ledMode != LS_SOLID && ledMode != LS_BLINK) {
      for (int i = 0; i < _leds / 2; i++) {
        leds[_leds - 1 - i] = leds[i];
      }
    }

    // uint8_t bri = calculate_max_brightness_for_power_vmA(ledcontroller->leds(), _leds, BRIGHTNESS, 12, 2500);

    ledcontroller->showLeds(255);
  }
}

void LedStripConfig::syncTime(int64_t time) {
  _globaltime = time;
  _lastUpdated = _globaltime;
}

void LedStripConfig::setStatus(ls_Modes s) {
  Serial.print("setStatus ");
  Serial.println(s);
  if (isTemporal(s) && !isTemporal(ledMode)) {
    _prevStatus = ledMode;
  }
  ledMode = s;
  _lastUpdated = millis();
}

void LedStripConfig::setStatus(ls_Modes s, int64_t time) {
  Serial.print("setStatus ");
  Serial.println(s);
  if (isTemporal(s) && !isTemporal(ledMode)) {
    _prevStatus = ledMode;
  }
  ledMode = s;
  _globaltime = time;
  _lastUpdated = _globaltime;
}

void LedStripConfig::rainbow() {
  uint8_t starthue = 128.0 + 128 * getCurrentStep(0.1);
  uint8_t endhue = 128.0 + 128 * getCurrentStep(0.06);
  
  uint8_t start = reverse ? endhue : starthue;
  uint8_t end = reverse ? starthue : endhue;

  if (start < end){
    // If we don't have this, the colour fill will flip around.
    fill_gradient(leds, _leds, CHSV(start,255,value * 255), CHSV(end,255,value * 255), FORWARD_HUES);
  } else{
    fill_gradient(leds, _leds, CHSV(start,255,value * 255), CHSV(end,255,value * 255), BACKWARD_HUES);
  }
}

void LedStripConfig::gradient(){ 
  uint16_t counter = (_globaltime * (((int)bpm >> 3) +3)) & 0xFFFF;
  counter = counter >> 8;
  fill_solid(leds, _leds, ColorFromPalette(gGradientPalettes[ledpalette], -counter, 255, LINEARBLEND));
  for(int i = 0; i < _leds; i++){
    uint8_t index = (i * (16 << (116 / 29)) / _leds) + counter;
    leds[i] = ColorFromPalette(gGradientPalettes[ledpalette], index, value * 255, LINEARBLEND);
  }
}

void LedStripConfig::flow(){ 
  uint16_t counter = (_globaltime * (((int)bpm >> 3) +3)) & 0xFFFF;
  counter = counter >> 8;

  uint16_t maxZones = _leds / 6; //only looks good if each zone has at least 6 LEDs
  uint16_t zones = (10 * maxZones) >> 8;
  if (zones & 0x01) zones++; //zones must be even
  if (zones < 2) zones = 2;
  uint16_t zoneLen = _leds / zones;
  uint16_t offset = (_leds - zones * zoneLen) >> 1;

  // SEGMENT.fill(SEGMENT.color_from_palette(-counter, false, true, 255));
  fill_solid(leds, _leds, ColorFromPalette(gGradientPalettes[ledpalette], -counter, 255, LINEARBLEND));

  for (int z = 0; z < zones; z++)
  {
    uint16_t pos = offset + z * zoneLen;
    for (int i = 0; i < zoneLen; i++)
    {
      uint8_t colorIndex = (i * 255 / zoneLen) - counter;
      uint16_t led = (z & 0x01) ? i : (zoneLen -1) -i;
      if (reverse) led = (zoneLen -1) -led;
      leds[pos + led] = ColorFromPalette(gGradientPalettes[ledpalette], colorIndex, 255, LINEARBLEND);
      // SEGMENT.setPixelColor(pos + led, SEGMENT.color_from_palette(colorIndex, false, true, 255));
    }
  }
}

void LedStripConfig::sparkles() {
  fadeToBlackBy(leds, _leds, (int)bpm);
  int pixel = random(_leds);
  leds[pixel] = CHSV(hue * 255, saturation * 255, value * 255);
  int pixel2 = random(_leds);
  if (pixel2 > _leds / 2) {
    leds[pixel2] = CHSV(hue * 255, saturation * 255, value * 255);
  }
  int pixel3 = random(_leds);
  if (pixel3 > _leds / 3) {
    leds[pixel2] = CHSV(hue * 255, saturation * 255, value * 255);
  }
}

void LedStripConfig::waveAnim() {
  // uint8_t val = 128.0 + 128 * getCurrentStep(0.5);
  uint16_t msperbeat = 1000.0 / (bpm / 60.0);
  uint8_t beat = map(_globaltime % msperbeat, 0, msperbeat, 0, 255);
  for(int i = 0; i < _leds; i++){
    int index = reverse ? _leds - i : i;
    uint8_t step = beat + (index * 10);
    leds[i] = ColorFromPalette(gGradientPalettes[ledpalette], step, value * 255, LINEARBLEND);
  }
}

void LedStripConfig::heartBeat(uint8_t pindex){
  uint8_t beat = 128.0 + 128 * getCurrentStep(0.5);
  for(int i = 0; i < _leds; i++){
    int index = reverse ? _leds - i : i;
    uint8_t colorIndex = beat + (index * 10);
    leds[i] = ColorFromPalette(gGradientPalettes[pindex], colorIndex, value * 255);
  }
}

void LedStripConfig::confetti(uint8_t pindex) {
  fadeToBlackBy(leds, _leds, 10);
  int pos = random16(_leds);
  int pos2 = random16(_leds);

  static float startConfetti = 0;
  static float endConfetti = 0;

  unsigned int msperbeata = (1000.0 / bpm) * 60.0;
  double step = 1000./(msperbeata/_deltams);

  if (startConfetti > msperbeata) {
    startConfetti = 0;
    leds[pos] += ColorFromPalette(gGradientPalettes[pindex], random8(255), value*255);
  }

  if (endConfetti > (msperbeata / 4)) {
    endConfetti = 0;
    leds[pos2] += ColorFromPalette(gGradientPalettes[pindex], random8(255), value*255);
  }
  startConfetti += step;
  endConfetti += step;
}

uint16_t brightnessScale = 150;
uint16_t indexScale = 150;

void LedStripConfig::noise(uint8_t pindex) {
  // for(int i = 0; i < _leds; i++) {
  //   uint8_t index = inoise8(i*brightnessScale, _globaltime+i*indexScale) % 255;
  //   leds[i] = ColorFromPalette(gGradientPalettes[pindex], index, value * 255, LINEARBLEND);
  // }

  for (int i = 0; i < _leds; i++) {
    uint8_t brightness = inoise8(i * brightnessScale, _globaltime / 5);
    uint8_t index = inoise8(i * indexScale, _globaltime / 10);
    leds[i] = ColorFromPalette(gGradientPalettes[pindex], index, value * brightness);
    //leds[i] = CHSV(0, 255, brightness);
  }
}

void LedStripConfig::ripple(uint8_t pindex) {
  static int stepRipple = -1;
  static uint8_t colour;
  static int center = 0;

  fadeToBlackBy(leds, _leds, (int)bpm);
  switch (stepRipple) {
    case -1:
      center = random(_leds);
      colour = random8();
      stepRipple = 0;
      break;
    case 0:
      leds[center] = ColorFromPalette(gGradientPalettes[pindex], colour, 255, LINEARBLEND);
      stepRipple++;
      break;
    case maxsteps:
      stepRipple = -1;
      break;
    default:
      leds[(center + stepRipple + _leds) % _leds] += ColorFromPalette(gGradientPalettes[pindex], colour, 255/stepRipple*2, LINEARBLEND);
      leds[(center - stepRipple + _leds) % _leds] += ColorFromPalette(gGradientPalettes[pindex], colour, 255/stepRipple*2, LINEARBLEND);
      stepRipple++;
      break;  
  }
}

// void LedStripConfig::pride() {
//   uint8_t sat8 = beatsin88(87, 220, 250);
//   uint8_t brightdepth = beatsin88(341, 96, 224);
//   uint16_t brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256));

//   uint16_t hue16 = _globaltime;//gHue * 256;
//   uint16_t hueinc16 = beatsin88(113, 1, 3000);
//   uint16_t brightnesstheta16 = _globaltime * bpm;
  
//   for(uint16_t i = 0 ; i < _leds; i++) {
//     hue16 += hueinc16;
//     uint8_t hue8 = hue16 / 256;

//     brightnesstheta16  += brightnessthetainc16;
//     uint16_t b16 = sin16(brightnesstheta16  ) + 32768;

//     uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
//     uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
//     bri8 += (255 - brightdepth);
    
//     CRGB newcolor = CHSV(hue8, sat8, bri8 * value);
    
//     uint16_t pixelnumber = i;
//     pixelnumber = (_leds-1) - pixelnumber;
    
//     nblend(leds[pixelnumber], newcolor, 64);
//   }
// }

// bool gReverseDirection = false;

// void LedStripConfig::fire(uint8_t pindex) {
//   // Array of temperature readings at each simulation cell
//   static uint8_t heat[NUM_LEDS];

//   // Step 1.  Cool down every cell a little
//   for( int i = 0; i < _leds; i++) {
//     heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / _leds) + 2));
//   }

//   // Step 2.  Heat from each cell drifts 'up' and diffuses a little
//   for( int k= _leds - 1; k >= 2; k--) {
//     heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
//   }
  
//   // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
//   if( random8() < SPARKING ) {
//     int y = random8(7);
//     heat[y] = qadd8( heat[y], random8(160,255) );
//   }

//   // Step 4.  Map from heat cells to LED colors
//   for( int j = 0; j < _leds; j++) {
//     // Scale the heat value from 0-255 down to 0-240
//     // for best results with color palettes.
//     uint8_t colorindex = scale8( heat[j], 240);
//     CRGB color = ColorFromPalette(gGradientPalettes[pindex], colorindex);
//     int pixelnumber;
//     if( gReverseDirection ) {
//       pixelnumber = (_leds-1) - j;
//     } else {
//       pixelnumber = j;
//     }
//     leds[pixelnumber] = color;
//   }
// }

void LedStripConfig::plasma(uint8_t pindex) {
  // int thisPhase = beatsin8(6,-64,64);
  // int thatPhase = beatsin8(7,-64,64);
  int thisPhase = getCurrentStep(0.1) * 64.0;
  int thatPhase = getCurrentStep(0.09) * 64.0;

  for (int i=0; i<_leds; i++) {
    int index = reverse ? _leds - i : i;
    int colorIndex = cubicwave8((index*23)+thisPhase)/2 + cos8((index*15)+thatPhase)/2;
    int thisBright = qsuba(colorIndex, beatsin8(7,0,96));
    leds[index] = ColorFromPalette(gGradientPalettes[pindex], colorIndex, thisBright * value, LINEARBLEND);
  }
}

// void LedStripConfig::cylon() {
//   double val = 0.5 + 0.5 * getCurrentStep(0.2);
//   int beat = val * _leds;

//   fadeToBlackBy(leds, _leds, bpm);
//   leds[beat] = CHSV(hue * 255, saturation * 255, value * 255);

// }

void LedStripConfig::discoBall(){
  uint8_t step = _globaltime / 1000; 

  uint8_t secondHand = step % 60;
  static uint8_t lastSecond = 99;
  static int wave1=0;                    // Current phase is calculated.
  static int wave2=0;
  static int wave3=0;

  if(lastSecond != secondHand){
    lastSecond = secondHand;
    CRGB p = CHSV(HUE_PURPLE, 255, 255);
    CRGB g = CHSV(HUE_GREEN, 255, 255);
    CRGB u = CHSV(HUE_BLUE, 255, 255);
    CRGB b = CRGB::Black;
    CRGB w = CRGB::White;

    switch(secondHand){
      case  0: targetPalette = RainbowColors_p; break;
      case  5: targetPalette = CRGBPalette16(u,u,b,b, p,p,b,b, u,u,b,b, p,p,b,b); break;
      case 10: targetPalette = OceanColors_p; break;
      case 15: targetPalette = CloudColors_p; break;
      case 20: targetPalette = LavaColors_p; break;
      case 25: targetPalette = ForestColors_p; break;
      case 30: targetPalette = PartyColors_p; break;
      case 35: targetPalette = CRGBPalette16(b,b,b,w, b,b,b,w, b,b,b,w, b,b,b,w); break;
      case 40: targetPalette = CRGBPalette16(u,u,u,w, u,u,u,w, u,u,u,w, u,u,u,w); break;
      case 45: targetPalette = CRGBPalette16(u,p,u,w, p,u,u,w, u,g,u,w, u,p,u,w); break;
      case 50: targetPalette = CloudColors_p; break;
      case 55: targetPalette = CRGBPalette16(u,u,u,w, u,u,p,p, u,p,p,p, u,p,p,w); break;
      case 60: break;
    }
  }

  nblendPaletteTowardPalette(currentPaletteBlack, targetPalette, 24);
  wave1 += beatsin8(10,-4,4);
  wave2 += beatsin8(15,-2,2);
  wave3 += beatsin8(12,-3,3);

  for(int k=0; k<_leds; k++){
      uint8_t tmp = sin8(7*k + wave1) + sin8(7*k + wave2) + sin8(7*k + wave3);
      leds[k] = ColorFromPalette(currentPaletteBlack, tmp, value * 255);
  }
}

// void LedStripConfig::wizard(){
//   fadeToBlackBy(leds, _leds, 20);
//   byte dothue = 0;
//   for(int i = 0; i < 4; i++) {
//     leds[beatsin16(i+5, 0, _leds-1 )] |= CHSV(dothue, 200, value * 255);
//     dothue += 32; // les color space
//   }
// }

// void LedStripConfig::chess(){
//   static int16_t posChess;
//   unsigned int maxChess = (_leds / 4) - 1;
//   static int lastSpeedChess;
//   int speedChess = map(255-bpm, 5, 255, 1, maxChess);
//   if (lastSpeedChess != speedChess) {
//     lastSpeedChess = speedChess;
//     posChess = 0;
//   }
//   // delta (can be negative, and/or odd numbers)
//   int8_t deltaChess = speedChess % 2 == 1 ? speedChess : speedChess + 1;
//   static uint8_t hueChess = 0;
//   leds[posChess] = CHSV(hueChess,255,255);

//   unsigned int sizeChess = abs(deltaChess);
//   for (size_t i = 0; i < sizeChess - 1; i++) {
//     leds[posChess+i+1] = CHSV(0,0,0);
//   }
//   int16_t next = (posChess + deltaChess);
//   if (next > _leds) {
//     posChess = 0;
//     hueChess = hueChess + random8(42,128);
//   } else {
//     posChess = next;
//   }
// }

// void LedStripConfig::flash(uint8_t pindex){
//   static unsigned int posFlash = 0;
//   EVERY_N_MILLIS_I(thisTimer,100) {
//     uint8_t timeval = beatsin8(10,1,(255 - bpm)*0.5);
//     thisTimer.setPeriod(timeval);
//     posFlash = (posFlash+1) % (_leds-1);
//     leds[posFlash] = ColorFromPalette(paletteArray[pindex], posFlash, 255, currentBlending);
//   }
//   fadeToBlackBy(leds, _leds, 8);
// }

// // Pacifica

// void LedStripConfig::pacifica() {
//   // Increment the four "color index start" counters, one for each wave layer.
//   // Each is incremented at a different speed, and the speeds vary over time.
//   static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
//   static uint32_t sLastms = 0;
//   uint32_t ms = _globaltime;
//   uint32_t deltams = ms - sLastms;
//   sLastms = ms;
//   uint16_t speedfactor1 = beatsin16(3, 179, 269);
//   uint16_t speedfactor2 = beatsin16(4, 179, 269);
//   uint32_t deltams1 = (deltams * speedfactor1) / 256;
//   uint32_t deltams2 = (deltams * speedfactor2) / 256;
//   uint32_t deltams21 = (deltams1 + deltams2) / 2;
//   sCIStart1 += (deltams1 * beatsin88(1011,10,13));
//   sCIStart2 -= (deltams21 * beatsin88(777,8,11));
//   sCIStart3 -= (deltams1 * beatsin88(501,5,7));
//   sCIStart4 -= (deltams2 * beatsin88(257,4,6));

//   // Clear out the LED array to a dim background blue-green
//   fill_solid( leds, _leds, CRGB( 2, 6, 10));

//   // Render each of four layers, with different scales and speeds, that vary over time
//   pacifica_one_layer( pacifica_palette_1, sCIStart1, beatsin16( 3, 11 * 256, 14 * 256), beatsin8( 10, 70, 130), 0-beat16( 301) );
//   pacifica_one_layer( pacifica_palette_2, sCIStart2, beatsin16( 4,  6 * 256,  9 * 256), beatsin8( 17, 40,  80), beat16( 401) );
//   pacifica_one_layer( pacifica_palette_3, sCIStart3, 6 * 256, beatsin8( 9, 10,38), 0-beat16(503));
//   pacifica_one_layer( pacifica_palette_3, sCIStart4, 5 * 256, beatsin8( 8, 10,28), beat16(601));

//   // Add brighter 'whitecaps' where the waves lines up more
//   pacifica_add_whitecaps();

//   // Deepen the blues and greens a bit
//   pacifica_deepen_colors();
// }

// // Add one layer of waves into the led array
// void LedStripConfig::pacifica_one_layer(CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff) {
//   uint16_t ci = cistart;
//   uint16_t waveangle = ioff;
//   uint16_t wavescale_half = (wavescale / 2) + 20;
//   for(uint16_t i = 0; i < _leds; i++) {
//     waveangle += 250;
//     uint16_t s16 = sin16(waveangle) + 32768;
//     uint16_t cs = scale16(s16 , wavescale_half) + wavescale_half;
//     ci += cs;
//     uint16_t sindex16 = sin16(ci) + 32768;
//     uint8_t sindex8 = scale16(sindex16, 240);
//     CRGB c = ColorFromPalette(p, sindex8, bri * value, LINEARBLEND);
//     leds[i] += c;
//   }
// }

// // Add extra 'white' to areas where the four layers of light have lined up brightly
// void LedStripConfig::pacifica_add_whitecaps() {
//   uint8_t basethreshold = beatsin8(9, 55, 65);
//   uint8_t wave = beat8(7);
  
//   for(uint16_t i = 0; i < _leds; i++) {
//     uint8_t threshold = scale8(sin8(wave), 20) + basethreshold;
//     wave += 7;
//     uint8_t l = leds[i].getAverageLight();
//     if(l > threshold) {
//       uint8_t overage = l - threshold;
//       uint8_t overage2 = qadd8(overage, overage);
//       leds[i] += CRGB(overage, overage2, qadd8(overage2, overage2));
//     }
//   }
// }

// // Deepen the blues and greens
// void LedStripConfig::pacifica_deepen_colors() {
//   for(uint16_t i = 0; i < _leds; i++) {
//     leds[i].blue = scale8(leds[i].blue,  145); 
//     leds[i].green= scale8(leds[i].green, 200); 
//     leds[i] |= CRGB(2, 5, 7);
//   }
// }
