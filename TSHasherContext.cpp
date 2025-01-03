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
#include "TSHasherContext.h"

#include <cmath>
#include <cstdint>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <regex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <CL/cl.hpp>

#include "Config.h"
#include "DeviceContext.h"
#include "Kernel.h"
#include "sha1.h"
#include "Table.h"
#include "TSUtil.h"
#include "TimerKiller.h"
#include "TunedParameters.h"


const char* TSHasherContext::KERNEL_NAME = "TeamSpeakHasher";
const char* TSHasherContext::KERNEL_NAME2 = "TeamSpeakHasher2";

const char* TSHasherContext::BUILD_OPTS_BASE = "-I . -I src";

const size_t TSHasherContext::MAX_DEVICES = 16;
const size_t TSHasherContext::DEV_DEFAULT_LOCAL_WORK_SIZE = 32;
const size_t TSHasherContext::DEV_DEFAULT_GLOBAL_WORK_SIZE = 64 * 4096;
const uint64_t TSHasherContext::MAX_GLOBALLOCAL_RATIO = (1 << 16);
const size_t TSHasherContext::KERNEL_STD_ITERATIONS = 1024;

#define VENDOR_ID_GENERIC 0
#define VENDOR_ID_AMD 1
#define VENDOR_ID_NV (1<<5)

//#define BUSYWAITING_WORKAROUND

#if defined(_WIN32) || defined(_WIN64)
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#include <Windows.h>
void TSHasherContext::clearConsole() {
  system("cls");
}

#else
#include <signal.h>

void TSHasherContext::clearConsole() {
  system("clear");
}
#endif

static int roundUpPowerOfTwo(double x)
{
    // Handle non-positive or small values
    if (x <= 1.0) {
        return 1;
    }

    // Floor of the log base-2
    int exponent = static_cast<int>(std::floor(std::log2(x)));

    // Compute 2^exponent
    double powerOfTwo = std::pow(2.0, exponent);

    // If that power is still less than x, go to the next exponent
    if (powerOfTwo < x) {
        exponent++;
    }

    // Raise 2 to the adjusted exponent
    return static_cast<int>(std::pow(2.0, exponent));
}

