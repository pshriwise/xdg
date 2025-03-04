#ifndef _XDG_STR_UTIL
#define _XDG_STR_UTIL

#include <string>
#include <vector>

namespace xdg
{

std::vector<std::string> tokenize(const std::string& str,
                                  const std::string& delimiters);

std::string& strtrim(std::string& s, std::string symbols=" \t\n\r\f\v");

std::string& to_lower(std::string& str);

bool ends_with(const std::string& value, const std::string& ending);

bool starts_with(const std::string& value, const std::string& beginning);

std::string& remove_substring(std::string& s, const std::string& sub);

} // namespace xdg

#endif