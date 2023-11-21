#include "libanalis.h"
#include "libfit.h"
#include "libosclec.h"
#include "routines.cpp"

// #include <RtypesCore.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TH1.h>
#include <TUUID.h>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <getopt.h>
#include <iostream>
// #include <iterator>
#include <map>
#include <ostream>
#include <string>
#include <toml++/toml.h>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
struct ParsedArgs {
  fs::path rootFileName = "";
  fs::path cfgFileName = "";
  Bool_t recreate = false;
};

void prntUsage(char *argv[]) {
  std::cerr << "Usage: " << argv[0] << " <root_file> [-c cfg_file] [-r]"
            << std::endl
            << std::endl
            << "\tExecutes PhotoMultiplier analysis." << std::endl;
}

ParsedArgs parseArgs(int argc, char *argv[]) {
  ParsedArgs parsedArgs;

  // Get unnamed arguments
  for (int i = 1; i < argc; ++i) {
    if (((std::string)argv[i]) == "-r")
      continue;
    else if (argv[i][0] == '-')
      ++i;
    else
      parsedArgs.rootFileName = argv[i];
  }

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

  if (parsedArgs.cfgFileName == "") {
    prntUsage(argv);
    exit(1);
  }

  // some error handling
  if (not fs::exists(parsedArgs.rootFileName)) {
    std::cerr << "Input file " << parsedArgs.rootFileName << " does not exist"
              << std::endl;
    // exit(1);
  }

  if (not fs::exists(parsedArgs.cfgFileName)) {
    if (not parsedArgs.cfgFileName.empty()) {
      std::cerr << "Cfg file " << parsedArgs.cfgFileName << " not found"
                << std::endl;
    }
    // try to look into default cfg directory
    parsedArgs.cfgFileName = getDefaultCfgFile(parsedArgs.cfgFileName);
    if (not fs::exists(parsedArgs.cfgFileName)) {
      std::cerr << "Cfg file " << parsedArgs.cfgFileName << " not found"
                << std::endl;
      exit(1);
    }
  }

  std::cout << "Running analysis on file: " << parsedArgs.rootFileName
            << std::endl
            << "Used configuration file: " << parsedArgs.cfgFileName
            << std::endl;

  // std::cout << "Parsed args\n"
  //           << parsedArgs.rootFileName << std::endl
  //           << parsedArgs.cfgFileName << std::endl
  //           << parsedArgs.recreate << std::endl;

  return parsedArgs;
}

