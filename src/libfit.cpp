#include "funies.cpp"
#include <RtypesCore.h>
#include <TCanvas.h>
#include <TDirectory.h>
#include <TF1.h>
#include <TF2.h>
#include <TFile.h>
#include <TFitResult.h>
#include <TGraph.h>
#include <TH1.h>
#include <TLegend.h>
#include <TMap.h>
#include <TMath.h>
#include <TMathBase.h>
#include <TObjString.h>
#include <TParameter.h>
#include <TStyle.h>
#include <filesystem>
#include <iostream>
#include <vector>
using namespace std;

const Int_t Nfpars = 9;
const Int_t Npars = 12;

struct FitPars {
  Float_t Q0;
  Float_t sig0;
  Float_t Q1;
  Float_t sig1;
  Float_t w;
  Float_t alpha;
  Float_t mu;
  Float_t A;
  const array<string, Nfpars> names = {"Q0", "sig0",  "Q1", "sig1",
                                       "w",  "alpha", "mu", "A"};
};

struct PrepPars {
  Float_t Q0;
  Float_t sig0;
  Float_t Q1;
  Float_t sig1;
  Float_t Qv;
  Float_t Qp;
  Float_t Qstart;
  Float_t Qend;
  Float_t w;
  Float_t alpha;
  Float_t mu;
  Float_t A;
  const array<string, Npars> parnames = {
      "Q0", "Q1", "Qv", "Qp", "Qend", "sig0", "sig1", "w", "alpha", "mu", "A"};
};

struct Pts {
  TGraph *g_Qpeaks = new TGraph();
  TGraph *g_Qvaleys = new TGraph();
  TGraph *g_dQpeaks = new TGraph();
  TGraph *g_dQvaleys = new TGraph();
  vector<Int_t> *vi_Qpeak = new std::vector<Int_t>;
  vector<Int_t> *vi_Qvaley = new std::vector<Int_t>;
  vector<Int_t> *vi_dQpeak = new std::vector<Int_t>;
  vector<Int_t> *vi_dQvaley = new std::vector<Int_t>;
  Int_t i_Qpeak;
  Int_t i_Qvaley;
  Int_t i_Q0;
  Int_t i_Qend;
};

