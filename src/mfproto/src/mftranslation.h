#ifndef MF_TRANSLATION_H
#define MF_TRANSLATION_H
#include "vector"
#include "wiring.h"

typedef std::vector<String> MF_TRANSLATION;
typedef struct {
  std::vector<String> str;
  std::vector<String> vals;
  uint16_t sel;
} MF_STRINGS;

enum MF_LANGUAGE { ENGLISH = 0, RUSSIAN };

#define PRINTF_STRING_MAX 256
String &getTranslation(MF_TRANSLATION text, MF_LANGUAGE language = ENGLISH);
String printfString(const char *format, ...);
#endif