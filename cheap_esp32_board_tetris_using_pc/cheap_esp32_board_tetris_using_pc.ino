// c++
#include <exception>
#include <stdexcept>
#include <cstdint>
#include <cstring>

// board headers / drivers
#include <SPI.h>
#include <TFT_eSPI.h>

#include <Arduino.h>




// pins
#define LED_RED 4
#define LED_GREEN 16
#define LED_BLUE 17

// backlight
#define TFT_BL 21





// c++ / arduino ide sucks so i gotta declare everything up top
class BitSet;

template<typename T>
class Vec2;

enum GameShapeType : uint8_t;
enum Direction : uint8_t;

GameShapeType getRandomGameShapeType();
std::array<Vec2<uint8_t>, 4> getShapeBlocks(GameShapeType shape_type, uint16_t shape);
void spawnNewShape(Vec2<uint8_t> *grid_size, GameShapeType *shape_type, uint16_t *shape, uint8_t *shape_rot_index, Vec2<uint8_t> *shape_pos);
void placeShape(Vec2<uint8_t> *grid_size, uint16_t *block_colors, BitSet *block_presence, GameShapeType *shape_type, uint16_t *shape, uint8_t *shape_rot_index, Vec2<uint8_t> *shape_pos);
uint8_t getDistanceToHit(Vec2<uint8_t> *grid_size, BitSet *block_presence, GameShapeType *shape_type, uint16_t *shape, Vec2<uint8_t> *shape_pos, Direction dir);
uint8_t dropShape(Vec2<uint8_t> *grid_size, uint16_t *block_colors, BitSet *block_presence, GameShapeType *shape_type, uint16_t *shape, uint8_t *shape_rot_index, Vec2<uint8_t> *shape_pos, bool hardDrop);
uint8_t moveShape(Vec2<uint8_t> *grid_size, BitSet *block_presence, GameShapeType *shape_type, uint16_t *shape, Vec2<uint8_t> *shape_pos, Direction dir);





// keyboard input
enum KeyboardKey : uint8_t {
  KEYBOARD_KEY_W = 0,        // wasd controls
  KEYBOARD_KEY_A,            // wasd controls
  KEYBOARD_KEY_S,            // wasd controls
  KEYBOARD_KEY_D,            // wasd controls
  KEYBOARD_KEY_ARROW_UP,     // arrow controls
  KEYBOARD_KEY_ARROW_LEFT,   // arrow controls
  KEYBOARD_KEY_ARROW_DOWN,   // arrow controls
  KEYBOARD_KEY_ARROW_RIGHT,  // arrow controls
  KEYBOARD_KEY_C,            // hold
  KEYBOARD_KEY_SPACE,        // hard dropping piece
  KEYBOARD_KEY_Z,            // rotating counter clockwise
  KEYBOARD_KEY_COUNT,        // length of enum
};

const std::vector<String> keyboardKeyNames[KEYBOARD_KEY_COUNT] = {
  { "w", "W" },
  { "a", "A" },
  { "s", "S" },
  { "d", "D" },
  { "Key.up" },
  { "Key.left" },
  { "Key.down" },
  { "Key.right" },
  { "c" },
  { "Key.space" },
  { "z", "Z" },
};

bool keyboardKeyStates[KEYBOARD_KEY_COUNT] = { false };
bool keyboardLastKeyStates[KEYBOARD_KEY_COUNT] = { false };
bool keyboardChangedStates[KEYBOARD_KEY_COUNT] = { false };



String readLineFromSerial() {
  String line = "";
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      String out = line;
      line = "";
      // trim CR
      out.trim();
      return out;
    } else if (c != '\r') {
      line += c;
    }
  }
  return String();
}

void keyboardInputReceived(const String &key, bool down) {
  bool foundKey = false;

  for (uint8_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
    std::vector<String> keyNames = keyboardKeyNames[i];

    for (uint8_t j = 0; j < keyNames.size(); j++) {
      String keyName = keyNames[j];
      if (keyName == key) {
        foundKey = true;

        keyboardLastKeyStates[i] = keyboardKeyStates[i];
        keyboardKeyStates[i] = down;
        keyboardChangedStates[i] = keyboardKeyStates[i] != keyboardLastKeyStates[i];

        // Serial.print("KEY: ");
        // Serial.print(keyName);
        // Serial.print(" ");
        // Serial.print(down);
        // Serial.print(" ");
        // Serial.println(keyboardChangedStates[i]);

        break;
      }
    }

    if (foundKey) {
      break;
    }
  }
}

void updatePressedKeys(const String &s) {
  // Expected format: KIND:KEY
  int colon = s.indexOf(':');
  if (colon > 0) {
    String kind = s.substring(0, colon);
    String key = s.substring(colon + 1);
    bool down = (kind.equalsIgnoreCase("DOWN"));
    keyboardInputReceived(key, down);
  } else {
    Serial.print("Bad message: ");
    Serial.println(s);
  }
}









// classes

// vec2
template<typename T>
class Vec2 {
public:
  T x, y;

  Vec2()
    : x(0), y(0) {}

  Vec2(T v)
    : x(v), y(v) {}

  Vec2(T _x, T _y)
    : x(_x), y(_y) {}

  // implicit conversion
  template<typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
  constexpr Vec2(const Vec2<U> &other)
    : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)) {}


  Vec2<T> operator+(const Vec2<T> &other) const {
    return Vec2<T>(x + other.x, y + other.y);
  }
  Vec2<T> operator-(const Vec2<T> &other) const {
    return Vec2<T>(x - other.x, y - other.y);
  }

  template<typename U>
  Vec2<T> operator+(U value) const {
    return Vec2<T>(x + value, y + value);
  }
  template<typename U>
  Vec2<T> operator-(U value) const {
    return Vec2<T>(x - value, y - value);
  }
  template<typename U>
  Vec2<T> operator*(U value) const {
    return Vec2<T>(x * value, y * value);
  }
  template<typename U>
  Vec2<T> operator/(U value) const {
    return Vec2<T>(x / value, y / value);
  }

  template<typename U>
  Vec2<T> &operator+=(const Vec2<U> &rhs) {
    x += static_cast<T>(rhs.x);
    y += static_cast<T>(rhs.y);
    return *this;
  }
  template<typename U>
  Vec2<T> &operator-=(const Vec2<U> &rhs) {
    x -= static_cast<T>(rhs.x);
    y -= static_cast<T>(rhs.y);
    return *this;
  }
  template<typename U>
  Vec2<T> &operator*=(const Vec2<U> &rhs) {
    x *= static_cast<T>(rhs.x);
    y *= static_cast<T>(rhs.y);
    return *this;
  }

  bool operator==(const Vec2<T> &other) const {
    return x == other.x && y == other.y;
  }
};

// bitset
class BitSet {
private:
  std::vector<uint8_t> bits;

public:
  // Default constructor
  BitSet() {}

  // Constructor to create a BitSet of given size
  BitSet(size_t _size)
    : bits(_size, 0) {}

  // Get
  bool get(size_t pos) const {
    if (pos < bits.size() * 8) {
      return (bits[pos / 8] >> (pos % 8)) & 1;
    } else {
      Serial.println("Tried to get bit after end of bitset");
      throw std::runtime_error("Tried to get bit after end of bitset");
    }
  }

  // Set a bit at a given position
  void set(size_t pos, bool value = true) {
    if (pos < bits.size() * 8) {
      if (value) {
        bits[pos / 8] |= 1 << (pos % 8);
      } else {
        bits[pos / 8] &= ~(1 << (pos % 8));
      }
    } else {
      Serial.println("Tried to set bit after end of bitset");
      throw std::runtime_error("Tried to set bit after end of bitset");
    }
  }

  // Toggle a bit at a given position
  void toggle(size_t pos) {
    set(pos, !get(pos));
  }

  // Clear all bits
  void reset() {
    std::fill(bits.begin(), bits.end(), 0);
  }

  // Get size of the BitSet
  size_t size() const {
    return bits.size();
  }
};


