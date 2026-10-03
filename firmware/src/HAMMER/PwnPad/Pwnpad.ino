/*
Copyright (C) 2025 TwelveSec
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.
*/

/*
 * PwnPad (custom training build) Arduino Uno / ATmega328P
 *
 * Challenge select: mode = MOD1*1 + MOD2*2 + MOD3*4 + MOD4*8 + MOD5*16
 * (a switch that is ON/closed pulls its pin to GND and counts as 1;
 *  MOD1 is the LEAST significant bit, MOD5 the most significant).
 *
 *  mode  MOD5..MOD1  challenge
 *  ----  ----------  ------------------------------------------------
 *    1   00001       UART shell @ 9600
 *    2   00010       UART shell @ hidden baud rate (31337 requested)
 *    4   00100       I2C device spoofing (emulate the auth device, replay token)
 *    5   00101       Firmware extraction via ISP (Morse keypad code)
 *    6   00110       PASSIVE slot: EEPROM pre-programmed, nothing runs
 *    7   00111       Voltage glitch (trigger on A4)
 *    8   01000       Voltage glitch v2 (trigger moved to an LED pin)
 *    9   01001       Timing side channel, 5-digit keypad PIN
 *   10   01010       Timing side channel v2, 4-digit keypad PIN
 *   11   01011       Black box: UART login + glitch
 *   12   01100       Black box: UART PIN timing attack (SoftwareSerial)
 *   13   01101       Simple power analysis (trigger on A4)
 *
 *  = 12 firmware-driven challenges + 1 passive (mode 6).
 *  Mode number = challenge number in the HAMMER field manual (1-13).
 *  Modes 0 and 14-31 are unassigned. (SWD/JTAG were removed.)
 */

/*************************** LIBRARIES ***************************/
#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <string.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

/*************************** DEFINED PINS ******************************/
#define MOD1 9
#define MOD2 A3
#define MOD3 A2
#define MOD4 A1
#define MOD5 A0
#define LED_0 2
#define LED_1 3
#define LED_2 4
#define BTN_DOT 5
#define BTN_DASH 6
#define BTN_SPACE 8
#define BTN_OK 7
#define TRIGGER_PIN A4   // SDA header pin, used as scope trigger

/*************************** FUNCTION DEFINITIONS ******************************/
void initialize(void);
int modeReader(void);
void challengeSelector(int mode);
void handheld_challenge_uart(void);
void handheld_challenge_uart_v2(void);
void handheld_challenge_i2c(void);
void handheld_challenge_i2c_v2(void);
void handheld_challenge_firmware_extraction(void);
void handheld_challenge_voltage_glitch(void);
void handheld_challenge_voltage_glitch_v2(void);
void handheld_challenge_side_channel_timing_attack(void);
void handheld_challenge_side_channel_timing_attack_v2(void);
void blackbox_chain_UART_Glitch_Attack(void);
void blackbox_chain_UART_Timing_Attack(void);
void handheld_challenge_simple_power_analysis(void);
void seedFromNoise(void);

/*******************************************************************************/
// Make sure a watchdog reset (REBOOT command) can never leave the WDT armed
// across the restart, whatever bootloader is installed.
void wdt_early_disable(void) __attribute__((naked, used, section(".init3")));
void wdt_early_disable(void) {
  MCUSR = 0;
  wdt_disable();
}

void setup(){
  initialize();
  challengeSelector(modeReader());
}

void loop()
{
  // Intentionally empty: every challenge owns its own infinite loop.
}

/*************************** FUNCTIONS ******************************/
void initialize(void){
  pinMode(MOD1, INPUT_PULLUP);
  pinMode(MOD2, INPUT_PULLUP);
  pinMode(MOD3, INPUT_PULLUP);
  pinMode(MOD4, INPUT_PULLUP);
  pinMode(MOD5, INPUT_PULLUP);
}

// FIX: the old seed used analogRead(0). A0 is MOD5 with the internal
// pull-up enabled, so it reads a constant (~1023 switch open, 0 switch
// closed) -> the "random" PIN/OTP was identical on every boot (it never
// varies). Seed from the ADC's internal temperature
// sensor and bandgap channels instead (no external pin involved) and mix in
// micros().
static uint16_t adcRaw(uint8_t admux) {
  ADMUX = admux;
  delayMicroseconds(200);
  ADCSRA |= _BV(ADSC);
  while (ADCSRA & _BV(ADSC)) {}
  return ADC;
}

void seedFromNoise(void) {
  uint32_t acc = 0x9E3779B9UL ^ micros();
  for (uint8_t i = 0; i < 32; i++) {
    uint16_t t = adcRaw(_BV(REFS1) | _BV(REFS0) | 0x08); // temp sensor
    uint16_t g = adcRaw(_BV(REFS0) | 0x0E);              // 1.1V bandgap
    acc = (acc * 33UL) ^ t ^ ((uint32_t)g << 7) ^ micros();
  }
  randomSeed(acc);
}

