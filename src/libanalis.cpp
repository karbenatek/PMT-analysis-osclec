#include "routines.h"
#include <RtypesCore.h>
#include <TCanvas.h>
#include <TDirectory.h>
#include <TF1.h>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TMath.h>
#include <TParameter.h>
#include <TProfile.h>
#include <TText.h>
#include <TTree.h>

#include <cstdint>

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
Float_t GetQ(std::vector<Double_t> *t, std::vector<Float_t> *x, Double_t max,
             Int_t iMax, Double_t &T0, Double_t &t0, Double_t &t1,
             Float_t cut_ratio, Float_t Threshold, Float_t pedestal) {
  // set position to maximum
  Float_t Q;
  Int_t i = iMax;
  Int_t i_T0 = GetT0(t, x, Threshold);
  T0 = t->at(i_T0);

  // set CFD threshold
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

namespace pmta {
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
  Float_t TimeOverTrigger_ns; // aka pulse length
  Float_t Energy;
  Float_t Amplitude;

  // init output TTree and create branches
  TTree *PMTAFTree = new TTree("pmtaf_tree", "Pulse events from OscLec data");
  PMTAFTree->Branch("time_10fs", &Time_10fs);
  PMTAFTree->Branch("time_over_trigger_ns", &TimeOverTrigger_ns);
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
  Double_t MeasTime_s = 0;
  Int_t throwed = 0;
  Double_t pedestal = 0;
  Double_t len_pedestal = 0;

  // 1st waveforms iteration to obtain pedestal
  Int_t nentries = PulseTree->GetEntriesFast();
  for (Long64_t jentry = 0; jentry < nentries; jentry++) { // events iterator
    PulseTree->GetEntry(jentry);

    Amplitude = GetMax(x, iMax);
    // v_iMax.push_back(iMax);
    h_t_peak->Fill(t->at(iMax));

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

  // get pedestal average
  pedestal /= len_pedestal;
  ;
  // 2nd loop
  for (Long64_t jentry = 0; jentry < nentries; jentry++) { // events iterator
    PulseTree->GetEntry(jentry);

    /*
    time_10fs
    time_over_trigger_ns
    energy
    date
    time
    */

    // get maximum
    Amplitude = GetMax(x, iMax);

    // integrate charge Q
    Energy = GetQ(t, x, Amplitude, iMax, T0, t0, t1, CutFraction, Threshold,
                  pedestal);
    TimeOverTrigger_ns = static_cast<Float_t>((t1 - t0) * 10e9);
    if (UseTotalTime)
      Time_10fs += stoull(s_to_10fs(T0));
    else
      Time_10fs = stoull(s_to_10fs(T0));

    h2_AQ->Fill(Energy, Amplitude);
    pf_AQ->Fill(Energy, Amplitude);

    // add time to total meas time when timer resets
    if (jentry > 0 && Time_10fs <= 0.000000001)
      MeasTime_s += static_cast<Double_t>(Time_10fs) * 10e14;

    h_t0->Fill(t0);
    h_t1->Fill(t1);
    PMTAFTree->Fill();

    // fill Wfm stack
    for (Int_t i = 0; i < x->size(); ++i) // wfms iterator
      wfm_Stack->Fill(t->at(i), static_cast<Double_t>(x->at(i)));

    // progress printout
    if ((jentry) % 1000 == 0 && jentry != 0)
      std::cout << jentry << "/" << nentries << std::endl;
  }

  std::cout << nentries << "/" << nentries << std::endl;
  std::cout << "Pulse analysis finished\n" << std::endl;

  // corelation slope
  std::cout << "From " << nentries << " wfms, " << throwed << " was deleted"
            << std::endl;

  /*
  //   Double_t MeasTime = 0;
//   Double_t PrevTime;
//   for (Int_t i = 0; i < PulseTree->GetEntries(); ++i) {
//     PulseTree->GetEntry(i);
//     if (i > 0 && Time_10fs <= 0.000000001)
//       MeasTime += PrevTime;
//     PrevTime = Time_10fs;
//   }
//   MeasTime += PrevTime;
  */
  // Fill histograms & output tree from vectors
  // for (Int_t i = 0; i < nentries; ++i) {
  //   PulseTree->GetEntry(i);
  //   Q = v_Q[i];
  //   A = v_A[i];

  //   /*time_10fs
  //   time_over_trigger_ns
  //   energy
  //   date
  //   time
  //   */

  //   Energy = Q;
  // }

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
}

void getHistogram(TDirectory *InputRootDir, TDirectory *OutputRootDir,
                  std::string BranchName, Int_t Bins, Double_t XLow = 0,
                  Double_t XHigh = 0) {
  TH1D *Histogram = new TH1D(BranchName.c_str(), "", Bins, XLow, XHigh);
  Float_t X = 0;

  TTree *PMTAFTree = (TTree *)InputRootDir->Get("pmtaf_tree");
  PMTAFTree->SetBranchAddress(BranchName.c_str(), &X);

  Int_t nEntries = PMTAFTree->GetEntries();
  for (Int_t iEntry = 0; iEntry < nEntries; iEntry++) {
    PMTAFTree->GetEntry(iEntry);
    Histogram->Fill(X);
  }

  OutputRootDir->WriteObject(Histogram, BranchName.c_str());
}

} // namespace pmta

// Double_t GetMeasTime(TTree *PulseTree) {
//   // Open input file
//   uint64_t Time_10fs;
//   PulseTree->SetBranchAddress("time_10fs", &Time_10fs);

//   Double_t MeasTime = 0;
//   Double_t PrevTime;
//   for (Int_t i = 0; i < PulseTree->GetEntries(); ++i) {
//     PulseTree->GetEntry(i);
//     if (i > 0 && Time_10fs <= 0.000000001)
//       MeasTime += PrevTime;
//     PrevTime = Time_10fs;
//   }
//   MeasTime += PrevTime;

//   return MeasTime;
// }

Double_t GetDarkPulses(TH1D *h_Q, const Float_t Threshold) {
  Double_t DarkPulses = 0;
  for (Int_t i = 1; i <= h_Q->GetNbinsX(); i++)
    if (h_Q->GetBinLowEdge(i) >= Threshold)
      DarkPulses += (Double_t)h_Q->GetBinContent(i);

  return DarkPulses;
}

namespace pmta {
// Double_t GetDarkRate(TDirectory *drDir, TDirectory *spDir,
//                      TDirectory *OutputRootDir, const Float_t pe_threshold) {
//   // load darkrate spectra and raw pulse to obtain measurement time
//   TH1D *h_Qdr = (TH1D *)drDir->Get("h_Q");
//   TTree *tr = (TTree *)drDir->Get("pulses");
//   // load spe spectra fit result
//   TParameter<Double_t> *Q0 = (TParameter<Double_t> *)spDir->Get("Q0");
//   TParameter<Double_t> *Q1 = (TParameter<Double_t> *)spDir->Get("Q1");

//   Double_t meas_time = GetMeasTime(tr);

//   Double_t Threshold = pe_threshold * Q1->GetVal() + Q0->GetVal();
//   Double_t dark_pulses = GetDarkPulses(h_Qdr, Threshold);
//   TParameter<Double_t> dark_rate;
//   dark_rate.SetVal(dark_pulses / meas_time);
//   if (OutputRootDir)
//     OutputRootDir->WriteObject(&dark_rate, "darkrate");

//   std::cout << dark_rate.GetVal() << std::endl;

//   return dark_pulses / meas_time;
// }

std::string formatAPratio(Int_t all, Int_t aps) {
  Double_t ratio = static_cast<Double_t>(aps) / all * 100.;
  std::string output_string = std::to_string(aps) + "/" + std::to_string(all) +
                              " = " + std::to_string(ratio) + " %";
  return output_string;
}

void doAfterPulseAnalysis(TDirectory *InputRootDir, TDirectory *OutputRootDir,
                          const Float_t Threshold,
                          const Double_t pulse_length_threshold) {

  TCanvas *c = new TCanvas();
  TTree *tr = InputRootDir->Get<TTree>("pulses");

  Int_t i_Wfm;
  std::vector<Double_t> *v_x = 0;
  std::vector<Float_t> *v_t = 0;
  Double_t t, t0;
  tr->SetBranchAddress("x", &v_x);
  tr->SetBranchAddress("t", &v_t);
  tr->SetBranchAddress("TimeSinceSeg1", &t);

  tr->GetEntry(0);

  // find trig time index
  int i0 = 0;

  while (v_t->at(i0) < 0)
    i0++;

  Int_t pulses, high_samples = 0, low_samples = 0;
  Bool_t state = 0, state_prev = 0;
  Double_t rise_time, fall_time, pulse_length;
  TH1I *h_AP = new TH1I("h_AP", "Afterpulses", 5, 0.5, 5.5);
  TH1D *h_PulseTime =
      new TH1D("h_PulseTime", "Pulse time", 100, v_t->front(), v_t->back());
  TH1D *h_PulseLength = new TH1D("h_PulseLength", "Pulse length", 20, 0, 40e-9);
  // TH1D *h_V = new TH1D("h_V", "", 100, -5, 0);
  long int nentries = tr->GetEntriesFast();
  // nentries = 10000;
  for (Long64_t jentry = 0; jentry < nentries; jentry++) { // events iterator
    tr->GetEntry(jentry);
    pulses = -1;
    for (Int_t i = 0; i < v_x->size(); ++i) {
      // determine state
      if (v_x->at(i) >= Threshold) {
        state = 1;
        high_samples++;
      } else {
        state = 0;
        low_samples++;
      }

      // rising edge
      if (state && !state_prev) {
        high_samples = 1;
        rise_time = v_t->at(i);
      }
      // falling edge
      else if (!state && state_prev) {
        low_samples = 1;
        fall_time = v_t->at(i);
        pulse_length = fall_time - rise_time;
        // pulse length validation
        if (pulse_length >= pulse_length_threshold) {
          h_PulseLength->Fill(fall_time - rise_time);
          h_PulseTime->Fill(rise_time);
          pulses++;
        }
      }
      state_prev = state;
    }

    h_AP->Fill(pulses);
    // progress printout
    if ((jentry) % 1000 == 0 && jentry != 0)
      std::cout << jentry << "/" << nentries << std::endl;
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
  c->Print("p_n.pdf");
  c->Clear();
  h_PulseLength->SetXTitle("Pulse length [s]");
  h_PulseLength->SetYTitle("Count");
  h_PulseLength->Draw();
  c->Print("p_len.pdf");
  c->Clear();

  h_PulseTime->SetXTitle("Pulse time from trigger [s]");
  h_PulseTime->SetYTitle("Count");
  h_PulseTime->Draw();
  c->Print("p_time.pdf");
  c->Clear();

  OutputRootDir->WriteObject(h_AP, "h_AP");
  OutputRootDir->WriteObject(h_PulseLength, "h_PulseLength");
  OutputRootDir->WriteObject(h_PulseTime, "h_PulseTime");

  std::cout << "Done\n\n";
}
} // namespace pmta