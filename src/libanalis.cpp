#include "routines.h"
#include <RtypesCore.h>
#include <TCanvas.h>
#include <TDirectory.h>
#include <TF1.h>
#include <TFile.h>
#include <TGraph.h>
#include <TH1.h>
#include <TH2.h>
#include <TMath.h>
#include <TParameter.h>
#include <TProfile.h>
#include <TText.h>
#include <TTree.h>

#include <TVirtualPad.h>
#include <cstdint>

#include <cstdlib>
#include <iostream>
#include <iterator>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

// Get time index of threshold rising edge
Int_t GetT0(std::vector<Double_t> *t, std::vector<Float_t> *x,
            const Float_t Threshold) {
  Int_t i = 0;
  Int_t x_size = x->size() - 1;

  // go to trigger time
  if (t->at(0) < 0) {
    while (t->at(i) < 0) {
      ++i;
    }
    --i;
  }

  // go to threshold time
  while (i < x_size && x->at(i) < Threshold)
    ++i;

  return i;
}

struct Q_duo {
  Float_t cfd = 0;
  Float_t w = 0;
};

// Integrate charge using with CFD pulse window
Float_t GetQByCFD(std::vector<Double_t> *t, std::vector<Float_t> *x,
                  Double_t max, Int_t iMax, Double_t &T0, Double_t &t0,
                  Double_t &t1, Float_t cut_ratio, Float_t Threshold,
                  Float_t pedestal) {
  // set position to maximum
  Float_t Q;
  Int_t i = iMax;
  Int_t i_T0 = GetT0(t, x, Threshold);
  T0 = t->at(i_T0);
  //  set CFD threshold
  Float_t cut_threshold =
      cut_ratio * (max - pedestal) + (1 - cut_ratio) * x->at(i_T0);
  Float_t X;
  // integrate to the right
  while (++i < x->size()) {

    X = x->at(i) - pedestal;
    Q += X;
    if (x->at(i) <= cut_threshold)
      break;
  }
  t1 = t->at(--i);

  // integrate to the left
  i = iMax - 1;
  while (i-- > 0) {
    X = x->at(i) - pedestal;
    Q += X;
    if (x->at(i) <= cut_threshold)
      break;
  }
  t0 = t->at(++i);

  return Q;
}

Float_t GetQByTW(std::vector<Double_t> *t, std::vector<Float_t> *x, Double_t t0,
                 Double_t t1, Bool_t FitPedestal) {
  Int_t i = 0, i0, i1;
  TGraph gAroundWindow;
  TF1 fPedestal("", "[0] + [1]*x");
  Float_t Pedestal = 0;
  Int_t nPedestal = 0;
  Double_t time;
  // goto t0
  while (i < x->size() && t->at(i) < t0) {
    if (FitPedestal)
      gAroundWindow.AddPoint(t->at(i), x->at(i));
    else {
      Pedestal += x->at(i);
      nPedestal++;
    }
    ++i;
  }
  i0 = i;

  // goto t1 from back
  i = x->size() - 1;
  while (i0 < i && t->at(i) > t1) {
    if (FitPedestal)
      gAroundWindow.AddPoint(t->at(i), x->at(i));
    else {
      Pedestal += x->at(i);
      nPedestal++;
    }
    --i;
  }
  i1 = i;

  if (FitPedestal) {
    // fit pedestal from points outside window
    fPedestal.SetParameters(0, 0);
    gAroundWindow.Fit(&fPedestal, "QN");
  } else
    Pedestal /= nPedestal;
  fPedestal.SetParameter(1, -10000);
  fPedestal.Draw("same");
  Float_t Q = 0;
  i = i0;
  // integrate in time window
  while (i <= i1) {
    Q += x->at(i);
    // subtract pedestal
    Q -= FitPedestal ? fPedestal.Eval(t->at(i)) : Pedestal;
    i++;
  }
  return Q;
}

// get maximum and its position
Float_t GetMax(std::vector<Float_t> *x, Int_t &iMax) {
  Float_t max = -100.;
  for (Int_t i = 1; i < (x->size() - 1); ++i) {
    if (x->at(i) > max) {
      max = x->at(i);
      iMax = i;
    }
  }
  return max;
}

