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
#include <iostream>
#include <cstdlib>
#include <string>
#include <filesystem>
#include <csignal>
#include <thread>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <inttypes.h>

static void walkMultiDirs(const char *folder, char ch);

static uintmax_t COUNT = 0U;
namespace fs = std::filesystem;
std::mutex outMutex;
static std::unordered_map<std::string, uintmax_t> curDirNum;

int main(int argc, char *argv[]) {
  if (argc > 1 && (argv[1][1] == 'm' || argv[1][1] == 'b')) {
    std::vector<std::thread> threads;
    for (int x = 2; x <= argc - 1; x++) { threads.emplace_back(walkMultiDirs, argv[x], argv[1][1]); curDirNum.emplace(argv[x], 0U); }
    for (auto &thread : threads) { if (thread.joinable()) { thread.join(); } }
    return EXIT_SUCCESS;
  }
  if (argc > 1 && argv[1][1] == 'l') {
    std::string line;
    while (!feof(stdin)) { std::getline(std::cin, line); COUNT++; }
    std::cout << COUNT << " items" << '\n' << std::flush;
  }
  return EXIT_SUCCESS;
}

static void walkMultiDirs(const char *folder, char ch) {
  try {
    for (const auto &entry : fs::directory_iterator(folder)) {
      std::lock_guard<std::mutex> lock(outMutex);
      std::filesystem::current_path(folder);
      if (ch == 'b') {
        std::string pathStr = entry.path().filename().string();
        if (fs::exists(pathStr) && fs::is_directory(pathStr)) { continue; }
        std::cout << pathStr << " " << fs::file_size(pathStr) << " bytes " << '\n' << std::flush;
      }
      curDirNum[folder]++;
    }
    std::cout << folder << ' ' << curDirNum[folder] << " items" << '\n' << std::flush;
  } catch (const fs::filesystem_error &e) { std::lock_guard<std::mutex> lock(outMutex); std::cerr << "Error: " << e.what() << std::endl; }
}