// ---- Compile-time XOR-obfuscated storage -------------------------------
// XFLAG()/XSECRET() XOR the string with OBF_KEY at compile time; only the
// XORed bytes land in flash, so `strings` on a flash dump finds nothing.
constexpr uint8_t OBF_KEY = 0x5A;

template <unsigned... I> struct IdxSeq {};
template <unsigned N, unsigned... I>
struct MakeIdxSeq : MakeIdxSeq<N - 1, N - 1, I...> {};
template <unsigned... I>
struct MakeIdxSeq<0, I...> { using type = IdxSeq<I...>; };

template <unsigned N>
struct ObfStr {
  uint8_t bytes[N];
  static constexpr unsigned size = N;   // includes the NUL terminator
  template <unsigned... I>
  constexpr ObfStr(const char (&s)[N], IdxSeq<I...>)
  : bytes{ (uint8_t)(s[I] ^ OBF_KEY)... } {}
  constexpr ObfStr(const char (&s)[N])
  : ObfStr(s, typename MakeIdxSeq<N>::type{}) {}
};

#define XFLAG(name, str) \
static constexpr ObfStr<sizeof(str)> name PROGMEM = ObfStr<sizeof(str)>(str)
#define XSECRET(name, str) XFLAG(name, str)
// Character count (no NUL) of an XFLAG/XSECRET object.
#define OBF_LEN(x) ((uint8_t)(sizeof(x) - 1))

void printObf(Stream &out, const uint8_t *data, uint8_t len) {
  char buf[64];
  if (len >= sizeof(buf)) len = sizeof(buf) - 1;
  for (uint8_t i = 0; i < len; i++) {
    buf[i] = (char)(pgm_read_byte(data + i) ^ OBF_KEY);
  }
  buf[len] = '\0';
  out.println(buf);
  memset(buf, 0, sizeof(buf));
}

// dst must hold len + 1 bytes.
void decodeObf(char *dst, const uint8_t *data, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) {
    dst[i] = (char)(pgm_read_byte(data + i) ^ OBF_KEY);
  }
  dst[len] = '\0';
}

// ---- Flags (FIX: no trailing spaces; "/\/\y" escapes now correct) -------
XFLAG(FLAG_UART1,     "TS{U@rt_15_@w3s0m3}");
XFLAG(FLAG_UART2,     "TS{N0w_Y0u_@r3_@n_el173_H@(k3r}");
XFLAG(I2C1_PREFIX,    "TS{(h@||eng3_07P_i5_");
XFLAG(FLAG_I2C_ADMIN, "TS{1_N33D_/\\/\\y_8u66y}");
XFLAG(FLAG_ISP,       "TS{F1rmw@re_S3cret}");
XFLAG(FLAG_GLITCH1,   "TS{Gl1th3s_@r3_Awe50m3_!}");
XFLAG(FLAG_GLITCH2,   "TS{0h_n0_y0u_foun6_my_7r1gg3r}");
XFLAG(FLAG_TIMING1,   "TS{Y0u_3nter36_th3_v@ul7}");
XFLAG(FLAG_TIMING2,   "TS{D@mn_y0u_@r3_g006}");
XFLAG(FLAG_BB_GLITCH, "TS{L0gin_Succ355fu1_!}");
XFLAG(FLAG_BB_TIMING, "TS{D0n7_M355_w17h_t1m3}");
XFLAG(FLAG_SPA,       "TS{5P@_ar3_H4rd_Bu7_r3w0rd1n9}");

// Bypass secrets that a flash dump must NOT reveal (only the Morse code and
// the I2C admin token are meant to be recoverable by firmware extraction).
XSECRET(SECRET_BB_PASS, "d3@1bt7*0PqSxc9~^");
XSECRET(SECRET_BB_PIN,  "25315294");
XSECRET(SECRET_SPA_PW,  "hacktheplanet");

// ==========================================================================
// HAMMER SHELL ENGINE
struct ShellCommand {
  const char *name;                 // matched case-insensitively
  void (*handler)(char *args);
  bool requires_auth;
};

struct ShellConfig {
  const uint8_t *flag_bytes;
  uint8_t flag_len;                 // characters, no NUL
  const ShellCommand *commands;
  uint8_t command_count;
  bool (*background_check)(void);   // polled while locked; true = unlock
  const __FlashStringHelper *guest_hint; // shown by STATUS while locked
  bool compact_banner;              // short banner for very slow links
};

static Stream *g_shell_io = nullptr;
static const ShellConfig *g_shell_cfg = nullptr;
static bool g_shell_authed = false;

