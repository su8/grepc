/*
Copyright 10/18/2025 https://github.com/su8/grepc
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
MA 02110-1301, USA.
*/
#include <cstdio>
#include <csignal>
#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <string>
#include <filesystem>
#include <thread>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <stdexcept>

#if defined(__x86_64__) || defined(__i386__)
#define GREPC_X86 1
#include <immintrin.h>
#endif

static void walkMultipleDirs(const char *folder, const char opt);
static uintmax_t countLines(void);
static uintmax_t countNewlinesScalar(const char *buf, size_t len);
#ifdef GREPC_X86
__attribute__((target("avx2")))
static uintmax_t countNewlinesAVX2(const char *buf, size_t len);
#endif

namespace fs = std::filesystem;
std::mutex mtx;
static std::unordered_map<std::string, uintmax_t> curDirNum;

int main(int argc, char *argv[]) {
  if (argc > 1 && (argv[1][1] == 'm' || argv[1][1] == 'b')) {
    std::vector<std::thread> threads;
    for (int x = 2; x <= argc - 1; x++) { threads.emplace_back(walkMultipleDirs, argv[x], argv[1][1]); curDirNum.emplace(argv[x], 0U); }
    for (auto &thread : threads) { if (thread.joinable()) { thread.join(); } }
    return EXIT_SUCCESS;
  }
  if (argc > 1 && argv[1][1] == 'l') {
    std::cout << countLines() << " items" << '\n' << std::flush;
  }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
  return EXIT_SUCCESS;
}
#pragma GCC diagnostic pop

static uintmax_t countNewlinesScalar(const char *buf, size_t len) {
  uintmax_t total = 0U;
  for (size_t i = 0; i < len; i++) { total += (buf[i] == '\n'); }
  return total;
}

#ifdef GREPC_X86
__attribute__((target("avx2")))
static uintmax_t countNewlinesAVX2(const char *buf, size_t len) {
  const __m256i nl = _mm256_set1_epi8('\n');
  const __m256i zero = _mm256_setzero_si256();
  uintmax_t total = 0U;
  size_t i = 0;

  while (i + 32 <= len) {
    /* 8-bit per-lane counters can hold at most 255 increments */
    size_t blocks = (len - i) / 32;
    if (blocks > 255) { blocks = 255; }
    __m256i acc = zero;
    for (size_t b = 0; b < blocks; b++, i += 32) {
      __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buf + i));
      /* cmpeq gives 0xFF (-1) on match, so subtracting adds 1 */
      acc = _mm256_sub_epi8(acc, _mm256_cmpeq_epi8(v, nl));
    }
    /* horizontally sum the 32 byte counters into four 64-bit lanes */
    __m256i sums = _mm256_sad_epu8(acc, zero);
    alignas(32) uint64_t out[4];
    _mm256_store_si256(reinterpret_cast<__m256i *>(out), sums);
    total += out[0] + out[1] + out[2] + out[3];
  }
  for (; i < len; i++) { total += (buf[i] == '\n'); }
  return total;
}
#endif

static uintmax_t countLines(void) {
  static char buf[1U << 20];
  uintmax_t count = 0U;
  char last = '\n';
  size_t n;
#ifdef GREPC_X86
  const bool useAVX2 = __builtin_cpu_supports("avx2");
#endif

  while ((n = fread(buf, 1, sizeof(buf), stdin)) > 0) {
#ifdef GREPC_X86
    count += useAVX2 ? countNewlinesAVX2(buf, n) : countNewlinesScalar(buf, n);
#else
    count += countNewlinesScalar(buf, n);
#endif
    last = buf[n - 1];
  }
  /* same behaviour as std::getline: a final line without '\n' still counts */
  if (last != '\n') { count++; }
  return count;
}

static void walkMultipleDirs(const char *folder, const char opt) {
  try {
    for (const auto &entry : fs::directory_iterator(folder)) {
      std::lock_guard<std::mutex> lock(mtx);
      fs::current_path(folder);
      if (opt == 'b') {
        std::string fileStr = entry.path().filename().string();
        if (fs::exists(fileStr) && fs::is_directory(fileStr)) { continue; }
        std::cout << fileStr << " " << fs::file_size(fileStr) << " bytes " << '\n' << std::flush;
      }
      curDirNum[folder]++;
    }
    fs::path curFolder = (folder[0] == '.') ? fs::current_path() : static_cast<fs::path>(folder);
    std::cout << curFolder.string() << ' ' << curDirNum[folder] << " items" << '\n' << std::flush;
  } catch (const std::exception &e) { std::lock_guard<std::mutex> lock(mtx); std::cerr << "Error: " << e.what() << std::endl; return; }
}