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

static volatile sig_atomic_t COUNT = 0;
namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
  unsigned short int gotPipe = 0U;
  std::string line;

  if (argc > 1 && argv[1][1] == 'l') {
    while (!feof(stdin)) {
      std::getline(std::cin, line);
      COUNT++;
      gotPipe = 1U;
    }
  }

  if (gotPipe == 0U) {
    try {
      for (const auto &entry : fs::directory_iterator(argc == 1 ? "./" : ((argv[1][1] == 'b') ? "./" : argv[1]))) {
        if (argc > 1 && argv[1][1] == 'b') {
          std::string pathStr = entry.path().filename().string();
          if (fs::exists(pathStr) && fs::is_directory(pathStr)) { continue; }
          std::cout << pathStr << " " << fs::file_size(pathStr) << " bytes " << '\n' << std::flush;
        }
        COUNT++;
      }
    } catch (const fs::filesystem_error& e) {
      std::cerr << "Error: " << e.what() << std::endl;
      return EXIT_FAILURE;
    }
  }

  std::cout << COUNT << " items" << '\n' << std::flush;
  return EXIT_SUCCESS;
}