void doCFDPulseAnalysis(TDirectory *InputRootDir, TDirectory *OutputRootDir,
                        const Float_t Threshold, const Float_t CutFraction,
                        const Bool_t UseTotalTime) {

  Int_t iSeg;
  char Date[11]; // in format yyyy-mm-dd
  char Time[9];  // in format hh:mm:ss
  uint64_t Time_10fs;
  std::vector<Float_t> *x = 0;
  std::vector<Double_t> *t = 0;

  // Attach to input TTree
  TTree *PulseTree = (TTree *)InputRootDir->Get("pmtaf_pulses");
  PulseTree->SetBranchAddress("i_seg", &iSeg);
  PulseTree->SetBranchAddress("date", Date);
  PulseTree->SetBranchAddress("time", Time);
  PulseTree->SetBranchAddress("time_10fs", &Time_10fs);
  PulseTree->SetBranchAddress("x", &x);
  PulseTree->SetBranchAddress("t", &t);
  PulseTree->GetEntry(0);

  // parsed variables
  Float_t TimeOverThreshold_ns; // aka pulse length
  Float_t Energy;
  Float_t Amplitude;
  Int_t iSweep = 0;

  // init output TTree and create branches
  TTree *PMTAFTree = new TTree("pmtaf_tree", "Pulse events from OscLec data");
  PMTAFTree->SetAutoSave(0);
  PMTAFTree->Branch("time_10fs", &Time_10fs);
  PMTAFTree->Branch("time_over_threshold_ns", &TimeOverThreshold_ns);
  PMTAFTree->Branch("i_sweep", &iSweep);
  PMTAFTree->Branch("amplitude_V", &Amplitude);
  PMTAFTree->Branch("energy", &Energy);
  PMTAFTree->Branch("date", Date, "date/C", 10);
  PMTAFTree->Branch("time", Time, "time/C", 8);

  TH2D *wfm_Stack = new TH2D("wfm_stack", "Waveform stack", t->size(),
                             t->front(), t->back(), 64, 0, 0);
  TH1D *h_t0 = new TH1D("h_t0", "t0", t->size(), t->front(), t->back());
  TH1D *h_t1 = new TH1D("h_t1", "t1", t->size(), t->front(), t->back());
  // TH1D *h_tts = new TH1D("h_tts", "Transit time spread", t->size(),
  // t->front(),
  //                        t->back());
  TH1D *h_t_peak =
      new TH1D("h_t_max", "Peak time", t->size(), t->front(), t->back());
  TH2D *h2_AQ = new TH2D("h2_AQ", "Voltage vs Charge", 64, 0, 0, 64, 0, 0);
  h2_AQ->GetXaxis()->SetTitle("Integrated charge [a.u.]");
  h2_AQ->GetYaxis()->SetTitle("Voltage [V]");

  TProfile *pf_AQ = new TProfile("pf_AQ", "Voltage vs Charge", 80, 0, 0, "");
  pf_AQ->GetXaxis()->SetTitle("Integrated charge [a.u.]");
  pf_AQ->GetYaxis()->SetTitle("Amplitude [V]");
  pf_AQ->SetMarkerColor(3);
  pf_AQ->SetLineColor(880);
  pf_AQ->SetLineStyle(1);
  pf_AQ->SetLineWidth(1);

  Int_t n_Seg = x->size();
  Double_t t_top = t->back();
  Int_t iMax;
  Double_t t0; // Pulse window start
  Double_t t1; // Pulse window end
  Double_t T0; // trigger time in s :]
  Double_t CurrTime_s = 0;
  Double_t PrevTime_s = 0;
  Double_t MeasTime_s = 0;
  TParameter<Double_t> MeasTimeTParameter_s;
  Int_t throwed = 0;
  Double_t pedestal = 0;
  Double_t len_pedestal = 0;

  // 1st waveforms iteration to obtain pedestal
  Int_t nEntries = PulseTree->GetEntries();

  for (Long64_t iEntry = 0; iEntry < nEntries; iEntry++) { // events iterator
    PulseTree->GetEntry(iEntry);

    Amplitude = GetMax(x, iMax);
    // v_iMax.push_back(iMax);
    h_t_peak->Fill(t->at(iMax));
    CurrTime_s = static_cast<Double_t>(Time_10fs) * 1e-14;
    // clock overflow
    // std::cout << CurrTime_s << std::endl;
    if (iEntry > 0 && PrevTime_s > CurrTime_s)
      MeasTime_s += PrevTime_s;
    // last event
    else if (iEntry == (nEntries - 1))
      MeasTime_s += CurrTime_s;

    PrevTime_s = CurrTime_s;

    // get sample for pedestal
    if (Amplitude <= Threshold)
      // in case when the signal does not reach Threshold, take whole waveform
      for (Float_t val : *x) {
        pedestal += static_cast<Double_t>(val);
        len_pedestal++;
      }
    else {
      // in other case, take first 10 samples
      for (int i = 0; i < 10; ++i) {
        pedestal += x->at(i);
      }
      len_pedestal += 10;
    }
  }
  MeasTimeTParameter_s.SetVal(MeasTime_s);

  // get pedestal average
  pedestal /= len_pedestal;
  // 2nd loop
  for (Long64_t iEntry = 0; iEntry < nEntries; iEntry++) { // events iterator
    PulseTree->GetEntry(iEntry);

    iSweep = iEntry;
    // get maximum
    Amplitude = GetMax(x, iMax);

    // integrate charge Q
    Energy = GetQByCFD(t, x, Amplitude, iMax, T0, t0, t1, CutFraction,
                       Threshold, pedestal);

    TimeOverThreshold_ns = static_cast<Float_t>((t1 - t0) * 1e9);

    if (UseTotalTime)
      Time_10fs += stoull(s_to_10fs(T0));
    else
      Time_10fs = stoull(s_to_10fs(T0));

    h2_AQ->Fill(Energy, Amplitude);
    pf_AQ->Fill(Energy, Amplitude);

    // add time to total meas time when timer resets
    if (iEntry > 0 && Time_10fs <= 0.000000001)
      MeasTime_s += static_cast<Double_t>(Time_10fs) * 1e-14;

    h_t0->Fill(t0);
    h_t1->Fill(t1);
    PMTAFTree->Fill();

    // fill Wfm stack
    for (Int_t i = 0; i < x->size(); ++i) // wfms iterator
      wfm_Stack->Fill(t->at(i), static_cast<Double_t>(x->at(i)));

    // progress printout
    if ((iEntry) % 1000 == 0 && iEntry != 0)
      std::cout << iEntry << "/" << nEntries << std::endl;
  }

  std::cout << nEntries << "/" << nEntries << std::endl;
  std::cout << "Pulse analysis finished\n" << std::endl;

  // corelation slope
  std::cout << "From " << nEntries << " wfms, " << throwed << " was deleted"
            << std::endl;

  // fit correlation histogram with line
  TF1 *f_A = new TF1("f_A", "[0] + [1]*x");

  f_A->SetParameters(0., 0.);
  pf_AQ->Fit(f_A, "Q", "", static_cast<Double_t>(Threshold),
             pf_AQ->GetXaxis()->GetXmax());

  // write output object to RootDir
  OutputRootDir->WriteObject(h2_AQ, "h2_AQ");
  OutputRootDir->WriteObject(h_t0, "h_t0");
  OutputRootDir->WriteObject(h_t1, "h_t1");
  OutputRootDir->WriteObject(h_t_peak, "h_t_peak");
  OutputRootDir->WriteObject(wfm_Stack, "wfm_Stack");
  OutputRootDir->WriteObject(pf_AQ, "pf_AQ");
  OutputRootDir->WriteObject(PMTAFTree, "pmtaf_tree");
  OutputRootDir->WriteObject(&MeasTimeTParameter_s, "meas_time_s");
}

