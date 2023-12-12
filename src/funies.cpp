#include <TMath.h>

namespace funies {
Double_t FunoiseQ0(Double_t *x, Double_t *par) {
  Double_t xx = x[0];
  Double_t Q0 = par[0];
  Double_t sig0 = par[1];
  Double_t w = par[2];
  Double_t A = par[3];
  Double_t f =
      A * (1 - w) / (sig0 * TMath::Sqrt(TMath::TwoPi())) *
      TMath::Exp(-(TMath::Power(xx - Q0, 2) / (2 * TMath::Power(sig0, 2))));

  return f;
}
Double_t FunoiseExp(Double_t *x, Double_t *par) {
  Double_t xx = x[0];
  Double_t Q0 = par[0];
  Double_t w = par[1];
  Double_t alpha = par[2];
  Double_t A = par[3];
  Double_t StepFun;
  if ((xx - Q0) >= 0)
    StepFun = 1;
  else
    StepFun = 0;
  Double_t f = w * StepFun * alpha * TMath::Exp(-alpha * (xx - Q0)) * A;

  return f;
}
Double_t Funoise(Double_t *x, Double_t *par) {
  Double_t xx = x[0];
  Double_t Q0 = par[0];
  Double_t sig0 = par[1];
  Double_t w = par[2];
  Double_t alpha = par[3];
  Double_t A = par[4];
  Double_t StepFun;
  if ((xx - Q0) >= 0)
    StepFun = 1;
  else
    StepFun = 0;
  Double_t f = ((1 - w) / (sig0 * TMath::Sqrt(TMath::TwoPi())) *
                    TMath::Exp(-(TMath::Power(xx - Q0, 2) /
                                 (2 * TMath::Power(sig0, 2)))) +
                w * StepFun * alpha * TMath::Exp(-alpha * (xx - Q0))) *
               A;
  return f;
}

Double_t Funsig(Double_t *x, Double_t *par) {
  Double_t xx = x[0];
  Double_t Q0 = par[0];
  Double_t Q1 = par[1];
  Double_t sig1 = par[2];
  Double_t mu = par[3];
  Double_t A = par[4];

  Double_t f = 0;
  for (Int_t n = 1; n <= 100; ++n) {
    f += TMath::Power(mu, n) * A /
         (TMath::Factorial(n) * sig1 * TMath::Sqrt(TMath::TwoPi() * n)) *
         TMath::Exp(-TMath::Power(xx - Q0 - n * Q1, 2) /
                    (2 * n * TMath::Power(sig1, 2)));
  }
  return f;
}
Double_t Funs(Double_t *x, Double_t *par) {
  Double_t xx = x[0];
  Double_t Q0 = par[0];
  Double_t Q1 = par[1];
  Double_t sig1 = par[2];
  Double_t w = par[3];
  Double_t alpha = par[4];
  Double_t mu = par[5];
  Double_t A = par[6];
  Double_t n = par[7];
  Double_t f = TMath::Power(mu, n) * A /
               (TMath::Factorial(n) * sig1 * TMath::Sqrt(TMath::TwoPi() * n)) *
               TMath::Exp(-TMath::Power(xx - Q0 - w / alpha - n * Q1, 2) /
                          (2 * n * TMath::Power(sig1, 2)));

  return f;
}
Double_t Fun(Double_t *x, Double_t *par) {
  Double_t xx = x[0];
  Double_t Q0 = par[0];
  Double_t sig0 = par[1];
  Double_t Q1 = par[2];
  Double_t sig1 = par[3];
  Double_t w = par[4];
  Double_t alpha = par[5];
  Double_t mu = par[6];
  Double_t A = par[7];

  //~ Double_t As = par[8];

  Double_t StepFun;
  if ((xx - Q0) >= 0)
    StepFun = 1;
  else
    StepFun = 0;
  Double_t f = A * ((1 - w) / (sig0 * TMath::Sqrt(TMath::TwoPi())) *
                        TMath::Exp(-(TMath::Power(xx - Q0, 2) /
                                     (2 * TMath::Power(sig0, 2)))) +
                    w * StepFun * alpha * TMath::Exp(-alpha * (xx - Q0)));
  for (Int_t n = 1; n <= 100; ++n) {
    f += A * TMath::Power(mu, n) /
         (TMath::Factorial(n) * sig1 * TMath::Sqrt(TMath::TwoPi() * n)) *
         TMath::Exp(-TMath::Power(xx - Q0 - w / alpha - n * Q1, 2) /
                    (2 * n * TMath::Power(sig1, 2)));
  }
  return f;
}
Double_t Fun_mu(Double_t *x, Double_t *par) {
  Double_t xx = x[0];
  Double_t mu = x[1];
  Double_t Q1 = par[0];
  Double_t sig1 = par[1];
  Double_t A = 1;
  Double_t Q0 = 0;
  Double_t f = 0;
  for (Int_t n = 1; n <= 100; ++n) {
    f += TMath::Power(mu, n) * A /
         (TMath::Factorial(n) * sig1 * TMath::Sqrt(TMath::TwoPi() * n)) *
         TMath::Exp(-TMath::Power(xx - Q0 - n * Q1, 2) /
                    (2 * n * TMath::Power(sig1, 2)));
  }

  return f;
}
} // namespace funies