void cmd_help(char *args) {
  (void)args;
  g_shell_io->println(F("Available commands:"));
  for (uint8_t i = 0; i < g_shell_cfg->command_count; i++) {
    const ShellCommand &c = g_shell_cfg->commands[i];
    if (c.requires_auth && !g_shell_authed) continue;
    g_shell_io->println(c.name);
  }
}

void cmd_flag(char *args) {
  (void)args;
  printObf(*g_shell_io, g_shell_cfg->flag_bytes, g_shell_cfg->flag_len);
}

void cmd_status(char *args) {
  (void)args;
  if (g_shell_authed) {
    g_shell_io->println(F("access level: admin"));
  } else {
    g_shell_io->println(F("access level: guest -- some commands hidden"));
    if (g_shell_cfg->guest_hint) g_shell_io->println(g_shell_cfg->guest_hint);
  }
}

void cmd_whoami(char *args) {
  (void)args;
  g_shell_io->println(g_shell_authed ? F("admin") : F("guest"));
}

void cmd_reboot(char *args) {
  (void)args;
  g_shell_io->println(F("Rebooting..."));
  g_shell_io->flush();
  wdt_enable(WDTO_15MS);
  while (1) {}
}

void cmd_led(char *args) {
  if (!args) { g_shell_io->println(F("Usage: LED <1-3> <H/L>")); return; }
  while (*args == ' ') args++;
  char id = *args;
  if (id) args++;
  while (*args == ' ') args++;
  char state = *args;
  if (state >= 'a' && state <= 'z') state -= 32;   // accept h/l
  int pin = -1;
  if (id == '1') pin = LED_0;
  else if (id == '2') pin = LED_1;
  else if (id == '3') pin = LED_2;
  if (pin == -1 || (state != 'H' && state != 'L')) {
    g_shell_io->println(F("Usage: LED <1-3> <H/L>"));
    return;
  }
  digitalWrite(pin, state == 'H' ? HIGH : LOW);
  g_shell_io->println(F("OK"));
}

void printHammerBannerFull(Stream &out) {
  out.println(F("       ___         ___         ___         ___         ___         ___       "));
  out.println(F("      /  /\\       /  /\\       /  /\\       /  /\\       /  /\\       /  /\\      ")); 
  out.println(F("     /  /:/      /  /::\\     /  /::|     /  /::|     /  /::\\     /  /::\\     "));
  out.println(F("    /  /:/      /  /:/\\:\\   /  /:|:|    /  /:|:|    /  /:/\\:\\   /  /:/\\:\\    "));
  out.println(F("   /  /::\\ ___ /  /::\\ \\:\\ /  /:/|:|__ /  /:/|:|__ /  /::\\ \\:\\ /  /::\\ \\:\\   "));
  out.println(F("  /__/:/\\:\\  //__/:/\\:\\_\\:/__/:/_|::::/__/:/_|::::/__/:/\\:\\ \\:/__/:/\\:\\_\\:\\  "));
  out.println(F("  \\__\\/  \\:\\/:\\__\\/  \\:\\/:\\__\\/  /~~/:\\__\\/  /~~/:\\  \\:\\ \\:\\_\\\\__\\/~|::\\/:/  "));
  out.println(F("       \\__\\::/     \\__\\::/      /  /:/      /  /:/ \\  \\:\\ \\:\\    |  |:|::/   "));
  out.println(F("       /  /:/      /  /:/      /  /:/      /  /:/   \\  \\:\\_\\/    |  |:|\\/    "));
  out.println(F("      /__/:/      /__/:/      /__/:/      /__/:/     \\  \\:\\      |__|:|~     ")); 
  out.println(F("      \\_H\\/       \\_A\\/       \\_M\\/       \\_M\\/       \\_E\\/       \\_R\\|      "));
  out.println(F(""));
  out.println(F( "B  R  E  A  K    I  T    U  N  T  I  L    Y  O  U    M  A  K  E    I  T "));
  out.println(F( " "));
  out.println(F( "================================================================================ "));
  out.println(F( "WARNING: Do not feed the device. "));
  out.println(F( "================================================================================ "));
  out.println(F( " "));
  out.println(F( "[BOOT] Waking up little people in the MCU "));
  out.println(F( "[BOOT] Initializing GPIO pins... "));
  out.println(F( "[GPIO] Pin 0:  beep beep  Hello? Can you hear me? "));
  out.println(F( "[BOOT] Establishing serial handshake... "));
  out.println(F( "[BOOT] Welcome to the Embedded Challenge System v2.1.3! "));
  out.println(F( "[BOOT] This is a custom PwnPad Firmware for PITCHU_CTF "));
  out.println(F( "[BOOT] Loading challenge... "));
  out.println(F( " "));
}

