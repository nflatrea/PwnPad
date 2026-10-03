#include <EEPROM.h>

constexpr uint8_t KEY = 0x5A;
constexpr int FLAG_LEN = 25; // strlen("TS{EEPR0M_L34ks_53cr3ts}") with \0

constexpr uint8_t FLAG[FLAG_LEN] = {
  'T'^KEY, 'S'^KEY, '{'^KEY, 'E'^KEY, 'E'^KEY,
  'P'^KEY, 'R'^KEY, '0'^KEY, 'M'^KEY, '_'^KEY,
  'L'^KEY, '3'^KEY, '4'^KEY, 'k'^KEY, 's'^KEY,
  '_'^KEY, '5'^KEY, '3'^KEY, 'c'^KEY, 'r'^KEY,
  '3'^KEY, 't'^KEY, 's'^KEY, '}'^KEY
};

void setup() {
  for (int i = 0; i < FLAG_LEN; i++) {
    EEPROM.write(i, FLAG[i] ^ KEY);
  }
}

void loop() {}
