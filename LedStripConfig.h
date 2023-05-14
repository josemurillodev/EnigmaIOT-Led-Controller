#ifndef LedStripConfig_h
#define LedStripConfig_h

#define USE_GET_MILLISECOND_TIMER

#include <ColorConverterLib.h>
#include <FastLED.h>

enum ls_Modes {
  LS_OFF = 255,
  LS_SOLID = 0,
  LS_LOADING = 1,
  LS_SUCCESS = 2,
  LS_INFO = 3,
  LS_ERROR = 4,
  LS_GRADIENT = 5,
  LS_WAVE = 6,
  LS_BLINK = 7,
  LS_HEARTHBEAT = 8,
  LS_RAINBOW = 9,
  LS_CONFETTI = 10,
  LS_NOISE = 11,
  LS_SPARKLES = 12,
  // LS_FIRE = 13,
  LS_PLASMA = 14,
  LS_RIPPLE = 15,
  // LS_CYLON = 16,
  LS_DISCOBALL = 17,
  // LS_CHESS = 18,
  // LS_WIZARD = 19,
  // LS_FLASH = 20,
  // LS_PACIFICA = 21
};

// enum ls_Palette {
//   LP_OCEAN = 0,
//   LP_CLOUD = 1,
//   LP_LAVA = 2,
//   LP_FOREST = 3,
//   LP_PARTY = 4,
//   LP_HEAT = 5
// };

static CRGBPalette16 currentPaletteBlack(CRGB::Black);

class LedStripConfig {
  public:
    LedStripConfig();
    void initLedStrip();
    void update(time_t time);
    void setRgb(uint8_t r, uint8_t g, uint8_t b);
    void writeRgb(uint8_t r, uint8_t g, uint8_t b);
    // void updateHsv(double h, double s, double v);
    void writeHsv(double h, double s, double v);
    void setHsv(double h, double s, double v);
    double getCurrentStep(float multiplier = 1.0);
    void setStatus(ls_Modes status);
    void setStatus(ls_Modes status, time_t time);
    void syncTime(time_t time);
    void setLeds(uint16_t count);
    void setReverse(bool rev);
    uint16_t getLeds();
    bool isOn = true;
    bool reverse = false;
    ls_Modes ledMode = LS_SOLID;
    uint8_t ledpalette = 0;
    float bpm = 120.0f;
    double hue = 0.0;
    double saturation = 0;
    double value = 0.5;
    uint8_t rgb_r;
    uint8_t rgb_g;
    uint8_t rgb_b;
  private:
    bool _started = false;
    time_t _globaltime;
    uint32_t _lastUpdated;
    uint32_t _prevms;
    uint16_t _leds = 16;
    double _deltams;
    ls_Modes _prevStatus;
    void rainbow();
    void gradient();
    // void pride();
    void sparkles();
    // void cylon();
    // void chess();
    void discoBall();
    // void wizard();
    void waveAnim();
    void heartBeat(uint8_t pindex);
    void confetti(uint8_t pindex);
    void noise(uint8_t pindex);
    // void fire(uint8_t pindex);
    void plasma(uint8_t pindex);
    void ripple(uint8_t pindex);
    // void flash(uint8_t pindex);
  // else if (ledMode == LS_FIRE) { fire(ledpalette); }
  // else if (ledMode == LS_PLASMA) { plasma(ledpalette); }
  // else if (ledMode == LS_PRIDE) { pride(); }
  // else if (ledMode == LS_CYLON) { cylon(); }
  // else if (ledMode == LS_DISCOBALL) { discoBall(); }
  // else if (ledMode == LS_WIZARD) { wizard(); }
  // else if (ledMode == LS_CHESS) { chess(); }
  // else if (ledMode == LS_FLASH) { flash(ledpalette); }
  // else if (ledMode == LS_PACIFICA) { pacifica(); }

    // void pacifica();
    // void pacifica_one_layer(CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff);
    // void pacifica_add_whitecaps();
    // void pacifica_deepen_colors();

    bool isTemporal(ls_Modes s) {
      return s == LS_SUCCESS || s == LS_INFO || s == LS_ERROR || s == LS_LOADING;
    };

    // CRGBPalette16 pacifica_palette_1 = 
    //     { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
    //       0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x14554B, 0x28AA50 };
    // CRGBPalette16 pacifica_palette_2 = 
    //     { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
    //       0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F };
    // CRGBPalette16 pacifica_palette_3 = 
    //     { 0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33, 
    //       0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF };

    CRGBPalette16 currentPalette;
    CRGBPalette16 targetPalette;
    // TBlendType    currentBlending = LINEARBLEND;
};

#endif