TSHasherContext::TSHasherContext(std::string identity,
  uint64_t startcounter,
  uint64_t bestcounter,
  double throttlefactor) :
  startcounter(startcounter),
  global_bestdifficulty(TSUtil::getDifficulty(identity, bestcounter)),
  global_bestdifficulty_counter(bestcounter),
  identity(identity),
  throttlefactor(throttlefactor) {
  MIN_TARGET_DIFFICULTY = 34;

  std::vector<cl::Platform> platforms;

  std::vector<uint32_t> vendor_ids;

  cl::Platform::get(&platforms);
  for (uint32_t i = 0; i < platforms.size(); i++) {
    auto& platform = platforms[i];
    auto platform_vendor = std::string(platform.getInfo<CL_PLATFORM_VENDOR>().c_str());

    uint32_t platform_vendor_id;
    if (platform_vendor == "Advanced Micro Devices, Inc." || platform_vendor == "AuthenticAMD") {
      platform_vendor_id = VENDOR_ID_AMD;
    }
    else if (platform_vendor == "NVIDIA Corporation") {
      platform_vendor_id = VENDOR_ID_NV;
    }
    else {
      platform_vendor_id = VENDOR_ID_GENERIC;
    }


    std::vector<cl::Device> tmp_devices;
    platform.getDevices(DEV_TYPE, &tmp_devices);
    devices.insert(std::end(devices), std::begin(tmp_devices), std::end(tmp_devices));
    vendor_ids.insert(std::end(vendor_ids), tmp_devices.size(), platform_vendor_id);
  }
  if (devices.size() == 0) {
    std::cerr << "Error: No devices have been found." << std::endl;
    exit(-1);
  }


  for (cl_uint device_id = 0; device_id < devices.size(); device_id++) {
    cl::Device& device = devices[device_id];

    cl::Context context({ device });

    cl::Program::Sources sources;
    sources.push_back({ KERNEL_CODE, strlen(KERNEL_CODE) });

    cl::Program program(context, sources);

    cl_device_type devicetype = device.getInfo <CL_DEVICE_TYPE>();

    cl_uint vector_width = device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_INT>();

    uint32_t vendor_id = vendor_ids[device_id];

    cl_uint sm_minor = 0;
    cl_uint sm_major = 0;

    if (vendor_id == VENDOR_ID_NV) {
      sm_minor = device.getInfo<CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV>();
      sm_major = device.getInfo<CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV>();
    }

    char build_opts[2048] = { 0 };
    snprintf(build_opts,
      sizeof(build_opts) - 1,
      "%s -D VENDOR_ID=%u "
      "-D CUDA_ARCH=%u "
      "-D VECT_SIZE=%u "
      "-D DEVICE_TYPE=%u "
      "-D _unroll "
      "-cl-std=CL1.2 -w",
      BUILD_OPTS_BASE,
      vendor_id,
      (sm_major * 100) + sm_minor,
      vector_width,
      (uint32_t)devicetype);


    if (program.build({ device }, build_opts) != CL_SUCCESS) {
      std::cout << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << std::endl;
      exit(1);
    }

    cl::Kernel kernel(program, KERNEL_NAME);
    cl::Kernel kernel2(program, KERNEL_NAME2);


    cl::CommandQueue command_queue(context, device);

    cl_uint max_compute_units = device.getInfo <CL_DEVICE_MAX_COMPUTE_UNITS>();

    // we remove leading and trailing whitespaces
    // note that the expression std::string(___.c_str()) is a workaround
    // to remove the null terminator from the returned string from getInfo
    std::string device_name = std::string(device.getInfo<CL_DEVICE_NAME>().c_str());
    auto regex = std::regex("^ +| +$|( ) +");
    device_name = std::regex_replace((device_name), regex, "$1");

    std::cout << "Found new device #" << device_id << ": " << device_name << ", " << max_compute_units << " compute units" << std::endl;

    size_t global_work_size = DEV_DEFAULT_GLOBAL_WORK_SIZE;
    size_t local_work_size = DEV_DEFAULT_LOCAL_WORK_SIZE;
    auto bestgloballocal = tune(&device, device_id, build_opts);
    global_work_size = std::max(bestgloballocal.first / roundUpPowerOfTwo(throttlefactor), bestgloballocal.second);

    local_work_size = bestgloballocal.second;

    // device memory
    const size_t size_results = global_work_size * sizeof(uint8_t);

    cl::Buffer d_results(context, CL_MEM_WRITE_ONLY, size_results);
    cl::Buffer d_identity(context, CL_MEM_READ_ONLY, identity.size());


    // host memory
    auto h_results = new uint8_t[global_work_size]();
    command_queue.enqueueWriteBuffer(d_results, CL_TRUE, 0, size_results, h_results);
    const cl_uint identity_length = (cl_uint)identity.size();
    command_queue.enqueueWriteBuffer(d_identity, CL_TRUE, 0, identity_length, identity.c_str());

    DeviceContext dev_ctx(device_name, device, context, program, kernel, kernel2, command_queue, this,
      max_compute_units, devicetype, global_work_size, local_work_size, d_results, d_identity, h_results, identity);

    dev_ctxs.push_back(dev_ctx);
  }

  Config::store();
}

std::string TSHasherContext::getDeviceIdentifier(cl::Device* device,
  cl_uint device_id) {
  auto devicename = std::string(device->getInfo<CL_DEVICE_NAME>().c_str())
    + "_" + std::string(device->getInfo<CL_DEVICE_VENDOR>().c_str())
    + "_" + std::to_string(device->getInfo<CL_DEVICE_VENDOR_ID>())
    + "_" + std::string(device->getInfo<CL_DEVICE_VERSION>().c_str())
    + "_" + std::string(device->getInfo<CL_DRIVER_VERSION>().c_str())
    + "_" + std::to_string(device_id);

  const auto target = std::regex{ R"([^\w])" };
  const auto replacement = std::string{ "_" };

  return std::regex_replace(devicename, target, replacement);
}

