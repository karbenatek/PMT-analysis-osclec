#include <filesystem>
namespace fs = std::filesystem;
#include <TFile.h>
#include <TTree.h>
#include <fstream>
#include <iostream>
#include <set>

Int_t n_digit(Int_t number) {
  Int_t count = 0;
  while (number != 0) {
    number = number / 10;
    count++;
  }
  return count;
}
void GetFileInfo(std::ifstream &file, Int_t &fpos_Wfm, Int_t &fpos_h,
                 Int_t &n_Wfm, Int_t &n_Seg) {
  char line[64];
  file.getline(line, 64);
  std::string l = line;

  while (l.compare("Time,Ampl") < 0 && !file.eof()) {
    if (l.find("Segments,") <= l.length()) {
      n_Wfm = stoi(l.substr(9));
      n_Seg = stoi(l.substr(9 + n_digit(n_Wfm) + 13)) + 1000;
    }
    if (fpos_h <= 0)
      fpos_h = -file.tellg();
    file.getline(line, 64);
    l = line;
    fpos_Wfm = file.tellg();
    if (fpos_h <= 0) {
      if (line[0] == '#')
        fpos_h = -fpos_h;
    }
  }
  file.seekg(0, file.beg);
}
Int_t ParseWfm(std::ifstream &file, Int_t &fpos_Wfm, Int_t &fpos_h,
               Int_t &i_Wfm, Int_t &i_Seg, char Date[21],
               Double_t &TimeSinceSeg1, std::vector<Double_t> &x,
               std::vector<Float_t> &t, Double_t &maxAmpl,
               bool InvertPolarity) {
  // some parsed values used in process
  Double_t Ampl;        //->x
  Float_t Time = -1e10; //->t
  Float_t p_Time;       // previous
  std::string l;
  std::string::size_type sz;
  char line[64];

  // wfm head process
  file.seekg(fpos_h, file.beg);
  file.getline(line, 64);
  l = line;
  l = l.substr(1); // skips "#" character
  i_Seg = std::stoi(l, &sz);
  ++i_Wfm;
  l = l.substr(sz + 1); // date, time since segment1... date format =
                        // 01-Aug-2022 11:55:02 ... 20 chars
  std::string sDate = l.substr(0, 20);
  for (Int_t i = 0; i < sDate.size(); ++i)
    Date[i] = sDate[i];
  l = l.substr(21);
  TimeSinceSeg1 = stod(l);
  fpos_h = file.tellg();
  // wfm data process
  file.seekg(fpos_Wfm, file.beg);
  t.clear();
  x.clear();
  Int_t i = 0;
  maxAmpl = -1;
  while (1) {
    p_Time = Time;
    fpos_Wfm = file.tellg();
    file.getline(line, 64);
    l = line;
    if (file.eof() || l.empty())
      return 0;
    Time = stof(l, &sz);
    l = l.substr(sz + 1);

    // thr sign defines a wfm polarity!
    Ampl = stod(l);
    if (InvertPolarity)
      Ampl = -stod(l);

    if (Time >= p_Time) {
      if (Ampl > maxAmpl)
        maxAmpl = Ampl;
      t.push_back(Time);
      x.push_back(Ampl);
      ++i;
    } else
      return 0;
  }
}

void ProcessFile(const fs::path fPath, TTree *fChain, Int_t &i_Wfm,
                 Int_t &i_Seg, char Date[21], Double_t &TimeSinceSeg1,
                 std::vector<Double_t> &x, std::vector<Float_t> &t,
                 Double_t thr = 0, bool InvertPolarity = 0) {
  std::ifstream file;
  file.open(fPath, std::fstream::in);

  Int_t fpos_h = 0;   // position for reading wfm data
  Int_t fpos_Wfm = 0; // position for reading wfm data
  Int_t n_Wfm;        // # of Wfms
  Int_t n_Seg;        // # of samples in Wfm ~ segment
  Double_t maxAmpl;   // max amplitude in the processed waveform - it is used to
                      // decide whether write wfm into the .root file
  GetFileInfo(file, fpos_Wfm, fpos_h, n_Wfm, n_Seg);
  std::cout << "Processing - " << fPath << std::endl;
  while (true) {
    ParseWfm(file, fpos_Wfm, fpos_h, i_Wfm, i_Seg, Date, TimeSinceSeg1, x, t,
             maxAmpl, InvertPolarity);
    if (thr == 0)
      fChain->Fill();
    else if (maxAmpl >= std::abs(thr))
      fChain->Fill(); // throw out waveforms under threshold

    if (i_Seg % 1000 == 0)
      std::cout << i_Seg << "/" << n_Wfm << std::endl;
    if (file.eof())
      break;
  }

  file.close();
}

// exported functions
namespace osclec {
void parseFiles(TDirectory *rootDir, std::set<fs::path> inputFileNames,
                Double_t thr, Bool_t InvertPolarity) {

  TTree *fChain = new TTree("pulses", "Waveforms from LECROYWM806Zi-B");
  // parsed variables
  Int_t i_Wfm = 0;
  Int_t i_Seg;
  char Date[22];
  Double_t TimeSinceSeg1;
  std::vector<Double_t> x;
  std::vector<Float_t> t;

  // def branches
  fChain->Branch("i_Wfm", &i_Wfm);
  fChain->Branch("i_Seg", &i_Seg);
  fChain->Branch("Date", Date, "Date/C", 22);
  fChain->Branch("TimeSinceSeg1", &TimeSinceSeg1);
  fChain->Branch("x", &x);
  fChain->Branch("t", &t);

  // process input files
  for (const std::string &inputFilePath : inputFileNames) {
    // std::cout << "fName: " << inputFileName << std::endl;
    ProcessFile(inputFilePath, fChain, i_Wfm, i_Seg, Date, TimeSinceSeg1, x, t,
                thr, InvertPolarity);
  }
  rootDir->WriteObject(fChain, "pulses");
}
} // namespace osclec