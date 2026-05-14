cmake_minimum_required(VERSION 3.13)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# CMAKE_SOURCE_DIR = emdw_de424/devel/emdw
# ../../../../     = repo root  (where src/ and tests/ live)
set(WUMPUS_ROOT ${CMAKE_SOURCE_DIR}/../../../../)

# ============================================================
# example: reference binary shipped with the emdw userguide
# ============================================================
add_executable(example example.cc)
target_link_libraries(example emdw)

# ============================================================
# wumpus: main DE424 tracker binary
# ============================================================
add_executable(wumpus
  ${WUMPUS_ROOT}/src/wumpus.cc
  ${WUMPUS_ROOT}/src/wumpus_model.cc
)
target_include_directories(wumpus PRIVATE ${WUMPUS_ROOT}/src)
target_link_libraries(wumpus emdw)

# ============================================================
# tests_basic: transition, emission, parser, BP
# ============================================================
add_executable(test_transition
  ${WUMPUS_ROOT}/tests/basic/test_transition.cc
  ${WUMPUS_ROOT}/src/wumpus_model.cc
)
target_include_directories(test_transition PRIVATE ${WUMPUS_ROOT}/src)
target_link_libraries(test_transition emdw)

add_executable(test_emission
  ${WUMPUS_ROOT}/tests/basic/test_emission.cc
  ${WUMPUS_ROOT}/src/wumpus_model.cc
)
target_include_directories(test_emission PRIVATE ${WUMPUS_ROOT}/src)
target_link_libraries(test_emission emdw)

add_executable(test_parser
  ${WUMPUS_ROOT}/tests/basic/test_parser.cc
  ${WUMPUS_ROOT}/src/wumpus_model.cc
)
target_include_directories(test_parser PRIVATE ${WUMPUS_ROOT}/src)
target_link_libraries(test_parser emdw)

add_executable(test_emdw_bp
  ${WUMPUS_ROOT}/tests/basic/test_emdw_bp.cc
  ${WUMPUS_ROOT}/src/wumpus_model.cc
)
target_include_directories(test_emdw_bp PRIVATE ${WUMPUS_ROOT}/src)
target_link_libraries(test_emdw_bp emdw)

# ============================================================
# tests_advanced: EM robustness, end-to-end sim, scalability
# ============================================================
add_executable(test_em_extremes
  ${WUMPUS_ROOT}/tests/advanced/test_em_extremes.cc
  ${WUMPUS_ROOT}/src/wumpus_model.cc
)
target_include_directories(test_em_extremes PRIVATE ${WUMPUS_ROOT}/src)
target_link_libraries(test_em_extremes emdw)

add_executable(test_e2e_sim
  ${WUMPUS_ROOT}/tests/advanced/test_e2e_sim.cc
  ${WUMPUS_ROOT}/src/wumpus_model.cc
)
target_include_directories(test_e2e_sim PRIVATE ${WUMPUS_ROOT}/src)
target_link_libraries(test_e2e_sim emdw)

add_executable(test_dataset2_scale
  ${WUMPUS_ROOT}/tests/advanced/test_dataset2_scale.cc
  ${WUMPUS_ROOT}/src/wumpus_model.cc
)
target_include_directories(test_dataset2_scale PRIVATE ${WUMPUS_ROOT}/src)
target_link_libraries(test_dataset2_scale emdw)
