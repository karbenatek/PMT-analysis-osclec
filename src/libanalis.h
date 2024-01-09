#ifndef LIBANALIS_H
#define LIBANALIS_H

#include <RtypesCore.h>
#include <TDirectory.h>

void doCFDPulseAnalysis(TDirectory *InputRootDir, TDirectory *OutputRootDir,
                        const Float_t Threshold, const Float_t CutFraction,
                        const Bool_t UseTotalTime);

void doTWPulseAnalysis(TDirectory *InputRootDir, TDirectory *OutputRootDir,
                       const Float_t Threshold, const Double_t t0,
                       const Double_t t1, const Bool_t UseTotalTime,
                       const Bool_t FitPedestal);

void makeTH1F(TDirectory *InputRootDir, TDirectory *OutputRootDir,
              std::string BranchName, Int_t Bins, Double_t XLow = 0,
              Double_t XHigh = 0);

Double_t GetDarkRate(TDirectory *drDir, TDirectory *spDir,
                     TDirectory *OutputRootDir, const std::string HistName,
                     Float_t Threshold, const Bool_t UseSPEThreshold = false);

void doMultiPulseAnalysis(TDirectory *InputRootDir, TDirectory *OutputRootDir,
                          const Float_t Threshold,
                          const Double_t pulse_length_threshold);

void doAfterPulseAnalysis(TDirectory *InputRootDir, TDirectory *OutputRootDir,
                          const Double_t TimeWindowEnd_us);

#endif // LIBANALIS_H
