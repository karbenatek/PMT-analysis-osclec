#ifndef LIBANALIS_H
#define LIBANALIS_H

#include <TDirectory.h>

namespace pmanalysis {
void doPulseAnalysis(TDirectory *inputRootDir, TDirectory *outputRootDir,
                     const Int_t bins_ampl, const Int_t bins_Q,
                     const Int_t bins_Qnoise, const Double_t threshold,
                     const Double_t max_ampl, const Double_t cut_fraction);

Double_t GetDarkRate(TDirectory *drDir, TDirectory *spDir,
                     TDirectory *outputRootDir, const Double_t pe_threshold);

void doAfterPulseAnalysis(TDirectory *inputRootDir, TDirectory *outputRootDir,
                          const Double_t threshold,
                          const Float_t pulse_length_threshold);
} // namespace pmanalysis

#endif // LIBANALIS_H
