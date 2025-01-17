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
#include "IdentityProgress.h"

#include <cstdint>
#include <algorithm>
#include <string>

const char* IdentityProgress::NICKNAME_STR = "nickname";
const char* IdentityProgress::IDENTITY_STR = "identity";
const char* IdentityProgress::CURRENTCOUNTER = "currentcounter";
const char* IdentityProgress::BESTCOUNTER = "bestcounter";

IdentityProgress::IdentityProgress()
{}

IdentityProgress::IdentityProgress(std::string nickname,
  std::string identity,
  uint64_t currentcounter,
  uint64_t bestcounter) :
  nickname(std::move(nickname)), identity(std::move(identity)),
  currentcounter(currentcounter), bestcounter(bestcounter)
{}

std::string IdentityProgress::toIniString() const {
  using namespace std;
  ostringstream out;
  out << "[" << string(IDENTITY_STR) << "]" << endl;
  out << string(NICKNAME_STR) << "=" << nickname << endl;
  out << string(IDENTITY_STR) << "=" << identity << endl;
  out << string(CURRENTCOUNTER) << "=" << currentcounter << endl;
  out << string(BESTCOUNTER) << "=" << bestcounter << endl;
  return out.str();
}

IdentityProgress IdentityProgress::parse(const std::string& segment) {
  std::string nickname;
  std::string identity;
  uint64_t currentcounter;
  uint64_t bestcounter;

  bool nickname_set = false;
  bool identity_set = false;
  bool currentcounter_set = false;
  bool bestcounter_set = false;

  try {
    std::string entry;
    std::string segment_spacefree = segment;
    std::replace(segment_spacefree.begin(), segment_spacefree.end(), ' ', '_');
    std::istringstream iss(segment_spacefree);
    while (iss >> entry) {
      using namespace std;
      auto prefix_nickname(string(NICKNAME_STR) + "=");
      auto prefix_identity(string(IDENTITY_STR) + "=");
      auto prefix_currentcounter(string(CURRENTCOUNTER) + "=");
      auto prefix_bestcounter(string(BESTCOUNTER) + "=");

      if (entry.compare(0, prefix_nickname.size(), prefix_nickname) == 0) {
        nickname = entry.substr(prefix_nickname.size());
        nickname_set = nickname.size() > 0;
      }
      else if (entry.compare(0, prefix_identity.size(), prefix_identity) == 0) {
        identity = entry.substr(prefix_identity.size());
        identity_set = identity.size() > 0;
      }
      else if (entry.compare(0, prefix_currentcounter.size(), prefix_currentcounter) == 0) {
        currentcounter = stoull(entry.substr(prefix_currentcounter.size()));
        currentcounter_set = true;
      }
      else if (entry.compare(0, prefix_bestcounter.size(), prefix_bestcounter) == 0) {
        bestcounter = stoull(entry.substr(prefix_bestcounter.size()));
        bestcounter_set = true;
      }
      else {
        // we are evaluating this in a strict manner
        // disallowing any unknown entry names
        throw std::invalid_argument("Identity section is invalid.");
      }
    }
  }
  catch (const std::exception& e) {
    throw e;
  }

  if (!identity_set || !currentcounter_set) {
    throw std::invalid_argument("Identity section is invalid.");
  }


  if (!nickname_set || nickname.empty()) {
    nickname = "(no_nickname)";
  }

  if (!bestcounter_set) {
    bestcounter = currentcounter;
  }

  return IdentityProgress(nickname, identity, currentcounter, bestcounter);
}
