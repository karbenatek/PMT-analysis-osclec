#include <filesystem>
#include <iostream>
#include <limits.h>
#include <ostream>
#include <set>
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