void doTWPulseAnalysis(TDirectory *InputRootDir, TDirectory *OutputRootDir,
                       const Float_t Threshold, const Double_t t0,
                       const Double_t t1, const Bool_t UseTotalTime,
                       const Bool_t FitPedestal) {

  Int_t iSeg;
  char Date[11]; // in format yyyy-mm-dd
  char Time[9];  // in format hh:mm:ss
  uint64_t Time_10fs;
  std::vector<Float_t> *x = 0;
  std::vector<Double_t> *t = 0;

  // Attach to input TTree
  TTree *PulseTree = (TTree *)InputRootDir->Get("pmtaf_pulses");
  PulseTree->SetBranchAddress("i_seg", &iSeg);
  PulseTree->SetBranchAddress("date", Date);
  PulseTree->SetBranchAddress("time", Time);
  PulseTree->SetBranchAddress("time_10fs", &Time_10fs);
  PulseTree->SetBranchAddress("x", &x);
  PulseTree->SetBranchAddress("t", &t);
  PulseTree->GetEntry(0);

  // parsed variables
  Float_t Energy;
  Float_t Amplitude;
  Int_t iSweep = 0;

  // init output TTree and create branches
  TTree *PMTAFTree = new TTree("pmtaf_tree", "Pulse events from OscLec data");
  PMTAFTree->SetAutoSave(0);
  PMTAFTree->Branch("time_10fs", &Time_10fs);
  PMTAFTree->Branch("i_sweep", &iSweep);
  PMTAFTree->Branch("amplitude_V", &Amplitude);
  PMTAFTree->Branch("energy", &Energy);
  PMTAFTree->Branch("date", Date, "date/C", 10);
  PMTAFTree->Branch("time", Time, "time/C", 8);

  TH2D *wfm_Stack = new TH2D("wfm_stack", "Waveform stack", t->size(),
                             t->front(), t->back(), 64, 0, 0);
  TH1D *h_t_peak =
      new TH1D("h_t_max", "Peak time", t->size(), t->front(), t->back());
  TH2D *h2_AQ = new TH2D("h2_AQ", "Voltage vs Charge", 64, 0, 0, 64, 0, 0);
  h2_AQ->GetXaxis()->SetTitle("Integrated charge [a.u.]");
  h2_AQ->GetYaxis()->SetTitle("Voltage [V]");

  TProfile *pf_AQ = new TProfile("pf_AQ", "Voltage vs Charge", 80, 0, 0, "");
  pf_AQ->GetXaxis()->SetTitle("Integrated charge [a.u.]");
  pf_AQ->GetYaxis()->SetTitle("Amplitude [V]");
  pf_AQ->SetMarkerColor(3);
  pf_AQ->SetLineColor(880);
  pf_AQ->SetLineStyle(1);
  pf_AQ->SetLineWidth(1);

  Int_t n_Seg = x->size();
  Double_t t_top = t->back();
  Int_t iMax;
  Double_t T0; // trigger time in s :]
  Double_t CurrTime_s = 0;
  Double_t PrevTime_s = 0;
  Double_t MeasTime_s = 0;

  Int_t nEntries = PulseTree->GetEntries();
  for (Long64_t iEntry = 0; iEntry < nEntries; iEntry++) { // events iterator
    PulseTree->GetEntry(iEntry);
    Energy = GetQByTW(t, x, t0, t1, FitPedestal);
    iSweep = iEntry;
    T0 = t->at(GetT0(t, x, Threshold));
    Amplitude = GetMax(x, iMax);

    h_t_peak->Fill(t->at(iMax));
    CurrTime_s = static_cast<Double_t>(Time_10fs) * 1e-14;

    // clock overflow
    if (iEntry > 0 && PrevTime_s > CurrTime_s)
      MeasTime_s += PrevTime_s;
    // last event
    else if (iEntry == (nEntries - 1))
      MeasTime_s += CurrTime_s;

    // add time to total meas time when timer resets
    if (iEntry > 0 && Time_10fs <= 0.000000001)
      MeasTime_s += CurrTime_s;

    if (UseTotalTime)
      Time_10fs += stoull(s_to_10fs(T0));
    else
      Time_10fs = stoull(s_to_10fs(T0));

    PrevTime_s = CurrTime_s;

    // fitt tree
    PMTAFTree->Fill();

    // fill corelation histograms
    h2_AQ->Fill(Energy, Amplitude);
    pf_AQ->Fill(Energy, Amplitude);

    // fill wfm stack
    for (Int_t i = 0; i < x->size(); ++i) // wfms iterator
      wfm_Stack->Fill(t->at(i), static_cast<Double_t>(x->at(i)));

    // progress printout
    if ((iEntry) % 1000 == 0 && iEntry != 0)
      std::cout << iEntry << "/" << nEntries << std::endl;
  }
  std::cout << nEntries << "/" << nEntries << std::endl;
  std::cout << "Pulse analysis finished\n" << std::endl;

  // fit correlation histogram with line
  TF1 *f_A = new TF1("f_A", "[0] + [1]*x");
  // f_A->SetParameters(0., 0.);
  pf_AQ->Fit(f_A, "Q", "", 0., pf_AQ->GetXaxis()->GetXmax());

  // write output object to RootDir
  OutputRootDir->WriteObject(h2_AQ, "h2_AQ");
  OutputRootDir->WriteObject(h_t_peak, "h_t_peak");
  OutputRootDir->WriteObject(wfm_Stack, "wfm_Stack");
  OutputRootDir->WriteObject(pf_AQ, "pf_AQ");
  OutputRootDir->WriteObject(PMTAFTree, "pmtaf_tree");
  SavePar(OutputRootDir, MeasTime_s, "meas_time_s");
}

