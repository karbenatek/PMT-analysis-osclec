#include "libanalis.h"
#include "libfit.h"
#include "libinfn.h"
#include "libosclec.h"
#include "routines.h"

// #include <RtypesCore.h>
#include <RtypesCore.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TH1.h>
#include <TUUID.h>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <getopt.h>
#include <iostream>
// #include <iterator>
#include <iterator>
#include <map>
#include <ostream>
#include <string>
#include <toml++/toml.h>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

struct ParsedArgs {
  fs::path cfgFileName = "";
  Bool_t recreate = false;
};

void prntUsage(char *argv[]) {
  std::cerr << "Usage: " << argv[0] << " [-c cfg_file] [-r]" << std::endl;
  std::cerr << "  Executes PhotoMultiplier analysis." << std::endl << std::endl;
  std::cerr << "Options:" << std::endl;
  std::cerr << "  -c cfg_file    Specify the configuration file to use"
            << std::endl;
  std::cerr << "  -r             Recreate all output root files" << std::endl;
}

ParsedArgs parseArgs(int argc, char *argv[]) {
  ParsedArgs parsedArgs;

  // Define the options and their arguments
  struct option long_options[] = {
      {nullptr, 0, nullptr, 0}, // Termination option
      {"config_file", required_argument, nullptr, 'c'},
      {"recreate", no_argument, nullptr, 'r'},
  };

  // Get named arguments
  int option;
  while ((option = getopt_long(argc, argv, "c:r::", long_options, nullptr)) !=
         -1) {
    switch (option) {
    case 'c':
      parsedArgs.cfgFileName = optarg;
      break;
    case 'r':
      parsedArgs.recreate = true;
      break;
    case '?':
      // Handle invalid options here
      std::cerr << "Invalid option." << std::endl;
      exit(1);
    default:
      std::cout << "default: " << optarg << std::endl;
      break;
    }
  }

  // wrong input handling
  if (parsedArgs.cfgFileName == "") {
    prntUsage(argv);
    exit(1);
  }

  if (not fs::exists(parsedArgs.cfgFileName)) {
    if (parsedArgs.cfgFileName.empty()) {
      std::cerr << "Cfg file not defined" << std::endl;
      prntUsage(argv);
    } else {
      std::cerr << "Cfg file " << parsedArgs.cfgFileName << " not found"
                << std::endl;
      exit(1);
    }
  }

  std::cout << "Used configuration file: " << parsedArgs.cfgFileName
            << std::endl;

  return parsedArgs;
}

void recreateRootFile(fs::path filePath) {
  if (fs::exists(filePath))
    std::remove(filePath.c_str());
  TFile *rootFile = TFile::Open(filePath.c_str(), "create");
  rootFile->Close();
  std::cout << "File " << filePath << " re/created" << std::endl;
}

/*
get TDirectory or ,if not exists, create (create = true)/ throw error (create
= false)
*/
TDirectory *getRootDir(TFile *rootFile, std::string directoryName,
                       Bool_t create = 0) {
  TDirectory *rootDir;
  rootDir = rootFile->GetDirectory(directoryName.c_str());

  if (not rootDir) {
    // if TDirectory does not exist, create new or throw error depending on
    // Bool_t create value
    if (create) {
      rootDir = rootFile->mkdir(directoryName.c_str());
      std::cout << "TDirectory \"" << directoryName << "\" created in TFile \""
                << rootFile->GetName() << "\"" << std::endl;
    } else {
      std::cerr << "TDirectory \"" << directoryName
                << "\" does not exist in file \"" << rootFile->GetName() << "\""
                << std::endl;
      exit(1);
    }
  }
  return rootDir;
}

