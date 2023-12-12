#include "routines.h"
#include <RtypesCore.h>
#include <TDirectory.h>
#include <TTree.h>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <math.h>
#include <ostream>
#include <string>
namespace fs = std::filesystem;

// pushes decimal point by 5 to left
std::string ns_to_10fs(std::string s) {
  std::string result;
  int decimalIndex = s.find('.');

  if (decimalIndex != std::string::npos) {
    // Extract the integer part before the decimal point
    std::string integerPart = s.substr(0, decimalIndex);

    // Extract the fractional part after the decimal point
    std::string fractionalPart = s.substr(decimalIndex + 1);

    // cut or extend fractional part
    if (fractionalPart.size() > 5) {
      if (fractionalPart.at(5) >= 5) {
        // coerce
        fractionalPart.insert(5, ".");
        Float_t fracDouble = round(std::stod(fractionalPart));
        fractionalPart = std::to_string(static_cast<Int_t>(fracDouble));
      }
      // cut
      fractionalPart = fractionalPart.substr(0, 5);
    } else
      // add zeros if fractional part is short
      while (fractionalPart.size() < 5)
        fractionalPart += '0';

    // concatenate previous decimal and fractional part
    result = integerPart + fractionalPart;
  }

  return result;
}

namespace infn {
void parseFile(fs::path InputCsvFilePath, TDirectory *outputRootDir) {
  // check if InputCsvFilePath is actual file
  if (not fs::exists(InputCsvFilePath)) {
    std::cerr << "File " << InputCsvFilePath << " does not exist" << std::endl;
    exit(1);
  }

  std::ifstream InputCsvFile(InputCsvFilePath);
  std::string EventLine, HeaderLine;
  std::string RequiredHeaderLine(
      "Channel,Time Coarse (u=ns),T.o.t. coarse (u=ns),Time fine (u=ns),T.o.t. "
      "fine (u=ns),Event time (ns),Event t.o.t. (ns),Energy (u= ADC "
      "channels),Acquisition time,Date");
  std::getline(InputCsvFile, HeaderLine);
  HeaderLine = HeaderLine.substr(0, HeaderLine.size() - 1);
  // check if header matches the RequiredHeaderLine
  if (abs(HeaderLine.compare(RequiredHeaderLine)) > 1) {
    std::cerr << "File " << InputCsvFilePath
              << " does not have required header line:" << std::endl
              << HeaderLine << std::endl
              << RequiredHeaderLine << std::endl;
    InputCsvFile.close();
    exit(1);
  }

  // parsed variables
  uint64_t Time_10fs;          // time unit is 10fs which allows ~ 2 days range
  Double_t TimeOverTrigger_ns; // aka pulse length
  Float_t Energy;
  Int_t Channel;
  char Date[11]; // in format yyyy-mm-dd
  char Time[9];  // in format hh:mm:ss

  // init TTree and create branches
  TTree *PMTAFTree = new TTree(
      "pmtaf_tree",
      ("Pulse events from " + std::string(InputCsvFilePath.c_str())).c_str());
  PMTAFTree->Branch("time_10fs", &Time_10fs);
  PMTAFTree->Branch("time_over_trigger_ns", &TimeOverTrigger_ns);
  PMTAFTree->Branch("energy", &Energy);
  PMTAFTree->Branch("channel", &Channel);
  PMTAFTree->Branch("date", Date, "date/C", 10);
  PMTAFTree->Branch("time", Time, "time/C", 8);

  // process variables
  uint64_t CurrentTime_10fs = 0;
  uint64_t PreviousTime_10fs = 0;
  Int_t ClockOveflows = 0;
  Int_t i0, i1, iLength, iLast;
  std::string SubString;
  while (std::getline(InputCsvFile, EventLine)) {
    // get Channel
    Channel = stoi(EventLine);

    // get Date
    i1 = EventLine.size() - 2;
    i0 = EventLine.find_last_of(',') + 1;
    SubString = EventLine.substr(i0, i1 + 1 - i0);
    SubString.copy(Date, SubString.size());
    Date[10] = '\0';

    // get Global Time
    i1 = i0 - 2;
    i0 = EventLine.find_last_of(',', i1) + 1;
    SubString = EventLine.substr(i0, i1 - i0 + 1);
    SubString.copy(Time, SubString.size());
    Time[8] = '\0';

    // get Energy
    i1 = i0 - 2;
    i0 = EventLine.find_last_of(',', i1) + 1;
    SubString = EventLine.substr(i0, i1 - i0 + 1);
    Energy = std::stod(SubString);

    // get TimeOverTrigger
    i1 = i0 - 2;
    i0 = EventLine.find_last_of(',', i1) + 1;
    SubString = EventLine.substr(i0, i1 - i0 + 1);
    TimeOverTrigger_ns = std::stod(SubString);

    // get Time
    i1 = i0 - 2;
    i0 = EventLine.find_last_of(',', i1) + 1;
    SubString = EventLine.substr(i0, i1 - i0 + 1);
    CurrentTime_10fs = std::stoull(ns_to_10fs(SubString));
    // if internal clock owerwlofs
    if (CurrentTime_10fs <= PreviousTime_10fs)
      ClockOveflows++;

    Time_10fs = static_cast<uint64_t>(CurrentTime_10fs) +
                static_cast<uint64_t>(10e12 * ClockOveflows);
    // 10e12 is an overflow threshold
    PreviousTime_10fs = CurrentTime_10fs;

    PMTAFTree->Fill();
  }
  InputCsvFile.close();
  outputRootDir->WriteObject(PMTAFTree, "pmtaf_tree");
};
} // namespace infn