void makeTH1F(TDirectory *InputRootDir, TDirectory *OutputRootDir,
              std::string BranchName, Int_t Bins, Double_t XLow = 0,
              Double_t XHigh = 0) {
  TH1F *Histogram = new TH1F(BranchName.c_str(), "", Bins, XLow, XHigh);
  Float_t X = 0;

  TTree *PMTAFTree = (TTree *)InputRootDir->Get("pmtaf_tree");
  PMTAFTree->SetBranchAddress(BranchName.c_str(), &X);

  Int_t nEntries = PMTAFTree->GetEntries();
  for (Int_t iEntry = 0; iEntry < nEntries; iEntry++) {
    PMTAFTree->GetEntry(iEntry);
    Histogram->Fill(X);
    if ((iEntry) % 10000 == 0 && iEntry != 0)
      std::cout << iEntry << "/" << nEntries << std::endl;
  }

  OutputRootDir->WriteObject(Histogram, BranchName.c_str());
}

Double_t GetMeasTime(TTree *PulseTree) {
  // Open input file
  uint64_t Time_10fs;
  PulseTree->SetBranchAddress("time_10fs", &Time_10fs);

  Double_t MeasTime = 0;
  Double_t PrevTime;
  for (Int_t i = 0; i < PulseTree->GetEntries(); ++i) {
    PulseTree->GetEntry(i);
    if (i > 0 && Time_10fs <= 0.000000001)
      MeasTime += PrevTime;
    PrevTime = Time_10fs;
  }
  MeasTime += PrevTime;

  return MeasTime;
}