/*
open TFile as READ (write = 0)/ WRITE (write = 1) or ,if not exists,
create(write = 1)/ throw error (write = 0)
*/
TFile *getRootFile(fs::path rootFilePath, Bool_t write = 0) {
  TFile *rootFile;
  if (fs::exists(rootFilePath)) {
    rootFile = TFile::Open(rootFilePath.c_str(), write ? "update" : "read");
  } else {
    // if TFile does not exist, create new or throw error depending on
    // Bool_t create value
    if (write) {
      rootFile = TFile::Open(rootFilePath.c_str(), "CREATE");
      std::cout << "File " << rootFilePath << " created" << std::endl;
    } else {
      std::cerr << "File " << rootFilePath << " does not exist" << std::endl;
      exit(1);
    }
  }
  return rootFile;
}
namespace toml {
/*
look for string type parameter in ParameterTable and throw error if does not
exist
 */
std::string getString(toml::table ParameterTable, std::string ParameterName) {
  if (ParameterTable[ParameterName.c_str()].is_string())
    return *ParameterTable[ParameterName.c_str()].value<std::string>();
  else {
    std::cerr << "String parameter \"" << ParameterName
              << "\" does not exist in following parameter table:" << std::endl
              << ParameterTable << std::endl
              << std::endl;
    exit(1);
  }
}

Float_t getInt(toml::table ParameterTable, std::string ParameterName) {
  if (ParameterTable[ParameterName.c_str()].is_integer())
    return *ParameterTable[ParameterName.c_str()].value<Int_t>();
  else {
    std::cerr << "Float_t parameter \"" << ParameterName
              << "\" does not exist in following parameter table:" << std::endl
              << ParameterTable << std::endl
              << std::endl;
    exit(1);
  }
}

Float_t getFloat(toml::table ParameterTable, std::string ParameterName) {
  if (ParameterTable[ParameterName.c_str()].is_number())
    return *ParameterTable[ParameterName.c_str()].value<Float_t>();
  else {
    std::cerr << "Float_t parameter \"" << ParameterName
              << "\" does not exist in following parameter table:" << std::endl
              << ParameterTable << std::endl
              << std::endl;
    exit(1);
  }
}

Double_t getDouble(toml::table ParameterTable, std::string ParameterName) {
  if (ParameterTable[ParameterName.c_str()].is_number())
    return *ParameterTable[ParameterName.c_str()].value<Double_t>();
  else {
    std::cerr << "Double_t parameter \"" << ParameterName
              << "\" does not exist in following parameter table:" << std::endl
              << ParameterTable << std::endl
              << std::endl;
    exit(1);
  }
}

Bool_t getBool(toml::table ParameterTable, std::string ParameterName) {
  if (ParameterTable[ParameterName.c_str()].is_boolean())
    return *ParameterTable[ParameterName.c_str()].value<Bool_t>();
  else {
    std::cerr << "Double_t parameter \"" << ParameterName
              << "\" does not exist in following parameter table:" << std::endl
              << ParameterTable << std::endl
              << std::endl;
    exit(1);
  }
}

} // namespace toml
class PMA {
private:
  // class variables
  toml::table cfg;
  fs::path cfgFileName;
  Bool_t recreate = false;
  std::vector<std::string> recreated_files;
  std::map<std::string, std::pair<Int_t, std::function<void()>>> moduleMap;

  // Check if root file was already recreated in actual session
  Bool_t is_recreated(std::string fileName) {

    return std::find(recreated_files.begin(), recreated_files.end(),
                     fileName) != recreated_files.end();
  }

  /*
  get TDirectory opened in read mode by parameter names for TFile and
  TDirectory from parameter table from config file
  */
  TDirectory *getDir(toml::table ParTable, std::string FileParName,
                     std::string DirParName) {

    fs::path RootFilePath = cfgFileName.parent_path();
    // check if ParName exists in cfg and set file path relative to the cfg file
    if (ParTable[FileParName.c_str()].is_string())
      RootFilePath = RootFilePath.append(
          *ParTable[FileParName.c_str()].value<std::string>());
    else {
      std::cerr << "Parameter " << FileParName
                << " does not exist in config file" << std::endl;
    }

    std::string RootDirName;
    if (ParTable[DirParName.c_str()].is_string())
      RootDirName = *ParTable[DirParName.c_str()].value<std::string>();
    else {
      std::cerr << "Parameter " << FileParName
                << " does not exist in config file" << std::endl;
    }

    // session
    TFile *RootFile = getRootFile(RootFilePath, false);
    TDirectory *RootDir = getRootDir(RootFile, RootDirName, false);
    return RootDir;
  }

