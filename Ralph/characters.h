#ifndef RALPH_CHARACTERS_H
#define RALPH_CHARACTERS_H

/*
  Ralph's 5x8 LCD custom characters.

  There are only 8 custom-character slots in a normal HD44780 16x2 LCD.
  These are deliberately reusable animation frames. The firmware swaps
  which frame is displayed rather than needing hundreds of LCD characters.

  Slot 0-3: Ralph face/pose frames
  Slot 4-5: house pieces
  Slot 6-7: sleeping/extra frames

  Each row uses 5 pixels; bit 4 is the leftmost pixel.
*/

const uint8_t RALPH_CHARS[8][8] = {
  // 0: happy/open face
  {
    0b00000,
    0b01010,
    0b00000,
    0b00100,
    0b10001,
    0b01110,
    0b00000,
    0b00000
  },

  // 1: blink
  {
    0b00000,
    0b00000,
    0b00000,
    0b00100,
    0b10001,
    0b01110,
    0b00000,
    0b00000
  },

  // 2: surprised
  {
    0b01010,
    0b01010,
    0b00000,
    0b00100,
    0b01110,
    0b01010,
    0b00000,
    0b00000
  },

  // 3: dizzy
  {
    0b10001,
    0b01010,
    0b10001,
    0b00100,
    0b10001,
    0b01010,
    0b10001,
    0b00000
  },

  // 4: house roof
  {
    0b00100,
    0b01110,
    0b11111,
    0b10101,
    0b10101,
    0b10101,
    0b00000,
    0b00000
  },

  // 5: house body
  {
    0b11111,
    0b10001,
    0b10101,
    0b10101,
    0b10001,
    0b11111,
    0b00000,
    0b00000
  },

  // 6: sleeping face
  {
    0b00000,
    0b00000,
    0b01010,
    0b00000,
    0b00100,
    0b01110,
    0b00000,
    0b00000
  },

  // 7: sleepy body/blanket
  {
    0b00000,
    0b00100,
    0b01110,
    0b11111,
    0b10101,
    0b11111,
    0b00000,
    0b00000
  }
};

#endif