// template <typename THist>
// Double_t GetDarkPulses(THist *Hist, const Float_t Threshold) {
//   Double_t DarkPulses = 0;
//   for (Int_t i = 1; i <= Hist->GetNbinsX(); i++)
//     if (Hist->GetBinLowEdge(i) >= Threshold)
//       DarkPulses += (Double_t)Hist->GetBinContent(i);

//   return DarkPulses;
// }
template <typename THist>
Double_t GetDarkPulses(THist *Hist, const Float_t Threshold) {
  Double_t DarkPulses = 0;
  for (Int_t i = 1; i <= Hist->GetNbinsX(); i++)
    if (Hist->GetBinLowEdge(i) >= Threshold)
      DarkPulses += static_cast<Double_t>(Hist->GetBinContent(i));

  return DarkPulses;
}

void GetDarkRate(TDirectory *drDir, TDirectory *spDir,
                 TDirectory *OutputRootDir, const std::string BranchName,
                 Float_t Threshold, const Bool_t UseSPEThreshold = false) {

  // attach TTree and required qunatity - energy/amplitude
  Float_t Value;
  TTree *PMTAFTree = drDir->Get<TTree>("pmtaf_tree");
  if (BranchName == "energy" || BranchName == "amplitude_V") {
    PMTAFTree->SetBranchAddress(BranchName.c_str(), &Value);
  } else {
    std::cerr << "Branch name \"" << BranchName
              << "\" is not allowed for dark rate analysis" << std::endl;
    exit(1);
  }

  // read dark rate measurement time
  Double_t MeasTime_s = LoadPar<Double_t>(drDir, "meas_time_s");

  // convert threshold from spe units
  if (UseSPEThreshold) {
    // load spe spectra fit result
    Double_t Q0 = LoadPar<Double_t>(spDir, "Q0");
    Double_t Q1 = LoadPar<Double_t>(spDir, "Q1");
    Threshold = Threshold * Q1 + Q0;
  }
  // Double_t dark_pulses = GetDarkPulses(hDr, Threshold);

  // go throught dark pulses events and count those over threshold
  Double_t nDarkPulses = 0;
  Int_t nEntries = PMTAFTree->GetEntries();
  for (Int_t iEntry = 0; iEntry < nEntries; iEntry++) {
    PMTAFTree->GetEntry(iEntry);
    if (Value >= Threshold)
      nDarkPulses++;
  }
  std::cout << "Dark rate: " << nDarkPulses / MeasTime_s << "dp/s" << std::endl;
  SavePar<Double_t>(OutputRootDir, nDarkPulses / MeasTime_s, "dark_rate");
}