  TDirectory *getDir(std::string MeasurementName, toml::table ParTable,
                     Bool_t write = 0) {
    fs::path filePath = cfgFileName.parent_path();
    std::string fileName;
    // set i/o fileName
    if (ParTable[write ? "output_file" : "input_file"].is_string())
      fileName =
          *ParTable[write ? "output_file" : "input_file"].value<std::string>();
    else
      std::cerr << "No " << (write ? "output_file" : "input_file")
                << " specified in config." << std::endl;

    filePath = filePath.append(fileName);

    // if recreate and write is true, check if file was already recreated in
    // current session
    if (recreate && write && !is_recreated(fileName)) {
      recreateRootFile(filePath);
      recreated_files.push_back(fileName);
    }
    TFile *rootFile = getRootFile(filePath, write);
    TDirectory *rootDir = getRootDir(rootFile, MeasurementName, write);
    return rootDir;
  }

  // ##### MODULES #####
  void runOscLecConverter() {
    TDirectory *OutputRootDir;
    toml::table MeasurementNames = *cfg["osclec_converter"].as_table();

    for (const auto &element : MeasurementNames) {
      // parse parameters
      toml::table ParameterTable = *element.second.as_table();
      std::string MeasurementName = element.first.data();
      Bool_t invert_polarity = toml::getBool(ParameterTable, "invert_polarity");
      Float_t threshold = toml::getFloat(ParameterTable, "threshold");
      fs::path rawdata_directory =
          toml::getString(ParameterTable, "rawdata_directory");

      // attach output TDir
      OutputRootDir = getDir(MeasurementName, ParameterTable, true);

      // run parser
      osclec::parseFiles(
          OutputRootDir,
          getFilesInDir(rawdata_directory, cfgFileName.parent_path()),
          threshold, invert_polarity);

      std::cout << "Data were seccessfully writen to\nTFile: \""
                << OutputRootDir->GetFile()->GetName() << "\" \nTDirectory: \""
                << MeasurementName << "\"\n\n";
      // close TFile
      OutputRootDir->GetFile()->Close();
    }
  }

  void runINFNConverter() {
    TDirectory *OutputRootDir;
    toml::table MeasurementNames = *cfg["infn_converter"].as_table();

    for (const auto &element : MeasurementNames) {
      // parse parameters
      toml::table ParTable = *element.second.as_table();
      std::string MeasurementName = element.first.data();
      fs::path InputCsvFileName = toml::getString(ParTable, "csvdata_file");
      fs::path InputCsvFilePath = cfgFileName.parent_path();
      InputCsvFilePath.append(InputCsvFileName.c_str());
      // attach output TDir
      OutputRootDir = getDir(MeasurementName, ParTable, true);

      // run parser
      infn::parseFile(InputCsvFilePath, OutputRootDir);

      // std::cout << "Data were seccessfully writen to\nTFile: \""
      //           << OutputRootDir->GetFile()->GetName() << "\" \nTDirectory:
      //           \""
      //           << MeasurementName << "\"\n\n";
      // close TFile
      OutputRootDir->GetFile()->Close();
    }
  }

  void runPulseCFDAnalysis() {
    TDirectory *InputRootDir;
    TDirectory *OutputRootDir;
    toml::table MeasurementNames = *cfg["pulse_cfd_analysis"].as_table();

    for (const auto &element : MeasurementNames) {
      // parse parameters
      std::string MeasurementName = element.first.data();
      toml::table ParameterTable = *element.second.as_table();
      Double_t cut_fraction = toml::getDouble(ParameterTable, "cut_fraction");
      Double_t threshold = toml::getDouble(ParameterTable, "threshold");

      // attach i/o TDirectories
      InputRootDir = getDir(MeasurementName, ParameterTable, false);
      OutputRootDir = getDir(MeasurementName, ParameterTable, true);

      doCFDPulseAnalysis(InputRootDir, OutputRootDir, threshold, cut_fraction,
                         false);
      InputRootDir->GetFile()->Close();
      OutputRootDir->GetFile()->Close();
    }
  }

