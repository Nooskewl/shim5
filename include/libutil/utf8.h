#ifndef NOO_UTF8_H
#define NOO_UTF8_H

#include <cstdint>

namespace noo {

namespace util {

int utf8_len(std::string text);
int utf8_len_bytes(std::string text, int char_count);
uint32_t utf8_char_next(std::string text, int &offset);
uint32_t utf8_char_offset(std::string text, int o);
uint32_t utf8_char(std::string text, int i);
std::string utf8_char_to_string(uint32_t ch);
std::string utf8_substr(std::string s, int start, int count = -1);

} // End namespace util

} // End namespace noo

#endif // NOO_UTF8_H