// ~1.4 KB of banner takes ~12 s at 1200 baud (SoftwareSerial), so slow
// links get this short one instead.
void printHammerBanner(Stream &out, bool compact) {
  if (!compact) { printHammerBannerFull(out); return; }
  out.println(F("[HAMMER] PwnPad Embedded Challenge System v2.1.3"));
}

// Shared line reader for the login-style challenges. Accepts CR, LF or
// CRLF, ignores the second half of CRLF, drops overflow, never stores the
// terminator. cap includes the NUL. Returns characters stored.
static bool g_prev_cr = false;
static uint8_t readLine(Stream &io, char *buf, uint8_t cap) {
  uint8_t n = 0;
  while (1) {
    if (io.available() <= 0) continue;
    char c = io.read();
    if (c == '\n' && g_prev_cr) { g_prev_cr = false; continue; }
    g_prev_cr = (c == '\r');
    if (c == '\r' || c == '\n') break;
    if (n < cap - 1) buf[n++] = c;
  }
  buf[n] = '\0';
  return n;
}

void hammerShell(Stream &io, const struct ShellConfig &cfg, bool authed_initial) {
  g_shell_io = &io;
  g_shell_cfg = &cfg;
  g_shell_authed = authed_initial;
  printHammerBanner(io, cfg.compact_banner);
  io.println(F("Type HELP for available commands..."));
  char cmdbuf[32];
  bool prev_cr = false;
  while (1) {
    io.print(F("\nuart> "));
    uint8_t idx = 0;
    cmdbuf[0] = '\0';
    while (1) {
      if (cfg.background_check && !g_shell_authed) {
        if (cfg.background_check()) {
          g_shell_authed = true;
          io.println(F("\n[!] Access granted."));
        }
      }
      if (io.available() > 0) {
        char c = io.read();
        if (c == '\n' && prev_cr) { prev_cr = false; continue; }
        prev_cr = (c == '\r');
        if (c == '\n' || c == '\r') break;
        if (c == 0x08 || c == 0x7F) {            // backspace
          if (idx > 0) { idx--; io.print(F("\b \b")); }
          continue;
        }
        if ((uint8_t)c < 0x20) continue;         // ignore other control bytes
        if (idx < sizeof(cmdbuf) - 1) {
          cmdbuf[idx++] = c;
          io.print(c);
        }
      }
    }
    cmdbuf[idx] = '\0';
    io.println(F(""));
    if (idx == 0) continue;                      // empty Enter -> new prompt
    char *args = nullptr;
    for (uint8_t i = 0; i < idx; i++) {
      if (cmdbuf[i] == ' ') {
        cmdbuf[i] = '\0';
        args = &cmdbuf[i + 1];
        break;
      }
    }
    bool matched = false;
    for (uint8_t i = 0; i < cfg.command_count; i++) {
      if (strcasecmp(cmdbuf, cfg.commands[i].name) == 0) {
        matched = true;
        if (cfg.commands[i].requires_auth && !g_shell_authed) {
          io.println(F("[-] Access denied."));
        } else {
          cfg.commands[i].handler(args);
        }
        break;
      }
    }
    if (!matched) io.println(F("Unknown command. Type HELP."));
  }
}

// HELP / FLAG / REBOOT shell used by most challenges (one shared table
// instead of a duplicate table per challenge -> less RAM).
static const ShellCommand STD_COMMANDS[] = {
  { "HELP",   cmd_help,   false },
  { "FLAG",   cmd_flag,   false },
  { "REBOOT", cmd_reboot, false },
};

static void runStdShell(Stream &io, const uint8_t *flag, uint8_t len, bool compact = false) {
  ShellConfig cfg = { flag, len, STD_COMMANDS,
                      sizeof(STD_COMMANDS) / sizeof(STD_COMMANDS[0]),
                      nullptr, nullptr, compact };
  hammerShell(io, cfg, true);
}
#define RUN_STD_SHELL(io, f) runStdShell((io), (f).bytes, OBF_LEN(f))

// ==========================================================================
// MODE SELECTION
// FIX: comment used to claim MOD1 was the highest bit; the code (and the
// hardware labelling) make MOD1 the LSB and MOD5 the MSB. The old code also
// called modeReader() twice (2+ s boot); one debounced read is enough.
static uint8_t readModeOnce(void) {
  uint8_t m = 0;
  if (!digitalRead(MOD1)) m |= 1;
  if (!digitalRead(MOD2)) m |= 2;
  if (!digitalRead(MOD3)) m |= 4;
  if (!digitalRead(MOD4)) m |= 8;
  if (!digitalRead(MOD5)) m |= 16;
  return m;
}

int modeReader(void) {
  delay(1000);                         // power / pull-up settle
  uint8_t last = readModeOnce();
  uint8_t stable = 0;
  for (uint8_t tries = 0; tries < 100 && stable < 5; tries++) {
    delay(20);
    uint8_t m = readModeOnce();
    if (m == last) stable++; else { last = m; stable = 0; }
  }
  return last;
}

