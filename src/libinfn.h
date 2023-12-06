#ifndef LIBINFN_H
#define LIBINFN_H
#include <TDirectory.h>
#include <filesystem>
namespace infn {
void parseFile(std::filesystem::path InputCsvFilePath,
               TDirectory *outputRootDir);
}
#endif