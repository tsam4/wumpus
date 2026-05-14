// test_parser: unit tests for read_grid_file
//
// Covers:
// - Reads a valid grid file correctly (rows, cols, cell values)
// - Row-major ordering: cells[y*cols + x]
// - Returns false on missing file
// - Returns false on empty file

#include "wumpus_model.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

using namespace std;

static string write_temp(const char* content) {
  string path = "/tmp/wumpus_test_data_file000.txt";
  ofstream f(path);
  f << content;
  return path;
}

void test_read_valid_grid() {
  // 2 rows x 3 cols
  string path = write_temp("1 0 1\n0 1 0\n");
  Grid g;
  assert(read_grid_file(path, &g) && "must succeed on valid file");
  assert(g.rows == 2 && "row count mismatch");
  assert(g.cols == 3 && "col count mismatch");
  // Row-major: cells[y*cols + x]
  assert(g.cells[0*3+0] == 1);
  assert(g.cells[0*3+1] == 0);
  assert(g.cells[0*3+2] == 1);
  assert(g.cells[1*3+0] == 0);
  assert(g.cells[1*3+1] == 1);
  assert(g.cells[1*3+2] == 0);
  remove(path.c_str());
  printf("PASS: test_read_valid_grid\n");
}

void test_read_missing_file() {
  Grid g;
  assert(!read_grid_file("/tmp/no_such_file_wumpus_xyz.txt", &g)
         && "must fail on missing file");
  printf("PASS: test_read_missing_file\n");
}

void test_read_empty_file() {
  string path = write_temp("");
  Grid g;
  assert(!read_grid_file(path, &g) && "must fail on empty file");
  remove(path.c_str());
  printf("PASS: test_read_empty_file\n");
}

int main() {
  test_read_valid_grid();
  test_read_missing_file();
  test_read_empty_file();
  printf("All parser tests passed.\n");
  return 0;
}
