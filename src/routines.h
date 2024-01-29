#ifndef ROUTINES_H
#define ROUTINES_H

#include <RtypesCore.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TH1.h>
#include <TPad.h>
#include <TParameter.h>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <set>
#include <string>

namespace fs = std::filesystem;

fs::path getDefaultCfgFile(fs::path inCfgName = "");

std::set<fs::path> getFilesInDir(fs::path DirectoryPath,
                                 fs::path ParrentPath = "");
/*
Concatenates a filepath relative to TFile containing rootDir with prefix
of rootDir name
*/
std::filesystem::path addAppendixToRelativeFilePath(std::string Appendix,
                                                    TDirectory *rootDir);

std::string s_to_10fs(std::string s, Int_t Shift = 14);

std::string s_to_10fs(Double_t t);

template <typename T>
inline T *LoadHist(TDirectory *rootDir, std::string HistName) {
  if (rootDir->Get(HistName.c_str()) == nullptr) {
    std::cout << "Histogram \"" << HistName << "\" not found in TDirectory \""
              << rootDir->GetName() << "\" of TFile \""
              << rootDir->GetFile()->GetName() << "\"" << std::endl;
    exit(1);
  }
  return rootDir->Get<T>(HistName.c_str());
}

template <typename T>
inline T LoadPar(TDirectory *rootDir, std::string ParName) {
  if (rootDir->Get(ParName.c_str()) == nullptr) {
    std::cout << "Parameter \"" << ParName << "\" not found in TDirectory \""
              << rootDir->GetName() << "\" of TFile \""
              << rootDir->GetFile()->GetName() << "\"" << std::endl;
    exit(1);
  }
  return rootDir->Get<TParameter<T>>(ParName.c_str())->GetVal();
}

template <typename T>
inline void SavePar(TDirectory *rootDir, T Par, std::string ParName) {
  if (rootDir) {
    TParameter<T> TPar;
    TPar.SetVal(Par);
    rootDir->WriteObject(&TPar, ParName.c_str());
  }
  return;
}

// TODO: add attaching of branches
template <typename T>
inline T LoadTree(TDirectory *rootDir, std::string TreeName,
                  std::string BranchName[]) {
  if (rootDir->Get(TreeName.c_str()) == nullptr) {
    std::cout << "TTree \"" << TreeName << "\" not found in TDirectory \""
              << rootDir->GetName() << "\" of TFile \""
              << rootDir->GetFile()->GetName() << "\"" << std::endl;
    exit(1);
  }
  rootDir->Get<TParameter<T>>(TreeName.c_str())->GetVal();
}

#endif // ROUTINES_H
