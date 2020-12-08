#pragma once
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <sstream>

namespace pyInterp {
  bool isOnlyWhitespace(std::string x) {
    bool is = false;
    for (int i = 0; i < x.length(); i++) {
      char c = x[i];

      if (
        c == ' '
        || c == '\t'
        || c == '\n'
        || c == '\r'
        || c == '\f'
        || c == '\v'
      ) is = true;
      else is = false;
    }

    return is;
  }

  void writeFile(std::string file, std::string data) {
    std::ofstream File(file);

    File << data;

    File.close();
  }
  
  std::string readFile(std::string file) {
    std::ifstream File(file);
    std::string line;
    std::string data = "";
    
    bool isFirst = true;
    while (std::getline(File, line)) {
      if (
        !(
          line.empty()
          || (
            isOnlyWhitespace(line)
          )
        )
      ) {
        if (!isFirst) data += "\n";
        else isFirst = false;
        data += line;
      }
    }

    File.close();

    return data;
  }
}