#include <cstdint>
#include <filesystem>
#include <ostream>
namespace fs = std::filesystem;
#include <TFile.h>
#include <TTree.h>
#include <fstream>
#include <iomanip>
#include <iostream>

#include <set>

std::string convertDateFormat(const std::string &dateStr) {
  std::stringstream ss(dateStr);
  std::tm date = {};

  ss >> std::get_time(&date, "%d-%b-%Y");

  if (ss.fail()) {
    return ""; // Invalid date format
  }

  std::stringstream result;
  result << std::put_time(&date, "%Y-%m-%d");

  return result.str();
}

std::string removeSpecialCharacters(const std::string &str) {
  std::string result;

  for (char c : str) {
    if (!std::isspace(static_cast<unsigned char>(c))) {
      result += c;
    } else if (static_cast<unsigned char>(c) == ' ')
      result += ',';
  }

  return result;
}

Int_t getLastDigitIndex(const std::string &str) {

  for (int i = str.length() - 1; i >= 0; i--) {
    if (isdigit(str[i])) {
      // std::cout << str << std::endl << " > " << i << std::endl;
      return i;
    }
  }

  return -1; // No digit found
}

Int_t n_digit(Int_t number) {
  Int_t count = 0;
  while (number != 0) {
    number = number / 10;
    count++;
  }
  return count;
}
std::string s_to_10fs(std::string s) {
  Int_t Shift = 14;
  std::string result;
  int decimalIndex = s.find('.');
  // std::cout << " > " << std::string("ab-") + s + std::string("-cd")
  //           << std::endl;
  // exit(0);
  if (decimalIndex == std::string::npos)
    s = "0.0";

  // Extract the integer part before the decimal point
  std::string integerPart = s.substr(0, decimalIndex);

  // Extract the fractional part after the decimal point
  std::string fractionalPart = s.substr(decimalIndex + 1);

  // cut or extend fractional part
  if (fractionalPart.size() > Shift) {
    if (fractionalPart.at(Shift) >= Shift) {
      // coerce
      fractionalPart.insert(Shift, ".");
      Double_t fracDouble = round(std::stod(fractionalPart));
      fractionalPart = std::to_string(static_cast<Int_t>(fracDouble));
    }
    // cut
    fractionalPart = fractionalPart.substr(0, Shift);
  } else
    // add zeros if fractional part is short
    while (fractionalPart.size() < Shift)
      fractionalPart += '0';

  // concatenate previous decimal and fractional part
  result = integerPart + fractionalPart;

  // std::cout << "#########" << std::endl
  //           << s << std::endl
  //           << result << std::endl;

  return result;
}
void GetFileInfo(std::ifstream &InputFile, Int_t &fpos_Wfm, Int_t &fpos_h,
                 Int_t &n_Wfm, Int_t &n_Seg) {
  char line[64];
  InputFile.getline(line, 64);
  std::string l = line;

  while (l.compare("Time,Ampl") < 0 && !InputFile.eof()) {
    if (l.find("Segments,") <= l.length()) {
      n_Wfm = stoi(l.substr(9));
      n_Seg = stoi(l.substr(9 + n_digit(n_Wfm) + 13)) + 1000;
    }
    if (fpos_h <= 0)
      fpos_h = -InputFile.tellg();
    InputFile.getline(line, 64);
    l = line;
    fpos_Wfm = InputFile.tellg();
    if (fpos_h <= 0) {
      if (line[0] == '#')
        fpos_h = -fpos_h;
    }
  }
  InputFile.seekg(0, InputFile.beg);
}
// InputFile, fpos_Wfm, fpos_h, iSeg, Date, Time, Time_10fs, x, t, maxAmpl,
// InvertPolarity
Int_t ParseWfm(std::ifstream &InputFile, Int_t &fpos_Wfm, Int_t &fpos_h,
               Int_t &iSeg, char Date[11], char GlobalTime[9],
               uint64_t &Time_10fs, std::vector<Double_t> &x,
               std::vector<Float_t> &t, Double_t &maxAmpl,
               bool InvertPolarity) {

  // some parsed values used in process
  Double_t Ampl;        //->x
  Float_t Time = -1e10; //->t
  Float_t p_Time;       // previous
  std::string l;
  std::string::size_type sz;
  char line[64];

  // wfm head process ... UGLY BUT DO NOT TOUCH UNTIL IT WORKS :)
  InputFile.seekg(fpos_h, InputFile.beg);
  InputFile.getline(line, 64);
  l = line;
  l = removeSpecialCharacters(l); // and actually replace ' ' with ',' :)
  l = l.substr(1);                // skips "#" i.e. 1st character in row
  iSeg = std::stoi(l, &sz);
  l = l.substr(sz + 1); // date, time since segment1... date format =
  std::string _Date = l.substr(0, l.find_first_of(','));
  l = l.substr(l.find_first_of(',') + 1);
  convertDateFormat(_Date).copy(Date, 10);
  Date[10] = '\0';
  std::string _GlobalTime = l.substr(0, l.find_first_of(','));
  _GlobalTime.copy(GlobalTime, 8);
  GlobalTime[8] = '\0';
  l = l.substr(l.find_first_of(',') + 1);
  l = l.substr(0, l.find_first_of(','));

  // std::cout << _GlobalTime << std::endl;

  // l = l.substr(21, getLastDigitIndex(l) - 1);

  // std::cout << l << "U" << std::endl;

  Time_10fs = stoull(s_to_10fs(l));

  // Time_10fs = 5;
  fpos_h = InputFile.tellg();
  // wfm data process
  InputFile.seekg(fpos_Wfm, InputFile.beg);
  t.clear();
  x.clear();
  Int_t i = 0;
  maxAmpl = -1;
  while (1) {
    p_Time = Time;
    fpos_Wfm = InputFile.tellg();
    InputFile.getline(line, 64);
    l = line;
    if (InputFile.eof() || l.empty())
      return 0;
    Time = stof(l, &sz);
    l = l.substr(sz + 1);

    // Threshold sign defines a wfm polarity!
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

void ProcessFile(const fs::path InputFilePath, TTree *PulseTree, Int_t &iSeg,
                 char Date[11], char Time[9], uint64_t &Time_10fs,
                 std::vector<Double_t> &x, std::vector<Float_t> &t,
                 Double_t Threshold = 0, bool InvertPolarity = 0) {
  std::ifstream InputFile;
  InputFile.open(InputFilePath, std::fstream::in);

  Int_t fpos_h = 0;   // position for reading wfm data
  Int_t fpos_Wfm = 0; // position for reading wfm data
  Int_t n_Wfm;        // # of Wfms
  Int_t n_Seg;        // # of samples in Wfm ~ segment
  Double_t maxAmpl;   // max amplitude in the processed waveform - it is used to
                      // decide whether write wfm into the .root file
  GetFileInfo(InputFile, fpos_Wfm, fpos_h, n_Wfm, n_Seg);
  std::cout << "Processing - " << InputFilePath << std::endl;
  while (true) {
    ParseWfm(InputFile, fpos_Wfm, fpos_h, iSeg, Date, Time, Time_10fs, x, t,
             maxAmpl, InvertPolarity);
    if (Threshold == 0)
      PulseTree->Fill();
    else if (maxAmpl >= std::abs(Threshold))
      PulseTree->Fill(); // throw out waveforms under threshold

    if (iSeg % 1000 == 0)
      std::cout << iSeg << "/" << n_Wfm << std::endl;
    if (InputFile.eof())
      break;
  }

  InputFile.close();
}

// exported functions
namespace osclec {
void parseFiles(TDirectory *rootDir, std::set<fs::path> InputFileNames,
                Double_t Threshold, Bool_t InvertPolarity) {

  // parsed variables
  Int_t iSeg;
  char Date[11];
  char Time[9];
  uint64_t Time_10fs;
  std::vector<Double_t> x;
  std::vector<Float_t> t;

  // init TTree and create branches
  TTree *PulseTree =
      new TTree("pmtaf_pulses", "Waveforms from LECROYWM806Zi-B");
  PulseTree->Branch("i_seg", &iSeg);
  PulseTree->Branch("date", Date, "date/C", 10);
  PulseTree->Branch("time", Time, "time/C", 8);
  PulseTree->Branch("time_10fs", &Time_10fs);
  PulseTree->Branch("x", &x);
  PulseTree->Branch("t", &t);

  // process input files
  for (const std::string &InputFilePath : InputFileNames) {
    // std::cout << "fName: " << InputFileName << std::endl;
    ProcessFile(InputFilePath, PulseTree, iSeg, Date, Time, Time_10fs, x, t,
                Threshold, InvertPolarity);
  }
  rootDir->WriteObject(PulseTree, "pmtaf_pulses");
}
} // namespace osclec