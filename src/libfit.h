#ifndef LIBFIT_H
#define LIBFIT_H

#include <TDirectory.h>
#include <string>

void FitSPE(TDirectory *inputRootDir, TDirectory *outputRootDir,
            std::string histName, Int_t n_smooth);

void FitGauss(TDirectory *inputRootDir, TDirectory *outputRootDir,
              std::string HistName);
void FitExGauss(TDirectory *inputRootDir, TDirectory *outputRootDir,
                std::string HistName);
void FitRightGauss(TDirectory *inputRootDir, TDirectory *outputRootDir,
                   std::string HistName, Int_t nSmooth);

#endif