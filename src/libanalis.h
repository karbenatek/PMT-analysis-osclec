#ifndef LIBANALIS_H
#define LIBANALIS_H

#include <RtypesCore.h>
#include <TDirectory.h>

namespace pmta {
void doCFDPulseAnalysis(TDirectory *InputRootDir, TDirectory *OutputRootDir,
                        const Float_t Threshold, const Float_t CutFraction,
                        const Bool_t UseTotalTime);

void makeTH1F(TDirectory *InputRootDir, TDirectory *OutputRootDir,
              std::string BranchName, Int_t Bins, Double_t XLow = 0,
              Double_t XHigh = 0);

Double_t GetDarkRate(TDirectory *drDir, TDirectory *spDir,
                     TDirectory *OutputRootDir, const std::string HistName,
                     Float_t Threshold, const Bool_t UseSPEThreshold = false);

void doAfterPulseAnalysis(TDirectory *inputRootDir, TDirectory *outputRootDir,
                          const Float_t threshold,
                          const Double_t pulse_length_threshold);
} // namespace pmta

#endif // LIBANALIS_H