// color
class RGB565 {
public:
  uint8_t r;
  uint8_t g;
  uint8_t b;

  static RGB565 from565Int16(uint16_t color) {
    RGB565 newColor;
    newColor.r = color >> 11;
    newColor.g = (color >> 5) & 63;
    newColor.b = color & 31;
    return newColor;
  }
  static uint16_t saturate565Int16(uint16_t color, float percentage) {
    RGB565 newColor = from565Int16(color);
    newColor.saturate(percentage);
    return newColor.to565Int16();
  }
  static uint16_t desaturate565Int16(uint16_t color, float percentage) {
    RGB565 newColor = from565Int16(color);
    newColor.desaturate(percentage);
    return newColor.to565Int16();
  }
  static constexpr uint16_t rgbTo565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }

  uint16_t to565Int16() {
    return (r << 11) | (g << 5) | b;
  }
  void saturate(float percentage) {
    r = lerp(r, 31, percentage);
    g = lerp(g, 63, percentage);
    b = lerp(b, 31, percentage);
  }
  void desaturate(float percentage) {
    r = lerp(r, 0, percentage);
    g = lerp(g, 0, percentage);
    b = lerp(b, 0, percentage);
  }
  RGB565 rgbLerp(RGB565 targetColor, float percentage) {
    RGB565 newColor = *this;
    newColor.r = lerp(newColor.r, targetColor.r, percentage);
    newColor.g = lerp(newColor.g, targetColor.g, percentage);
    newColor.b = lerp(newColor.b, targetColor.b, percentage);
    return newColor;
  }
};






#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

#define BLOCK_GRID_BLOCKS_X 10
#define BLOCK_GRID_BLOCKS_Y 20


constexpr uint16_t BLOCK_SIZE = SCREEN_HEIGHT / BLOCK_GRID_BLOCKS_Y;

constexpr uint16_t BLOCK_BEVEL_SIZE = BLOCK_SIZE / 4;

constexpr uint16_t BLOCK_GRID_WIDTH = BLOCK_GRID_BLOCKS_X * BLOCK_SIZE;
constexpr uint16_t BLOCK_GRID_HEIGHT = BLOCK_GRID_BLOCKS_Y * BLOCK_SIZE;

TFT_eSPI tft = TFT_eSPI();






// bit 0 sets whether the origin is in the corner or not
// bit 1-2 sets size
// bit 3 sets X and Y of origin

#define SHAPE_SIZE_SHIFT 1
#define SHAPE_ORIGIN_SHIFT 3

class GameShapeMeta {
public:
  uint8_t data;

  GameShapeMeta()
    : data(0) {}

  GameShapeMeta(uint8_t corner, uint8_t size, uint8_t origin)
    : data(corner | (size << SHAPE_SIZE_SHIFT) | (origin << SHAPE_ORIGIN_SHIFT)) {}

  uint8_t getCorner() const {
    return data & 0b1;
  }
  uint8_t getSize() const {
    return ((data >> SHAPE_SIZE_SHIFT) & 0b11) + 1;
  }
  uint8_t getOrigin() const {
    return ((data >> SHAPE_ORIGIN_SHIFT) & 0b1) + 1;
  }
  Vec2<uint8_t> getOriginVec2() const {
    return Vec2<uint8_t>(getOrigin());
  }
};







// // container drawing
void drawSmallContainer(Vec2<uint16_t> pos, Vec2<uint16_t> size) {

  // get unmodified corners before starting to write to display
  uint16_t corners[4] = {
    tft.readPixel(pos.x, pos.y),
    tft.readPixel(pos.x + size.x - 1, pos.y),
    tft.readPixel(pos.x, pos.y + size.y - 1),
    tft.readPixel(pos.x + size.x - 1, pos.y + size.y - 1),
  };

  tft.startWrite();
  tft.setAddrWindow(pos.x, pos.y, size.x, size.y);

  uint16_t bgColor = RGB565::rgbTo565(255, 255, 255);
  uint16_t outlineColor = RGB565::rgbTo565(128, 128, 128);

  {
    // row 1
    tft.pushColor(corners[0]);
    for (uint16_t x = 1; x < size.x - 1; x++) {
      tft.pushColor(bgColor);
    }
    tft.pushColor(corners[1]);
    // row 2
    for (uint16_t x = 0; x < 2; x++) {
      tft.pushColor(bgColor);
    }
    for (uint16_t x = 2; x < size.x - 2; x++) {
      tft.pushColor(outlineColor);
    }
    for (uint16_t x = size.x - 2; x < size.x; x++) {
      tft.pushColor(bgColor);
    }
    // row 3
    tft.pushColor(bgColor);
    for (uint16_t x = 1; x < 3; x++) {
      tft.pushColor(outlineColor);
    }
    for (uint16_t x = 3; x < size.x - 3; x++) {
      tft.pushColor(bgColor);
    }
    for (uint16_t x = size.x - 3; x < size.x - 1; x++) {
      tft.pushColor(outlineColor);
    }
    tft.pushColor(bgColor);
  }

  // middle section
  for (uint16_t i = 0; i < size.y - 6; i++) {
    tft.pushColor(bgColor);
    tft.pushColor(outlineColor);
    for (uint16_t x = 2; x < size.x - 2; x++) {
      tft.pushColor(bgColor);
    }
    tft.pushColor(outlineColor);
    tft.pushColor(bgColor);
  }

  {
    // row 3
    tft.pushColor(bgColor);
    for (uint16_t x = 1; x < 3; x++) {
      tft.pushColor(outlineColor);
    }
    for (uint16_t x = 3; x < size.x - 3; x++) {
      tft.pushColor(bgColor);
    }
    for (uint16_t x = size.x - 3; x < size.x - 1; x++) {
      tft.pushColor(outlineColor);
    }
    tft.pushColor(bgColor);
    // row 2
    for (uint16_t x = 0; x < 2; x++) {
      tft.pushColor(bgColor);
    }
    for (uint16_t x = 2; x < size.x - 2; x++) {
      tft.pushColor(outlineColor);
    }
    for (uint16_t x = size.x - 2; x < size.x; x++) {
      tft.pushColor(bgColor);
    }
    // row 1
    tft.pushColor(corners[2]);
    for (uint16_t x = 1; x < size.x - 1; x++) {
      tft.pushColor(bgColor);
    }
    tft.pushColor(corners[3]);
  }

  tft.endWrite();
}

// void rawDrawSmallContainerOutline(Vec2<uint16_t> pos, Vec2<uint16_t> size, Vec2<uint16_t> buf_size, uint16_t buf[], uint16_t color) {
//   for (uint16_t x = 1; x < size.x - 1; x++) {
//     buf[((pos.y + 0) * buf_size.x) + (pos.x + x)] = color;
//     buf[((pos.y + size.y - 1) * buf_size.x) + (pos.x + x)] = color;
//   }

//   for (uint16_t y = 1; y < size.y - 1; y++) {
//     buf[((pos.y + y) * buf_size.x) + (pos.x + 0)] = color;
//     buf[((pos.y + y) * buf_size.x) + (pos.x + size.x - 1)] = color;
//   }

//   buf[((pos.y + 1) * buf_size.x) + (pos.x + 1)] = color;
//   buf[((pos.y + 1) * buf_size.x) + (pos.x + size.x - 1 - 1)] = color;
//   buf[((pos.y + size.y - 1 - 1) * buf_size.x) + (pos.x + 1)] = color;
//   buf[((pos.y + size.y - 1 - 1) * buf_size.x) + (pos.x + size.x - 1 - 1)] = color;
// }

// void drawSmallContainer(Vec2<uint16_t> pos, Vec2<uint16_t> size) {

//   // get unmodified corners before starting to write to display
//   uint16_t corners[4] = {
//     tft.readPixel(pos.x, pos.y),
//     tft.readPixel(pos.x + size.x - 1, pos.y),
//     tft.readPixel(pos.x, pos.y + size.y - 1),
//     tft.readPixel(pos.x + size.x - 1, pos.y + size.y - 1),
//   };