std::pair<uint64_t, uint64_t> TSHasherContext::tune(cl::Device* device,
  cl_uint device_id,
  const char* build_opts) {
  using namespace std::chrono;
  auto devicename = std::string(device->getInfo<CL_DEVICE_NAME>().c_str());
  auto regex = std::regex{ R"([^\w])" };
  devicename = std::regex_replace(devicename, regex, std::string{ "_" });
  std::string deviceidentifier = getDeviceIdentifier(device, device_id);
  auto conf = Config::tuned.find(deviceidentifier);
  if (conf != Config::tuned.end()) {
    return { conf->second.globalworksize, conf->second.localworksize };
  }

  std::cout << "  Tuning... " << std::endl;


  uint64_t besttime = UINT64_MAX;
  std::pair<uint64_t, uint64_t> result;
  cl_ulong tunestartcounter = 10000000000000000000ULL;
  const cl_uchar tunetargetdifficulty = 60;
  const std::string tuneidentity = "AAABBBCCCDDDEEEFFFGGGHHHIIIJJJKKKLLLMMMNNNOOOPPPQQQRRRSSSTTTUUUVVVWWWXXXYYYZZZAAABBBCCCDDDEEEFFFGGG";
  const size_t identity_length = tuneidentity.size();


  cl::Context context({ *device });
  cl::Program::Sources sources;
  sources.push_back({ KERNEL_CODE, strlen(KERNEL_CODE) });
  cl::Program program(context, sources);

  if (program.build({ *device }, build_opts) != CL_SUCCESS) {
    std::cout << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(*device) << std::endl;
    exit(1);
  }

  cl::Kernel kernel(program, KERNEL_NAME);

  size_t max_local_worksize = 0;
  kernel.getWorkGroupInfo<size_t>(*device, CL_KERNEL_WORK_GROUP_SIZE, &max_local_worksize);
  max_local_worksize = std::min(max_local_worksize, device->getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>()[0]);


  cl::CommandQueue command_queue(context, *device);


  const size_t max_global_work_size = MAX_GLOBALLOCAL_RATIO * max_local_worksize;
  const size_t size_results = max_global_work_size * sizeof(uint8_t);

  cl::Buffer tune_d_results(context, CL_MEM_WRITE_ONLY, size_results);
  cl::Buffer tune_d_identity(context, CL_MEM_READ_ONLY, tuneidentity.size());
  uint8_t* tune_h_results = new uint8_t[max_global_work_size]();
  command_queue.enqueueWriteBuffer(tune_d_results, CL_TRUE, 0, size_results, tune_h_results);
  command_queue.enqueueWriteBuffer(tune_d_identity, CL_TRUE, 0, identity_length, tuneidentity.c_str());



  std::cout << "  ";
  const size_t start_local_worksize = max_local_worksize > 128 ? 16 : 1;
  const size_t end_local_worksize = max_local_worksize;
  const size_t repetitions = 30;
  const size_t warmup_runs = 5;
  const size_t max_time_per_kernel_ms = 150;

  for (size_t localsize = start_local_worksize;
    localsize <= end_local_worksize;
    localsize *= 2) {
    size_t globalsize = localsize;
    duration<uint64_t, std::nano> time = steady_clock::duration::zero();
    while (globalsize <= max_global_work_size && duration_cast<milliseconds>(time / repetitions).count() < max_time_per_kernel_ms) {
      time_point<high_resolution_clock> starttime;

      for (size_t q = 0; q < repetitions + warmup_runs; q++) {
        // we leave the first two kernel runs for warmup purposes
        if (q == warmup_runs) {
          starttime = high_resolution_clock::now();
        }
        cl_int err;
        err = kernel.setArg(0, tunestartcounter);
        err |= kernel.setArg(1, (cl_uint)KERNEL_STD_ITERATIONS);
        err |= kernel.setArg(2, tunetargetdifficulty);
        err |= kernel.setArg(3, tune_d_identity);
        err |= kernel.setArg(4, (cl_uint)identity_length);
        err |= kernel.setArg(5, tune_d_results);

        if (err != CL_SUCCESS) {
          std::cout << "A critical error occurred while setting kernel arguments." << std::endl;
          exit(-1);
        }

        err = command_queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(globalsize), cl::NDRange(localsize));
        if (err != CL_SUCCESS) {
          std::cout << "A critical error occurred while enqueuing the kernel." << std::endl;
          exit(-1);
        }
        err = command_queue.finish();
        err = command_queue.enqueueReadBuffer(tune_d_results, CL_TRUE, 0, globalsize * sizeof(uint8_t), tune_h_results);
      }

      time = high_resolution_clock::now() - starttime;
      auto time_ns = duration_cast<nanoseconds>(time).count();
      uint64_t time_norm = time_ns / globalsize;

      std::cout << ".";

      if (duration_cast<milliseconds>(time / repetitions).count() < max_time_per_kernel_ms && time_norm < besttime) {
        besttime = time_norm;
        result.first = globalsize;
        result.second = localsize;
      }
      globalsize *= 2;
    }
  }

  TunedParameters tunedparams(devicename, deviceidentifier, result.second, result.first);
  Config::tuned.insert(std::make_pair(deviceidentifier, tunedparams));

  std::cout << std::endl << std::endl << "  Tuning found global_work_size=" << result.first
    << ", local_work_size=" << result.second << " to be optimal." << std::endl;
  return result;
}