std::string formatAPratio(Int_t all, Int_t aps) {
  Double_t ratio = static_cast<Double_t>(aps) / all * 100.;
  std::string output_string = std::to_string(aps) + "/" + std::to_string(all) +
                              " = " + std::to_string(ratio) + " %";
  return output_string;
}

void doMultiPulseAnalysis(TDirectory *InputRootDir, TDirectory *OutputRootDir,
                          const Float_t Threshold,
                          const Double_t PulseLengthThreshold_ns) {

  // variables to attach
  Int_t iSeg;
  char Date[11]; // in format yyyy-mm-dd
  char Time[9];  // in format hh:mm:ss
  uint64_t Time_10fs;
  std::vector<Float_t> *x = 0;
  std::vector<Double_t> *t = 0;

  // Attach to input TTree
  TTree *PulseTree = (TTree *)InputRootDir->Get("pmtaf_pulses");
  PulseTree->SetBranchAddress("i_seg", &iSeg);
  PulseTree->SetBranchAddress("date", Date);
  PulseTree->SetBranchAddress("time", Time);
  PulseTree->SetBranchAddress("time_10fs", &Time_10fs);
  PulseTree->SetBranchAddress("x", &x);
  PulseTree->SetBranchAddress("t", &t);

  // output variables
  Float_t TimeOverThreshold_ns; // aka pulse length
  Float_t Amplitude;
  Int_t iSweep;

  // attach to output ttree
  TTree *PMTAFTree = new TTree("pmtaf_tree", "Multipulse analysis output");
  PMTAFTree->SetAutoSave(0);
  PMTAFTree->Branch("time_10fs", &Time_10fs);
  PMTAFTree->Branch("time_over_threshold_ns", &TimeOverThreshold_ns);
  PMTAFTree->Branch("i_sweep", &iSweep);
  PMTAFTree->Branch("amplitude_V", &Amplitude);
  // PMTAFTree->Branch("energy", &Energy);
  PMTAFTree->Branch("date", Date, "date/C", 10);
  PMTAFTree->Branch("time", Time, "time/C", 8);

  Bool_t state = 0, state_prev = 0;
  Double_t rise_time, fall_time, pulse_length;
  Long64_t nEntries = PulseTree->GetEntries();
  for (Long64_t iEntry = 0; iEntry < nEntries; iEntry++) { // events iterator
    PulseTree->GetEntry(iEntry);
    iSweep = iEntry;
    // waveform samples iterator
    for (Int_t i = 0; i < x->size(); ++i) {
      // determine state
      if (x->at(i) >= Threshold) {
        state = 1;
      } else {
        state = 0;
      }

      // rising edge
      if (state && !state_prev) {
        rise_time = t->at(i);
        Amplitude = x->at(i);
      }

      // in pulse
      if (state && state_prev) {
        // update amplitude
        if (x->at(i) > Amplitude)
          Amplitude = x->at(i);
      }

      // falling edge
      if (!state && state_prev) {
        fall_time = t->at(i);
        pulse_length = (fall_time - rise_time) * 1e9; // convert to ns units

        // pulse length threshold check
        if (pulse_length >= PulseLengthThreshold_ns) {
          TimeOverThreshold_ns = pulse_length;
          Time_10fs = stoull(s_to_10fs(rise_time));
          PMTAFTree->Fill();
        }
      }
      state_prev = state;
    }

    // progress printout
    if ((iEntry) % 1000 == 0 && iEntry != 0)
      std::cout << iEntry << "/" << nEntries << std::endl;
  }
  OutputRootDir->WriteObject(PMTAFTree, "pmtaf_tree");
}

