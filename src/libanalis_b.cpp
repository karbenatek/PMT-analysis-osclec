#include <RtypesCore.h>
#include <TCanvas.h>
#include <TDirectory.h>
#include <TF1.h>
#include <TFile.h>
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
Int_t GetT0(std::vector<Float_t> *t, std::vector<Double_t> *x,
            const Double_t threshold) {
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
  while (i < x_size && x->at(i) < threshold)
    ++i;

  return i;
}

struct Q_duo {
  Double_t cfd = 0;
  Double_t w = 0;
};

// Integrate charge using with CFD pulse window
Q_duo GetQ(std::vector<Float_t> *t, std::vector<Double_t> *x, Double_t max,
           Int_t iMax, Float_t &T0, Float_t &t0, Float_t &t1,
           Double_t cut_ratio, Double_t threshold, Double32_t pedestal) {
  // set position to maximum
  Q_duo Q;
  Int_t i = iMax;
  Int_t i_T0 = GetT0(t, x, threshold);
  T0 = t->at(i_T0);

  // set CFD threshold
  Double_t cut_threshold =
      cut_ratio * (max - pedestal) + (1 - cut_ratio) * x->at(i_T0);
  Double_t X;
  // integrate to the right
  while (++i < x->size()) {

    X = x->at(i) - pedestal;
    Q.cfd += X;
    if (x->at(i) <= cut_threshold)
      break;
  }
  t1 = t->at(--i);

  // integrate to the left
  i = iMax - 1;
  while (i-- > 0) {
    X = x->at(i) - pedestal;
    Q.cfd += X;
    if (x->at(i) <= cut_threshold)
      break;
  }
  t0 = t->at(++i);

  return Q;
}

// get maximum and its position
Double_t GetMax(std::vector<Double_t> *x, Int_t &iMax) {
  Double_t max = -1e3;
  for (Int_t i = 1; i < (x->size() - 1); ++i) {
    if (x->at(i) > max) {
      max = x->at(i);
      iMax = i;
    }
  }
  return max;
}