//   tft.startWrite();
//   tft.setAddrWindow(pos.x, pos.y, size.x, size.y);

//   Serial.println("Buf 1");
//   uint16_t buf[size.x * size.y];
//   Serial.println("Buf 2");

//   uint16_t bgColor = RGB565::rgbTo565(255, 255, 255);
//   uint16_t outlineColor = RGB565::rgbTo565(128, 128, 128);

//   for (uint16_t y = 2; y < size.y - 2; y++) {
//     for (uint16_t x = 2; x < size.x - 2; x++) {
//       buf[(y * size.x) + x] = bgColor;
//     }
//   }

//   // fill in the corners
//   buf[(0) + (0)] = corners[0];
//   buf[(0) + (size.x - 1)] = corners[1];
//   buf[(size.y - 1) + (0)] = corners[2];
//   buf[(size.y - 1) + (size.x - 1)] = corners[3];

//   // outline
//   rawDrawSmallContainerOutline(Vec2<uint16_t>(), size, size, buf, bgColor);

//   // inner outline
//   rawDrawSmallContainerOutline(Vec2<uint16_t>(1, 1), size - Vec2<uint16_t>(2, 2), size, buf, outlineColor);

//   // push buffer to display
//   for (uint16_t i = 0; i < size.x * size.y; i++) {
//     tft.pushColor(buf[i]);
//   }

//   tft.endWrite();
// }








// font drawing
void drawCharPixels(Vec2<uint16_t> pos, uint64_t ch, Vec2<uint8_t> font_size, uint16_t color = RGB565::rgbTo565(255, 255, 255)) {
  uint16_t prevBuf[font_size.x * font_size.y];
  for (uint16_t y = 0; y < font_size.y; y++) {
    for (uint16_t x = 0; x < font_size.x; x++) {
      prevBuf[(y * font_size.x) + x] = tft.readPixel(pos.x + x, pos.y + y);
    }
  }

  tft.startWrite();
  tft.setAddrWindow(pos.x, pos.y, font_size.x, font_size.y);

  for (uint16_t y = 0; y < font_size.y; y++) {
    for (uint16_t x = 0; x < font_size.x; x++) {
      uint16_t i = (y * font_size.x) + x;

      uint16_t curColor;

      if ((ch >> i) & 1) {
        curColor = color;
      } else {
        curColor = prevBuf[i];
      }

      tft.pushColor(curColor);
    }
  }

  tft.endWrite();



  // tft.startWrite();
  // tft.setAddrWindow(pos.x, pos.y, font_size.x, font_size.y);
  // for (uint16_t i = 0; i < font_size.x * font_size.y; i++) {
  //   uint16_t curColor;
  //   if ((ch >> i) & 1) {
  //     curColor = color;
  //   } else {
  //     curColor = RGB565::rgbTo565(0, 0, 0);
  //   }

  //   tft.pushColor(curColor);
  // }
  // tft.endWrite();
}

void drawChar(uint64_t *font, Vec2<uint16_t> pos, uint64_t ch, Vec2<uint8_t> font_size, uint16_t color = RGB565::rgbTo565(255, 255, 255)) {
  // 32 (space) is the first printable character
  drawCharPixels(pos, font[ch - 32], font_size, color);
}

void drawString(uint64_t *font, Vec2<uint16_t> pos, const String &str, Vec2<uint8_t> font_size, uint16_t color = RGB565::rgbTo565(255, 255, 255)) {
  Vec2<uint16_t> curPos = pos;
  for (uint16_t i = 0; i < str.length(); i++) {
    char ch = str[i];

    // handle special characters
    if (ch == '\n') {
      curPos.y += 8;
      curPos.x = pos.x;
      continue;
    }

    drawChar(font, curPos, ch, font_size, color);
    curPos.x += 8;
  }
}









// game grid / block drawing
void drawBlockRaw(Vec2<uint16_t> *pos, uint16_t color) {
  uint16_t buf[BLOCK_SIZE * BLOCK_SIZE] = { 0 };

  uint16_t curColor;

  // top
  curColor = RGB565::saturate565Int16(color, 0.4);
  for (uint8_t cy = 0; cy < BLOCK_BEVEL_SIZE; cy++) {
    for (uint8_t cx = cy; cx < BLOCK_SIZE - cy; cx++) {
      uint8_t y = cy;
      uint8_t x = cx;
      buf[(y * BLOCK_SIZE) + x] = curColor;
    }
  }

  // bottom
  curColor = RGB565::desaturate565Int16(color, 0.4);
  for (uint8_t cy = 0; cy < BLOCK_BEVEL_SIZE; cy++) {
    for (uint8_t cx = cy; cx < BLOCK_SIZE - cy; cx++) {
      uint8_t y = (BLOCK_SIZE - 1 - cy);
      uint8_t x = cx;
      buf[(y * BLOCK_SIZE) + x] = curColor;
    }
  }

  // left
  curColor = RGB565::saturate565Int16(color, 0.2);
  for (uint8_t cx = 0; cx < BLOCK_BEVEL_SIZE; cx++) {
    for (uint8_t cy = cx + 1; cy < BLOCK_SIZE - (cx + 1); cy++) {
      uint8_t y = cy;
      uint8_t x = cx;
      buf[(y * BLOCK_SIZE) + x] = curColor;
    }
  }

  // right
  curColor = RGB565::desaturate565Int16(color, 0.2);
  for (uint8_t cx = 0; cx < BLOCK_BEVEL_SIZE; cx++) {
    for (uint8_t cy = cx + 1; cy < BLOCK_SIZE - (cx + 1); cy++) {
      uint8_t y = cy;
      uint8_t x = (BLOCK_SIZE - 1 - cx);
      buf[(y * BLOCK_SIZE) + x] = curColor;
    }
  }

  // center
  for (uint8_t cy = BLOCK_BEVEL_SIZE; cy < BLOCK_SIZE - BLOCK_BEVEL_SIZE; cy++) {
    for (uint8_t cx = BLOCK_BEVEL_SIZE; cx < BLOCK_SIZE - BLOCK_BEVEL_SIZE; cx++) {
      uint8_t y = cy;
      uint8_t x = cx;
      buf[(y * BLOCK_SIZE) + x] = curColor;
    }
  }

  tft.startWrite();
  tft.setAddrWindow(pos->x, pos->y, BLOCK_SIZE, BLOCK_SIZE);
  for (uint16_t i = 0; i < BLOCK_SIZE * BLOCK_SIZE; i++) {
    tft.pushColor(buf[i]);
  }
  tft.endWrite();
}

void drawEmptyBlockRaw(Vec2<uint16_t> pos) {
  uint16_t buf[BLOCK_SIZE * BLOCK_SIZE] = { 0 };

  for (uint8_t cy = 0; cy < BLOCK_SIZE; cy++) {
    for (uint8_t cx = 0; cx < BLOCK_SIZE; cx++) {
      uint16_t color;
      if (cy == 0 || cx == 0) {
        color = RGB565::rgbTo565(20, 20, 20);
      } else {
        color = RGB565::rgbTo565(0, 0, 0);
      }

      buf[(cy * BLOCK_SIZE) + cx] = color;
    }
  }

  tft.startWrite();
  tft.setAddrWindow(pos.x, pos.y, BLOCK_SIZE, BLOCK_SIZE);
  for (uint16_t i = 0; i < BLOCK_SIZE * BLOCK_SIZE; i++) {
    tft.pushColor(buf[i]);
  }
  tft.endWrite();
}

void drawBlockGrid(Vec2<uint8_t> *grid_size, uint16_t *block_colors, BitSet *block_presence) {
  uint16_t startX = SCREEN_WIDTH - BLOCK_GRID_WIDTH;
  uint16_t startY = SCREEN_HEIGHT - BLOCK_SIZE;

  for (uint8_t y = 0; y < grid_size->y; y++) {
    for (uint8_t x = 0; x < grid_size->x; x++) {
      uint16_t i = (y * grid_size->x) + x;

      Vec2<uint16_t> drawPos(startX + x * BLOCK_SIZE, startY - y * BLOCK_SIZE);

      if (block_presence->get(i)) {
        drawBlockRaw(&drawPos, block_colors[i]);
      } else {
        drawEmptyBlockRaw(drawPos);
      }
    }
  }
}