void recreateRootFile(fs::path filePath) {
  if (fs::exists(filePath))
    std::remove(filePath.c_str());
  TFile *rootFile = TFile::Open(filePath.c_str(), "create");
  rootFile->Close();
  std::cout << "File " << filePath << " re/created" << std::endl;
}
TDirectory *getRootDir(TFile *rootFile, std::string directoryName,
                       Bool_t create = 0) {
  /*
  get TDirectory or ,if not exists, create (create = true)/ throw error (create
  = false)
  */
  TDirectory *rootDir;
  rootDir = rootFile->GetDirectory(directoryName.c_str());

  if (not rootDir) {
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

TFile *getRootFile(fs::path rootFilePath, Bool_t write = 0) {
  /*
  open TFile as READ (write = 0)/ WRITE (write = 1) or ,if not exists,
  create(write = 1)/ throw error (write = 0)
  */
  TFile *rootFile;
  if (fs::exists(rootFilePath)) {
    rootFile = TFile::Open(rootFilePath.c_str(), write ? "update" : "read");
  } else {
    if (write) {
      rootFile = TFile::Open(rootFilePath.c_str(), "CREATE");
      std::cout << "File " << rootFilePath << " created" << std::endl;
    } else {
      std::cerr << "File " << rootFilePath << " does not exist" << std::endl;
      exit(1);
    }
  }
  return rootFile;

  // TH1F *h = new TH1F("h", "", 40, -1, 1);
  // h->FillRandom("gaus");
  // rootFile->WriteObject(h, "haha");
  // rootFile->Close();
}

class PMA {
private:
  // class variables
  toml::table cfg;
  fs::path rootFileName;
  fs::path cfgFileName;
  Bool_t recreate = false;
  std::vector<std::string> recreated_files;
  std::map<std::string, std::pair<Int_t, std::function<void()>>> moduleMap;

  // class methods (private)
  Bool_t is_recreated(std::string fileName) {
    return std::find(recreated_files.begin(), recreated_files.end(),
                     fileName) != recreated_files.end();
  }
  TDirectory *getDir(std::string MeasurementName, toml::table ParTable,
                     std::string FileParName) {
    fs::path filePath = cfgFileName.parent_path();
    if (ParTable[FileParName.c_str()].is_string())
      filePath =
          filePath.append(*ParTable[FileParName.c_str()].value<std::string>());
    else {
      std::cerr << "Parameter " << FileParName
                << " does not exist in config file" << std::endl;
    }

    // session
    TFile *rootFile = getRootFile(filePath, false);
    TDirectory *rootDir = getRootDir(rootFile, MeasurementName, false);
    return rootDir;
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
      fileName = rootFileName.c_str();

    filePath = filePath.append(fileName);

    // if recreate is true, check if file was already recreated in current
    // session
    if (recreate && not is_recreated(fileName)) {
      recreateRootFile(filePath);
      recreated_files.push_back(fileName);
    }
    TFile *rootFile = getRootFile(filePath, write);
    TDirectory *rootDir = getRootDir(rootFile, MeasurementName, write);
    return rootDir;
  }

  // MODULES
  void runOscLecConverter() {
    TDirectory *outputRootDir;
    toml::table MeasurementNames = *cfg["osclec_converter"].as_table();

    for (const auto &element : MeasurementNames) {
      // parse parameters
      toml::table ParTable = *element.second.as_table();
      std::string MeasurementName = element.first.data();
      Bool_t invert_polarity = *ParTable["invert_polarity"].value<Bool_t>();
      Double_t threshold = *ParTable["threshold"].value<Double_t>();
      fs::path rawdata_directory =
          *ParTable["rawdata_directory"].value<std::string>();

      // attach output TDir
      outputRootDir = getDir(MeasurementName, ParTable, true);

      // run parser
      osclec::parseFiles(
          outputRootDir,
          getFilesInDir(rawdata_directory, cfgFileName.parent_path()),
          threshold, invert_polarity);

      std::cout << "Data were seccessfully writen to\nTFile: \""
                << outputRootDir->GetFile()->GetName() << "\" \nTDirectory: \""
                << MeasurementName << "\"\n\n";
      // close TFile
      outputRootDir->GetFile()->Close();
    }
  }

  void runPulseAnalysis() {
    TDirectory *inputRootDir;
    TDirectory *outputRootDir;
    toml::table MeasurementNames = *cfg["pulse_analysis"].as_table();

    for (const auto &element : MeasurementNames) {
      // parse parameters
      std::string MeasurementName = element.first.data();
      toml::table ParTable = *element.second.as_table();
      Int_t bins_ampl = *ParTable["bins_ampl"].value<Int_t>();
      Int_t bins_Q = *ParTable["bins_Q"].value<Int_t>();
      Int_t bins_Qnoise = *ParTable["bins_Qnoise"].value<Int_t>();
      Double_t cut_fraction = *ParTable["cut_fraction"].value<Double_t>();
      Double_t threshold = *ParTable["threshold"].value<Double_t>();
      Double_t max_ampl = *ParTable["max_ampl"].value<Double_t>();

      // attach i/o TDirectories
      inputRootDir = getDir(MeasurementName, ParTable, false);
      outputRootDir = getDir(MeasurementName, ParTable, true);

      pmanalysis::doPulseAnalysis(inputRootDir, outputRootDir, bins_ampl,
                                  bins_Q, bins_Qnoise, threshold, max_ampl,
                                  cut_fraction);
      inputRootDir->GetFile()->Close();
      outputRootDir->GetFile()->Close();
    }
  }

  void runSinglephotonFit() {
    TDirectory *inputRootDir;
    TDirectory *outputRootDir;
    toml::table MeasurementNames = *cfg["singlephoton_fit"].as_table();

    for (const auto &element : MeasurementNames) {
      // parse parameters
      std::string MeasurementName = element.first.data();
      toml::table ParTable = *element.second.as_table();
      std::string histName = "h_Q";
      if (ParTable["hist_name"].is_string())
        histName = *ParTable["hist_name"].value<std::string>();
      Int_t nSmooth = 5;
      if (ParTable["n_smooth"].is_integer())
        nSmooth = *ParTable["n_smooth"].value<Int_t>();

      // attach i/o TDirectories
      inputRootDir = getDir(MeasurementName, ParTable, false);
      outputRootDir = getDir(MeasurementName, ParTable, true);

      FitSPE(inputRootDir, outputRootDir, histName, nSmooth);
      inputRootDir->GetFile()->Close();
      outputRootDir->GetFile()->Close();
    }
  }
  void runGaussFit() {
    TDirectory *inputRootDir;
    TDirectory *outputRootDir;

    toml::table MeasurementNames = *cfg["gauss_fit"].as_table();

    for (const auto &element : MeasurementNames) {
      // parse parameters
      std::string MeasurementName = element.first.data();
      toml::table ParTable = *element.second.as_table();
      // attach i/o TDirectories
      inputRootDir = getDir(MeasurementName, ParTable, false);
      outputRootDir = getDir(MeasurementName, ParTable, true);

      FitGauss(inputRootDir, outputRootDir);
      inputRootDir->GetFile()->Close();
      outputRootDir->GetFile()->Close();
    }
  }
  void runDarkrate() {
    toml::table MeasurementNames = *cfg["darkrate"].as_table();
    TDirectory *spRootDir;
    TDirectory *drRootDir;
    TDirectory *outputRootDir;
    for (const auto &element : MeasurementNames) {
      std::string MeasurementName = element.first.data();
      // parse parameters
      toml::table ParTable = *element.second.as_table();
      Double_t pe_threshold = *ParTable["pe_threshold"].value<Double_t>();
      std::string spMeasurementName =
          *ParTable["sp_measurement_name"].value<std::string>();
      drRootDir = getDir(MeasurementName, ParTable, false);
      outputRootDir = getDir(MeasurementName, ParTable, true);

      getRootDir(TFile * rootFile, std::string directoryName) rootDir =
          getRootDir(rootFile, MeasurementName);
      TDirectory *sprootDir = getRootDir(rootFile, spMeasurementName);
      pmanalysis::GetDarkRate(rootDir, sprootDir, pe_threshold);
      sprootDir->Clear();
      rootDir->Clear();
    }
  }

  void runAfterPulseAnalysis() {
    TDirectory *inputRootDir;
    TDirectory *outputRootDir;
    toml::table MeasurementNames = *cfg["afterpulse_analysis"].as_table();

    for (const auto &element : MeasurementNames) {
      std::string MeasurementName = element.first.data();
      // parse parameters
      toml::table ParTable = *element.second.as_table();
      Double_t threshold = *ParTable["threshold"].value<Double_t>();
      Float_t pulse_length_threshold =
          *ParTable["pulse_length_threshold"].value<Float_t>();

      inputRootDir = getDir(MeasurementName, ParTable, false);
      outputRootDir = getDir(MeasurementName, ParTable, true);

      pmanalysis::doAfterPulseAnalysis(inputRootDir, outputRootDir, threshold,
                                       pulse_length_threshold);
      inputRootDir->GetFile()->Close();
      outputRootDir->GetFile()->Close();
    }
  }

  void initModuleMap() {
    // define map of available modules
    moduleMap = {
        {"osclec_converter", {0, [this]() { this->runOscLecConverter(); }}},
        {"pulse_analysis", {1, [this]() { this->runPulseAnalysis(); }}},
        {"afterpulse_analysis",
         {1, [this]() { this->runAfterPulseAnalysis(); }}},
        {"singlephoton_fit", {2, [this]() { this->runSinglephotonFit(); }}},
        {"gauss_fit", {2, [this]() { this->runGaussFit(); }}},
        {"darkrate", {3, [this]() { this->runDarkrate(); }}},
        // pattern: {"module_name", {priority, [this]() {
        // this->moduleFunction(); }}}, priority defines the order of module
        // execution
    };
  }

  void callModule(std::string module_name) {
    checkModule(module_name);
    moduleMap[module_name].second();
  }

  Bool_t checkModule(std::string module_name) {

    if (moduleMap.find(module_name) != moduleMap.end())
      return 1;
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

  void runModules() {
    /*
    run modules ordered by priority
    */

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
  PMA(const fs::path _rootFileName, const fs::path _cfgFileName) {
    rootFileName = _rootFileName;
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
  PMA pma(args.rootFileName, args.cfgFileName);
  pma.run(args.recreate);
  return 0;
}