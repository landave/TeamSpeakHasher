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
#ifndef TSHASHERCONTEXT_H_
#define TSHASHERCONTEXT_H_

#include "DeviceContext.h"
#include "TimerKiller.h"
#include "TSUtil.h"

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#include <Windows.h>
#else
#include <signal.h>
#endif

#define DEV_TYPE    CL_DEVICE_TYPE_GPU
// #define DEV_TYPE      CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU
// #define DEV_TYPE      CL_DEVICE_TYPE_CPU

// forward declaration because of cyclic dependency
// between DeviceContext and TSHasherContext
class DeviceContext;

class TSHasherContext {
 public:
  TSHasherContext(std::string identity,
                  uint64_t startcounter,
                  uint64_t bestcounter,
                  uint64_t throttlefactor);

  void compute();
  void printinfo(const std::vector<DeviceContext> &dev_ctxs);
  static void launch_kernel_single(DeviceContext *dev_ctx);

  TimerKiller timerkiller;

  volatile uint64_t startcounter;
  volatile uint8_t global_bestdifficulty;
  volatile uint64_t global_bestdifficulty_counter;

 private:
  std::pair<uint64_t, uint64_t> tune(cl::Device *device,
                                     cl_uint device_id,
                                     const char* build_opts);

  std::string identity;

  uint64_t throttlefactor;
  std::mutex startcounter_mutex;
  std::vector<DeviceContext> dev_ctxs;
  std::vector<cl::Device> devices;
  std::chrono::time_point<std::chrono::high_resolution_clock> starttime;

  static void __stdcall kernel_result_available(cl_event event,
                                      cl_int event_command_exec_status,
                                      void* user_data);
  static void clearConsole();
  std::string getFormattedDouble(double x);
  std::string getFormattedDuration(double seconds);

  std::string getDeviceIdentifier(cl::Device *device, cl_uint device_id);

  static const char* KERNEL_CODE;
  static const char* KERNEL_NAME;
  static const char* KERNEL_NAME2;
  static const char* BUILD_OPTS_BASE;

  static const size_t MAX_DEVICES;
  static const size_t DEV_DEFAULT_LOCAL_WORK_SIZE;
  static const size_t DEV_DEFAULT_GLOBAL_WORK_SIZE;
  static const uint64_t MAX_GLOBALLOCAL_RATIO;

  static const size_t KERNEL_STD_ITERATIONS;

  uint8_t MIN_TARGET_DIFFICULTY;
};

#endif