void challengeSelector(int mode){
  switch(mode){
    case 1:  handheld_challenge_uart(); break;
    case 2:  handheld_challenge_uart_v2(); break;
    case 3:  handheld_challenge_i2c(); break;
    case 4:  handheld_challenge_i2c_v2(); break;
    case 5:  handheld_challenge_firmware_extraction(); break;
    case 6:  // passive: external EEPROM already programmed, nothing to run
      break;
    case 7:  handheld_challenge_voltage_glitch(); break;
    case 8:  handheld_challenge_voltage_glitch_v2(); break;
    case 9:  handheld_challenge_side_channel_timing_attack(); break;
    case 10: handheld_challenge_side_channel_timing_attack_v2(); break;
    case 11: blackbox_chain_UART_Glitch_Attack(); break;
    case 12: blackbox_chain_UART_Timing_Attack(); break;
    case 13: handheld_challenge_simple_power_analysis(); break;
    default:
      Serial.begin(9600);
      delay(10);
      Serial.print(F("No challenge assigned to switch setting "));
      Serial.println(mode);
  }
}

// ======================>>> CHALLENGES
static void ledsInit(bool on) {
  pinMode(LED_0, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  digitalWrite(LED_0, on);
  digitalWrite(LED_1, on);
  digitalWrite(LED_2, on);
}
static void buttonsInit(void) {
  pinMode(BTN_DOT, INPUT);
  pinMode(BTN_DASH, INPUT);
  pinMode(BTN_OK, INPUT);
  pinMode(BTN_SPACE, INPUT);
}

// Ch1/Ch2: identical open shell (with LED command), different baud rate.
static const ShellCommand UART_SHELL_COMMANDS[] = {
  { "HELP",   cmd_help,   false },
  { "LED",    cmd_led,    false },
  { "FLAG",   cmd_flag,   false },
  { "REBOOT", cmd_reboot, false },
};

static void runUartShell(const uint8_t *flag, uint8_t len) {
  ShellConfig cfg = { flag, len, UART_SHELL_COMMANDS,
                      sizeof(UART_SHELL_COMMANDS) / sizeof(UART_SHELL_COMMANDS[0]),
                      nullptr, nullptr, false };
  hammerShell(Serial, cfg, true);
}

void handheld_challenge_uart(){
  ledsInit(true);
  Serial.begin(9600);
  runUartShell(FLAG_UART1.bytes, OBF_LEN(FLAG_UART1));
}

void handheld_challenge_uart_v2(){
  ledsInit(true);
  // NOTE: at 16 MHz the UART divider really produces 31250 baud (0.3 %
  // off 31337); any terminal set to either value will work.
  Serial.begin(31337);
  delay(10);
  runUartShell(FLAG_UART2.bytes, OBF_LEN(FLAG_UART2));
}

void handheld_challenge_i2c(){
  char OTP[sizeof(I2C1_PREFIX) + 10 + 1];   // prefix + 10 digits + '}' + NUL
  size_t otp_len;
  seedFromNoise();                           // before Wire takes over A4/A5
  Wire.begin();
#ifdef WIRE_HAS_TIMEOUT
  Wire.setWireTimeout(25000, true);
#endif
  decodeObf(OTP, I2C1_PREFIX.bytes, OBF_LEN(I2C1_PREFIX));
  otp_len = strlen(OTP);
  for (int r = 0; r < 10; r++) {
    OTP[otp_len++] = '0' + random(10);
  }
  OTP[otp_len++] = '}';
  OTP[otp_len] = '\0';
  while(1) {
    for (size_t i = 0; i < otp_len; i++){
      Wire.beginTransmission((uint8_t)OTP[i]);
      Wire.endTransmission();
    }
    delay(1000);
  }
}

// Ch4 "Ghost of the Machine": the board is an I2C master that polls an
// "auth device" at 0x50. Nothing ACKs at first, so a sniffer only sees the
// address byte being NACKed. The trainee must emulate a slave at 0x50 that
// ACKs. Once something ACKs, the board writes a 4-byte token to it (register
// 0x01) and then reads 4 bytes back: the trainee captures the token in their
// onReceive handler and replays it in onRequest. The token is random on
// every boot and never stored in flash, so no firmware dump helps here.
#define ADMIN_DEVICE_ADDR 0x50
#define ADMIN_REG_TOKEN   0x01
static uint8_t g_admin_token[4];

bool i2c_v2_check_admin_code(void) {
  // Throttled: polling every idle loop would make the prompt laggy.
  static unsigned long last_check = 0;
  unsigned long now = millis();
  if (now - last_check < 500) return false;
  last_check = now;

  Wire.beginTransmission(ADMIN_DEVICE_ADDR);
  Wire.write((uint8_t)ADMIN_REG_TOKEN);
  Wire.write(g_admin_token, sizeof(g_admin_token));
  if (Wire.endTransmission() != 0) return false;   // nobody ACKed: stay silent

  if (Wire.requestFrom((uint8_t)ADMIN_DEVICE_ADDR, (uint8_t)sizeof(g_admin_token))
      < sizeof(g_admin_token)) {
    while (Wire.available()) Wire.read();          // drain partial replies
    return false;
  }
  bool ok = true;
  for (uint8_t j = 0; j < sizeof(g_admin_token); j++) {
    if ((uint8_t)Wire.read() != g_admin_token[j]) ok = false;
  }
  return ok;
}

static const ShellCommand I2C_ADMIN_SHELL_COMMANDS[] = {
  { "HELP",   cmd_help,   false },
  { "STATUS", cmd_status, false },
  { "WHOAMI", cmd_whoami, true  },
  { "FLAG",   cmd_flag,   true  },
};

void handheld_challenge_i2c_v2(){
  seedFromNoise();
  for (uint8_t i = 0; i < sizeof(g_admin_token); i++) g_admin_token[i] = random(256);
  Wire.begin();
#ifdef WIRE_HAS_TIMEOUT
  Wire.setWireTimeout(25000, true);
#endif
  Serial.begin(9600);
  ShellConfig cfg = {
    FLAG_I2C_ADMIN.bytes, OBF_LEN(FLAG_I2C_ADMIN),
    I2C_ADMIN_SHELL_COMMANDS, sizeof(I2C_ADMIN_SHELL_COMMANDS) / sizeof(I2C_ADMIN_SHELL_COMMANDS[0]),
    i2c_v2_check_admin_code,
    F("[i] admin session needs the auth device on the I2C bus to answer"),
    false
  };
  hammerShell(Serial, cfg, false);
}

// Ch5: Morse keypad. The code is plaintext in flash on purpose.
static const char MORSE_SECRET[] PROGMEM = ".... ....- -- -- ...-- .-."; // H4MM3R
#define MORSE_BUF_LEN (sizeof(MORSE_SECRET) + 8)

static void waitRelease(uint8_t pin) {
  while (digitalRead(pin) == HIGH) { delay(10); }
  delay(50);
}

void handheld_challenge_firmware_extraction() {
  char MorseInput[MORSE_BUF_LEN] = {'\0'};
  uint8_t morse_index = 0;
  const uint8_t keys[3] = { BTN_DOT, BTN_DASH, BTN_SPACE };
  const char syms[3] = { '.', '-', ' ' };

  ledsInit(true);
  buttonsInit();
  Serial.begin(9600);

  Serial.println(F(" [BOOT] Entering SECURE SHELL in 3s..."));
  delay(3000);
  Serial.println(F(" [UART] Yay! Opened SECURE SHELL at 9600 bauds"));
  Serial.println(F(" [UART] Oops, you cannot access UART shell before giving the right code!"));
  Serial.println(F(" [HINT] Use the morse keypad to enter the code. Press OK to submit."));

  while (1) {
    for (uint8_t k = 0; k < 3; k++) {
      if (digitalRead(keys[k]) == HIGH) {
        if (morse_index < MORSE_BUF_LEN - 1) {
          MorseInput[morse_index++] = syms[k];
          MorseInput[morse_index] = '\0';
        }
        Serial.print(F(" [CODE] "));
        Serial.println(MorseInput);
        waitRelease(keys[k]);
      }
    }

    if (digitalRead(BTN_OK) == HIGH) {
      Serial.println(F(" [UART] Pressed OK!"));
      if (strcmp_P(MorseInput, MORSE_SECRET) == 0) {
        Serial.println(F(" [UART] ACCESS GRANTED!"));
        ledsInit(false);
        Serial.println(F(" Type HELP for available commands..."));
        RUN_STD_SHELL(Serial, FLAG_ISP);   // never returns; REBOOT resets
      } else {
        Serial.println(F(" [UART] ACCESS DENIED"));
        ledsInit(true); delay(200);
        ledsInit(false); delay(200);
        ledsInit(true);
        morse_index = 0;
        memset(MorseInput, '\0', sizeof(MorseInput));
      }
      waitRelease(BTN_OK);
    }
  }
}

void handheld_challenge_voltage_glitch(){
  const uint16_t MAX = 200;
  volatile uint16_t i;
  volatile uint16_t j;
  volatile uint32_t cnt;
  Serial.begin(9600);
  pinMode(TRIGGER_PIN, OUTPUT);
  digitalWrite(TRIGGER_PIN, LOW);
  Serial.println(F("\nI will never tell you my secret !"));
  while(1){
    cnt = 0;
    digitalWrite(TRIGGER_PIN, HIGH);
    for (i = 0; i < MAX; i++){
      for (j = 0; j < MAX; j++){
        cnt++;
      }
    }
    digitalWrite(TRIGGER_PIN, LOW);
    if (i != MAX || j != MAX || cnt != 40000UL){
      Serial.println(F("---------------------"));
      Serial.print(F("i: "));   Serial.println(i);
      Serial.print(F("j: "));   Serial.println(j);
      Serial.print(F("cnt: ")); Serial.println(cnt);
      RUN_STD_SHELL(Serial, FLAG_GLITCH1);
    }
  }
}

void handheld_challenge_voltage_glitch_v2(){
  const uint16_t MAX = 200;
  volatile uint16_t i;
  volatile uint16_t j;
  volatile uint32_t cnt;
  Serial.begin(9600);
  pinMode(LED_1, OUTPUT);                 // trigger now lives on an LED pin
  digitalWrite(LED_1, LOW);
  Serial.println(F("\nHahaha, I changed my trigger go and find it"));
  while(1){
    cnt = 0;
    digitalWrite(LED_1, HIGH);
    for (i = 0; i < MAX; i++){
      for (j = 0; j < MAX; j++){
        cnt++;
      }
    }
    digitalWrite(LED_1, LOW);
    if (i != MAX || j != MAX || cnt != 40000UL){
      RUN_STD_SHELL(Serial, FLAG_GLITCH2);
    }
  }
}

static void successBlink(void) {
  for (uint8_t b = 0; b < 3; b++) {
    digitalWrite(LED_0, HIGH); delay(100); digitalWrite(LED_0, LOW); delay(100);
    digitalWrite(LED_1, HIGH); delay(100); digitalWrite(LED_1, LOW); delay(100);
    digitalWrite(LED_2, HIGH); delay(100); digitalWrite(LED_2, LOW); delay(100);
  }
}

// Ch9: 5-digit PIN on three buttons. Side channel: delay(10) per matching
// digit before the failure LED flash. (delay(300) is just debounce.)
void handheld_challenge_side_channel_timing_attack(void){
  const uint8_t LEN = 5;
  int PIN[5];
  int UPIN[5] = {0};
  uint8_t upini = 0;
  ledsInit(true);
  buttonsInit();
  seedFromNoise();
  for (uint8_t p = 0; p < LEN; p++) PIN[p] = random(1, 4);   // 1..3
  Serial.begin(9600);
  delay(500);
  Serial.println(F("\nYou will never enter my vault!"));
  // FIX: digits are pushed through one function that wraps immediately;
  // the old code could write UPIN[5] if two buttons were held at once.
  auto push = [&](int v) {
    UPIN[upini] = v;
    if (++upini >= LEN) upini = 0;
    delay(300);
  };
  while(1){
    if (digitalRead(BTN_DOT)   == HIGH) push(1);
    if (digitalRead(BTN_DASH)  == HIGH) push(2);
    if (digitalRead(BTN_SPACE) == HIGH) push(3);
    if (digitalRead(BTN_OK) == HIGH){
      bool ok = true;
      upini = 0;
      for (uint8_t i = 0; i < LEN; i++){
        if (UPIN[i] != PIN[i]){
          ok = false;
          ledsInit(false); delay(500); ledsInit(true);
          break;
        }
        delay(10);                       // intentional timing leak
      }
      for (uint8_t i = 0; i < LEN; i++) UPIN[i] = 0;   // FIX: no stale digits
      if (ok){
        successBlink();
        RUN_STD_SHELL(Serial, FLAG_TIMING1);
      }
      delay(300);
    }
    delay(10);
  }
}

// Ch10: 4-digit PIN, OK is the 4th key; checked automatically.
void handheld_challenge_side_channel_timing_attack_v2(){
  const uint8_t LEN = 4;
  int PIN_v2[4];
  int UPIN_v2[4] = {0};
  uint8_t upini = 0;
  Serial.begin(9600);
  ledsInit(true);        // FIX: LEDs used to start LOW, hiding the first error flash
  buttonsInit();
  seedFromNoise();
  for (uint8_t p = 0; p < LEN; p++) PIN_v2[p] = random(1, 5);  // 1..4
  delay(1000);
  Serial.println(F("You are no good enough"));
  while(1) {
    int v = 0;
    if (digitalRead(BTN_DOT)   == HIGH) v = 1;
    else if (digitalRead(BTN_DASH)  == HIGH) v = 2;
    else if (digitalRead(BTN_SPACE) == HIGH) v = 3;
    else if (digitalRead(BTN_OK)    == HIGH) v = 4;
    if (v) {
      UPIN_v2[upini++] = v;
      delay(300);
      if (upini == LEN) {
        bool ok = true;
        upini = 0;
        for (uint8_t i = 0; i < LEN; i++){
          if (UPIN_v2[i] != PIN_v2[i]){
            ok = false;
            ledsInit(false); delay(500); ledsInit(true);
            break;
          }
          delay(10);                     // intentional timing leak
        }
        for (uint8_t i = 0; i < LEN; i++) UPIN_v2[i] = 0;
        if (ok){
          successBlink();
          RUN_STD_SHELL(Serial, FLAG_TIMING2);
        }
        delay(300);
      }
    }
    delay(10);
  }
}

// Ch11: UART login, glitch the compare. Requested baud 57634 really runs
// at ~57143 (UART divider limits); 57600 terminals still connect.
void blackbox_chain_UART_Glitch_Attack(void){
  char password[sizeof(SECRET_BB_PASS)];
  decodeObf(password, SECRET_BB_PASS.bytes, OBF_LEN(SECRET_BB_PASS));
  const uint8_t plen = OBF_LEN(SECRET_BB_PASS);
  char provided[sizeof(SECRET_BB_PASS) + 1];
  Serial.begin(57634);
  Serial.println(F("\nWelcome to my Login Interface!"));
  Serial.println(F("You managed to identify my UART baudrate, now please login."));
  Serial.println(F("[+] Please Provide Your Password:"));
  while(1){
    readLine(Serial, provided, sizeof(provided));
    // FIX: whole line is consumed (no leftover '\n' = no phantom second
    // attempt) and the NUL is compared too, so "password+junk" no longer passes.
    int success = 1;
    for (uint8_t i = 0; i <= plen; i++) {
      if (provided[i] != password[i]) {
        success = 0;
        break;
      }
    }
    if (success == 1) {
      Serial.println(F("[+] Login OK"));
      RUN_STD_SHELL(Serial, FLAG_BB_GLITCH);
    } else {
      Serial.println(F("[-] Login FAILED"));
      Serial.println(F("[+] Please Provide Your Password:"));
    }
  }
}

// Ch12: 8-digit PIN over SoftwareSerial 1200 baud, early-exit compare with
// delay(10) per correct digit.
void blackbox_chain_UART_Timing_Attack(void){
  SoftwareSerial mySerial(12, 11);   // RX, TX
  mySerial.begin(1200);
  char pin[sizeof(SECRET_BB_PIN)];
  decodeObf(pin, SECRET_BB_PIN.bytes, OBF_LEN(SECRET_BB_PIN));
  const uint8_t plen = OBF_LEN(SECRET_BB_PIN);
  char provided[sizeof(SECRET_BB_PIN) + 1];
  ledsInit(true);
  delay(1000);
  mySerial.println(F("Please Enter Your 8 Digit PIN:"));
  while(1){
    // FIX: no more delay(500) poll loop (it added up to 500 ms of random
    // jitter that buried the 10 ms/digit signal) and the line terminator is
    // consumed, so each attempt is answered exactly once.
    readLine(mySerial, provided, sizeof(provided));
    bool ok = true;
    for (uint8_t i = 0; i <= plen; i++) {
      if (provided[i] != pin[i]) {
        ledsInit(false); delay(500); ledsInit(true);
        ok = false;
        break;
      }
      delay(10);
    }
    if (ok){
      mySerial.println(F("[+] PIN OK"));
      runStdShell(mySerial, FLAG_BB_TIMING.bytes, OBF_LEN(FLAG_BB_TIMING), true); // compact banner @1200 baud
    } else {
      mySerial.println(F("[-] Wrong PIN!"));
      mySerial.println(F("Please Enter Your 8 Digit PIN:"));
    }
  }
}

// Ch13: trigger is HIGH while waiting for input and falls when Enter is
// received; the strcmp runs right after the falling edge.
// FIX: the old reader stored '\n' in the buffer and stopped at 14 chars, so
// the password only worked with CRLF terminals and failed with LF or CR.
void handheld_challenge_simple_power_analysis(void){
  char password[sizeof(SECRET_SPA_PW)];
  char input_password[sizeof(SECRET_SPA_PW) + 2];
  decodeObf(password, SECRET_SPA_PW.bytes, OBF_LEN(SECRET_SPA_PW));
  Serial.begin(9600);
  pinMode(TRIGGER_PIN, OUTPUT);
  seedFromNoise();
  while(1){
    digitalWrite(TRIGGER_PIN, HIGH);
    delay(250);
    Serial.println(F("[Enter Password To Tell You My Secret] : "));
    readLine(Serial, input_password, sizeof(input_password));
    digitalWrite(TRIGGER_PIN, LOW);
    if(strcmp(input_password, password) == 0){
      RUN_STD_SHELL(Serial, FLAG_SPA);
    }
    else{
      delay(random(100));
      Serial.println(F("[NOPE - GO AWAY!]"));
    }
  }
}
