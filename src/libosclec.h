#ifndef LIBOSCLEC_H
#define LIBOSCLEC_H

#include <TDirectory.h>
#include <filesystem>
#include <set>
namespace fs = std::filesystem;
namespace osclec {

void parseFiles(TDirectory *rootDir, std::set<fs::path> inputFileNames,
                Float_t thr, Bool_t InvertPolarity);
} // namespace osclec

#endif