filesystem::path getImgPath(string imgName, TDirectory *rootDir) {
  filesystem::path imgPath = rootDir->GetFile()->GetName();
  imgPath =
      imgPath.parent_path().append(((string)rootDir->GetName()) + imgName);
  return imgPath;
}
Float_t getMeanAround(TH1F *h_Q, Int_t i, Int_t HalfWindth) {
  Float_t Sum = 0;
  Int_t N = 0;
  for (Int_t j = i - HalfWindth; j <= i + HalfWindth; j++) {
    if (j < 1 || j > h_Q->GetNbinsX())
      continue;
    else {
      N++;
      Sum += h_Q->GetBinContent(j);
    }
  }
  return Sum / N;
}
void FindExtrPts(TH1F *h_Q0, TH1F *h_Qs, TH1F *h_dQ, Pts *pts,
                 PrepPars *prepPars, int n_Smooth = 5, int n_dSmooth = 10) {

  TCanvas *c = new TCanvas("c");
  h_Qs->Smooth(n_Smooth);
  // h_Qs->Smooth(n_Smooth);
  // h_Qs->ResetStats();

  // h_Qs->Draw();

  // exit(0);
  // go to Q0 -> pedestal gauss peak
  //  while ( h_Qs->GetBinContent(i) < h_Qs->GetBinContent(i+1)) ++i;
  prepPars->Qstart = h_Qs->GetBinLowEdge(1);
  // cout << prepPars->Qstart << endl;
  // exit(0);

  pts->i_Q0 = h_Qs->FindBin(0) + 1;
  cout << pts->i_Q0 << endl;
  // exit(0);

  cout << "i_Q0 = " << pts->i_Q0 << endl;
  int i = pts->i_Q0 + 1;
  cout << h_Qs->GetBinContent(1) << endl;
  // exit(0);
  Float_t bottom_threshold =
      2; // i.e. bins with relative error greater than 25%
  // find local extremes in spectra
  while (getMeanAround(h_Qs, ++i, 2) >= bottom_threshold) {
    if (h_Qs->GetBinContent(i - 1) > h_Qs->GetBinContent(i) &&
        h_Qs->GetBinContent(i) < h_Qs->GetBinContent(i + 1))
      pts->vi_Qvaley->push_back(i);
    if (h_Qs->GetBinContent(i - 1) < h_Qs->GetBinContent(i) &&
        h_Qs->GetBinContent(i) >= h_Qs->GetBinContent(i + 1))
      pts->vi_Qpeak->push_back(i);
  }
  pts->i_Qend = i;
  cout << "i_end = " << i << endl;
  // In case that peak is before valey, take look to h_Q (not smoothed)
  // In h_Qs, a peak can be smoothed so there is no local extreme

  if (not pts->vi_Qvaley->empty() && not pts->vi_Qpeak->empty()) {
    if (pts->vi_Qvaley->at(0) >= pts->vi_Qpeak->at(0)) {
      i = pts->vi_Qpeak->at(0);
      while (i > 4 && h_Qs->GetBinContent(i) > h_Qs->GetBinContent(i - 1))
        --i;
      pts->vi_Qvaley->insert(pts->vi_Qvaley->begin(), i);
    }
  }

  // If no valey found, set it next to the right from pedestal
  if (pts->vi_Qvaley->empty())
    pts->vi_Qvaley->push_back(pts->i_Q0 + 1);

  // If the first valey is after peak, set it next to the right from pedestal
  if (pts->vi_Qvaley->begin() > pts->vi_Qpeak->begin())
    pts->vi_Qvaley->at(0) = pts->i_Q0 + 1;

  // Fill local extremes into TGraph objects
  // Peaks
  for (int p, j = 0; j < pts->vi_Qpeak->size(); ++j) {
    p = pts->vi_Qpeak->at(j);
    pts->g_Qpeaks->AddPoint(h_Qs->GetBinCenter(p), h_Qs->GetBinContent(p));
  }
  // Valeys
  for (int v, j = 0; j < pts->vi_Qvaley->size(); ++j) {
    v = pts->vi_Qvaley->at(j);
    pts->g_Qvaleys->AddPoint(h_Qs->GetBinCenter(v), h_Qs->GetBinContent(v));
  }

  // making derivative of spectra
  double dQ;
  for (int j = 1; j < pts->i_Qend; j++) {
    dQ = h_Qs->GetBinContent(j) - h_Qs->GetBinContent(j + 1);
    h_dQ->SetBinContent(j, dQ);
  }
  h_dQ->Smooth(n_dSmooth);
  h_dQ->ResetStats();

  // finding inflection points
  i = 0;
  while (h_Qs->GetBinContent(i) < h_Qs->GetBinContent(i + 1))
    ++i;

  while (++i < pts->i_Qend) {
    if (h_dQ->GetBinContent(i - 1) >= h_dQ->GetBinContent(i) &&
        h_dQ->GetBinContent(i) < h_dQ->GetBinContent(i + 1))
      pts->vi_Qvaley->push_back(i);
    if (h_dQ->GetBinContent(i - 1) <= h_dQ->GetBinContent(i) &&
        h_dQ->GetBinContent(i) > h_dQ->GetBinContent(i + 1))
      pts->vi_Qpeak->push_back(i);
  }
  for (int v, p, j = 0; j < min(pts->vi_Qvaley->size(), pts->vi_Qpeak->size());
       ++j) {
    v = pts->vi_Qvaley->at(j);
    p = pts->vi_Qpeak->at(j);
    pts->g_dQvaleys->AddPoint(h_dQ->GetBinCenter(v), h_dQ->GetBinContent(v));
    pts->g_dQpeaks->AddPoint(h_dQ->GetBinCenter(p), h_dQ->GetBinContent(p));
  }

  //   int i_Qv, i_Qp;
  // Sut first valey/peak as a real deal valey/peak
  if (not pts->vi_Qvaley->empty() && not pts->vi_Qpeak->empty()) {
    pts->i_Qvaley = pts->vi_Qvaley->at(0);
    pts->i_Qpeak = pts->vi_Qpeak->at(0);
  }
  // If there are none, take it from inflection points
  else {
    pts->i_Qvaley = pts->vi_Qvaley->at(0);
    pts->i_Qpeak = pts->vi_Qpeak->at(0);
  }

  // parameters needed for Fit function Init and for it's parameter estimation
  prepPars->Q0 = h_Qs->GetBinCenter(pts->i_Q0);
  prepPars->Qv = h_Qs->GetBinCenter(pts->i_Qvaley);
  prepPars->Qp = h_Qs->GetBinCenter(pts->i_Qpeak);
  prepPars->Qend = h_Qs->GetBinCenter(pts->i_Qend);

  // TGraph plot configuration
  pts->g_dQpeaks->SetMarkerStyle(8);
  pts->g_dQpeaks->SetMarkerColor(3);
  pts->g_dQpeaks->SetMarkerSize(1);
  pts->g_dQvaleys->SetMarkerStyle(8);
  pts->g_dQvaleys->SetMarkerColor(2);
  pts->g_dQvaleys->SetMarkerSize(1);
  pts->g_Qpeaks->SetMarkerStyle(8);
  pts->g_Qpeaks->SetMarkerColor(3);
  pts->g_Qpeaks->SetMarkerSize(1);
  pts->g_Qvaleys->SetMarkerStyle(8);
  pts->g_Qvaleys->SetMarkerColor(2);
  pts->g_Qvaleys->SetMarkerSize(1);

  // h_Qs->GetXaxis()->SetRange(0, 40); // h_Qs->GetMaximum() * 0.1);
  // h_Qs->GetYaxis()->SetRangeUser(0, h_Qs->GetMaximum() * 0.1);
  h_Qs->SetLineColor(4);
  h_Qs->Draw();

  h_Q0->Draw("same");
  pts->g_Qvaleys->Draw("P same");
  pts->g_Qpeaks->Draw("P same");

  pts->g_Qpeaks->Print();
  pts->g_Qvaleys->Print();
  c->SetLogy();
  // c->Print("c.pdf");
  // exit(0);
}

inline void DrawFrame(TH1F *h_Q0, Pts pts) {
  TGraph *g_W = new TGraph();
  g_W->SetLineColor(1);
  g_W->SetLineWidth(1);
  g_W->SetLineStyle(2);

  g_W->AddPoint(h_Q0->GetBinCenter(pts.i_Q0), h_Q0->GetBinContent(pts.i_Qend));
  g_W->AddPoint(h_Q0->GetBinCenter(pts.i_Q0), h_Q0->GetBinContent(pts.i_Q0));
  g_W->AddPoint(h_Q0->GetBinCenter(pts.i_Qend), h_Q0->GetBinContent(pts.i_Q0));
  g_W->AddPoint(h_Q0->GetBinCenter(pts.i_Qend),
                h_Q0->GetBinContent(pts.i_Qend));
  g_W->AddPoint(h_Q0->GetBinCenter(pts.i_Q0), h_Q0->GetBinContent(pts.i_Qend));
  g_W->Draw("same");
}

inline TF1 *InitFun(PrepPars *prepPars) {

  TF1 *f = new TF1("f_Q", funies::Fun, prepPars->Q0, prepPars->Qend, 8);
  f->SetLineColor(1);
  f->SetLineWidth(2);
  f->SetParNames("Q0", "sig0", "Q1", "sig1", "w", "alpha", "mu", "A");
  f->FixParameter(0, 0);
  f->FixParameter(1, 0);

  return f;
}

