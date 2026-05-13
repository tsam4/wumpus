cmake_minimum_required(VERSION 3.13)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# ============================================================
# example: reference binary shipped with the emdw userguide
# ============================================================
add_executable(example example.cc)
target_link_libraries(example emdw)

# ============================================================
# wumpus: main DE424 tracker binary
# Compiled from src/ which lives two levels above this CMakeLists.
# ============================================================
add_executable(wumpus
  ${CMAKE_SOURCE_DIR}/../../src/wumpus.cc
  ${CMAKE_SOURCE_DIR}/../../src/wumpus_model.cc
)
target_include_directories(wumpus PRIVATE ${CMAKE_SOURCE_DIR}/../../src)
target_link_libraries(wumpus emdw)

# ============================================================
# tests_basic: unit tests covering transition, emission, parser, BP
# These map directly to the required tests in mini_project.pdf.
# ============================================================
add_executable(test_transition
  ${CMAKE_SOURCE_DIR}/../../tests/basic/test_transition.cc
  ${CMAKE_SOURCE_DIR}/../../src/wumpus_model.cc
)
target_include_directories(test_transition PRIVATE ${CMAKE_SOURCE_DIR}/../../src)
target_link_libraries(test_transition emdw)

add_executable(test_emission
  ${CMAKE_SOURCE_DIR}/../../tests/basic/test_emission.cc
  ${CMAKE_SOURCE_DIR}/../../src/wumpus_model.cc
)
target_include_directories(test_emission PRIVATE ${CMAKE_SOURCE_DIR}/../../src)
target_link_libraries(test_emission emdw)

add_executable(test_parser
  ${CMAKE_SOURCE_DIR}/../../tests/basic/test_parser.cc
  ${CMAKE_SOURCE_DIR}/../../src/wumpus_model.cc
)
target_include_directories(test_parser PRIVATE ${CMAKE_SOURCE_DIR}/../../src)
target_link_libraries(test_parser emdw)

add_executable(test_emdw_bp
  ${CMAKE_SOURCE_DIR}/../../tests/basic/test_emdw_bp.cc
  ${CMAKE_SOURCE_DIR}/../../src/wumpus_model.cc
)
target_include_directories(test_emdw_bp PRIVATE ${CMAKE_SOURCE_DIR}/../../src)
target_link_libraries(test_emdw_bp emdw)

# ============================================================
# tests_advanced: EM robustness, end-to-end sim, scalability
# These go beyond the required suite and live in tests/advanced/.
# ============================================================
add_executable(test_em_extremes
  ${CMAKE_SOURCE_DIR}/../../tests/advanced/test_em_extremes.cc
  ${CMAKE_SOURCE_DIR}/../../src/wumpus_model.cc
)
target_include_directories(test_em_extremes PRIVATE ${CMAKE_SOURCE_DIR}/../../src)
target_link_libraries(test_em_extremes emdw)

add_executable(test_e2e_sim
  ${CMAKE_SOURCE_DIR}/../../tests/advanced/test_e2e_sim.cc
  ${CMAKE_SOURCE_DIR}/../../src/wumpus_model.cc
)
target_include_directories(test_e2e_sim PRIVATE ${CMAKE_SOURCE_DIR}/../../src)
target_link_libraries(test_e2e_sim emdw)

add_executable(test_dataset2_scale
  ${CMAKE_SOURCE_DIR}/../../tests/advanced/test_dataset2_scale.cc
  ${CMAKE_SOURCE_DIR}/../../src/wumpus_model.cc
)
target_include_directories(test_dataset2_scale PRIVATE ${CMAKE_SOURCE_DIR}/../../src)
target_link_libraries(test_dataset2_scale emdw)