void drawBlocksInShape(Vec2<uint8_t> *grid_size, GameShapeType *shape_type, uint16_t *shape, Vec2<uint8_t> *shape_pos, uint16_t color) {
  uint16_t startX = SCREEN_WIDTH - BLOCK_GRID_WIDTH;
  uint16_t startY = SCREEN_HEIGHT - BLOCK_SIZE;

  std::array<Vec2<uint8_t>, 4> blocks = getShapeBlocks(*shape_type, *shape);
  for (uint8_t i = 0; i < 4; i++) {
    Vec2<uint8_t> blockPos = blocks[i];
    Vec2<uint8_t> pos = *shape_pos + blockPos;

    if (pos.x >= grid_size->x || pos.y >= grid_size->y) {
      continue;
    }

    Vec2<uint16_t> drawPos(startX + (pos.x * BLOCK_SIZE), startY - (pos.y * BLOCK_SIZE));

    drawBlockRaw(&drawPos, color);
  }
}

void drawEmptyBlocksInShape(Vec2<uint8_t> *grid_size, GameShapeType *shape_type, uint16_t *shape, Vec2<uint8_t> *shape_pos) {
  uint16_t startX = SCREEN_WIDTH - BLOCK_GRID_WIDTH;
  uint16_t startY = SCREEN_HEIGHT - BLOCK_SIZE;

  std::array<Vec2<uint8_t>, 4> blocks = getShapeBlocks(*shape_type, *shape);
  for (uint8_t i = 0; i < 4; i++) {
    Vec2<uint8_t> blockPos = blocks[i];
    Vec2<uint8_t> pos = *shape_pos + blockPos;

    if (pos.x >= grid_size->x || pos.y >= grid_size->y) {
      continue;
    }

    Vec2<uint16_t> drawPos(startX + (pos.x * BLOCK_SIZE), startY - (pos.y * BLOCK_SIZE));

    drawEmptyBlockRaw(drawPos);
  }
}



// uint16_t getRainbowColor(uint8_t i) {
//   switch (i) {
//     case 0:
//       return RGB565::rgbTo565(255, 0, 0);
//     case 1:
//       return RGB565::rgbTo565(255, 255, 0);
//     case 2:
//       return RGB565::rgbTo565(0, 255, 0);
//     case 3:
//       return RGB565::rgbTo565(0, 255, 255);
//     case 4:
//       return RGB565::rgbTo565(0, 0, 255);
//     case 5:
//       return RGB565::rgbTo565(255, 0, 255);
//   }
//   return 0;
// }






// game variables
enum GameShapeType : uint8_t {
  SHAPE_I = 0,
  SHAPE_O = 1,
  SHAPE_J = 2,
  SHAPE_L = 3,
  SHAPE_S = 4,
  SHAPE_Z = 5,
  SHAPE_T = 6,

  SHAPE_COUNT,
  SHAPE_NONE = SHAPE_COUNT,
};

const GameShapeMeta game_shape_meta[] = {
  GameShapeMeta(1, 3, 1),  // I
  GameShapeMeta(1, 1, 0),  // O
  GameShapeMeta(0, 2, 0),  // J
  GameShapeMeta(0, 2, 0),  // L
  GameShapeMeta(0, 2, 0),  // S
  GameShapeMeta(0, 2, 0),  // Z
  GameShapeMeta(0, 2, 0),  // T
};

const uint16_t game_shape_blocks[] = {
  0b0000111100000000,  // I
  0b1111,              // O
  0b001111000,         // J
  0b100111000,         // L
  0b110011000,         // S
  0b011110000,         // Z
  0b010111000,         // T
};

const uint32_t game_shape_colors[] = {
  RGB565::rgbTo565(0, 255, 255),  // I
  RGB565::rgbTo565(255, 255, 0),  // O
  RGB565::rgbTo565(0, 0, 255),    // J
  RGB565::rgbTo565(255, 127, 0),  // L
  RGB565::rgbTo565(0, 255, 0),    // S
  RGB565::rgbTo565(255, 0, 0),    // Z
  RGB565::rgbTo565(128, 0, 255),  // T
};

// bit 1-2 = value of x
// bit 3 = sign of x
// bit 4-5 = value of y
// bit 6 = sign of y
constexpr uint8_t createWallKickTest(int8_t x, int8_t y) {
  uint8_t result = 0;

  if (x < 0) {
    result |= 1 << 2;
    result |= -x;
  } else {
    result |= x;
  }

  if (y < 0) {
    result |= 1 << 5;
    result |= -y << 3;
  } else {
    result |= y << 3;
  }

  return result;
}

constexpr Vec2<int8_t> wallKickTestToVec2(uint8_t kick_test) {
  Vec2<int8_t> vec(kick_test & 0b11, (kick_test >> 3) & 0b11);
  if ((kick_test >> 2) & 0b1) {
    vec.x = -vec.x;
  }
  if ((kick_test >> 5) & 0b1) {
    vec.y = -vec.y;
  }
  return vec;
}

const uint8_t game_shape_kick_tests[][8][5] = {
  // J L S Z T
  {
    { createWallKickTest(0, 0), createWallKickTest(+0, 0), createWallKickTest(+1, +1), createWallKickTest(0, -2), createWallKickTest(+1, -2) },  // 0 -> L
    { createWallKickTest(0, 0), createWallKickTest(-0, 0), createWallKickTest(-1, +1), createWallKickTest(0, -2), createWallKickTest(-1, -2) },  // 0 -> R
    { createWallKickTest(0, 0), createWallKickTest(+0, 0), createWallKickTest(+1, -1), createWallKickTest(0, +2), createWallKickTest(+1, +2) },  // R -> 0
    { createWallKickTest(0, 0), createWallKickTest(+0, 0), createWallKickTest(+1, -1), createWallKickTest(0, +2), createWallKickTest(+1, +2) },  // R -> 2
    { createWallKickTest(0, 0), createWallKickTest(-0, 0), createWallKickTest(-1, +1), createWallKickTest(0, -2), createWallKickTest(-1, -2) },  // 2 -> R
    { createWallKickTest(0, 0), createWallKickTest(+0, 0), createWallKickTest(+1, +1), createWallKickTest(0, -2), createWallKickTest(+1, -2) },  // 2 -> L
    { createWallKickTest(0, 0), createWallKickTest(-0, 0), createWallKickTest(-1, -1), createWallKickTest(0, +2), createWallKickTest(-1, +2) },  // L -> 2
    { createWallKickTest(0, 0), createWallKickTest(-0, 0), createWallKickTest(-1, -1), createWallKickTest(0, +2), createWallKickTest(-1, +2) },  // L -> 0
  },
  // I
  {
    { createWallKickTest(0, 0), createWallKickTest(-1, 0), createWallKickTest(+2, 0), createWallKickTest(-1, +2), createWallKickTest(+2, -1) },  // 0 -> L
    { createWallKickTest(0, 0), createWallKickTest(-2, 0), createWallKickTest(+1, 0), createWallKickTest(-2, -1), createWallKickTest(+1, +2) },  // 0 -> R
    { createWallKickTest(0, 0), createWallKickTest(+2, 0), createWallKickTest(-1, 0), createWallKickTest(+2, +1), createWallKickTest(-1, -2) },  // R -> 0
    { createWallKickTest(0, 0), createWallKickTest(-1, 0), createWallKickTest(+2, 0), createWallKickTest(-1, +2), createWallKickTest(+2, -1) },  // R -> 2
    { createWallKickTest(0, 0), createWallKickTest(+1, 0), createWallKickTest(-2, 0), createWallKickTest(+1, -2), createWallKickTest(-2, +1) },  // 2 -> R
    { createWallKickTest(0, 0), createWallKickTest(+2, 0), createWallKickTest(-1, 0), createWallKickTest(+2, +1), createWallKickTest(-1, -2) },  // 2 -> L
    { createWallKickTest(0, 0), createWallKickTest(-2, 0), createWallKickTest(+1, 0), createWallKickTest(-2, -1), createWallKickTest(+1, +2) },  // L -> 2
    { createWallKickTest(0, 0), createWallKickTest(+1, 0), createWallKickTest(-2, 0), createWallKickTest(+1, -2), createWallKickTest(-2, +1) },  // L -> 0
  },
};

