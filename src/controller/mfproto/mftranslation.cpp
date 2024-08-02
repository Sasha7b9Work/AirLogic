#include "mftranslation.h"

String &getTranslation(MF_TRANSLATION text, MF_LANGUAGE language) {
  if (text.size() > (size_t)language)
    return text.at(language);
  else
    return text.at(ENGLISH);
}

String printfString(const char *format, ...) {
  char buf[PRINTF_STRING_MAX + 1];
  va_list list;
  va_start(list, format);
  vsnprintf(buf, PRINTF_STRING_MAX, format, list);
  va_end(list);
  return String(buf);
}