PrepPars ParEst1(TH1F *h_Q, TH1F *h_Qs, PrepPars prepPars, Pts pts) {
  Float_t Q0, Q1, Qv, Qp, Qend, sig0, sig1, w, alpha, mu, A;
  h_Qs->Draw();
  h_Q->Draw("same");
  Q0 = prepPars.Q0;
  Qv = prepPars.Qv;
  Qp = prepPars.Qp;
  Int_t i_Qv = h_Q->FindBin(Qv), i_Qp = h_Q->FindBin(Qp);

  Qend = prepPars.Qend;
  sig0 = (Qv - Q0) / 10;
  alpha = 1 / (Qv - Q0);
  A = h_Q->GetEntries();

  TF1 *f_Nexp = new TF1("f_N", "[0]*[1]*exp(-([1]*(x-[2])))", Q0, Qend);
  f_Nexp->SetParNames("A", "alpha", "Q0");
  f_Nexp->SetParameters(A, alpha, Q0);
  A = h_Qs->GetBinContent(i_Qv) / f_Nexp->Eval(Qv);
  f_Nexp->SetParameter("A", A);
  f_Nexp->FixParameter(2, Q0);
  f_Nexp->SetLineColor(3);
  f_Nexp->SetLineWidth(1);
  h_Qs->Fit(f_Nexp, "N", "", Q0, Qv);

  A = f_Nexp->GetParameter("A");
  alpha = f_Nexp->GetParameter("alpha");
  f_Nexp->Draw("same");
  Q1 = Qp - Q0 - w / alpha;
  sig1 = (Qp - Qv) / 1.4;

  TH1F *h_Qsig = (TH1F *)h_Qs->Clone("h_Qsig");
  Float_t dQ;
  for (int i = 0; i <= h_Q->GetNbinsX(); ++i)
    if (i >= i_Qv && i <= pts.i_Qend) {
      dQ = h_Qs->GetBinContent(i) - f_Nexp->Eval(h_Qs->GetBinCenter(i));
      //~ if (dQ < 0) dQ = 0;
      h_Qsig->SetBinContent(i, dQ);
    } else
      h_Qsig->SetBinContent(i, 0);
  h_Qsig->ResetStats();
  h_Qsig->Draw("same hist");
  Float_t A1 = h_Qsig->GetMaximum();
  Float_t QA1 = h_Qsig->GetBinCenter(h_Qsig->GetMaximumBin());

  mu = A1 / A * sig1 * 2.5;
  // mu = 2;
  // mu = - utl::LambertW<0>(-A1*sig1*1.8119051);

  TF1 *f_sig = new TF1("f_sig", funies::Funsig, Qv, Qend, 5);
  f_sig->SetLineColor(2);
  f_sig->SetParNames("Q0", "Q1", "sig1", "mu", "A");
  f_sig->SetParameters(Q0, Q1, sig1, mu, A);
  A = A * A1 / f_sig->Eval(QA1);
  f_sig->SetParameter("A", A);
  f_sig->Draw("same");

  w = 1;

  int i = 0;

  PrepPars newPars;
  newPars.Q0 = Q0;
  newPars.Q1 = Q1;
  newPars.Qv = Qv;
  newPars.Qp = Qp;
  newPars.Qend = Qend;
  newPars.sig0 = sig0;
  newPars.sig1 = sig1;
  newPars.w = w;
  newPars.alpha = alpha;
  newPars.mu = mu;
  newPars.A = A;

  return newPars;
}