void TSHasherContext::compute() {
  TSHasherContext::starttime = std::chrono::high_resolution_clock::now();
  std::vector<std::thread> threads;
  for (cl_uint device_id = 0; device_id < devices.size(); device_id++) {
    DeviceContext* dev_ctx = &dev_ctxs[device_id];
    std::thread t([dev_ctx]() -> void { run_kernel_loop(dev_ctx); });
    threads.push_back(std::move(t));
  }

  // this loops until stopped
  TSHasherContext::printinfo(dev_ctxs);

  for (std::thread& t : threads) {
    t.join();
  }
}


std::string TSHasherContext::getFormattedDouble(double x) {
  if (x > 1000000000000) { return std::to_string(x / 1000000000000) + " T"; }
  if (x > 1000000000) { return std::to_string(x / 1000000000) + " G"; }
  if (x > 1000000) { return std::to_string(x / 1000000) + " M"; }
  return std::to_string(x) + " ";
}

std::string TSHasherContext::getFormattedDuration(double seconds) {
  //round towards zero
  uint64_t second = static_cast<uint64_t>(seconds);
  std::string out = "";
  if (second / 86400 > 0) {
    uint64_t days = second / 86400;
    out += std::to_string(days) + " days ";
    second -= days * 86400;
  }
  if (second / 3600 > 0) {
    uint64_t h = second / 3600;
    out += std::to_string(h) + " h ";
    second -= h * 3600;
  }
  if (second / 60 > 0) {
    uint64_t min = second / 60;
    out += std::to_string(min) + " min ";
    second -= min * 60;
  }
  out += std::to_string(second) + " s";
  return out;
}

