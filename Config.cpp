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
#include "Config.h"

#include "Table.h"
#include "TSUtil.h"
#include "TunedParameters.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <utility>

std::map<std::string, IdentityProgress> Config::conf;
std::map<std::string, TunedParameters> Config::tuned;

const char* Config::FILENAME = "tshasher.ini";

void Config::printidentities() {
  Table table({ "[ID]",
                "Nickname",
                "Current Counter",
                "Best Counter",
                "Best Difficulty" },
              true);
  size_t i = 0;
  for (auto &c : Config::conf) {
    uint64_t bestdifficulty = TSUtil::getDifficulty(c.second.identity,
                                                    c.second.bestcounter);
    table.addRow({ std::to_string(i),
                   c.second.nickname,
                   std::to_string(c.second.currentcounter),
                   std::to_string(c.second.bestcounter),
                   std::to_string(bestdifficulty) });
    i++;
  }
  std::cout << table.getTable();
}


bool Config::load() {
  try {
    std::ifstream file(Config::FILENAME);

    std::string configstr((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    std::map<std::string, IdentityProgress> newconf;
    std::map<std::string, TunedParameters> newtuned;

    const std::regex regex(R"(\[[[:alpha:]]+\][^\[]+)");

    std::string::const_iterator searchStart(configstr.cbegin());
    std::smatch smatch;
    const std::string &prefix_idprogress =
                  "[" + std::string(IdentityProgress::IDENTITY_STR) + "]";
    const std::string &prefix_tunedparams =
                  "[" + std::string(TunedParameters::TUNEDPARAMETER_STR) + "]";

    while (std::regex_search(searchStart, configstr.cend(), smatch, regex)) {
      const std::string &segment = smatch[0];
      if (segment.compare(0,
                          prefix_idprogress.size(),
                          prefix_idprogress)
          == 0) {
        auto segment_payload = segment.substr(prefix_idprogress.size());
        IdentityProgress newident = IdentityProgress::parse(segment_payload);
        if (newconf.find(newident.identity) != newconf.end()) {
          return false;
        }
        newconf.insert(std::make_pair(newident.identity, newident));
      } else if (segment.compare(0,
                                 prefix_tunedparams.size(),
                                 prefix_tunedparams)
                 == 0) {
        auto segment_payload = segment.substr(prefix_tunedparams.size());
        auto newt = TunedParameters::parse(segment_payload);
        if (newtuned.find(newt.deviceidentifier) != newtuned.end()) {
          return false;
        }
        newtuned.insert(std::make_pair(newt.deviceidentifier, newt));
      } else {
        return false;
      }
      searchStart += smatch.position() + smatch.length();
    }

    Config::conf = newconf;
    Config::tuned = newtuned;
  }
  catch (std::exception&) {
    return false;
  }

  return true;
}

bool Config::store() {
  try {
    std::ofstream out(Config::FILENAME);

    for (auto const &identity : Config::conf) {
      out << identity.second.toIniString();
    }

    for (auto const &t : Config::tuned) {
      out << t.second.toIniString();
    }
  }
  catch (std::exception&) {
    return false;
  }

  return true;
}