  void runPulseTWAnalysis() {
    TDirectory *InputRootDir;
    TDirectory *OutputRootDir;
    toml::table MeasurementNames = *cfg["pulse_tw_analysis"].as_table();

    for (const auto &element : MeasurementNames) {
      // parse parameters
      std::string MeasurementName = element.first.data();
      toml::table ParameterTable = *element.second.as_table();
      Double_t t0_ns = toml::getDouble(ParameterTable, "t0_ns");
      Double_t t1_ns = toml::getDouble(ParameterTable, "t1_ns");
      Bool_t FitPedestal = toml::getBool(ParameterTable, "fit_pedestal");
      Float_t Threshold = toml::getFloat(ParameterTable, "threshold");

      // attach i/o TDirectories
      InputRootDir = getDir(MeasurementName, ParameterTable, false);
      OutputRootDir = getDir(MeasurementName, ParameterTable, true);

      doTWPulseAnalysis(InputRootDir, OutputRootDir, Threshold, t0_ns * 1e-9,
                        t1_ns * 1e-9, false, FitPedestal);
      InputRootDir->GetFile()->Close();
      OutputRootDir->GetFile()->Close();
    }
  }

  void getHistogram() {
    TDirectory *InputRootDir;
    TDirectory *OutputRootDir;
    toml::table MeasurementNames = *cfg["get_hist"].as_table();

    for (const auto &element : MeasurementNames) {
      // parse parameters
      std::string MeasurementName = element.first.data();
      toml::table ParameterTable = *element.second.as_table();
      std::string BranchName = toml::getString(ParameterTable, "branch_name");
      Int_t Bins = toml::getInt(ParameterTable, "bins");
      Double_t XLow = toml::getDouble(ParameterTable, "x_low");
      Double_t XHigh = toml::getDouble(ParameterTable, "x_high");

      // attach i/o TDirectories
      InputRootDir = getDir(MeasurementName, ParameterTable, false);
      OutputRootDir = getDir(MeasurementName, ParameterTable, true);

      makeTH1F(InputRootDir, OutputRootDir, BranchName, Bins, XLow, XHigh);

      InputRootDir->GetFile()->Close();
      OutputRootDir->GetFile()->Close();
    }
  }

  void runSinglephotonFit() {
    TDirectory *InputRootDir;
    TDirectory *OutputRootDir;
    toml::table MeasurementNames = *cfg["spe_fit"].as_table();

    for (const auto &element : MeasurementNames) {
      // parse parameters
      std::string MeasurementName = element.first.data();
      toml::table ParameterTable = *element.second.as_table();
      std::string HistName = toml::getString(ParameterTable, "hist_name");

      Int_t nSmooth = toml::getInt(ParameterTable, "n_smooth");

      // attach i/o TDirectories
      InputRootDir = getDir(MeasurementName, ParameterTable, false);
      OutputRootDir = getDir(MeasurementName, ParameterTable, true);

      FitSPE(InputRootDir, OutputRootDir, HistName, nSmooth);
      InputRootDir->GetFile()->Close();
      OutputRootDir->GetFile()->Close();
    }
  }

  void runGaussFit() {
    TDirectory *InputRootDir;
    TDirectory *OutputRootDir;

    toml::table MeasurementNames = *cfg["gauss_fit"].as_table();

    for (const auto &element : MeasurementNames) {
      // parse parameters
      std::string MeasurementName = element.first.data();
      toml::table ParameterTable = *element.second.as_table();
      // attach i/o TDirectories
      InputRootDir = getDir(MeasurementName, ParameterTable, false);
      OutputRootDir = getDir(MeasurementName, ParameterTable, true);
      std::string HistName = toml::getString(ParameterTable, "hist_name");

      FitGauss(InputRootDir, OutputRootDir, HistName);
      InputRootDir->GetFile()->Close();
      OutputRootDir->GetFile()->Close();
    }
  }