const uint8_t (*game_shape_kick_references[])[8][5] = {
  &game_shape_kick_tests[1],  // I
  nullptr,                    // O
  &game_shape_kick_tests[0],  // J
  &game_shape_kick_tests[0],  // L
  &game_shape_kick_tests[0],  // S
  &game_shape_kick_tests[0],  // Z
  &game_shape_kick_tests[0],  // T
};


enum Direction : uint8_t {
  DIRECTION_UP = 0,
  DIRECTION_DOWN,
  DIRECTION_LEFT,
  DIRECTION_RIGHT,
};





// font
uint64_t game_font[] = { 0x0, 0xc000c0c1e1e0c, 0x363636, 0x36367f367f3636, 0xc1f301e033e0c, 0x63660c18336300, 0x6e333b6e1c361c, 0x30606, 0x180c0606060c18, 0x60c1818180c06, 0x663cff3c6600, 0xc0c3f0c0c00, 0x60c0c0000000000, 0x3f000000, 0xc0c0000000000, 0x103060c183060, 0x3e676f7b73633e, 0x3f0c0c0c0c0e0c, 0x3f33061c30331e, 0x1e33301c30331e, 0x78307f33363c38, 0x1e3330301f033f, 0x1e33331f03061c, 0xc0c0c1830333f, 0x1e33331e33331e, 0xe18303e33331e, 0xc0c00000c0c00, 0x60c0c000c0c00, 0x180c0603060c18, 0x3f00003f0000, 0x60c1830180c06, 0xc000c1830331e, 0x1e037b7b7b633e, 0x33333f33331e0c, 0x3f66663e66663f, 0x3c66030303663c, 0x1f36666666361f, 0x7f46161e16467f, 0xf06161e16467f, 0x7c66730303663c, 0x3333333f333333, 0x1e0c0c0c0c0c1e, 0x1e333330303078, 0x6766361e366667, 0x7f66460606060f, 0x63636b7f7f7763, 0x6363737b6f6763, 0x1c36636363361c, 0xf06063e66663f, 0x381e3b3333331e, 0x6766363e66663f, 0x1e33380e07331e, 0x1e0c0c0c0c2d3f, 0x3f333333333333, 0xc1e3333333333, 0x63777f6b636363, 0x63361c1c366363, 0x1e0c0c1e333333, 0x7f664c1831637f, 0x1e06060606061e, 0x406030180c0603, 0x1e18181818181e, 0x63361c08, 0xff00000000000000, 0x180c0c, 0x6e333e301e0000, 0x3b66663e060607, 0x1e3303331e0000, 0x6e33333e303038, 0x1e033f331e0000, 0xf06060f06361c, 0x1f303e33336e0000, 0x6766666e360607, 0x1e0c0c0c0e000c, 0x1e33333030300030, 0x67361e36660607, 0x1e0c0c0c0c0c0e, 0x636b7f7f330000, 0x333333331f0000, 0x1e3333331e0000, 0xf063e66663b0000, 0x78303e33336e0000, 0xf06666e3b0000, 0x1f301e033e0000, 0x182c0c0c3e0c08, 0x6e333333330000, 0xc1e3333330000, 0x367f7f6b630000, 0x63361c36630000, 0x1f303e3333330000, 0x3f260c193f0000, 0x380c0c070c0c38, 0x18181800181818, 0x70c0c380c0c07, 0x3b6e, 0x7f6363361c0800, 0x18181818001818, 0x18187e03037e1818, 0x3f67060f26361c, 0x633e63633e6300, 0xc0c3f0c3f1e3333, 0x18181800181818, 0x1e331c36361cc67c, 0x63, 0x7e81b98585b9817e, 0x7e007c36363c, 0xcc663366cc00, 0x30303f000000, 0x3f000000, 0x7e81a59da59d817e, 0xff, 0x1c36361c, 0x3f000c0c3f0c0c, 0x1e060c180e, 0xe180c180e, 0xc1830, 0x3063e6666666600, 0xd8d8d8dedbdbfe, 0x1800000000, 0x1c30180000000000, 0x1e0c0c0e0c, 0x3e001c36361c, 0x3366cc663300, 0xc0f3f6ecdb3363c3, 0xf03366cc7b3363c3, 0xc3f6ecdf3c66cc07, 0x1e3303060c000c, 0x63637f63361c03 };
Vec2<uint8_t> game_font_size(8, 8);





// game functions
void setBlock(uint16_t *block_colors, BitSet *block_presence, Vec2<uint8_t> *blockPos, Vec2<uint8_t> *gridSize, uint16_t color) {
  // if (blockPos->x > gridSize->x) {
  //   Serial.print("X exceeded grid size ");
  //   Serial.println(blockPos->x);
  //   return;
  // } else if (blockPos->y > gridSize->y) {
  //   Serial.print("Y exceeded grid size ");
  //   Serial.println(blockPos->y);
  //   return;
  // }

  uint16_t i = (blockPos->y * gridSize->x) + blockPos->x;
  block_presence->set(i);
  block_colors[i] = color;
}

void removeBlock(uint16_t *block_colors, BitSet *block_presence, Vec2<uint8_t> *blockPos, Vec2<uint8_t> *gridSize) {
  // if (blockPos->x > gridSize->x) {
  //   Serial.print("X exceeded grid size ");
  //   Serial.println(blockPos->x);
  //   return;
  // } else if (blockPos->y > gridSize->y) {
  //   Serial.print("Y exceeded grid size ");
  //   Serial.println(blockPos->y);
  //   return;
  // }

  uint16_t i = (blockPos->y * gridSize->x) + blockPos->x;
  block_presence->set(i, false);
  block_colors[i] = 0;
}

GameShapeType getRandomGameShapeType() {
  return static_cast<GameShapeType>(random(0, SHAPE_COUNT));
  // return static_cast<GameShapeType>((rand() / (float)(RAND_MAX + 1)) * SHAPE_COUNT);
}

std::array<Vec2<uint8_t>, 4> getShapeBlocks(GameShapeType shape_type, uint16_t shape) {
  uint8_t resultIdx = 0;
  std::array<Vec2<uint8_t>, 4> result;

  uint8_t size = game_shape_meta[shape_type].getSize();

  for (uint8_t y = 0; y < size; y++) {
    for (uint8_t x = 0; x < size; x++) {
      uint8_t i = (y * size) + x;
      uint8_t posBit = (shape >> i) & 0b1;
      if (posBit) {
        result[resultIdx++] = Vec2<uint8_t>(x, y);

        if (resultIdx == 4) {
          break;
        }
      }
    }
  }

  return result;
}

void spawnNewShape(Vec2<uint8_t> *grid_size, GameShapeType *shape_type, uint16_t *shape, uint8_t *shape_rot_index, Vec2<uint8_t> *shape_pos) {
  *shape_type = getRandomGameShapeType();
  *shape = game_shape_blocks[*shape_type];
  *shape_rot_index = 0;
  *shape_pos = Vec2<uint8_t>(grid_size->x / 2 - 1, grid_size->y);
}

