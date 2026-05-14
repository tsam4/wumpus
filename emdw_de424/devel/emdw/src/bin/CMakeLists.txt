cmake_minimum_required(VERSION 3.13)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Root of the project repo (5 levels up from this file)
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../..)

set(WUMPUS_SRC     ${PROJECT_ROOT}/src)
set(WUMPUS_INC     ${PROJECT_ROOT}/include)
set(TESTS_BASIC    ${PROJECT_ROOT}/tests_basic)
set(TESTS_ADVANCED ${PROJECT_ROOT}/tests_advanced)

add_executable(example example.cc)
target_link_libraries(example emdw)

# ── Main wumpus executable ──────────────────────────────────────────────────
add_executable(wumpus
    ${WUMPUS_SRC}/wumpus.cc
    ${WUMPUS_SRC}/wumpus_model.cc
)
target_include_directories(wumpus PRIVATE ${WUMPUS_INC})
target_link_libraries(wumpus emdw)

# ── Basic unit tests ────────────────────────────────────────────────────────
foreach(TEST_NAME test_emission test_transition test_emdw_bp)
    add_executable(${TEST_NAME}
        ${TESTS_BASIC}/${TEST_NAME}.cc
        ${WUMPUS_SRC}/wumpus_model.cc
    )
    target_include_directories(${TEST_NAME} PRIVATE ${WUMPUS_INC})
    target_link_libraries(${TEST_NAME} emdw)
endforeach()

# ── Advanced tests ──────────────────────────────────────────────────────────
foreach(TEST_NAME test_dataset2_scale test_e2e_sim test_em_extremes test_occ_prior_stress)
    add_executable(${TEST_NAME}
        ${TESTS_ADVANCED}/${TEST_NAME}.cc
        ${WUMPUS_SRC}/wumpus_model.cc
    )
    target_include_directories(${TEST_NAME} PRIVATE ${WUMPUS_INC})
    target_link_libraries(${TEST_NAME} emdw)
endforeach()
