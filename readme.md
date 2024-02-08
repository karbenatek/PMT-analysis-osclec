# PMTAF

Are you excited into **PMT**s **AF**? Then **P**hoto **M**ultiplyer **T**ube **A**nalysis **F**ramework (**PMTAF**) migh be the right tool for you!

## Description

This project has aim to cover as many analyses as possible, relevant for PMT characterization and calibration. It utilizes a config file that defines what a how it should be done. As the project is not too complex it should encourage **YOU** to contribute with **your own code!**


Project also contains bunch of python scripts related to a custom hardware and plenty of undocumented analysis.

The project is primarily developed to serve for PMT testing, characterization and calibration at [Joint Laboratory Optics](https://jointlab.upol.cz/jlo/en/content/joint-laboratory-optics) in Olomouc, Czechia. Secondarily, it is developed for PMT validation and calibration for HyperK project where JLO contributes.

##### Credits:

Palacky University Olomouc & JLO 

Antonín Lindner - antoninlindner@gmail.com

## Table of Contents

- [Installation](#installation)
- [Usage](#usage)
- [Configuration file](#configuration-file)
- [Data formats](#data-formats)
- [Modules](#modules)
- [Module implementation](#module-implemantation)

## Installation
0. Make sure you have following dependencies installed:
    - [ROOT](https://root.cern/install/) 6.28/x (older version might work but weren't tested)
    - [conan 2.x](https://docs.conan.io/2/)
1. Clone the repository: `git clone https://github.com/karbenatek/PMTAF.git`
2. Navigate to the project directory: `cd PMTAF`
3. Build framework executable: `make install` 
(When running first time, you might be asked by `conan` to detect profile. Run `conan profile detect` and repeat `make install`. If running on Ubuntu 22.x based linux distribution please pay attention on [following issue](https://github.com/conan-io/conan/issues/4322))

## Usage
A binary executable is compiled as `bin/pmta` which can be added to system path by running `. setup.sh`.

By running `pmta`, the usage is printed:
```bash
Usage: pmta [-c cfg_file] [-r]
  Executes PhotoMultiplier analysis.

Options:
  -c cfg_file    Specify the configuration file to use
  -r            Recreate all output root files
```

The missing part now is corectly writen configuration file `cfg_file` and input files, defined inside.  

#### Note:
When option `-r` is used, every `output_file` is recreated only once and any of `input_file`'s won't be recreated at all to not break analysis dependencies.

## Configuration file
The config files are using [TOML](https://toml.io/en/) format. Each operation of PMTAF requires following elements:
1. **Module name**  
This defines what operation a.k.a module will be called. See the [list of existing modules](#modules) or [how to implement your own module](#module-implementation).

2. **Measurement name**  
This value can be custom, but is intended to briefly describe nature of the data/measurement. Measurement name matches a name of root TDirectory.

3. **Module parameters**  
These are specific for called module. Describes module execution parameters and i/o files/directories.

Operation defined in config file has following format:  
```
[module1.data1]
parameter0 = 123
parameter1 = 456
input_file = "fileX.root"
output_file = "fileY.root"

[module2.data1]
input_file = "fileY.root"
output_file = "fileY.root"
```

Notes:
* Order of operations in codfig file does not matter. Their actual order is defined by module priorities harcoded in framework.
* `input_file/output_file` are commonly used parameters. Refering of both to same file is OK.

## Data formats
PMTAF uses [cern-root native file format](https://root.cern/manual/root_files/) with commonly used appendix `.root`. Storeable object insides a root files can be [TTree](https://root.cern.ch/doc/master/classTTree.html) which contains events - columnar data entries with custom structure - [TBranches](https://root.cern.ch/doc/master/classTBranch.html).

PMTAF utilizes two kinds of TTrees:
### 1. `pmtaf_pulses`
Is used to store most raw data about pulses, i.e. actual waveforms.

Branches:
* `date` - date of acquisition, Format: `yyyy-mm-dd`. Type: `char[10]`.
* `time` - usually system time (unprecise). Format: `hh:mm:ss`. Type: `char[8]`.
* `time_10fs` - precise, relative time of acquisition. Type: `uint64_t` (max value corresponds to ~ 2 days).
* `i_seg` - index of segment, usually just indexing. Type: `Int_t`.
* `x` - signal amplitude values. Type: `std::vector<Float_t>`.
* `t` - signal time values, coresponding to `x`. Type: `std::vector<Double_t>`.

### 2. `pmtaf_tree`
Is used to store processed data from `pmtaf_pulses`, i.e. pulse max amplitude, pulse integrated charge.

Branches:
* `date`*
* `time`*
* `time_10fs` - might differ from the one in `pmtaf_pulses` - e.g. can be extracted from `t` as a trigger time for transit time spread measurement.
* `time_over_trigger_10fs` - pulse length. Type: `Double_t`.
* `i_seg`* - here it can serve for multipulse analysis, e.g. afterpulse analysis when multiple pulses can be found in same vaweform.
* `amplitude` - pulse max amplitude. Type: `Float_t`.
* `energy` - summ of pulse signal within some time window. Type: `Float_t`.

\* Branch is similar or identical to the same named one in `pmtaf_pulses`

Notes:  
Some branche names has suffix (e.g. `time_ns` => `ns`) which defines physical units. If physical quantity has not such suffix, than is is format dependat (e.g. `date` and `time`), integer (e.g. `i_seg`) or uses default SI units (e.g. `amplitude`, `energy` - Volts)

## Modules





# WORK IN PROGRESS