void placeShape(Vec2<uint8_t> *grid_size, uint16_t *block_colors, BitSet *block_presence, GameShapeType *shape_type, uint16_t *shape, uint8_t *shape_rot_index, Vec2<uint8_t> *shape_pos) {
  std::array<Vec2<uint8_t>, 4> blocks = getShapeBlocks(*shape_type, *shape);

  // array containing y levels that were modified by placing this shape
  uint8_t changedHeightsCount = 0;
  uint8_t changedHeights[4];

  // place the blocks
  // and populate changedHeights
  uint16_t color = game_shape_colors[*shape_type];
  for (uint8_t i = 0; i < 4; i++) {
    Vec2<uint8_t> blockPos = blocks[i];
    Vec2<uint8_t> pos = *shape_pos + blockPos;

    setBlock(block_colors, block_presence, &pos, grid_size, color);

    bool foundChangedHeight = false;
    for (uint8_t j = 0; j < changedHeightsCount; j++) {
      if (changedHeights[j] == pos.y) {
        foundChangedHeight = true;
        break;
      }
    }
    if (!foundChangedHeight) {
      changedHeights[changedHeightsCount++] = pos.y;
    }
  }


  // array containing y levels that were cleared
  uint8_t clearedHeightsCount = 0;
  uint8_t clearedHeights[4];

  // check if any of the modified y levels clear lines
  for (uint8_t i = 0; i < changedHeightsCount; i++) {
    uint8_t y = changedHeights[i];

    uint16_t baseI = y * grid_size->x;

    bool clearLine = true;
    for (uint8_t x = 0; x < grid_size->x; x++) {
      uint16_t i = baseI + x;

      if (!block_presence->get(i)) {
        clearLine = false;
        break;
      }
    }

    if (clearLine) {
      clearedHeights[clearedHeightsCount++] = y;
    }
  }

  // move blocks above cleared lines downward
  if (clearedHeightsCount > 0) {
    uint8_t clearedIdx = 0;
    uint8_t targetY = clearedHeights[clearedIdx];
    uint8_t y = targetY;
    uint8_t copyY = y;

    while (y < grid_size->y) {
      while (copyY == targetY) {
        copyY++;
        clearedIdx++;
        if (clearedIdx < clearedHeightsCount) {
          targetY = clearedHeights[clearedIdx];
        }
      }


      // copy blocks from copy row into this row
      for (uint8_t x = 0; x < grid_size->x; x++) {
        uint16_t curIdx = (y * grid_size->x) + x;
        uint16_t targetIdx = (copyY * grid_size->x) + x;

        bool presence;
        uint16_t color;
        if (targetIdx >= grid_size->x * grid_size->y) {
          presence = false;
        } else {
          presence = block_presence->get(targetIdx);
          if (presence) {
            color = block_colors[targetIdx];
          }
        }

        block_presence->set(curIdx, presence);
        if (presence) {
          block_colors[curIdx] = color;
        }
      }
      y++;
      copyY++;
    }
  }

  drawBlockGrid(grid_size, block_colors, block_presence);

  spawnNewShape(grid_size, shape_type, shape, shape_rot_index, shape_pos);
}

uint8_t getDistanceToHit(Vec2<uint8_t> *grid_size, BitSet *block_presence, GameShapeType *shape_type, uint16_t *shape, Vec2<uint8_t> *shape_pos, Direction dir) {
  std::array<Vec2<uint8_t>, 4> blocks = getShapeBlocks(*shape_type, *shape);

  uint8_t minDist = 1000;
  for (uint8_t i = 0; i < 4; i++) {
    Vec2<uint8_t> blockPos = blocks[i];
    Vec2<uint8_t> pos = *shape_pos + blockPos;

    int8_t posDim;
    if (dir == DIRECTION_UP || dir == DIRECTION_DOWN) {
      posDim = pos.y;
    } else {
      posDim = pos.x;
    }

    int8_t target;
    if (dir == DIRECTION_UP) {
      target = grid_size->y - 1;
    } else if (dir == DIRECTION_DOWN) {
      target = 0;
    } else if (dir == DIRECTION_LEFT) {
      target = 0;
    } else if (dir == DIRECTION_RIGHT) {
      target = grid_size->x - 1;
    }

    int8_t dim = posDim;
    while (dim != target) {
      if (dir == DIRECTION_UP || dir == DIRECTION_RIGHT) {
        dim++;
      } else if (dir == DIRECTION_DOWN || dir == DIRECTION_LEFT) {
        dim--;
      }

      uint8_t cx;
      uint8_t cy;
      if (dir == DIRECTION_UP || dir == DIRECTION_DOWN) {
        cy = dim;
        cx = pos.x;
      } else {
        cy = pos.y;
        cx = dim;
      }

      uint8_t presence;
      if (cy >= grid_size->y) {
        presence = 0;
      } else {
        uint16_t index = (cy * grid_size->x) + cx;
        presence = block_presence->get(index) == 1;
      }

      if (presence) {
        minDist = min(static_cast<uint8_t>(abs(dim - posDim) - 1), minDist);
      }
    }
    if (dim == target) {
      minDist = min(static_cast<uint8_t>(abs(target - posDim)), minDist);
    }
  }

  return minDist;
}

uint8_t dropShape(Vec2<uint8_t> *grid_size, uint16_t *block_colors, BitSet *block_presence, GameShapeType *shape_type, uint16_t *shape, uint8_t *shape_rot_index, Vec2<uint8_t> *shape_pos, bool hardDrop) {
  uint8_t distance = getDistanceToHit(grid_size, block_presence, shape_type, shape, shape_pos, DIRECTION_DOWN);

  if (hardDrop) {
    if (distance > 0) {
      drawEmptyBlocksInShape(grid_size, shape_type, shape, shape_pos);
      shape_pos->y -= distance;
      drawBlocksInShape(grid_size, shape_type, shape, shape_pos, game_shape_colors[*shape_type]);
    }
    placeShape(grid_size, block_colors, block_presence, shape_type, shape, shape_rot_index, shape_pos);
  } else {
    if (distance == 0) {
      placeShape(grid_size, block_colors, block_presence, shape_type, shape, shape_rot_index, shape_pos);
    } else {
      drawEmptyBlocksInShape(grid_size, shape_type, shape, shape_pos);
      shape_pos->y--;
      drawBlocksInShape(grid_size, shape_type, shape, shape_pos, game_shape_colors[*shape_type]);
    }
  }

  return distance;
}

uint8_t moveShape(Vec2<uint8_t> *grid_size, BitSet *block_presence, GameShapeType *shape_type, uint16_t *shape, Vec2<uint8_t> *shape_pos, Direction dir) {
  uint8_t distance = getDistanceToHit(grid_size, block_presence, shape_type, shape, shape_pos, dir);

  if (distance > 0) {
    drawEmptyBlocksInShape(grid_size, shape_type, shape, shape_pos);
    if (dir == DIRECTION_UP) {
      shape_pos->y += 1;
    } else if (dir == DIRECTION_DOWN) {
      shape_pos->y -= 1;
    } else if (dir == DIRECTION_LEFT) {
      shape_pos->x -= 1;
    } else if (dir == DIRECTION_RIGHT) {
      shape_pos->x += 1;
    }
    drawBlocksInShape(grid_size, shape_type, shape, shape_pos, game_shape_colors[*shape_type]);
  }

  return distance;
}

void rotateShapeBlocks(Vec2<uint8_t> *grid_size, BitSet *block_presence, GameShapeType *shape_type, uint16_t *shape, Vec2<uint8_t> *shape_pos, bool rotateDir) {
  std::array<Vec2<uint8_t>, 4> blocks = getShapeBlocks(*shape_type, *shape);

  const GameShapeMeta &meta = game_shape_meta[*shape_type];

  bool corner = meta.getCorner();
  Vec2<int8_t> origin = meta.getOriginVec2();
  uint8_t size = meta.getSize();

  uint16_t newShape = 0;

  for (uint8_t i = 0; i < 4; i++) {
    Vec2<int8_t> blockPos = blocks[i];
    Vec2<int8_t> pos = blockPos - origin;

    Vec2<int8_t> newPos;
    if (rotateDir) {
      newPos.x = pos.y;
      newPos.y = -pos.x;
      if (corner) {
        newPos.y--;
      }
    } else {
      newPos.x = -pos.y;
      newPos.y = pos.x;
      if (corner) {
        newPos.x--;
      }
    }

    newPos += origin;

    newShape |= 1 << ((newPos.y * size) + newPos.x);
  }

  *shape = newShape;
}