PrepPars ParEst2(TH1D *h_Q, TH1D *h_Qs, PrepPars prepPars, Pts pts) {
  double Q0, Q1, Qv, Qp, Qend, sig0, sig1, w, alpha, mu, A;

  h_Qs->Draw();
  h_Q->Draw("same");

  Q0 = prepPars.Q0;
  Qv = prepPars.Qv;
  Qp = prepPars.Qp;
  int i_Qv = h_Q->FindBin(Qv), i_Qp = h_Q->FindBin(Qp);

  Qend = prepPars.Qend;
  sig0 = (Qv - Q0) / 10;
  w = 0.5;
  // A = h_Q->GetEntries();
  A = h_Q->GetMaximum();

  cout << "\n\nblah = " << A << endl;
  alpha = 1 / (Qv - Q0);

  // noise parameters
  TF1 *f_N = new TF1("f_N", funies::Funoise, Q0, Qend, 5);
  f_N->SetParNames("Q0", "sig0", "w", "alpha", "A");
  //~ f_N->SetLineWidth(1);
  f_N->SetLineStyle(2);
  f_N->SetLineColor(6);

  f_N->SetParameters(Q0, sig0, w, alpha, A);
  cout << "Qv = " << Qv << endl;
  cout << "i_Qv = " << i_Qv << endl;
  // A = A*h_Qs->GetBinContent(i_Qv)/f_N->Eval(Qv);
  f_N->SetParameter("A", A);
  f_N->SetParLimits(0, 0, Qv);
  f_N->SetParLimits(1, 0, (Qv - Q0) / 2);
  f_N->SetParLimits(2, 0, 1);
  f_N->SetParLimits(4, 0, A);
  cout << "\nEstimated noise parameters: \n";
  for (int i = 0; i < 5; ++i)
    cout << f_N->GetParName(i) << " = " << f_N->GetParameter(i) << endl;

  cout << endl
       << "NOISE PARAMETER ESTIMATION FIT\n_______________________________\n";
  // h_Q0->Fit(f_N,"","",Q0,Qv);
  f_N->Draw("same");

  // subtraction of noise fit result from original spectre
  TH1D *h_Qsig = (TH1D *)h_Qs->Clone("h_Qsig");
  double dQ;
  for (int i = 0; i <= h_Qs->GetNbinsX(); ++i)
    if (i >= i_Qv && i <= pts.i_Qend) {
      dQ = h_Qs->GetBinContent(i) - f_N->Eval(h_Qs->GetBinCenter(i));
      //~ if (dQ < 0) dQ = 0;
      h_Qsig->SetBinContent(i, dQ);
    } else
      h_Qsig->SetBinContent(i, 0);
  h_Qsig->ResetStats();
  h_Qsig->Draw("same hist");

  Q0 = f_N->GetParameter("Q0");
  sig0 = f_N->GetParameter("sig0");

  // A = f_N->GetParameter("A");
  alpha = f_N->GetParameter("alpha");
  w = f_N->GetParameter("w");
  f_N->SetParameter("alpha", alpha);

  // gauss bunch parameters -- Q1, sig1, mu
  TF1 *f_sig = new TF1("f_sig", funies::Funsig, Qv, Qend, 5);
  f_sig->SetLineColor(3);
  f_sig->SetParNames("Q0", "Q1", "sig1", "mu", "A");
  Q1 = h_Qsig->GetBinCenter(h_Qsig->GetMaximumBin());
  sig1 = (Qp - Qv);

  double mu1, mu2;
  TGraph *g_Qsig = new TGraph();
  g_Qsig->SetMarkerStyle(8);
  g_Qsig->SetMarkerColor(3);
  g_Qsig->SetMarkerSize(1);

  vector<int> v_Qsig;

  for (int i = i_Qv; i < pts.i_Qend; ++i)
    if (h_Qsig->GetBinContent(i - 1) < h_Qsig->GetBinContent(i) &&
        h_Qsig->GetBinContent(i) >= h_Qsig->GetBinContent(i + 1)) {
      v_Qsig.push_back(i);
      g_Qsig->AddPoint(h_Qsig->GetBinCenter(i), h_Qsig->GetBinContent(i));
    }
  int n = v_Qsig.size();
  if (n > 2) {
    n = 2;
    Q1 = g_Qsig->GetPointX(1) - g_Qsig->GetPointX(0);
  } else
    Q1 = g_Qsig->GetPointX(0);
  A = h_Qsig->GetMaximum();
  mu = TMath::Power(g_Qsig->GetPointY(n - 1) / A * TMath::Factorial(n) *
                        TMath::Sqrt(TMath::TwoPi() * n),
                    1. / n);
  g_Qsig->Draw("P same");

  mu1 = h_Qsig->GetMaximum() * sig1 * 2.5 / A;

  f_sig->SetParameters(Q0, Q1, sig1, mu, A);
  A = A * g_Qsig->GetPointY(0) / f_sig->Eval(g_Qsig->GetPointX(0));
  f_sig->SetParameter("A", A);
  cout << endl
       << "SIGNAL PARAMETER ESTIMATION FIT\n_______________________________\n";
  h_Qsig->Fit(f_sig, "N", "", Qv, Qend);
  f_sig->Draw("same");
  Q1 = f_sig->GetParameter("Q1");
  sig1 = f_sig->GetParameter("sig1");
  mu = f_sig->GetParameter("mu");
  A = f_sig->GetParameter("A");

  cout << "h_s parameters: \n";
  for (int i = 0; i < 5; ++i)
    cout << f_sig->GetParName(i) << " = " << f_sig->GetParameter(i) << endl;

  //~ h_Qsig->Fit(f_sig,"R");
  //~ TCanvas * cs = new TCanvas();
  //~ h_Qsig->Fit(f_sig,"R");
  f_sig->Draw("same");

  PrepPars newPars;
  newPars.Q0 = Q0;
  newPars.Q1 = Q1;
  newPars.Qv = Qv;
  newPars.Qp = Qp;
  newPars.Qend = Qend;
  newPars.sig0 = sig0;
  newPars.sig1 = sig1;
  newPars.w = w;
  newPars.alpha = alpha;
  newPars.mu = mu;
  newPars.A = A;

  return newPars;
}

void prnt(string s) {
  std::cout << s << std::endl;
  return;
}

