#include <RtypesCore.h>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits.h>
#include <math.h>
#include <ostream>
#include <set>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

fs::path getDefaultCfgFile(fs::path inCfgName = "") {
  /*
  get specified or default (cfg.toml) cfg file in exe_file_path/../cfg/
  */
  // set default
  if (inCfgName.empty())
    inCfgName = "cfg.toml";

  char buffer[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
  if (len != -1) {
    buffer[len] = '\0';
    std::string pathStr(buffer);
    fs::path cfgPath =
        fs::path(pathStr).parent_path().parent_path().append("cfg").append(
            inCfgName.c_str());

    if (fs::exists(cfgPath)) {
      return cfgPath;
    } else {
      std::cerr << "Cfg file " << cfgPath << "not found" << std::endl;
      exit(1);
    }
  }
  return fs::path("/");
}

std::set<fs::path> getFilesInDir(fs::path DirectoryPath,
                                 fs::path ParrentPath = "") {
  /*
  get sorted file paths in specified directory
  */
  fs::path FilePath;
  std::set<fs::path> sorted_paths;
  for (auto &dir_entry :
       fs::directory_iterator(ParrentPath.append(DirectoryPath.c_str()))) {
    sorted_paths.insert(dir_entry.path());
  }
  return sorted_paths;
}

std::string s_to_10fs(std::string s, Int_t Shift = 14) {
  std::string result;
  int decimalIndex = s.find('.');
  // std::cout << " > " << std::string("ab-") + s + std::string("-cd")
  //           << std::endl;
  // exit(0);
  if (decimalIndex == std::string::npos)
    s = "0.0";

  // Extract the integer part before the decimal point
  std::string integerPart = s.substr(0, decimalIndex);

  // Extract the fractional part after the decimal point
  std::string fractionalPart = s.substr(decimalIndex + 1);

  // cut or extend fractional part
  if (fractionalPart.size() > Shift) {
    if (fractionalPart.at(Shift) >= Shift) {
      // coerce
      fractionalPart.insert(Shift, ".");
      Float_t fracDouble = round(std::stod(fractionalPart));
      fractionalPart = std::to_string(static_cast<Int_t>(fracDouble));
    }
    // cut
    fractionalPart = fractionalPart.substr(0, Shift);
  } else
    // add zeros if fractional part is short
    while (fractionalPart.size() < Shift)
      fractionalPart += '0';

  // concatenate previous decimal and fractional part
  result = integerPart + fractionalPart;

  // std::cout << "#########" << std::endl
  //           << s << std::endl
  //           << result << std::endl;

  return result;
}

std::string s_to_10fs(Double_t t) {
  std::cout << "####" << std::endl;
  std::cout << t << std::endl;
  t = t * 10e13;
  std::cout << t << std::endl;
  t = round(t);
  std::string s = std::to_string(t);
  std::cout << s << std::endl;
  s = s.substr(0, s.find_first_of('.'));
  std::cout << s << std::endl;

  Int_t exp;
  Double_t mant = std::frexp(t, &exp);
  // std::cout << mant << std::endl;
  // std::cout << exp << std::endl;

  return s;
}