bool solveWallKicks(Vec2<uint8_t> *grid_size, BitSet *block_presence, GameShapeType *shape_type, uint16_t *shape, uint8_t *shape_rot_index, Vec2<uint8_t> *shape_pos, bool rotateDir) {
  const uint8_t(*kick_tests_list)[8][5] = game_shape_kick_references[*shape_type];

  // shape cant (doesnt have a reason to wall kick, e.g. the O tetromino)
  if (kick_tests_list == nullptr) {
    return false;
  }

  const uint8_t(*kick_tests)[5] = &(*kick_tests_list)[(*shape_rot_index * 2) + (rotateDir ? 1 : 0)];

  std::array<Vec2<uint8_t>, 4> blocks = getShapeBlocks(*shape_type, *shape);

  bool found_kick = false;
  Vec2<int8_t> new_pos;
  for (uint8_t i = 0; i < 5; i++) {
    // add kick test offset to shape position
    new_pos = Vec2<int8_t>(*shape_pos) + wallKickTestToVec2((*kick_tests)[i]);

    // check that all blocks are in grid and no blocks overlap
    bool valid_kick = true;
    for (uint8_t j = 0; j < 4; j++) {
      Vec2<int8_t> block_pos = new_pos + Vec2<int8_t>(blocks[j]);

      if (block_pos.x < 0 || block_pos.y < 0 || block_pos.x >= grid_size->x || block_pos.y >= grid_size->y) {
        valid_kick = false;
        break;
      }

      uint16_t presence_index = (block_pos.y * grid_size->x) + block_pos.x;

      if (block_presence->get(presence_index)) {
        valid_kick = false;
        break;
      }
    }

    if (valid_kick) {
      found_kick = true;
      break;
    }
  }

  if (found_kick) {
    *shape_rot_index = (*shape_rot_index + (rotateDir ? 1 : -1)) % 4;
    *shape_pos = Vec2<uint8_t>(new_pos);
  }

  return found_kick;
}

void rotateShape(Vec2<uint8_t> *grid_size, BitSet *block_presence, GameShapeType *shape_type, uint16_t *shape, uint8_t *shape_rot_index, Vec2<uint8_t> *shape_pos, bool rotateDir) {
  // create a temporary shape and try and wall kick
  uint16_t new_shape = *shape;
  uint8_t new_shape_rot_index = *shape_rot_index;
  Vec2<uint8_t> new_shape_pos = *shape_pos;
  rotateShapeBlocks(grid_size, block_presence, shape_type, &new_shape, &new_shape_pos, rotateDir);

  bool success = solveWallKicks(grid_size, block_presence, shape_type, &new_shape, &new_shape_rot_index, &new_shape_pos, rotateDir);
  if (success) {
    drawEmptyBlocksInShape(grid_size, shape_type, shape, shape_pos);
    *shape = new_shape;
    *shape_rot_index = new_shape_rot_index;
    *shape_pos = new_shape_pos;
    drawBlocksInShape(grid_size, shape_type, shape, shape_pos, game_shape_colors[*shape_type]);
  }
}

// void rotateShape(Vec2<uint8_t> *grid_size, BitSet *block_presence, GameShapeType *shape_type, uint16_t *shape, Vec2<uint8_t> *shape_pos, bool rotateDir) {
//   drawEmptyBlocksInShape(grid_size, shape_type, shape, shape_pos);
//   rotateShapeBlocks(grid_size, block_presence, shape_type, shape, shape_pos, rotateDir);
//   drawBlocksInShape(grid_size, shape_type, shape, shape_pos, game_shape_colors[*shape_type]);
// }




// game variables
Vec2<uint8_t> game_grid_size;

uint16_t *game_block_colors;
BitSet game_block_presence;

GameShapeType game_shape_type;
uint16_t game_shape;
uint8_t game_shape_rot_index;
Vec2<uint8_t> game_shape_pos;

// counters
unsigned long lastInputUpdate;
unsigned long lastGameShapeMove;
unsigned long lastGameShapeDrop;
unsigned long lastGameManualShapeDrop;