PrepPars ParEst3(TH1F *h_Q, TH1F *h_Qs, PrepPars prepPars, Pts pts) {
  Float_t Q0, Q1, Qv, Qp, Qend, sig0, sig1, w, alpha, mu, A, Q_integ,
      Qnoise_integ;
  h_Qs->Draw();
  h_Q->Draw("same");
  Q0 = prepPars.Q0;
  Qv = prepPars.Qv;
  Qp = prepPars.Qp;
  Qend = prepPars.Qend;

  Int_t i_Qv = h_Q->FindBin(Qv), i_Qp = h_Q->FindBin(Qp);

  sig0 = 0.5 * h_Q->GetBinWidth(0);
  alpha = 5 / (Qv - Q0);
  Qnoise_integ = h_Q->Integral();

  A = Qnoise_integ;

  // w = h_Qnoise->Integral(2, h_Qnoise->GetNbinsX()) / A;
  w = 1;
  TF1 *f_N = new TF1("f_N", funies::Funoise, 0, Qend, 5);
  f_N->SetParNames("Q0", "sig0", "w", "alpha", "A");
  //~ f_N->SetLineWidth(1);
  f_N->SetLineStyle(1);
  f_N->SetLineWidth(1);
  f_N->SetLineColor(6);

  f_N->SetParLimits(0, -1 * h_Q->GetBinWidth(1), 1 * h_Q->GetBinWidth(1));
  f_N->FixParameter(1, 0);
  f_N->FixParameter(2, 1);
  // f_N->SetParLimits(1, 0, 0.8 * h_Qnoise->GetBinWidth(1));
  // f_N->SetParLimits(2, 0, 1);
  f_N->SetParLimits(3, 0, 50 / (Qv - Q0));

  f_N->SetParameters(0, sig0, w, alpha, A);

  cout << "\n\nFITTING NOISE\n";
  h_Q->Fit(f_N, "N", "", (Q0 + Qv) / 2, Qv);
  f_N->Draw("same");

  for (int i = 0; i < 3; ++i)
    f_N->FixParameter(i, f_N->GetParameter(i));
  h_Q->Fit(f_N, "N", "", Q0, Qv);
  f_N->Draw("same");
  Q0 = f_N->GetParameter(0);
  sig0 = f_N->GetParameter(1);
  w = f_N->GetParameter(2);
  alpha = f_N->GetParameter(3) * 2;
  A = f_N->GetParameter(4);
  mu = 0.2;
  cout << "mu = " << mu << endl;

  TF1 *f_sig = new TF1("f_sig", funies::Fun, Q0, Qend, 8);
  f_sig->SetParNames("Q0", "sig0", "Q1", "sig1", "w", "alpha", "mu", "A");
  f_sig->FixParameter(0, Q0);
  f_sig->SetParameter(1, sig0);
  f_sig->SetParLimits(1, 0, 0.5 * Qv);
  f_sig->SetParameter(2, Qp);

  f_sig->SetParLimits(2, Qp - (Qp - Qv) / 2, Qp + (Qp - Qv) / 2);
  f_sig->SetParameter(3, Qp - Qv);
  f_sig->SetParLimits(3, 0, 2 * (Qp - Qv));

  f_sig->SetParameter(4, w);
  f_sig->SetParLimits(4, 0, 1);
  f_sig->SetParameter(5, alpha);
  f_sig->SetParLimits(5, 0, 50 / (Qv - Q0));
  f_sig->FixParameter(6, mu); // noise/all? :)
  f_sig->SetParameter(7, A);
  h_Q->Fit(f_sig, "N", "", Qv / 2, Qend);
  f_sig->Draw("same");

  PrepPars newPars;
  newPars.Q0 = f_sig->GetParameter(0);
  newPars.sig0 = f_sig->GetParameter(1);
  newPars.Q1 = f_sig->GetParameter(2);
  newPars.sig1 = f_sig->GetParameter(3);
  newPars.Qv = Qv;
  newPars.Qp = Qp;
  newPars.Qend = Qend;
  newPars.Qstart = prepPars.Qstart;
  newPars.w = f_sig->GetParameter(4);
  newPars.alpha = f_sig->GetParameter(5);
  newPars.mu = f_sig->GetParameter(6);
  newPars.A = f_sig->GetParameter(7);

  return newPars;
}

inline void SetPars(TF1 *f, PrepPars Pars, Bool_t printout = false) {
  // f->SetParameters(Pars.Q0, Pars.sig0, Pars.Q1, Pars.sig1, Pars.w,
  // Pars.alpha,
  //                  Pars.mu, Pars.A);

  Float_t par[] = {Pars.Q0, Pars.sig0,  Pars.Q1, Pars.sig1,
                   Pars.w,  Pars.alpha, Pars.mu, Pars.A};
  for (int i = 0; i < f->GetNpar(); ++i) {
    cout << f->GetParName(i) << " = " << par[i] << endl;
    f->SetParameter(i, par[i]);
  }
}

void SetParLimFixs(TF1 *f, TH1F *h_Q, PrepPars Pars,
                   array<Bool_t, 8> fix = {0, 0, 0, 0, 0, 0, 0, 0},
                   array<Bool_t, 8> leave = {0, 0, 0, 0, 0, 0, 0, 0}) {
  Float_t Q0 = Pars.Q0;
  Float_t Q1 = Pars.Q1;
  Float_t Qv = Pars.Qv;
  Float_t Qp = Pars.Qp;
  Float_t Qend = Pars.Qend;
  Float_t Qstart = TMath::Abs(Pars.Qstart);
  Float_t sig0 = Pars.sig0;
  Float_t sig1 = Pars.sig1;
  Float_t w = Pars.w;
  Float_t alpha = Pars.alpha;
  Float_t mu = Pars.mu;
  Float_t A = Pars.A;
  array<array<Float_t, 2>, 9> l;

  Int_t i = 0;
  l[i++] = {static_cast<float>(-0.02 * Qstart),
            static_cast<float>(0.02 * Qstart)};                   // Q0
  l[i++] = {0, static_cast<float>(Qstart * 0.5)};                 // sig0
  l[i++] = {static_cast<float>(0.8 * (Qp - Q0)), 2 * (Qp - Q0)};  // Q1
  l[i++] = {static_cast<float>((Qp - Qv) * 0.1), (Qp - Qv) * 10}; // sig1
  l[i++] = {0.005, 0.99};                                         // w
  l[i++] = {0, 1000 / (Qv - Q0)};                                 // aplha
  l[i++] = {0, 0.5};                                              // mu
  l[i++] = {A / 100, A * 100};                                    // A

  for (int i = 0; i < f->GetNpar(); ++i) {
    if (leave[i])
      f->SetParLimits(i, 0, 0);
    else {
      if (!fix[i])
        f->SetParLimits(i, l[i][0], l[i][1]);
      else {
        f->FixParameter(i, 0);
      }
    }
  }
}

TFitResult *FitCompare(TH1F *h_Q, TF1 *f_Q, vector<PrepPars> v_prepPars) {
  int i_best = 0, i = 0;
  Double_t chi = 1e100;
  TFitResultPtr r;
  TFitResult *r_best = new TFitResult();
  for (auto Pars : v_prepPars) {
    SetParLimFixs(f_Q, h_Q, Pars);
    SetPars(f_Q, Pars, true);

    r = h_Q->Fit(f_Q, "S N E M", "", v_prepPars.at(0).Qv * 0.5,
                 v_prepPars.at(0).Qend);

    if (chi > r->Chi2()) {
      chi = r->Chi2();
      *r_best = *r;
      i_best = i;
    }

    cout << "Fit #" << i << "\n\tChi^2 = " << f_Q->GetChisquare() << endl;
    ++i;
  }
  for (int j = 0; j < Nfpars; ++j) {
    f_Q->SetParameter(j, r_best->Parameter(j));
  }
  cout << "blafitres chi2 =" << r_best->Chi2() << endl;

  return r_best;
}

