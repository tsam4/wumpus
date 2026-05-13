// basic test: parser for space-separated grids (mini_project.pdf)

#include "wumpus_model.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static void write_file(const fs::path& p, const std::vector<std::string>& lines) {
  std::ofstream out(p);
  for (const auto& line : lines) out << line << "\n";
}

int main() {
  // temp dataset
  fs::path dir = fs::temp_directory_path() / "wumpus_parser_test";
  fs::create_directories(dir);

  fs::path f0 = dir / "data_file000.txt";
  fs::path f1 = dir / "data_file001.txt";
  write_file(f0, {"1 0 0", "0 1 0"});
  write_file(f1, {"0 0 1", "1 0 0"});

  Grid g;
  bool ok = read_grid_file(f0.string(), &g);
  if (!ok) return 1;
  assert(g.rows == 2);
  assert(g.cols == 3);
  assert(g.cells.size() == 6);
  assert(g.cells[0] == 1);
  assert(g.cells[4] == 1);

  auto obs = load_dataset(dir.string());
  assert(obs.size() == 2);
  assert(obs[0].rows == 2 && obs[0].cols == 3);
  assert(obs[1].rows == 2 && obs[1].cols == 3);

  fs::remove_all(dir);
  return 0;
}