void TSHasherContext::printinfo(const std::vector<DeviceContext>& dev_ctxs) {
  do {
    clearConsole();
    Table overalltable({ "Overview","" }, true);

    using namespace std::chrono;
    auto currenttime = high_resolution_clock::now();
    auto runningtime_ns = duration_cast<nanoseconds>(currenttime - starttime).count();
    const double runningtime = runningtime_ns / 1e9;
    overalltable.addRow({ "Running time: ", getFormattedDuration(runningtime) });

    uint64_t computed_hashes_total = 0;
    double currentspeed_total = 0;

    std::vector<Table> devicetables;
    for (cl_uint device_id = 0; device_id < dev_ctxs.size(); device_id++) {
      const DeviceContext& dev_ctx = dev_ctxs[device_id];

      if (dev_ctx.bestdifficulty > global_bestdifficulty) {
        global_bestdifficulty = dev_ctx.bestdifficulty;
        global_bestdifficulty_counter = dev_ctx.bestdifficulty_counter;
      }

      Table devtable({ "Device " + dev_ctx.device_name + "[" + std::to_string(device_id) + "]", "" }, true);


      devtable.addRow({ "Local/Global work size", std::to_string(dev_ctx.local_work_size) + "/" + std::to_string(dev_ctx.global_work_size) });


      const uint64_t computed_hashes_device = dev_ctx.completediterations_total;
      computed_hashes_total += computed_hashes_device;

      const double avgspeed_device = computed_hashes_device / runningtime;
      const double currentspeed_device = dev_ctx.getAvgSpeed();
      currentspeed_total += currentspeed_device;

      devtable.addRow({ "Current speed", getFormattedDouble(currentspeed_device) + "Hash/s" });
      devtable.addRow({ "Average speed", getFormattedDouble(avgspeed_device) + "Hash/s" });
      devtable.addRow({ "Scheduling", std::to_string(dev_ctx.completed_kernels / runningtime) + " Kernels/s" });

      devicetables.push_back(std::move(devtable));
    }

    const double unitspersecond_global = computed_hashes_total / runningtime;

    overalltable.addRow({ "Current speed [total]", getFormattedDouble(currentspeed_total) + "Hash/s" });
    overalltable.addRow({ "Average speed [total]", getFormattedDouble(unitspersecond_global) + "Hash/s" });


    const bool slowphase = TSUtil::isSlowPhase(identity.size(), startcounter);
    if (slowphase) {
      overalltable.addRow({ "Estimated time until slow phase", "0 (IN SLOW PHASE!!!)" });
    }
    else {
      auto its_until_slow = TSUtil::itsUntilSlowPhase(identity.size(), startcounter);
      auto time_until_slow = its_until_slow / currentspeed_total;
      overalltable.addRow({ "Estimated time until slow phase", getFormattedDuration(time_until_slow) });
    }

    uint64_t nextlevel = std::max((uint64_t)(global_bestdifficulty + 1ull),
      (uint64_t)MIN_TARGET_DIFFICULTY);

    // we want to estimate the remaining time for the next level,
    // accounting for the slow phase
    uint64_t nextlevel_estits = ((uint64_t)1 << nextlevel);
    uint64_t remaining_fastits = TSUtil::itsUntilSlowPhase(identity.size(), startcounter);
    uint64_t nextlevel_estits_adjusted = slowphase ? nextlevel_estits : (2 * nextlevel_estits - std::min(remaining_fastits, nextlevel_estits));
    double nextlevel_esttime_seconds = nextlevel_estits_adjusted / currentspeed_total;

    if (nextlevel_esttime_seconds > 60 && global_bestdifficulty + 1 < MIN_TARGET_DIFFICULTY) {
      MIN_TARGET_DIFFICULTY -= (uint8_t)std::ceil(std::log2(nextlevel_esttime_seconds / 60));
      MIN_TARGET_DIFFICULTY = std::max((uint8_t)32, MIN_TARGET_DIFFICULTY);
      // we need to recompute the time estimations
      nextlevel = std::max((uint64_t)(global_bestdifficulty + 1ull), (uint64_t)MIN_TARGET_DIFFICULTY);
      nextlevel_estits = ((uint64_t)1 << nextlevel);
      remaining_fastits = TSUtil::itsUntilSlowPhase(identity.size(), startcounter);
      nextlevel_estits_adjusted = slowphase ? nextlevel_estits : (2 * nextlevel_estits - std::min(remaining_fastits, nextlevel_estits));
      nextlevel_esttime_seconds = nextlevel_estits_adjusted / currentspeed_total;
    }
    overalltable.addRow({ "Security level" ,
      std::to_string((uint32_t)global_bestdifficulty) + " (with counter=" + std::to_string(global_bestdifficulty_counter) + ")" });

    overalltable.addRow({ "Estimated time until level " + std::to_string(nextlevel), getFormattedDuration(nextlevel_esttime_seconds) });
    overalltable.addRow({ "Current counter", std::to_string(startcounter) });


    // we start to print
    if (slowphase) {
      std::cout << "WARNING: You have entered the slow phase. With a fresh identity you could double your speed." << std::endl;
    }
    std::cout << overalltable.getTable();
    std::cout << std::endl << std::endl;
    for (auto& devtable : devicetables) {
      std::cout << devtable.getTable() << std::endl;
    }

    std::cout << std::endl << "(press Ctrl+C to stop and save progress)" << std::endl;
  } while (timerkiller.running() && timerkiller.wait_for(std::chrono::milliseconds(1000)));

  // we wait for all scheduled kernels to finish
  for (auto& dev : dev_ctxs) {
    dev.command_queue.finish();
  }
  std::cout << std::endl << "===========================================================" << std::endl;
  std::cout << std::endl << "Stopped at counter: " << startcounter << std::endl;
}