void setup() {
  // game setup

  // variables
  game_grid_size = Vec2<uint8_t>(BLOCK_GRID_BLOCKS_X, BLOCK_GRID_BLOCKS_Y);

  // initialize grid
  uint16_t game_blocks_count = game_grid_size.x * (game_grid_size.y + 4);
  game_block_colors = (uint16_t *)malloc(sizeof(uint16_t) * game_blocks_count);
  std::memset(game_block_colors, 0, game_blocks_count);

  game_block_presence = BitSet(ceil((game_grid_size.x * game_grid_size.y) / 8.0f));
  game_block_presence.reset();



  // initialize counters
  unsigned long curMillis = millis();
  lastInputUpdate = curMillis;
  lastGameShapeMove = curMillis;
  lastGameShapeDrop = curMillis;
  lastGameManualShapeDrop = curMillis;


  // serial init
  Serial.begin(115200);


  // Wait for serial (optional; useful when using Serial Monitor)
  // while (!Serial) { delay(10); }
  Serial.println("ESP32 ready. Waiting for key events...");


  // tft init

  // Start the tft display
  tft.init();

  // Set the TFT display rotation in portrait mode (but upside down)
  tft.setRotation(2);

  // 16 bit color depth
  tft.writecommand(0x3A);  // COLMOD command
  tft.writedata(0x55);     // 0x55 = 16-bit (RGB565)

  // This colors on this display is inverted (0 -> 255)
  tft.invertDisplay(true);

  // Clear the screen before writing to it
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);



  // for (uint8_t y = 0; y < BLOCK_GRID_BLOCKS_Y; y++) {
  //   RGB565 prevColor = RGB565::from565Int16(getRainbowColor(y % 6));
  //   RGB565 nextColor = RGB565::from565Int16(getRainbowColor((y + 1) % 6));

  //   for (uint8_t x = 0; x < BLOCK_GRID_BLOCKS_X; x++) {
  //     uint16_t curColor = prevColor.rgbLerp(nextColor, (float)x / (BLOCK_GRID_BLOCKS_X - 1)).to565Int16();
  //     setBlock(game_block_colors, &game_block_presence, x, y, BLOCK_GRID_BLOCKS_X, BLOCK_GRID_BLOCKS_Y, curColor);
  //   }
  // }



  drawBlockGrid(&game_grid_size, game_block_colors, &game_block_presence);




  // start game
  spawnNewShape(&game_grid_size, &game_shape_type, &game_shape, &game_shape_rot_index, &game_shape_pos);







  // pin setup

  // back RGB led
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  // disable back led
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);


  // keep direct display pin modifications after the tft commands to prevent conflicts
  // pinMode(TFT_BL, OUTPUT);
  // digitalWrite(TFT_BL, LOW);








  // ui



  // border of game blocks
  uint16_t infoGameBorderWidth = 2;

  tft.drawRect(SCREEN_WIDTH - BLOCK_GRID_WIDTH - infoGameBorderWidth, 0, infoGameBorderWidth, SCREEN_HEIGHT, RGB565::rgbTo565(255, 255, 255));


  // info
  Vec2<uint16_t> infoSize(SCREEN_WIDTH - BLOCK_GRID_WIDTH - infoGameBorderWidth, SCREEN_HEIGHT);

  Vec2<uint16_t> infoCenter = infoSize / 2;


  // score
  Vec2<uint16_t> scoreContainerSize = Vec2<uint16_t>((8 * 5) + 6, (8 * 1) + 6);
  Vec2<uint16_t> scoreContainerPos = Vec2<uint16_t>(infoCenter.x - (scoreContainerSize.x / 2));

  // score background
  {
    uint16_t currentY = scoreContainerPos.y + (scoreContainerSize.y / 2);
    // 1 short line
    tft.startWrite();
    tft.setAddrWindow(0, currentY, infoSize.x, 2);
    for (uint16_t y = 0; y < 2; y++) {
      for (uint16_t x = 0; x < infoSize.x; x++) {
        tft.pushColor(RGB565::rgbTo565(255, 255, 255));
      }
    }
    tft.endWrite();
    currentY += 2;

    // 2 tall line
    tft.startWrite();
    tft.setAddrWindow(0, currentY, infoSize.x, 8);
    for (uint16_t y = 0; y < 8; y++) {
      for (uint16_t x = 0; x < infoSize.x; x++) {
        tft.pushColor(RGB565::rgbTo565(128, 128, 128));
      }
    }
    tft.endWrite();
    currentY += 8;

    // 3 short line
    tft.startWrite();
    tft.setAddrWindow(0, currentY, infoSize.x, 2);
    for (uint16_t y = 0; y < 2; y++) {
      for (uint16_t x = 0; x < infoSize.x; x++) {
        tft.pushColor(RGB565::rgbTo565(255, 255, 255));
      }
    }
    tft.endWrite();
    currentY += 2;

    // 4 short line
    tft.startWrite();
    tft.setAddrWindow(0, currentY, infoSize.x, 2);
    for (uint16_t y = 0; y < 2; y++) {
      for (uint16_t x = 0; x < infoSize.x; x++) {
        tft.pushColor(RGB565::rgbTo565(128, 128, 128));
      }
    }
    tft.endWrite();
    currentY += 2;

    // 5 tall line (for score)
    tft.startWrite();
    tft.setAddrWindow(0, currentY, infoSize.x, 12);  // 8 + 4
    for (uint16_t y = 0; y < 12; y++) {
      for (uint16_t x = 0; x < infoSize.x; x++) {
        tft.pushColor(RGB565::rgbTo565(255, 255, 255));
      }
    }
    tft.endWrite();
    currentY += 12;

    // 6 short line
    tft.startWrite();
    tft.setAddrWindow(0, currentY, infoSize.x, 2);
    for (uint16_t y = 0; y < 2; y++) {
      for (uint16_t x = 0; x < infoSize.x; x++) {
        tft.pushColor(RGB565::rgbTo565(128, 128, 128));
      }
    }
    tft.endWrite();
    currentY += 2;

    // 7 short line
    tft.startWrite();
    tft.setAddrWindow(0, currentY, infoSize.x, 2);
    for (uint16_t y = 0; y < 2; y++) {
      for (uint16_t x = 0; x < infoSize.x; x++) {
        tft.pushColor(RGB565::rgbTo565(255, 255, 255));
      }
    }
    tft.endWrite();
    currentY += 2;
  }

  // score title container
  {
    drawSmallContainer(scoreContainerPos, scoreContainerSize);
    drawString(game_font, scoreContainerPos + Vec2<uint16_t>(3), "SCORE", game_font_size, RGB565::rgbTo565(0, 0, 0));
  }

  // level container
  {
    Vec2<uint16_t> levelContainerSize = Vec2<uint16_t>((8 * 5) + 6, (8 * 2) + 6);
    Vec2<uint16_t> levelContainerPos = Vec2<uint16_t>(infoCenter.x - (levelContainerSize.x / 2), infoCenter.y - levelContainerSize.y - 4);

    drawSmallContainer(levelContainerPos, levelContainerSize);
    drawString(game_font, levelContainerPos + Vec2<uint16_t>(3), "LEVEL", game_font_size, RGB565::rgbTo565(0, 0, 0));
  }

  // lines container
  {
    Vec2<uint16_t> linesContainerSize = Vec2<uint16_t>((8 * 5) + 6, (8 * 2) + 6);
    Vec2<uint16_t> linesContainerPos = Vec2<uint16_t>(infoCenter.x - (linesContainerSize.x / 2), infoCenter.y + 4);

    drawSmallContainer(linesContainerPos, linesContainerSize);
    drawString(game_font, linesContainerPos + Vec2<uint16_t>(3), "LINES", game_font_size, RGB565::rgbTo565(0, 0, 0));
  }

  // next piece container
  {

  }

  // hold piece container
  {
    // Vec2<uint16_t> holdPieceContainerPos = Vec2<uint16_t>(infoContainerPadding);
    // Vec2<uint16_t> holdPieceContainerSize = Vec2<uint16_t>(BLOCK_SIZE + 6);
    // drawSmallContainer(holdPieceContainerPos, holdPieceContainerSize);
    // drawString(game_font, scoreContainerPos + Vec2<uint16_t>(3), "Score", game_font_size, RGB565::rgbTo565(0, 0, 0));
  }
}

void loop() {
  unsigned long curMillis = millis();
  if ((curMillis - lastInputUpdate) > 5) {
    String s = readLineFromSerial();
    if (s.length() > 0) {
      // if signal is of keyboard input type
      // add signal types if we add multiple types (e.g. mouse AND keyboard)
      if (true) {
        updatePressedKeys(s);
      }
    }

    lastInputUpdate = curMillis;
  }

  if ((curMillis - lastGameShapeDrop) > 1000) {
    lastGameShapeDrop = curMillis;
    dropShape(&game_grid_size, game_block_colors, &game_block_presence, &game_shape_type, &game_shape, &game_shape_rot_index, &game_shape_pos, false);
  } else {
    if (keyboardKeyStates[KEYBOARD_KEY_SPACE] && keyboardChangedStates[KEYBOARD_KEY_SPACE]) {
      lastGameShapeDrop = curMillis;
      dropShape(&game_grid_size, game_block_colors, &game_block_presence, &game_shape_type, &game_shape, &game_shape_rot_index, &game_shape_pos, true);
    }
  }

  if ((keyboardKeyStates[KEYBOARD_KEY_W] && keyboardChangedStates[KEYBOARD_KEY_W]) || (keyboardKeyStates[KEYBOARD_KEY_ARROW_UP] && keyboardChangedStates[KEYBOARD_KEY_ARROW_UP])) {
    // rotate clockwise
    rotateShape(&game_grid_size, &game_block_presence, &game_shape_type, &game_shape, &game_shape_rot_index, &game_shape_pos, true);
  } else if (keyboardKeyStates[KEYBOARD_KEY_Z] && keyboardChangedStates[KEYBOARD_KEY_Z]) {
    // rotate counter clockwise
    rotateShape(&game_grid_size, &game_block_presence, &game_shape_type, &game_shape, &game_shape_rot_index, &game_shape_pos, false);
  }

  if ((curMillis - lastGameManualShapeDrop) > 200) {
    if (keyboardKeyStates[KEYBOARD_KEY_S] || keyboardKeyStates[KEYBOARD_KEY_ARROW_DOWN]) {
      uint8_t distMoved = moveShape(&game_grid_size, &game_block_presence, &game_shape_type, &game_shape, &game_shape_pos, DIRECTION_DOWN);
      if (distMoved > 0) {
        lastGameShapeDrop = curMillis;
        lastGameManualShapeDrop = curMillis;
      }
    }
  }

  if ((curMillis - lastGameShapeMove) > 200) {
    if (keyboardKeyStates[KEYBOARD_KEY_A] || keyboardKeyStates[KEYBOARD_KEY_ARROW_LEFT]) {
      uint8_t distMoved = moveShape(&game_grid_size, &game_block_presence, &game_shape_type, &game_shape, &game_shape_pos, DIRECTION_LEFT);
      if (distMoved > 0) {
        lastGameShapeMove = curMillis;
      }
    } else if (keyboardKeyStates[KEYBOARD_KEY_D] || keyboardKeyStates[KEYBOARD_KEY_ARROW_RIGHT]) {
      uint8_t distMoved = moveShape(&game_grid_size, &game_block_presence, &game_shape_type, &game_shape, &game_shape_pos, DIRECTION_RIGHT);
      if (distMoved > 0) {
        lastGameShapeMove = curMillis;
      }
    }
  }

  for (uint8_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
    keyboardChangedStates[i] = false;
  }
}