  void runGaussFromRightFit() {
    TDirectory *InputRootDir;
    TDirectory *OutputRootDir;

    toml::table MeasurementNames = *cfg["gauss_from_right_fit"].as_table();

    for (const auto &element : MeasurementNames) {
      // parse parameters
      std::string MeasurementName = element.first.data();
      toml::table ParameterTable = *element.second.as_table();

      // attach i/o TDirectories
      InputRootDir = getDir(MeasurementName, ParameterTable, false);
      OutputRootDir = getDir(MeasurementName, ParameterTable, true);
      Int_t nSmooth = toml::getInt(ParameterTable, "n_smooth");
      std::string HistName = toml::getString(ParameterTable, "hist_name");

      FitRightGauss(InputRootDir, OutputRootDir, HistName, nSmooth);
      InputRootDir->GetFile()->Close();
      OutputRootDir->GetFile()->Close();
    }
  }

  void runTTSFit() {
    TDirectory *InputRootDir;
    TDirectory *OutputRootDir;

    toml::table MeasurementNames = *cfg["tts_fit"].as_table();

    for (const auto &element : MeasurementNames) {
      // parse parameters
      std::string MeasurementName = element.first.data();
      toml::table ParameterTable = *element.second.as_table();
      // attach i/o TDirectories
      InputRootDir = getDir(MeasurementName, ParameterTable, false);
      OutputRootDir = getDir(MeasurementName, ParameterTable, true);
      std::string HistName = toml::getString(ParameterTable, "hist_name");

      FitExGauss(InputRootDir, OutputRootDir, HistName);
      InputRootDir->GetFile()->Close();
      OutputRootDir->GetFile()->Close();
    }
  }

  void getDarkRate() {
    toml::table MeasurementNames = *cfg["darkrate"].as_table();
    TDirectory *spRootDir;
    TDirectory *drRootDir;
    TDirectory *OutputRootDir;
    for (const auto &element : MeasurementNames) {
      std::string MeasurementName = element.first.data();
      // parse parameters
      toml::table ParameterTable = *element.second.as_table();
      std::string BranchName = toml::getString(ParameterTable, "branch_name");
      Double_t Threshold = toml::getDouble(ParameterTable, "threshold");
      Bool_t UseSPEThreshold =
          toml::getBool(ParameterTable, "use_spe_threshold");

      drRootDir = getDir(MeasurementName, ParameterTable, false);
      spRootDir = getDir(ParameterTable, "sp_file_path", "sp_measurement_name");
      OutputRootDir = getDir(MeasurementName, ParameterTable, true);

      GetDarkRate(drRootDir, spRootDir, OutputRootDir, BranchName, Threshold,
                  UseSPEThreshold);
      drRootDir->GetFile()->Close();
      spRootDir->GetFile()->Close();
      OutputRootDir->GetFile()->Close();
    }
  }

  void runMultiPulseAnalysis() {
    TDirectory *InputRootDir;
    TDirectory *OutputRootDir;
    toml::table MeasurementNames = *cfg["multipulse_analysis"].as_table();

    for (const auto &element : MeasurementNames) {
      std::string MeasurementName = element.first.data();
      // parse parameters
      toml::table ParameterTable = *element.second.as_table();
      Float_t threshold = toml::getFloat(ParameterTable, "threshold");
      Double_t pulse_length_threshold_ns =
          toml::getDouble(ParameterTable, "pulse_length_threshold_ns");

      InputRootDir = getDir(MeasurementName, ParameterTable, false);
      OutputRootDir = getDir(MeasurementName, ParameterTable, true);

      doMultiPulseAnalysis(InputRootDir, OutputRootDir, threshold,
                           pulse_length_threshold_ns);
      InputRootDir->GetFile()->Close();
      OutputRootDir->GetFile()->Close();
    }
  }

  void runAfterPulseAnalysis() {
    TDirectory *InputRootDir;
    TDirectory *OutputRootDir;
    toml::table MeasurementNames = *cfg["afterpulse_analysis"].as_table();

    for (const auto &element : MeasurementNames) {
      std::string MeasurementName = element.first.data();
      // parse parameters
      toml::table ParameterTable = *element.second.as_table();
      Double_t TimeWindowEnd_us =
          toml::getDouble(ParameterTable, "time_window_end_us");

      InputRootDir = getDir(MeasurementName, ParameterTable, false);
      OutputRootDir = getDir(MeasurementName, ParameterTable, true);

      doAfterPulseAnalysis(InputRootDir, OutputRootDir, TimeWindowEnd_us);
      InputRootDir->GetFile()->Close();
      OutputRootDir->GetFile()->Close();
    }
  }