namespace pmanalysis {
void doPulseAnalysis(TDirectory *inputRootDir, TDirectory *outputRootDir,
                     const Int_t bins_ampl, const Int_t bins_Q,
                     const Int_t bins_Qnoise, const Double_t threshold,
                     const Double_t max_ampl, const Double_t cut_fraction) {
  TTree *tr = (TTree *)inputRootDir->Get("pulses");
  // Attach to TTree
  Int_t i_Wfm;
  std::vector<Double_t> *x = 0;
  std::vector<Float_t> *t = 0;
  tr->SetBranchAddress("i_Wfm", &i_Wfm);
  tr->SetBranchAddress("x", &x);
  tr->SetBranchAddress("t", &t);
  tr->GetEntry(0);
  Int_t n_Seg = x->size();

  TH2F *wfm_Stack =
      new TH2F("wfm_stack", "Waveform stack", t->size(), t->front(), t->back(),
               bins_ampl * 4, -threshold, max_ampl);
  TH1F *h_t0 = new TH1F("h_t0", "t0", t->size(), t->front(), t->back());
  TH1F *h_t1 = new TH1F("h_t1", "t1", t->size(), t->front(), t->back());
  TH1F *h_tts = new TH1F("h_tts", "Transit time spread", t->size(), t->front(),
                         t->back());
  TH1F *h_t_peak =
      new TH1F("h_t_max", "Peak time", t->size(), t->front(), t->back());

  Float_t t_top = h_tts->GetBinLowEdge(h_tts->GetNbinsX());
  std::vector<Q_duo> v_Q;
  std::vector<Double_t> v_A;
  std::vector<Double_t> v_Qnoise;
  std::vector<Double_t> v_Anoise;

  // Pulse analysis output parameters
  Double_t A;
  Int_t iMax;
  std::vector<Int_t> v_iMax;
  Double_t Qcdf, Qw, Qtop = -1e10, Qbot = 1e10, Atop = -1e10;
  Q_duo Q;
  Float_t t0; // Pulse window start
  Float_t t1; // Pulse window end
  Float_t T0; //
  Double_t AQ_ratio;
  Int_t throwed = 0;
  Double32_t pedestal = 0;
  Double32_t len_pedestal = 0;

  // 1st waveforms iteration
  Int_t nentries = tr->GetEntriesFast();
  for (Long64_t jentry = 0; jentry < nentries; jentry++) { // events iterator
    tr->GetEntry(jentry);

    A = GetMax(x, iMax);
    v_A.push_back(A);
    v_iMax.push_back(iMax);
    h_t_peak->Fill(t->at(iMax));

    if (A > Atop)
      Atop = A;

    // get sample for pedestal
    if (A <= threshold)
      // in case when the signal does not reach threshold, take whole waveform
      for (Double_t val : *x) {
        pedestal += val;
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
    tr->GetEntry(jentry);

    // read maximum and its position from 1st loop
    A = v_A.at(jentry);
    iMax = v_iMax.at(jentry);
    // integrate charge Q
    Q = GetQ(t, x, A, iMax, T0, t0, t1, cut_fraction, threshold, pedestal);

    // update max & min Q values
    if (Q.cfd > Qtop)
      Qtop = Q.cfd;
    if (Q.cfd < Qbot)
      Qbot = Q.cfd;

    // fill separated "noise" histogram
    if (A <= threshold) {
      v_Qnoise.push_back(Q.cfd);
      v_Anoise.push_back(A);
    }
    // fill Q histogram (only vector so far :)
    v_Q.push_back(Q);

    // timing
    if (T0 < t_top) {
      h_tts->Fill(T0);
    }
    h_t0->Fill(t0);
    h_t1->Fill(t1);

    AQ_ratio += A / Q.cfd;

    // fill Wfm stack
    for (Int_t i = 0; i < x->size(); ++i) // wfms iterator
      wfm_Stack->Fill(t->at(i), (Float_t)x->at(i));

    // progress printout
    if ((jentry) % 1000 == 0 && jentry != 0)
      std::cout << jentry << "/" << nentries << std::endl;
  }

  std::cout << nentries << "/" << nentries << std::endl;
  std::cout << "Pulse analysis finished\n" << std::endl;

  // corelation slope
  AQ_ratio = AQ_ratio / (nentries - throwed);
  std::cout << "From " << nentries << " wfms, " << throwed << " was deleted"
            << std::endl;
  std::cout << "A/Q ratio = " << AQ_ratio << std::endl;

  // another output histograms
  TH1D *h_A = new TH1D("h_A", "Amplitudes", bins_ampl, 0, Atop);
  h_A->GetXaxis()->SetTitle("Voltage [V]");
  h_A->GetYaxis()->SetTitle("Events [count]");

  TH1D *h_Q = new TH1D("h_Q", "Charge", bins_Q, Qbot, Qtop);
  h_Q->GetXaxis()->SetTitle("Integrated charge [a.u.]");
  h_Q->GetYaxis()->SetTitle("Events [count]");

  TH1D *h_Anoise =
      new TH1D("h_Anoise", "Amplitudes (noise)", bins_ampl, 0, threshold);
  h_Anoise->GetXaxis()->SetTitle("Voltage [V]");
  h_Anoise->GetYaxis()->SetTitle("Events [count]");

  TH1D *h_Qnoise = new TH1D("h_Qnoise", "Charge (noise)", bins_Qnoise, 0, Qtop);
  h_Q->GetXaxis()->SetTitle("Integrated charge [a.u.]");
  h_Q->GetYaxis()->SetTitle("Events [count]");

  TH2D *h2_AQ = new TH2D("h2_AQ", "Voltage vs Charge", bins_Q, 0, Qtop,
                         bins_ampl, 0, Atop);
  h2_AQ->GetXaxis()->SetTitle("Integrated charge [a.u.]");
  h2_AQ->GetYaxis()->SetTitle("Voltage [V]");

  TProfile *pf_AQ =
      new TProfile("pf_AQ", "Voltage vs Charge", bins_Q, 0, Qtop, 0, Atop);
  pf_AQ->GetXaxis()->SetTitle("Integrated charge [a.u.]");
  pf_AQ->GetYaxis()->SetTitle("Voltage [V]");
  pf_AQ->SetMarkerColor(3);
  pf_AQ->SetLineColor(880);
  pf_AQ->SetLineStyle(1);
  pf_AQ->SetLineWidth(1);

  // Fill histograms from vectors
  for (Int_t i = 0; i < v_Q.size(); ++i) {
    Q = v_Q[i];
    A = v_A[i];

    h2_AQ->Fill(Q.cfd, A);
    pf_AQ->Fill(Q.cfd, A);
    h_Q->Fill(Q.cfd);
    h_A->Fill(A - pedestal);
  }
  for (Int_t i = 0; i < v_Qnoise.size(); ++i) {
    h_Qnoise->Fill(v_Qnoise[i]);
    h_Anoise->Fill(v_Anoise[i] - pedestal);
  }

  // fit correlation histogram with line
  TF1 *f_A = new TF1("f_A", "[0] + [1]*x", h_Q->GetXaxis()->GetXmin(),
                     h_Q->GetXaxis()->GetXmax());

  f_A->SetParameters(0., AQ_ratio);
  pf_AQ->Fit(f_A, "Q");

  TParameter<Double_t> *A0 = new TParameter("A0", f_A->GetParameter(0));
  TParameter<Double_t> *A1 = new TParameter("A1", f_A->GetParameter(1));

  // write correlation fit line parameters and all histograms
  outputRootDir->WriteObject(A0, "A0");
  outputRootDir->WriteObject(A1, "A1");
  outputRootDir->WriteObject(h_A, "h_A");
  outputRootDir->WriteObject(h_Anoise, "h_Anoise");
  outputRootDir->WriteObject(h_Q, "h_Q");
  outputRootDir->WriteObject(h_Qnoise, "h_Qnoise");
  outputRootDir->WriteObject(h2_AQ, "h2_AQ");
  outputRootDir->WriteObject(h_t0, "h_t0");
  outputRootDir->WriteObject(h_t1, "h_t1");
  outputRootDir->WriteObject(h_tts, "h_tts");
  outputRootDir->WriteObject(h_t_peak, "h_t_peak");
  outputRootDir->WriteObject(wfm_Stack, "wfm_Stack");
  outputRootDir->WriteObject(pf_AQ, "pf_AQ");
  // write and close files
  // fOut->Write();
  // fOut->Close();
  // rootFile->Write();
  // rootFile->~TFile();
  // h2_AQ->Delete();
}
} // namespace pmanalysis

Float_t GetMeasTime(TTree *tr) {
  // Open input file
  Double_t TimeSinceSeg1;
  tr->SetBranchAddress("TimeSinceSeg1", &TimeSinceSeg1);

  Float_t MeasTime = 0;
  Float_t PrevTime;
  for (Int_t i = 0; i < tr->GetEntries(); ++i) {
    tr->GetEntry(i);
    if (i > 0 && TimeSinceSeg1 <= 0.000000001)
      MeasTime += PrevTime;
    PrevTime = TimeSinceSeg1;
  }
  MeasTime += PrevTime;

  return MeasTime;
}

Double_t GetDarkPulses(TH1D *h_Q, const Double_t threshold) {
  Double_t DarkPulses = 0;
  for (Int_t i = 1; i <= h_Q->GetNbinsX(); i++)
    if (h_Q->GetBinLowEdge(i) >= threshold)
      DarkPulses += (Double_t)h_Q->GetBinContent(i);

  return DarkPulses;
}
namespace pmanalysis {
Double_t GetDarkRate(TDirectory *drDir, TDirectory *spDir,
                     TDirectory *outputRootDir, const Double_t pe_threshold) {
  // load darkrate spectra and raw pulse to obtain measurement time
  TH1D *h_Qdr = (TH1D *)drDir->Get("h_Q");
  TTree *tr = (TTree *)drDir->Get("pulses");
  // load spe spectra fit result
  TParameter<Double_t> *Q0 = (TParameter<Double_t> *)spDir->Get("Q0");
  TParameter<Double_t> *Q1 = (TParameter<Double_t> *)spDir->Get("Q1");

  Double_t meas_time = GetMeasTime(tr);

  Double_t threshold = pe_threshold * Q1->GetVal() + Q0->GetVal();
  Double_t dark_pulses = GetDarkPulses(h_Qdr, threshold);
  TParameter<Double_t> dark_rate;
  dark_rate.SetVal(dark_pulses / meas_time);
  if (outputRootDir)
    outputRootDir->WriteObject(&dark_rate, "darkrate");

  std::cout << dark_rate.GetVal() << std::endl;

  return dark_pulses / meas_time;
}

std::string formatAPratio(Int_t all, Int_t aps) {
  Double_t ratio = static_cast<Double_t>(aps) / all * 100.;
  std::string output_string = std::to_string(aps) + "/" + std::to_string(all) +
                              " = " + std::to_string(ratio) + " %";
  return output_string;
}

void doAfterPulseAnalysis(TDirectory *inputRootDir, TDirectory *outputRootDir,
                          const Double_t threshold,
                          const Float_t pulse_length_threshold) {

  TCanvas *c = new TCanvas();
  TTree *tr = inputRootDir->Get<TTree>("pulses");

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
  TH1F *h_PulseTime =
      new TH1F("h_PulseTime", "Pulse time", 100, v_t->front(), v_t->back());
  TH1F *h_PulseLength = new TH1F("h_PulseLength", "Pulse length", 20, 0, 40e-9);
  // TH1D *h_V = new TH1D("h_V", "", 100, -5, 0);
  long int nentries = tr->GetEntriesFast();
  // nentries = 10000;
  for (Long64_t jentry = 0; jentry < nentries; jentry++) { // events iterator
    tr->GetEntry(jentry);
    pulses = -1;
    for (Int_t i = 0; i < v_x->size(); ++i) {
      // determine state
      if (v_x->at(i) >= threshold) {
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

  outputRootDir->WriteObject(h_AP, "h_AP");
  outputRootDir->WriteObject(h_PulseLength, "h_PulseLength");
  outputRootDir->WriteObject(h_PulseTime, "h_PulseTime");

  std::cout << "Done\n\n";
}
} // namespace pmanalysis