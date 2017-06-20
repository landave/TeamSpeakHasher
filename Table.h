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
#ifndef TABLE_H_
#define TABLE_H_

#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>


class Table {
 public:
  Table(std::vector<std::string> cols, bool leftaligned) {
    this->cols = cols;
    this->leftaligned = leftaligned;
    col_lengths = std::vector<size_t>(cols.size());
    size_t i = 0;
    for (const auto &col : cols) {
      col_lengths[i] = col.size();
      i++;
    }
  }

  void addRow(std::vector<std::string> row) {
    if (row.size() != cols.size()) {
      throw std::invalid_argument("Row size must match col size.");
    }

    size_t i = 0;
    for (const auto &entry : row) {
      if (entry.size() > col_lengths[i]) {
        col_lengths[i] = entry.size();
      }
      i++;
    }

    rows.push_back(row);
  }

  std::string getTable() {
    total_length = std::accumulate(col_lengths.begin(),
                                   col_lengths.end(),
                                   static_cast<size_t>(0));
    total_length += cols.size() + 1;

    std::stringstream table;

    table << "+" << std::string(total_length - 2, '-') << "+" << std::endl;

    size_t i;
    i = 0;
    for (const auto &col : cols) {
      table << "|";
      table << (leftaligned ? std::left : std::right)
            << std::setw(col_lengths[i]) << col;
      i++;
    }
    table << "|" << std::endl;

    table << "+" << std::string(total_length - 2, '-') << "+" << std::endl;

    for (const auto &row : rows) {
      i = 0;
      for (const auto &entry : row) {
        table << "|";
        table << (leftaligned ? std::left : std::right)
              << std::setw(col_lengths[i]) << entry;
        i++;
      }
      table << "|" << std::endl;
    }


    table << "+" << std::string(total_length - 2, '-') << "+" << std::endl;

    return table.str();
  }

 private:
  bool leftaligned;
  std::vector<std::string> cols;
  std::vector<std::vector<std::string>> rows;
  std::vector<size_t> col_lengths;
  size_t total_length;
};

#endif