void Drawfun(TF1 *f, TLegend *leg, int n = 10) {
  Float_t Q0 = f->GetParameter("Q0");
  Float_t sig0 = f->GetParameter("sig0");
  Float_t Q1 = f->GetParameter("Q1");
  Float_t sig1 = f->GetParameter("sig1");
  Float_t w = f->GetParameter("w");
  Float_t alpha = f->GetParameter("alpha");
  Float_t mu = f->GetParameter("mu");
  Float_t A = f->GetParameter("A");

  f->SetLineWidth(2);
  f->SetLineColor(2);
  f->Draw("same");

  TF1 *f_Q0 = new TF1("f_Q0", funies::FunoiseQ0, f->GetXmin(), f->GetXmax(), 4);
  f_Q0->SetParameters(Q0, sig0, w, A);
  f_Q0->SetLineColor(3);
  f_Q0->SetLineWidth(1);
  f_Q0->Draw("same");
  leg->AddEntry(f_Q0, "Pedestal Gauss", "l");
  TF1 *f_Exp = new TF1("f_Exp", funies::FunoiseExp, Q0, f->GetXmax(), 4);
  f_Exp->SetParameters(Q0, w, alpha, A);
  f_Exp->SetLineColor(4);
  f_Exp->SetLineWidth(1);
  f_Exp->Draw("same");
  leg->AddEntry(f_Exp, "Exponential noise", "l");
  TF1 *f_s;
  vector<TF1 *> v_f;
  string f_name;
  for (int i = 1; i <= n; ++i) {
    f_name = "f_s" + to_string(i);
    v_f.push_back(
        new TF1(f_name.data(), funies::Funs, f->GetXmin(), f->GetXmax(), 8));
    v_f.back()->SetParameters(Q0, Q1, sig1, w, alpha, mu, A, i);
    v_f.back()->SetLineColor(6);
    v_f.back()->SetLineWidth(1);
    v_f.back()->Draw("same");
  }
  leg->AddEntry(v_f.back(), "Gauss bunch", "l");
}

TH1D *LoadTH1D(TDirectory *rootDir, std::string HistName) {
  TH1D *h = rootDir->Get<TH1D>(HistName.c_str());
  if (rootDir->Get("h_Q") == nullptr) {
    cout << "Histogram " << HistName << " not found in TDirectory "
         << rootDir->GetName() << endl;
    exit(1);
  }
  return h;
}

TH1F *LoadTH1F(TDirectory *rootDir, std::string HistName) {
  TH1F *h = rootDir->Get<TH1F>(HistName.c_str());
  if (rootDir->Get("h_Q") == nullptr) {
    cout << "Histogram " << HistName << " not found in TDirectory "
         << rootDir->GetName() << endl;
    exit(1);
  }
  return h;
}

