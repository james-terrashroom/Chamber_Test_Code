#ifndef LEDARRAY_H
#define LEDARRAY_H

#include <Adafruit_NeoPixel.h>

class LEDArray {
public:
  enum class AnimationType {
    Chase = 0,
    Strobe = 1,
    Solid = 2,
  };

  struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    Color(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0) : r(red), g(green), b(blue) {
    }

    bool operator==(const Color &other) const {
      return r == other.r && g == other.g && b == other.b;
    }

    bool operator!=(const Color &other) const {
      return !(*this == other);
    }

    const char *toString() const {
      static char buf[16];
      snprintf(buf, sizeof(buf), "%d,%d,%d", r, g, b);
      return buf;
    }
  };

  struct Gradient {
    static const int MAX_COLORS = 7;
    Color colors[MAX_COLORS];
    int colorCount;

    Gradient() : colorCount(0) {
    }

    void addColor(const Color &color) {
      if (colorCount < MAX_COLORS) {
        colors[colorCount++] = color;
      } else {
        Serial.println("Gradient::addColor: too many colors");
      }
    }

    bool operator==(const Gradient &other) const {
      if (colorCount != other.colorCount) {
        return false;
      }
      for (int i = 0; i < colorCount; i++) {
        if (colors[i] != other.colors[i]) {
          return false;
        }
      }
      return true;
    }

    bool operator!=(const Gradient &other) const {
      return !(*this == other);
    }

    const char *toString() const {
      static char buf[64];
      char *ptr = buf;
      for (int i = 0; i < colorCount; i++) {
        if (i > 0) {
          *ptr++ = ':';
        }
        ptr += snprintf(ptr, sizeof(buf) - (ptr - buf), "%s", colors[i].toString());
      }
      return buf;
    }
  };

  LEDArray(uint16_t num_leds, uint8_t pin, neoPixelType type = NEO_GRB + NEO_KHZ800)
      : pixels(num_leds, pin, type), animation(AnimationType::Solid), on(false), animationIndex(0),
        animationSpeed(0) {
    setAnimation(AnimationType::Solid);
  }

  void start() {
  }

  void begin() {
    pixels.begin();
  }

  void startup() {
    for (int i = 0; i < pixels.numPixels(); i++) {
      pixels.setPixelColor(i, 255, 255, 255);
      delay(25);
      pixels.show();
    }
    pixels.fill(255, 255, 255);
  }
  void fillWhite() {
    pixels.fill(255, 255, 255);
    pixels.show();
  }

  void clear() {
    pixels.clear();
    pixels.show();
  }

  void fill(Color color) {
    pixels.fill(toPixelColor(color), 0, pixels.numPixels());
  }

  void setAnimationSpeed(uint8_t speed) {
    animationSpeed = speed;
  }

  void setOn(bool value) {
    if (on != value) {
      Serial.printf("setOn: %d\n", value);
      on = value;
      changePending = true;
    }
  }

  void setGradient(Gradient grad) {
    if (gradient != grad) {
      Serial.printf("setGradient: %d %s\n", grad.colorCount, grad.toString());
      gradient = grad;
      changePending = true;
    }
  }

  void setAnimation(AnimationType type) {
    if (type != animation) {
      Serial.printf("setAnimation: %d\n", static_cast<int>(type));
      animation = type;
      changePending = true;
    }
  }

  void setBrightness(uint8_t bright = 255) {
    if (bright != brightness) {
      Serial.printf("setBrightness: %d\n", bright);
      brightness = bright;
      changePending = true;
    }
  }
  
/*
  void setColorParam(const BlynkParam &param) {
    Gradient gradient = Gradient();
    AnimationType animation = AnimationType::Solid;

    int count = 0;
    for (auto it = param.begin(); it < param.end(); ++it) {
      Serial.printf("%d: %d\n", count, it.asInt());
      count++;
    }

    auto createColor = [](BlynkParam::iterator &it) {
      int r = it.asInt();
      ++it;
      int g = it.asInt();
      ++it;
      int b = it.asInt();
      ++it;
      return Color(r, g, b);
    };

    for (auto it = param.begin(); it < param.end();) {
      Color color = createColor(it);

      if (animation == AnimationType::Solid) {
        ++it;               // param[3] is unused
        if (it.isValid()) { // param[4] is chase/strobe
          animation = static_cast<AnimationType>(it.asInt());
          ++it;
          continue; // Ignore first color in this case since it is always green.
        }
      }
      gradient.addColor(color);
      Serial.printf("add color: %s\n", color.toString());
    }

    setAnimation(animation);
    setGradient(gradient);
  }
*/

