#include "xdg/util/str_utils.h"

#include <algorithm>

namespace xdg {
// from DAGMC
std::vector<std::string>
tokenize(const std::string& str,
         const std::string& delimiters)
{
  const char* delim = delimiters.c_str();

  std::vector<std::string> tokens;
  std::string::size_type last = str.find_first_not_of(delim, 0);
  std::string::size_type pos = str.find_first_of(delim, last);
  if (std::string::npos == pos)
    tokens.push_back(str);
  else
    while (std::string::npos != pos && std::string::npos != last) {
      tokens.push_back(str.substr(last, pos - last));
      last = str.find_first_not_of(delim, pos);
      pos = str.find_first_of(delim, last);
      if (std::string::npos == pos) pos = str.size();
    }

  // trim all outgoing strings to remove null termination values '\0` (if present)
  for (auto& token : tokens)
    token.erase(std::find(token.begin(), token.end(), '\0'), token.end());

  return tokens;
}

// from OpenMC
std::string& strtrim(std::string& s, std::string symbols)
{
  s.erase(s.find_last_not_of(symbols.c_str()) + 1);
  s.erase(0, s.find_first_not_of(symbols.c_str()));
  return s;
}

} // namespace xdg