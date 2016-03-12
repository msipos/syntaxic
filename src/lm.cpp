#include "core/utf8_util.hpp"
#include "lm.hpp"

#include <cctype>

#define TWISTY1(x) ((uint8_t) ((((((uint8_t) x) ^ 0x69) << 1) + 51) >> 2))

#define TWIST1(x) { for(int z = 0; z < 14; z++) { x += 7853; x *= 3; x = x % 31247; x ^= 0xA9787386; } }
#define TWIST2(x) { for(int z = 0; z < 6; z++) { x += 378; x *= x; x = x % 57942; x ^= 0xF0843728; } }
#define TWIST3(x) { for(int z = 0; z < 9; z++) { x += 738; x *= (x*x); x = x % 15783; x ^= 0xA046FA8A; } }
#define TWIST4(x) { for(int z = 0; z < 12; z++) { x += 7789; x *= x % 14; x = x % 32897; x ^= 0x329870AC; } }
#define TWIST5(x) { for(int z = 0; z < 27; z++) { x += 7892; x *= x % 7; x = x % 91890; x ^= 0x382908CA; } }

void email_to_array(const std::string& email, uint8_t* arr) {
  uint64_t seed = 0x38190f80;
  for (unsigned int i = 0; i < email.size(); i++) {
    uint8_t c = email[i];
    seed = (seed*101 + c) ^ 0x89189082;
  }

  uint8_t* sarr = (uint8_t*) (&seed);
  for (int i = 0; i < 20; i++) {
    arr[i] = sarr[i % 8] + i*i*i % 127;
  }
}

LMKey string_to_key(const std::string& key) {
  LMKey okey;
  int j = 0;
  for (int i = 0; i < 24; i++) {
    uint8_t c = tolower(key[i]);
    if (c == '-') continue;
    if (c >= 'a' && c <= 'z') c = c - 'a';
    else if (c >= '0' && c <= '9') c = 26 + c - '0';
    okey.b[j] = c;
    j++;
  }
  return okey;
}

bool well_formed(const std::string& key) {
  if (key.size() != 24) return false;
  if (key[4] != '-') return false;
  if (key[9] != '-') return false;
  if (key[14] != '-') return false;
  if (key[19] != '-') return false;
  for (int i = 0; i < 24; i++) {
    uint8_t c = key[i];
    if (c == '-') continue;
    if (c < '0') return false;
    if (c > '9' && c < 'A') return false;
    if (c > 'Z' && c < 'a') return false;
    if (c > 'z') return false;
  }
  return true;
}

bool well_formed(LMKey key) {
  uint32_t checksum = 0;
  for (int i = 0; i < 19; i++) {
    checksum += key.b[i];
    checksum = checksum << 1;
    checksum += 43;
    checksum = checksum >> 1;
  }
  return key.b[19] == (checksum % 36);
}

bool check(LMKey key, const std::string& email) {
  std::string str = utf8_string_lower(email);
  uint8_t arr[20];
  email_to_array(email, arr);

#ifdef LMCHECK0
#define ID 0
  {
    uint32_t x = arr[ID];
    TWIST1(x); x ^= 0x367889A2; TWIST1(x); TWIST2(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

#ifdef LMCHECK1
#define ID 1
  {
    uint32_t x = arr[ID];
    TWIST1(x); x ^= 0x3289AC89; TWIST2(x); TWIST1(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

#ifdef LMCHECK2
#define ID 2
  {
    uint32_t x = arr[ID];
    TWIST3(x); x ^= 0xFC893080; TWIST1(x); TWIST2(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

#ifdef LMCHECK3
#define ID 3
  {
    uint32_t x = arr[ID];
    TWIST3(x); x ^= 0xFAFA3278; TWIST2(x); TWIST1(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

#ifdef LMCHECK4
#define ID 4
  {
    uint32_t x = arr[ID];
    TWIST2(x); x ^= 0x328907AC; TWIST1(x); TWIST3(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

#ifdef LMCHECK5
#define ID 5
  {
    uint32_t x = arr[ID];
    TWIST4(x); x ^= 0x32315CAD; TWIST3(x); TWIST1(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

#ifdef LMCHECK6
#define ID 6
  {
    uint32_t x = arr[ID];
    TWIST2(x); x ^= 0xCDAB8971; TWIST2(x); TWIST2(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

#ifdef LMCHECK7
#define ID 7
  {
    uint32_t x = arr[ID];
    TWIST4(x); x ^= 0x8398ACBB; TWIST4(x); TWIST1(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

#ifdef LMCHECK8
#define ID 8
  {
    uint32_t x = arr[ID];
    TWIST4(x); x ^= 0xBA783601; TWIST3(x); TWIST2(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

#ifdef LMCHECK9
#define ID 9
  {
    uint32_t x = arr[ID];
    TWIST1(x); x ^= 0x79308CA0; TWIST2(x); TWIST1(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

#ifdef LMCHECK10
#define ID 10
  {
    uint32_t x = arr[ID];
    TWIST1(x); x ^= 0xFF9BA77C; TWIST1(x); TWIST1(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

#ifdef LMCHECK11
#define ID 11
  {
    uint32_t x = arr[ID];
    TWIST4(x); x ^= 0xACC31F98; TWIST2(x); TWIST2(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

#ifdef LMCHECK12
#define ID 12
  {
    uint32_t x = arr[ID];
    TWIST1(x); x ^= 0x3810CBD2; TWIST4(x); TWIST4(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

#ifdef LMCHECK13
#define ID 13
  {
    uint32_t x = arr[ID];
    TWIST3(x); x ^= 0x3418ACDB; TWIST4(x); TWIST3(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

#ifdef LMCHECK14
#define ID 14
  {
    uint32_t x = arr[ID];
    TWIST2(x); x ^= 0xCBA3891A; TWIST4(x); TWIST4(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

#ifdef LMCHECK15
#define ID 15
  {
    uint32_t x = arr[ID];
    TWIST5(x); x ^= 0xCD839A11; TWIST4(x); TWIST3(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

#ifdef LMCHECK16
#define ID 16
  {
    uint32_t x = arr[ID];
    TWIST3(x); x ^= 0x38910CD1; TWIST5(x); TWIST3(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

#ifdef LMCHECK17
#define ID 17
  {
    uint32_t x = arr[ID];
    TWIST2(x); x ^= 0x09570CAA; TWIST5(x); TWIST1(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

#ifdef LMCHECK18
#define ID 18
  {
    uint32_t x = arr[ID];
    TWIST5(x); x ^= 0xFF893FFA; TWIST5(x); TWIST5(x);
    if (x % 36 != key.b[ID]) return false;
  }
#undef ID
#endif

  return true;
}