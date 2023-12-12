#ifndef ROUTINES_H
#define ROUTINES_H

#include <RtypesCore.h>
#include <filesystem>
#include <set>
#include <string>

namespace fs = std::filesystem;

fs::path getDefaultCfgFile(fs::path inCfgName = "");

std::set<fs::path> getFilesInDir(fs::path DirectoryPath,
                                 fs::path ParrentPath = "");

std::string s_to_10fs(std::string s, Int_t Shift = 14);

std::string s_to_10fs(Double_t t);

#endif // ROUTINES_H