  void initModuleMap() {
    // define map of available modules
    moduleMap = {
        {"osclec_converter", {0, [this]() { this->runOscLecConverter(); }}},
        {"infn_converter", {1, [this]() { this->runINFNConverter(); }}},
        {"pulse_cfd_analysis", {1, [this]() { this->runPulseCFDAnalysis(); }}},
        {"pulse_tw_analysis", {1, [this]() { this->runPulseTWAnalysis(); }}},
        {"multipulse_analysis",
         {1, [this]() { this->runMultiPulseAnalysis(); }}},
        {"get_hist", {2, [this]() { this->getHistogram(); }}},
        {"afterpulse_analysis",
         {2, [this]() { this->runAfterPulseAnalysis(); }}},
        {"spe_fit", {3, [this]() { this->runSinglephotonFit(); }}},
        {"gauss_fit", {4, [this]() { this->runGaussFit(); }}},
        {"tts_fit", {4, [this]() { this->runTTSFit(); }}},
        {"gauss_from_right_fit",
         {4, [this]() { this->runGaussFromRightFit(); }}},
        {"darkrate", {4, [this]() { this->getDarkRate(); }}},
        // pattern: {"module_name", {priority, [this]() {
        // this->moduleFunction(); }}}, priority defines the order of module
        // execution
    };
  }

  void callModule(std::string module_name) {
    checkModule(module_name);
    std::cout << "##################################\nCallign module: \""
              << module_name << "\"\n##################################\n";
    moduleMap[module_name].second();
  }

  void checkModule(std::string module_name) {

    if (moduleMap.find(module_name) != moduleMap.end())
      return;
    else {
      std::cout << "Module not found: " << module_name << std::endl;
      exit(1);
    }
  }

  std::vector<std::string> getModuleVec(toml::table module_table) {
    std::string module_name;
    std::vector<std::string> modules;
    for (const auto &module : module_table) {

      module_name = module.first.data();
      checkModule(module_name);
      modules.push_back(module.first.data());
    }
    return modules;
  };

  /*
  run modules ordered by priority
  */
  void runModules() {

    // get modules in cfg
    toml::table module_table = *cfg.as_table();
    std::vector<std::string> modules = getModuleVec(module_table);
    std::string module_name;

    Int_t len_modules = module_table.size();
    Int_t i = 0;
    Int_t priority = 0;
    Int_t module_priority;
    // loop over selected modules until all being thrown out
    while (not modules.empty()) {
      module_name = modules.at(i);
      module_priority = moduleMap.at(module_name).first;

      // call modules with matching priority
      if (moduleMap.at(module_name).first == priority) {
        modules.erase(std::next(modules.begin(), i));
        callModule(module_name);
      } else
        i++;

      // when iterated over all modules, raise priority and iterate again
      if (i >= (modules.size())) {
        ++priority;
        i = 0;
      }
    }
  };

public:
  PMA(const fs::path _cfgFileName) {
    cfgFileName = _cfgFileName;

    initModuleMap();

    // parse .toml cfg file
    std::ifstream inputFile(cfgFileName.c_str());
    cfg = toml::parse(inputFile);
  }

  void run(Bool_t _recreate = false) {
    recreate = _recreate;
    // if (recreate || not fs::exists(rootFileName)) {
    //   recreateRootFile(rootFileName);
    //   recreated_files.push_back(rootFileName);
    // }
    // rootFile = TFile::Open(rootFileName.c_str(), "update");

    runModules();

    // rootFile->Close();
  }
};

int main(int argc, char *argv[]) {
  // parse command line arguments
  ParsedArgs args = parseArgs(argc, argv);

  // run analysis
  PMA pma(args.cfgFileName);
  pma.run(args.recreate);
  return 0;
}