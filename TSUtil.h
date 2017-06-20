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
#ifndef TSUTIL_H_
#define TSUTIL_H_

#include "sha1.h"

#include <cstdint>
#include <string>
#include <vector>


class TSUtil {
 public:
  static bool isSlowPhase(size_t identity_length, uint64_t counter) {
    return identity_length + TSUtil::decimalLength(counter) + 1 + 8 > 128;
  }

  static uint64_t itsUntilSlowPhase(size_t identity_length, uint64_t counter) {
    if (isSlowPhase(identity_length, counter)) { return 0; }
    uint64_t requiredCounterLength = (128 - 8 - 1 - identity_length) + 1;
    return powlong(10, requiredCounterLength - 1) - counter;
  }

  static uint8_t decimalLength(uint64_t num) {
    if (num == 0) { return 1; }
    uint8_t len = 0;
    while (num != 0) {
      len++;
      num /= 10;
    }
    return len;
  }

  static uint64_t itsConstantCounterLength(uint64_t counter) {
    uint64_t counterlen = decimalLength(counter);
    return counterlen < 20 ? (powlong(10, counterlen) - counter) : UINT64_MAX;
  }

  static uint8_t getDifficulty(std::string publickey, uint64_t counter) {
    std::string hashinput = publickey + std::to_string(counter);

    SHA1 ctx;
    ctx.update(hashinput);
    std::vector<uint8_t> hash = ctx.final();

    uint8_t zerobytes = 0;
    while (zerobytes < 20 && hash[zerobytes] == 0) {
      zerobytes++;
    }
    uint8_t zerobits = 0;
    if (zerobytes < 20) {
      uint8_t lastbyte = hash[zerobytes];
      while ((lastbyte & 1) == 0) {
        zerobits++;
        lastbyte >>= 1;
      }
    }

    return static_cast<uint8_t>(8 * zerobytes + zerobits);
  }

 private:
  static uint64_t powlong(uint64_t base, uint64_t exp) {
    uint64_t result = 1;
    while (exp != 0) {
      if ((exp & 1) != 0) { result *= base; }
      exp >>= 1;
      base *= base;
    }
    return result;
  }
};

#endif
