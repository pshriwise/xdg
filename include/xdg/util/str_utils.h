#ifndef _XDG_STR_UTIL
#define _XDG_STR_UTIL

#include <string>
#include <vector>

namespace xdg
{

std::vector<std::string> tokenize(const std::string& str,
                                  const std::string& delimiters);

std::string& strtrim(std::string& s);


} // namespace xdg

#endif