int increment = 0;
  void blinking() {
    increment+=3;
    //int colorIndex = (animationIndex % (FADE_INDEX * (grad.colorCount - 1))) / FADE_INDEX;
      fill(Color(255, 255, 255));

      int bright = increment % FADE_INDEX;
      if (bright > FADE_INDEX / 2) {
        bright = FADE_INDEX - bright;
      }
      bright = map(bright, 0, 255, 10, 255);
      pixels.setBrightness(bright);
      pixels.show();
  }

  const int FADE_INDEX = 512;
  void loop() {
    if (!on) {
      if (changePending) {
        fill(Color(0, 0, 0));
        pixels.show();
        changePending = false;
        Serial.println("OFF");
      }
      return;
    }

    // Create a local copy of gradient to make sure it doesn't change while we are using it.
    Gradient grad = gradient;

    animationIndex += animationSpeed;
    switch (animation) {
      case AnimationType::Strobe:
        {
          int colorIndex = (animationIndex % (FADE_INDEX * (grad.colorCount))) / FADE_INDEX; // Removed the "-1" after grad.colorCount so that strobe would show all the colors passed to it
          Color color = grad.colors[colorIndex]; // removed the "+1" after colorIndex so that stobe would show all the colors passed to it
          fill(color);

          int bright = animationIndex % FADE_INDEX;
          if (bright > FADE_INDEX / 2) {
            bright = FADE_INDEX - bright;
          }
          bright = map(bright, 0, 255, 10, 255);
          pixels.setBrightness(bright);
          pixels.show();
          break;
        }
      case AnimationType::Chase:
        {
          pixels.setBrightness(brightness); // sets brightness back to user setting
          int numPixels = pixels.numPixels();
          int colorCount = grad.colorCount;
          float floatPixels = static_cast<float>(numPixels);
          float idealSegmentSize = floatPixels / colorCount;
          float accumulatedError = 0.0;

          int r[numPixels];
          int g[numPixels];
          int b[numPixels];

          int p = 0; // Index of pixel we are setting.
          for (int i = 0; i < colorCount; i++) {
            Color first = grad.colors[i];
            Color second = grad.colors[(i + 1) % colorCount];
            float segmentSize =
                std::min(std::round(idealSegmentSize + accumulatedError), floatPixels - p);
            accumulatedError += idealSegmentSize - segmentSize;

            float rDiff = (second.r - first.r) / segmentSize;
            float gDiff = (second.g - first.g) / segmentSize;
            float bDiff = (second.b - first.b) / segmentSize;

            for (int j = 0; j < segmentSize; j++) {
              r[p] = std::round(first.r + rDiff * j);
              g[p] = std::round(first.g + gDiff * j);
              b[p] = std::round(first.b + bDiff * j);
              p++;
            }
          }

          int spin = animationIndex / 10;
          for (int i = 0; i < numPixels; i++) {
            pixels.setPixelColor((i + spin) % numPixels, r[i], g[i], b[i]);
          }
          pixels.show();
          break;
        }

      case AnimationType::Solid:
        {
          if (changePending) {
            changePending = false;
            fill(grad.colors[0]);
            // TODO: add handle max brightness for strip
            pixels.setBrightness(brightness);
            pixels.show();
            Serial.println("SOLID");
          }
          break;
        }
    }
  }

  uint32_t toPixelColor(Color color) const {
    return pixels.Color(color.r, color.g, color.b);
  }

  uint16_t size() const {
    return pixels.numPixels();
  }

  void test(void) {
    pixels.fill(255, 255, 255);
    pixels.show();
  }

private:
  Adafruit_NeoPixel pixels;

  AnimationType animation;
  Gradient gradient;
  uint8_t brightness = 1;
  bool on;
  bool changePending = true; // Needed to prevent loop from setting unchanged pixels

  uint32_t animationIndex = 1;
  uint8_t animationSpeed = 1;
};

#endif
