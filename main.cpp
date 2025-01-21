/*
Copyright (c) 2017 landave

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <thread>

#include "Config.h"
#include "TSHasherContext.h"

// we need a global pointer to the TSHasherContext for the consoleHandler
TSHasherContext* hasherctxptr = nullptr;

#if defined(_WIN32) || defined(_WIN64)
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#include <Windows.h>
BOOL WINAPI consoleHandler(DWORD signal) {
  if (signal == CTRL_C_EVENT)
    hasherctxptr->timerkiller.kill();

  return TRUE;
}

#else
#include <signal.h>
void consoleHandler(int signal) {
  if (signal == SIGINT) {
    hasherctxptr->timerkiller.kill();
  }
}
#endif


const char* inputarguments_add = "add [-publickey PUBLICKEY] [-startcounter STARTCOUNTER] [-nickname NICKNAME]";

const char* inputarguments_compute = "compute  [-throttle throttlefactor]  [-retune]";

const char* inputarguments_help = "help";

enum StringCode {
  eADD,
  eCOMPUTE,
  ePUBLICKEY,
  eSTARTCOUNTER,
  eNICKNAME,
  eTHROTTLE,
  eRETUNE,
  eHELP,
  eERR
};

StringCode getStringCode(const std::string& str) {
  if (str == "add") { return eADD; }
  if (str == "compute") { return eCOMPUTE; }

  if (str == "-publickey") { return ePUBLICKEY; }
  if (str == "-startcounter") { return eSTARTCOUNTER; }
  if (str == "-nickname") { return eNICKNAME; }

  if (str == "-throttle") { return eTHROTTLE; }
  if (str == "-retune") { return eRETUNE; }
  if (str == "-h" ||
    str == "--h" ||
    str == "-help" ||
    str == "--help" ||
    str == "h" || str == "help")
  {
    return eHELP;
  }
  return eERR;
}

void handleAdd(int argc, char* argv[]) {
  bool configavailable = Config::load();
  std::string publickey;
  bool publickey_set = false;
  std::string nickname;
  uint64_t startcounter = 0;

  const std::string inputformat_add = "TeamspeakHasher " + std::string(inputarguments_add) + "\n";
  if (argc < 4) {
    std::cout << std::endl << "Error: Too few arguments. The input format is as follows." << std::endl << inputformat_add;
    exit(-1);
  }

  int i = 2;

  while (i < argc) {
    if (i + 1 >= argc) {
      std::cout << std::endl << "Error: Too few arguments. The input format is as follows." << std::endl << inputformat_add;
      exit(-1);
    }

    switch (getStringCode(argv[i])) {
    case ePUBLICKEY:
      publickey = std::string(argv[i + 1]);
      if (publickey.size() < 64 || publickey.size() > 108) {
        std::cout << "Error: Only public keys of length at least 64 and at most 108 are supported." << std::endl;
        exit(-1);
      }
      publickey_set = true;
      break;
    case eSTARTCOUNTER:
      try { startcounter = std::stoull(std::string(argv[i + 1])); }
      catch (std::exception&) {
        std::cout << "Error: Invalid start counter. The startcounter needs to be at least 0 and at most " << UINT64_MAX << "." << std::endl;
        exit(-1);
      }
      break;
    case eNICKNAME:
      nickname = std::string(argv[i + 1]);
      break;
    default:
      std::cout << std::endl << "Error: Invalid arguments. The input format is as follows." << std::endl << inputformat_add;
      exit(-1);
      break;
    }

    i += 2;
  }

  if (!publickey_set) {
    std::cout << std::endl << "Error: Too few arguments. Please provide a public key." << std::endl;
    exit(-1);
  }

  auto currentconfig = Config::conf.find(publickey);
  if (configavailable && currentconfig != Config::conf.end()) {
    if (currentconfig->second.currentcounter != startcounter) {
      std::cout << "The given public key exists already in the progress file with startcounter=" << currentconfig->second.currentcounter << std::endl;
      std::cout << "Do you want to overwrite the saved startcounter with your given startcounter? (y/n)" << std::endl;
      std::string input;
      std::cin >> input;
      if (input == "y") {
        currentconfig->second.currentcounter = startcounter;
      }
    }
  }
  else {
    Config::conf[publickey] = IdentityProgress(nickname, publickey, startcounter, startcounter);
  }

  bool stored = Config::store();
  if (!stored) {
    std::cout << "Error: Public key could not be saved. Make sure the current directory is writable." << std::endl;
  }
  else {
    std::cout << "Public key has been added successfully." << std::endl;
  }
}

void handleCompute(int argc, char* argv[]) {
  bool configavailable = Config::load();
  const auto inputformat_compute = "TeamspeakHasher " + std::string(inputarguments_compute) + "\n";

  double throttlefactor = 1;

  if (!configavailable || Config::conf.empty()) {
    std::cout << "Error: Please add a public key first." << std::endl;
    exit(-1);
  }

  int i = 2;

  while (i < argc) {
    switch (getStringCode(std::string(argv[i]))) {
    case eTHROTTLE:
      if (i + 1 >= argc) {
        std::cout << std::endl << "Error: Too few arguments. The input format is as follows." << std::endl << inputformat_compute;
        exit(-1);
      }
      try {
          throttlefactor = std::stod(std::string(argv[i + 1]));
      }
      catch (std::exception&) {
          std::cout << "Error: Invalid throttle value. The throttle factor must be a number greater or equal than 1." << std::endl;
          exit(-1);
      }
      i += 2;
      break;
    case eRETUNE:
      Config::tuned.clear();
      i++;
      break;
    default:
      std::cout << std::endl << "Error: Invalid arguments. The input format is as follows." << std::endl << inputformat_compute;
      exit(-1);
      break;
    }
  }


  Config::printidentities();

  std::cout << "Please select an identity: ";
  size_t id;
  do {
    bool err = false;
    try { std::cin >> id; }
    catch (std::exception&) { err = true; }
    if (id >= Config::conf.size() || err) {
      std::cout << "Invalid choice. Please choose a valid identity." << std::endl;
    }
  } while (id >= Config::conf.size());

  auto selection = Config::conf.begin();
  std::advance(selection, id);

  std::string publickey = selection->second.identity;
  uint64_t startcounter = selection->second.currentcounter;
  uint64_t bestcounter = selection->second.bestcounter;


  std::cout << "Initializing OpenCL..." << std::endl;

  TSHasherContext hasherctx(publickey, startcounter, bestcounter, throttlefactor);

  hasherctxptr = &hasherctx;

  std::cout << std::endl << std::endl << "Initialization done, starting computation..." << std::endl << std::endl;

  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  // we save our progress every 5 minutes
  std::thread progress_saver([&selection, &hasherctx]() -> void {
    while (hasherctx.timerkiller.wait_for(std::chrono::minutes(5))) {
      selection->second.currentcounter = hasherctx.startcounter;
      selection->second.bestcounter = hasherctx.global_bestdifficulty_counter;
      Config::store();
    }
    });

  #if defined(_WIN32) || defined(_WIN64)
  if (!SetConsoleCtrlHandler(&consoleHandler, TRUE)) {
    std::cout << std::endl << "ERROR: Could not set console handler" << std::endl;
    return;
  }
  #else
  signal(SIGINT, &consoleHandler);
  #endif

  hasherctx.compute();

  progress_saver.join();

  selection->second.currentcounter = hasherctx.startcounter;
  selection->second.bestcounter = hasherctx.global_bestdifficulty_counter;

  bool stored = Config::store();
  if (stored) {
    std::cout << "The progress has been saved successfully." << std::endl;
  }
  else {
    std::cout << "Error: The progress could not be saved." << std::endl;
  }
}

void handleHelp(int argc, char* argv[]) {
  std::cout << "Usage: TeamspeakHasher COMMAND [OPTIONS]" << std::endl << std::endl;

  std::cout << "The following commands are supported:" << std::endl << std::endl;

  std::cout << std::string(inputarguments_add) << std::endl;
  std::cout << std::string(inputarguments_compute) << std::endl;
  std::cout << std::string(inputarguments_help) << std::endl;

  std::cout << std::endl;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    handleHelp(argc, argv);
    exit(-1);
  }

  switch (getStringCode(std::string(argv[1]))) {
  case eADD:
    handleAdd(argc, argv);
    break;
  case eCOMPUTE:
    handleCompute(argc, argv);
    break;
  case eHELP:
    handleHelp(argc, argv);
    break;
  default:
    std::cout << "Error: Invalid command." << std::endl;
    exit(-1);
    break;
  }

  return 0;
}