void FitSPE(TDirectory *inputRootDir, TDirectory *outputRootDir,
            string histName, Int_t n_smooth) {
  FitPars *fitPars = new FitPars;
  PrepPars *prepPars = new PrepPars;
  Pts *pts = new Pts;

  // Load histogram
  TH1F *h_Q = LoadTH1F(inputRootDir, histName);

  TH1F *h_Qnoise = LoadTH1F(inputRootDir, histName + "noise");

  vector<Int_t> v_all[4];
  vector<Int_t> *v_Qp = &v_all[0], *v_Qv = &v_all[1], *v_dQp = &v_all[2],
                *v_dQv = &v_all[3];

  // Clone histogram
  TH1F *h_Q0 = (TH1F *)h_Q->Clone("h_Q0");
  TH1F *h_Qs = (TH1F *)h_Q->Clone("h_Qs"); // smoothed (in FindExtrPts)
  TH1F *h_dQ = (TH1F *)h_Q->Clone("h_dQ"); // diferencial

  FindExtrPts(h_Q0, h_Qs, h_dQ, pts, prepPars, n_smooth, n_smooth);

  TCanvas *c_ei =
      new TCanvas("c", "Extremes and inflection points", 1000, 1400);

  c_ei->Divide(1, 2);

  c_ei->cd(1);
  h_Qs->SetLineColor(2);

  h_Qs->Draw("HIST");
  h_Q0->Draw("HIST same");

  h_Q0->GetXaxis()->SetRange(0, prepPars->Qend * 1.2);

  DrawFrame(h_Q0, *pts);

  c_ei->GetPad(1)->SetLogy();

  if (pts->g_Qpeaks->GetN() > 0 && pts->g_Qvaleys->GetN() > 0) {
    pts->g_Qpeaks->Draw("P same");
    pts->g_Qvaleys->Draw("P same");
  }
  c_ei->cd(2);
  h_dQ->Draw("HIST");

  pts->g_dQpeaks->Draw("P same");
  pts->g_dQvaleys->Draw("P same");
  c_ei->SetLogy();

  //   c_ei->Print("out.pdf");

  //   Fit part
  TCanvas *c_est = new TCanvas("c_est", "Parameter estiamation results");
  gStyle->SetOptStat(0);
  c_est->Divide(1, 3);

  TF1 *f_Q = InitFun(prepPars); // fit function
  vector<FitPars> v_FitPars;
  vector<PrepPars> v_prepPars;

  c_est->cd(1);
  // v_prepPars.push_back(ParEst1(h_Q0, h_Qs, *prepPars, *pts));

  c_est->cd(2);
  // v_prepPars.push_back(ParEst2(h_Q0, h_Qs, *prepPars, *pts));

  c_est->cd(3);
  v_prepPars.push_back(ParEst3(h_Q0, h_Qs, *prepPars, *pts));
  c_est->Print(getImgPath("estim.pdf", outputRootDir).c_str());

  // AddParsFromFile("data/m1/G/940/fit_940.json", v_Pars, pars);
  // AddParsFromFile("data/m1/G/860/fit_860.json", v_Pars, pars);
  // AddParsFromFile("data/m1/G/1040/fit_1040.json", v_Pars, pars);

  // set low weights for pedestal
  // for (int i = 1; i <= h_Q0->GetNbinsX(); ++i) {
  //   if (i < pts->i_Qvaley - 2)
  //     h_Q0->SetBinError(i, 0.5 * h_Q0->GetBinContent(i));
  //   else {
  //     for (int j = 0; j < 4; ++j) {
  //       h_Q0->SetBinError(i + j, (0.4 / (j + 1)) * h_Q0->GetBinContent(i));
  //     }
  //     break;
  //   }

  //   // else
  //   //   h_Q0->SetBinError(i, TMath::Sqrt(h_Q0->GetBinContent(i)));
  // }
  // h_Q0->Sumw2();
  // exit(0);
  // fit with estimated parameters and pick result with smallest chi^2/ndof
  TFitResult BestFitResult = *FitCompare(h_Q0, f_Q, v_prepPars);
  TCanvas *c_fit = new TCanvas("c_fit", "Fit results");

  // rescale histogram to photo-electrons and plot
  Double_t pe = f_Q->GetParameter("Q1");
  Double_t q_bin = h_Q0->GetBinWidth(1) / pe;
  Double_t q_first =
      (-f_Q->GetParameter("Q0")) / f_Q->GetParameter("Q1") + q_bin / 2.;
  Double_t q_last = h_Q0->GetNbinsX() * q_bin + q_bin / 2.;
  cout << h_Q0->GetBinLowEdge(1) * q_bin << endl;
  q_first = h_Q0->GetBinLowEdge(1) / pe;
  q_last = q_first + (h_Q0->GetNbinsX() + 1) * q_bin;

  TH1D *h_Q0scaled = (TH1D *)h_Q0->Clone("");
  h_Q0scaled->SetBins(h_Q0->GetNbinsX(), q_first, q_last);
  // new TH1D("h_Q0scaled", "", h_Q0->GetNbinsX(), q_first, q_last);
  h_Q0scaled->SetMarkerStyle(7);
  h_Q0scaled->SetMarkerColor(1);
  h_Q0scaled->SetMarkerSize(2);
  h_Q0scaled->GetYaxis()->SetRangeUser(0,
                                       h_Q0->GetBinContent(pts->i_Qpeak) * 1.3);
  h_Q0scaled->GetXaxis()->SetRangeUser(prepPars->Qstart / pe,
                                       h_Q0scaled->GetBinCenter(pts->i_Qend));
  h_Q0scaled->GetXaxis()->SetTitle("Charge [pe]");
  h_Q0scaled->Draw("H P");
  // h_Q0->Draw("H P");
  // h_Q0scaled->Write();

  // restale function to 1pe units on x axis
  f_Q->SetRange(0, h_Q0scaled->GetBinCenter(pts->i_Qend));
  f_Q->SetParameter("Q0", f_Q->GetParameter("Q0") / pe);
  f_Q->SetParameter("sig0", f_Q->GetParameter("sig0") / pe);
  f_Q->SetParameter("Q1", 1.);
  f_Q->SetParameter("sig1", f_Q->GetParameter("sig1") / pe);
  f_Q->SetParameter("alpha", f_Q->GetParameter("alpha") * pe);
  f_Q->SetParameter("A", f_Q->GetParameter("A") / pe);

  cout << "\tChi^2/NDF = " << BestFitResult.Chi2() / BestFitResult.Ndf()
       << endl;

  // outputs to files
  TLegend *leg_fit = new TLegend(0.6, 0.5, 0.9, 0.9);
  Drawfun(f_Q, leg_fit);
  leg_fit->AddEntry(h_Q0, "PMT Spectre", "L P");
  leg_fit->AddEntry(f_Q, "FIT", "L");
  leg_fit->AddEntry("n",
                    ((string) "Chi^{2}/NDF = " +
                     to_string(BestFitResult.Chi2() / BestFitResult.Ndf()))
                        .data(),
                    "n");
  f_Q->Draw("same");
  leg_fit->Draw();
  c_ei->Print(getImgPath("_extr.pdf", outputRootDir).c_str());
  c_fit->Print(getImgPath("_fit.pdf", outputRootDir).c_str());

  // save fit parameters to root file
  TParameter<Double_t> parval;
  TObjString parname;
  for (int i = 0; i < BestFitResult.NPar(); ++i) {
    parval.SetVal(BestFitResult.Parameter(i));
    // parval.Write(BestFitResult.GetParameterName(i).c_str());
    // cout << BestFitResult.GetParameterName(i) << ":" << parval.GetVal() <<
    // endl;
    outputRootDir->WriteObject(&parval,
                               BestFitResult.GetParameterName(i).c_str());
  }
}

TF1 *getExp_Gauss(Double_t xmax) {
  TF1 *f =
      new TF1("ExpAndGauss",
              "([0]*exp(-0.5*((x-[1])/[2])^2) + [3]*exp(-[4]*x))", 0, xmax);
  f->SetParNames("A", "mu", "sig", "w", "alpha");
  return f;
}
struct GausPars {
  Double_t A = 0;
  Double_t mu;
  Double_t sig;
  Double_t w;
  Double_t alpha;
  Double_t Chi2 = 500;
};

