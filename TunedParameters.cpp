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
#include "TunedParameters.h"

#include <string>

const char* TunedParameters::DEVICENAME_STR = "devicename";
const char* TunedParameters::DEVICEIDENTIFIER_STR = "deviceidentifier";
const char* TunedParameters::TUNEDPARAMETER_STR = "tunedparameter";
const char* TunedParameters::LOCALWSIZE_STR = "localworksize";
const char* TunedParameters::GLOBALWSIZE_STR = "globalworksize";

TunedParameters::TunedParameters() {}

TunedParameters::TunedParameters(std::string devicename,
  std::string deviceidentifier,
  uint64_t localworksize,
  uint64_t globalworksize) :
  devicename(std::move(devicename)),
  deviceidentifier(std::move(deviceidentifier)),
  localworksize(localworksize), globalworksize(globalworksize)
{}

std::string TunedParameters::toIniString() const {
  using namespace std;
  ostringstream out;
  out << "[" << string(TUNEDPARAMETER_STR) << "]" << endl;
  out << string(DEVICENAME_STR) << "=" << devicename << endl;
  out << string(DEVICEIDENTIFIER_STR) << "=" << deviceidentifier << endl;
  out << string(LOCALWSIZE_STR) << "=" << localworksize << endl;
  out << string(GLOBALWSIZE_STR) << "=" << globalworksize << endl;
  return out.str();
}

TunedParameters TunedParameters::parse(const std::string& segment) {
  std::string devicename;
  std::string deviceidentifier;
  uint64_t localworksize;
  uint64_t globalworksize;

  bool devicename_set = false;
  bool deviceidentifier_set = false;
  bool localworksize_set = false;
  bool globalworksize_set = false;

  try {
    std::string entry;
    std::istringstream iss(segment);
    while (iss >> entry) {
      using namespace std;
      auto prefix_devicename(string(DEVICENAME_STR) + "=");
      auto prefix_deviceidentifier(string(DEVICEIDENTIFIER_STR) + "=");
      auto prefix_localworksize(string(LOCALWSIZE_STR) + "=");
      auto prefix_globalworksize(string(GLOBALWSIZE_STR) + "=");

      if (entry.compare(0,
        prefix_devicename.size(),
        prefix_devicename)
        == 0) {
        devicename = entry.substr(prefix_devicename.size());
        devicename_set = devicename.size() > 0;
      }
      else if (entry.compare(0,
        prefix_deviceidentifier.size(),
        prefix_deviceidentifier)
        == 0) {
        deviceidentifier = entry.substr(prefix_deviceidentifier.size());
        deviceidentifier_set = deviceidentifier.size() > 0;
      }
      else if (entry.compare(0,
        prefix_localworksize.size(),
        prefix_localworksize)
        == 0) {
        localworksize = stoull(entry.substr(prefix_localworksize.size()));
        localworksize_set = true;
      }
      else if (entry.compare(0,
        prefix_globalworksize.size(),
        prefix_globalworksize)
        == 0) {
        globalworksize = stoull(entry.substr(prefix_globalworksize.size()));
        globalworksize_set = true;
      }
      else {
        // we are evaluating this in a strict manner
        // disallowing any unknown entry names
        throw std::invalid_argument("Tuned parameter section is invalid.");
      }
    }
  }
  catch (const std::exception& e) {
    throw e;
  }

  if (!devicename_set || !deviceidentifier_set ||
    !localworksize_set || !globalworksize_set) {
    throw std::invalid_argument("Tuned parameter section is invalid.");
  }

  return TunedParameters(devicename,
    deviceidentifier,
    localworksize,
    globalworksize);
}
