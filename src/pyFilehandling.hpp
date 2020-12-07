#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

namespace pyInterp {
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
      if (!isFirst) data += "\n";
      else isFirst = false;
      
      data += line;
    }

    File.close();

    return data;
  }
}