/*-------------------------------------------------------------------------------
 This file is part of Ranger.

 Ranger is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Ranger is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Ranger. If not, see <http://www.gnu.org/licenses/>.

 Written by:

 Marvin N. Wright
 Institut für Medizinische Biometrie und Statistik
 Universität zu Lübeck
 Ratzeburger Allee 160
 23562 Lübeck

 http://www.imbs-luebeck.de
 wright@imbs.uni-luebeck.de
 #-------------------------------------------------------------------------------*/

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iterator>

#include "Data.h"
#include "utility.h"

Data::Data() :
    num_rows(0), num_rows_rounded(0), num_cols(0), sparse_data(0), num_cols_no_sparse(0), externalData(true) {
}

Data::~Data() {
}

size_t Data::getVariableID(std::string variable_name) {
  std::vector<std::string>::iterator it = std::find(variable_names.begin(), variable_names.end(), variable_name);
  if (it == variable_names.end()) {
    throw std::runtime_error("Variable " + variable_name + " not found.");
  }
  return (std::distance(variable_names.begin(), it));
}

void Data::addSparseData(unsigned char* sparse_data, size_t num_cols_sparse) {
  num_cols = num_cols_no_sparse + num_cols_sparse;
  num_rows_rounded = roundToNextMultiple(num_rows, 4);
  this->sparse_data = sparse_data;
}

bool Data::loadFromFile(std::string filename) {

  bool result;

  // Open input file
  std::ifstream input_file;
  input_file.open(filename);
  if (!input_file.good()) {
    throw std::runtime_error("Could not open input file.");
  }

  // Count number of rows
  size_t line_count = 0;
  std::string line;
  while (getline(input_file, line)) {
    ++line_count;
  }
  num_rows = line_count - 1;
  input_file.close();
  input_file.open(filename);

  // Check if comma, semicolon or whitespace seperated
  std::string header_line;
  getline(input_file, header_line);

  // Find out if comma, semicolon or whitespace seperated and call appropriate method
  if (header_line.find(",") != std::string::npos) {
    result = loadFromFileOther(input_file, header_line, ',');
  } else if (header_line.find(";") != std::string::npos) {
    result = loadFromFileOther(input_file, header_line, ';');
  } else {
    result = loadFromFileWhitespace(input_file, header_line);
  }

  externalData = false;
  input_file.close();
  return result;
}

bool Data::loadFromFileWhitespace(std::ifstream& input_file, std::string header_line) {

  // Read header
  std::string header_token;
  std::stringstream header_line_stream(header_line);
  while (header_line_stream >> header_token) {
    variable_names.push_back(header_token);
  }
  num_cols = variable_names.size();
  num_cols_no_sparse = num_cols;

  // Read body
  reserveMemory();
  bool error = false;
  std::string line;
  size_t row = 0;
  while (getline(input_file, line)) {
    double token;
    std::stringstream line_stream(line);
    size_t column = 0;
    while (line_stream >> token) {
      set(column, row, token, error);
      ++column;
    }
    if (column > num_cols) {
      throw std::runtime_error("Could not open input file. Too many columns in a row.");
    } else if (column < num_cols) {
      throw std::runtime_error("Could not open input file. Too few columns in a row. Are all values numeric?");
    }
    ++row;
  }
  num_rows = row;
  return error;
}

bool Data::loadFromFileOther(std::ifstream& input_file, std::string header_line, char seperator) {

  // Read header
  std::string header_token;
  std::stringstream header_line_stream(header_line);
  while (getline(header_line_stream, header_token, seperator)) {
    variable_names.push_back(header_token);
  }
  num_cols = variable_names.size();
  num_cols_no_sparse = num_cols;

  // Read body
  reserveMemory();
  bool error = false;
  std::string line;
  size_t row = 0;
  while (getline(input_file, line)) {
    std::string token_string;
    double token;
    std::stringstream line_stream(line);
    size_t column = 0;
    while (getline(line_stream, token_string, seperator)) {
      std::stringstream token_stream(token_string);
      token_stream >> token;
      set(column, row, token, error);
      ++column;
    }
    ++row;
  }
  num_rows = row;
  return error;
}

// TODO: Speedup here for many different value?
void Data::getAllValues(std::vector<double>& all_values, std::vector<size_t>& sampleIDs, size_t varID) {

  // All values for varID (no duplicates) for given sampleIDs
  if (varID < num_cols_no_sparse) {
    all_values.reserve(sampleIDs.size());
    for (size_t i = 0; i < sampleIDs.size(); ++i) {
      double value = get(sampleIDs[i], varID);
      std::vector<double>::iterator it = std::find(all_values.begin(), all_values.end(), value);
      if (it == all_values.end()) {
        all_values.push_back(value);
      }
    }
  } else {
    // If GWA data just use 0, 1. A split on 2 would always put all to the left.
    all_values = std::vector<double>( { 0, 1 });
  }

}
