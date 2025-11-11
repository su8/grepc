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
#include <iostream>
#include <string>
#include <filesystem>
#include <thread>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <stdexcept>

static void walkMultipleDirs(const char *folder, const char opt);

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
    uintmax_t count = 0U;
    for (std::string line; std::getline(std::cin, line); count++) { ; }
    std::cout << count << " items" << '\n' << std::flush;
  }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
  return EXIT_SUCCESS;
}
#pragma GCC diagnostic pop

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