void TSHasherContext::run_kernel_loop(DeviceContext* dev_ctx) {
  while (dev_ctx->tshasherctx->timerkiller.running()) {
    const size_t identity_length = dev_ctx->identitystring.size();
    cl::Kernel& kernel = TSUtil::isSlowPhase(identity_length, dev_ctx->tshasherctx->startcounter) ? dev_ctx->kernel2 : dev_ctx->kernel;

    dev_ctx->tshasherctx->startcounter_mutex.lock();
    auto dev_startcounter = dev_ctx->tshasherctx->startcounter;
    auto global_max_iterations = std::min((uint64_t)dev_ctx->global_work_size * KERNEL_STD_ITERATIONS, TSUtil::itsConstantCounterLength(dev_startcounter));
    const uint64_t iterations = global_max_iterations / dev_ctx->global_work_size;
    if (iterations == 0) {
      // if there are too few iterations until the counter length increases, we just skip these
      dev_ctx->tshasherctx->startcounter += global_max_iterations;
      dev_ctx->tshasherctx->startcounter_mutex.unlock();
      continue;
    }
    cl_int err;
    err = kernel.setArg(0, (cl_ulong)dev_ctx->tshasherctx->startcounter);
    err |= kernel.setArg(1, (cl_uint)iterations);
    const uint8_t bestdifficulty = std::max(dev_ctx->tshasherctx->global_bestdifficulty, dev_ctx->bestdifficulty);
    const uint8_t targetdifficulty = std::max(dev_ctx->tshasherctx->MIN_TARGET_DIFFICULTY, (uint8_t)(1 + bestdifficulty));
    err |= kernel.setArg(2, (cl_uchar)targetdifficulty);
    err |= kernel.setArg(3, dev_ctx->d_identity);
    err |= kernel.setArg(4, (cl_uint)identity_length);
    err |= kernel.setArg(5, dev_ctx->d_results);

    if (err != CL_SUCCESS) {
      std::cout << "A critical error occurred while " << "setting kernel arguments." << std::endl;
      exit(-1);
    }

   


    dev_ctx->lastscheduled_startcounter = dev_ctx->tshasherctx->startcounter;
    dev_ctx->tshasherctx->startcounter += static_cast<uint64_t>(dev_ctx->global_work_size) * iterations;
    dev_ctx->tshasherctx->startcounter_mutex.unlock();
    dev_ctx->schedulediterations_total += static_cast<uint64_t>(dev_ctx->global_work_size) * iterations;
    dev_ctx->lastschedulediterations_total = static_cast<uint64_t>(dev_ctx->global_work_size) * iterations;


    err = dev_ctx->command_queue.enqueueNDRangeKernel(kernel, cl::NullRange,
      cl::NDRange(dev_ctx->global_work_size),
      cl::NDRange(dev_ctx->local_work_size),
      NULL);
    if (err != CL_SUCCESS) {
      std::cout << "A critical error occurred while enqueuing the kernel." << std::endl;
      exit(-1);
    }

    // this is a workaround to reduce the cpu load for implementations (as NVIDIA's) doing busy waiting
    #ifdef BUSYWAITING_WORKAROUND
    err = dev_ctx->command_queue.flush();
    if (dev_ctx->devicetype != CL_DEVICE_TYPE_CPU && dev_ctx->tshasherctx->throttlefactor == 1) {
      // no throttling: we aim for maximum performance
      // we sleep for the recent minimal time (slightly damped) needed to execute the kernel
      // this reduces cpu load significantly
      std::this_thread::sleep_for(dev_ctx->getRecentMinTime() * 0.9);
    }
    else if (dev_ctx->devicetype != CL_DEVICE_TYPE_CPU) {
      // throttling: we aim for reduced cpu load
      std::this_thread::sleep_for(dev_ctx->getRecentMinTime() * 0.94);
    }
    #endif

    dev_ctx->command_queue.finish();

    if (!dev_ctx->tshasherctx->timerkiller.running()) {
      return;
    }
    const size_t size_results = dev_ctx->global_work_size * sizeof(uint8_t);
    err = dev_ctx->command_queue.enqueueReadBuffer(dev_ctx->d_results, CL_TRUE, 0, size_results, dev_ctx->h_results);
    read_kernel_result(dev_ctx);
    dev_ctx->measureTime();

    // Additional sleep when Throttling
    if (dev_ctx->tshasherctx->throttlefactor > 1.00001) {
        std::this_thread::sleep_for(dev_ctx->getRecentMinTime() * (dev_ctx->tshasherctx->throttlefactor - 1));
        // exclude throttling time from timer
        dev_ctx->resetTimer();
    }
  }
}