void doAfterPulseAnalysis(TDirectory *InputRootDir, TDirectory *OutputRootDir,
                          const Double_t TimeWindowEnd_us) {

  TCanvas *c = new TCanvas();
  // TTree *tr = InputRootDir->Get<TTree>("pulses");

  uint64_t Time_10fs;
  Int_t iSweep, iSweep_prev;

  TTree *PMTAFTree = InputRootDir->Get<TTree>("pmtaf_tree");
  PMTAFTree->SetBranchAddress("time_10fs", &Time_10fs);
  PMTAFTree->SetBranchAddress("i_sweep", &iSweep);
  PMTAFTree->GetEntry(0);
  iSweep_prev = -999;
  Double_t Time_us;

  TH1I *h_AP = new TH1I("h_AP", "Afterpulses", 5, 0.5, 5.5);
  TH1D *h_PulseTime =
      new TH1D("h_PulseTime", "Pulse time", 101, 0, TimeWindowEnd_us);

  long int nEntries = PMTAFTree->GetEntries();
  Int_t nPulses = 0;
  for (Long64_t iEntry = 0; iEntry < nEntries; iEntry++) { // events iterator
    PMTAFTree->GetEntry(iEntry);
    Time_us = static_cast<Double_t>(Time_10fs) * 1e-8;
    // std::cout << Time_10fs << std::endl;
    // std::cout << static_cast<Double_t>(Time_10fs) << std::endl;
    // std::cout << static_cast<Double_t>(Time_10fs) * 1e-6 << std::endl;
    // std::cout << Time_us << std::endl;

    // exit(0);
    // Pulse is after TimeWindowEnd
    if (Time_us > TimeWindowEnd_us)
      continue;

    if (iSweep != iSweep_prev) {
      h_AP->Fill(nPulses - 1);
      nPulses = 1;
    } else
      nPulses++;
    h_PulseTime->Fill(Time_us);
    iSweep_prev = iSweep;

    // progress printout
    if ((iEntry) % 1000 == 0 && iEntry != 0)
      std::cout << iEntry << "/" << nEntries << std::endl;
  }

  // printout

  TText *text = new TText();
  text->SetTextSize(0.035);
  h_AP->SetXTitle("Afterpulses count");
  h_AP->SetYTitle("Count");
  h_AP->Draw();
  for (Int_t i = 1; i < h_AP->GetNbinsX(); ++i) {
    if (h_AP->GetBinContent(i) > 0) {
      text->DrawText(
          static_cast<Double_t>(i) - 0.4, h_AP->GetBinContent(i) * 1.3,
          formatAPratio(h_AP->GetEntries(), h_AP->GetBinContent(i)).c_str());
    }
    // text->DrawText(0.7, 0.5, "bbb");
  }

  c->SetLogy();
  c->Print(addAppendixToRelativeFilePath("_count.pdf", OutputRootDir).c_str());
  c->Clear();

  h_PulseTime->SetXTitle("Pulse time from trigger [us]");
  h_PulseTime->SetYTitle("Count");
  h_PulseTime->Draw();
  c->Print(addAppendixToRelativeFilePath("_timing.pdf", OutputRootDir).c_str());
  c->Clear();

  OutputRootDir->WriteObject(h_AP, "h_AP");
  OutputRootDir->WriteObject(h_PulseTime, "h_PulseTime");
}