void FitGauss(TDirectory *inputRootDir, TDirectory *outputRootDir) {
  TCanvas *c = new TCanvas();
  TH1D *h_Q = LoadTH1D(inputRootDir, "h_Q");
  TF1 *f2 = new TF1("Gauss", "gaus");
  // f->SetParameter(0, h_Q->GetMaximum());
  // f->SetParameter(1, h_Q->GetMean());
  // f->SetParameter(2, h_Q->GetStdDev());
  h_Q->Fit(f2, "Q", "");
  h_Q->Draw("same");
  c->SetLogy();
  c->Print(getImgPath("_gaussfit_hQ.pdf", outputRootDir).c_str());
}

void FitGauss_fromRight(TDirectory *rootDir) {
  TCanvas *c = new TCanvas();
  TH1D *h_Q = LoadTH1D(rootDir, "h_Q");
  TH1D *h_Qs = (TH1D *)h_Q->Clone("h_Qs");
  h_Qs->SetLineColor(8);
  TGraph *g_Chi2 = new TGraph();
  TGraph *g_A = new TGraph();
  TGraph *g_sig = new TGraph();
  TF1 *f = getExp_Gauss(h_Q->GetBinCenter(h_Q->GetNbinsX()));

  h_Qs->Smooth(5);
  // h_Qs->Draw();
  Double_t A, mu, sig, w, alpha;

  Int_t bins_from_right = 5;
  Int_t bins_from_left = h_Q->FindBin(0) + 5;
  Int_t most_right_bin = h_Q->GetNbinsX();
  Double_t Chi2_limit = 1.5;
  GausPars BestPars;

  while (BestPars.Chi2 > Chi2_limit) {
    for (Int_t h_i = most_right_bin - bins_from_right; h_i >= bins_from_left;
         --h_i) {
      if (h_Qs->GetBinContent(h_i) < 50)
        continue;
      f->FixParameter(0, h_Qs->GetBinContent(h_i));
      // f->SetParLimits(0, 16, h_Qs->GetMaximum());
      f->FixParameter(1, h_Q->GetBinCenter(h_i));
      f->SetParameter(2, (most_right_bin - h_i) * h_Q->GetBinWidth(1) / 2.);
      f->SetParLimits(2, 0, (most_right_bin - h_i) * h_Q->GetBinWidth(1));
      f->FixParameter(3, 0);
      f->FixParameter(4, 0);
      h_Qs->Fit(f, "Q N", "", h_Q->GetBinCenter(h_i),
                h_Q->GetBinCenter(most_right_bin));

      // GoodPars.push_back(GausPars());
      // GoodPars.back().A = f->GetParameter("A");
      // GoodPars.back().mu = f->GetParameter("mu");
      // GoodPars.back().sig = f->GetParameter("sig");

      if (f->GetChisquare() / f->GetNDF() < Chi2_limit &&
          f->GetParameter("A") > BestPars.A) {

        BestPars.Chi2 = f->GetChisquare() / f->GetNDF();
        BestPars.A = f->GetParameter("A");
        BestPars.mu = f->GetParameter("mu");
        BestPars.sig = f->GetParameter("sig");
        cout << "Chi2 = " << BestPars.Chi2 << "\t,\t"
             << "A = " << BestPars.A << endl;
      }
      // g_A->AddPoint(h_Q->GetBinCenter(h_i), f->GetParameter("A"));
      // g_sig->AddPoint(h_Q->GetBinCenter(h_i), f->GetParameter("sig"));
      // g_Chi2->AddPoint(h_Q->GetBinCenter(h_i), f->GetChisquare() /
      // f->GetNDF());
    }
    cout << "Chi2_limit = " << Chi2_limit << "\t";
    cout << "Chi2_best = " << BestPars.Chi2 << endl;
    Chi2_limit += 0.1;
  }
  cout << "Chi2_limit = " << Chi2_limit << "\t";
  cout << "Chi2_best = " << BestPars.Chi2 << endl;
  f->SetParameter(0, BestPars.A);
  f->SetParLimits(0, BestPars.A * 0.8, BestPars.A * 1.2);

  f->SetParLimits(1, BestPars.mu - BestPars.sig * 0.3,
                  BestPars.mu + BestPars.sig * 0.3);
  f->SetParameter(1, BestPars.mu);
  f->SetParameter(2, BestPars.sig);
  f->SetParLimits(2, BestPars.sig * 0.5, BestPars.sig * 1.5);

  h_Q->Fit(f, "Q", "", BestPars.mu - BestPars.sig * 0.3,
           h_Q->GetBinCenter(most_right_bin));

  BestPars.Chi2 = f->GetChisquare() / f->GetNDF();
  BestPars.A = f->GetParameter("A");
  BestPars.mu = f->GetParameter("mu");
  BestPars.sig = f->GetParameter("sig");
  // TF1 *f2 = new TF1("Gauss", "gaus");
  // // f->ReleaseParameter(0);
  // // f->SetParameter(0, h_Q->GetMaximum());
  // // f->ReleaseParameter(1);
  // // f->SetParameter(1, h_Q->GetMean());
  // // f->ReleaseParameter(2);
  // // f->SetParameter(2, h_Q->GetStdDev());
  // h_Q->Fit(f2, "N Q", "");
  // cout << f2->GetChisquare() / f2->GetNDF() << endl;

  // g_sig->Draw();
  // c->Print("gaussfit_sig.pdf");
  // c->Clear();
  // g_A->Draw();
  // c->Print("gaussfit_A.pdf");
  // c->Clear();
  // g_Chi2->Draw();
  // // g_Chi2->GetYaxis()->SetRangeUser(0, 2);
  // c->Print("gaussfit_Chi2.pdf");
  // c->Clear();
  // f->Draw("");
  // h_Qs->Draw("same");

  h_Q->Draw("same");
  c->Print(getImgPath("_gaussfit_r.pdf", rootDir).c_str());

  // rootDir->WriteObject(f, rootDir->GetName());
}