void TSHasherContext::read_kernel_result(DeviceContext* dev_ctx) {

  dev_ctx->completediterations_total += dev_ctx->lastschedulediterations_total;
  dev_ctx->completed_kernels++;

  // read the result
  uint8_t oldbestdifficulty = dev_ctx->bestdifficulty;
  for (uint32_t thread = 0; thread < dev_ctx->global_work_size; thread++) {
    if (dev_ctx->h_results[thread]) {
      // target found: now we search for the correct counter
      uint64_t its_per_worker = dev_ctx->lastschedulediterations_total / dev_ctx->global_work_size;
      uint64_t searchstartcounter = dev_ctx->lastscheduled_startcounter + its_per_worker * thread;
      uint8_t bestdifficulty = 0;
      uint64_t bestdifficulty_counter = 0;

      for (uint64_t counter = searchstartcounter;
        counter < searchstartcounter + its_per_worker;
        counter++) {
        uint8_t currentdifficulty = TSUtil::getDifficulty(dev_ctx->identitystring, counter);
        if (currentdifficulty > bestdifficulty) {
          bestdifficulty = currentdifficulty;
          bestdifficulty_counter = counter;
        }
      }

      if (bestdifficulty > dev_ctx->bestdifficulty) {
        dev_ctx->bestdifficulty = bestdifficulty;
        dev_ctx->bestdifficulty_counter = bestdifficulty_counter;
      }
      else if (bestdifficulty <= oldbestdifficulty) {
        // this should never happen
        std::cout << std::endl << "A critical error occurred." << std::endl;
        std::cout << "Claimed target could not be found." << std::endl;
        exit(-1);
      }
    }
  }
}
