#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>

struct CSV {
  // Simple numeric CSV loader. Supports optional header row (auto-detect if non-numeric in first row).
  static std::vector<std::vector<double>> load_numeric(const std::string& path, char sep = ',') {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Could not open CSV: " + path);

    std::vector<std::vector<double>> rows;
    std::string line;
    bool first_row = true;
    bool has_header = false;

    auto is_numberish = [](const std::string& s) {
      // Accept + - . digits e/E
      for (char c : s) {
        if (std::isdigit((unsigned char)c) || c=='+' || c=='-' || c=='.' || c=='e' || c=='E') continue;
        if (std::isspace((unsigned char)c)) continue;
        return false;
      }
      return !s.empty();
    };

    while (std::getline(in, line)) {
      if (line.empty()) continue;
      std::stringstream ss(line);
      std::string cell;
      std::vector<std::string> cells;
      while (std::getline(ss, cell, sep)) cells.push_back(cell);

      if (first_row) {
        // If any cell looks non-numeric => header
        for (auto &c : cells) {
          if (!is_numberish(c)) { has_header = true; break; }
        }
        first_row = false;
        if (has_header) continue;
      }

      std::vector<double> row;
      row.reserve(cells.size());
      for (auto &c : cells) {
        row.push_back(std::stod(c));
      }
      rows.push_back(std::move(row));
    }
    return rows;
  }
};

