# Install script for directory: /Users/taros/Documents/DE424MP/emdw_de424/devel/emdw/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Library/Developer/CommandLineTools/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/taros/Documents/DE424MP/emdw_de424/devel/emdw/build/src/emdw-base/cmake_install.cmake")
  include("/Users/taros/Documents/DE424MP/emdw_de424/devel/emdw/build/src/emdw-beliefprop/cmake_install.cmake")
  include("/Users/taros/Documents/DE424MP/emdw_de424/devel/emdw/build/src/emdw-clustering/cmake_install.cmake")
  include("/Users/taros/Documents/DE424MP/emdw_de424/devel/emdw/build/src/emdw-factors/cmake_install.cmake")
  include("/Users/taros/Documents/DE424MP/emdw_de424/devel/emdw/build/src/emdw-graphing/cmake_install.cmake")
  include("/Users/taros/Documents/DE424MP/emdw_de424/devel/emdw/build/src/test/cmake_install.cmake")
  include("/Users/taros/Documents/DE424MP/emdw_de424/devel/emdw/build/src/utils-dependancyglue/cmake_install.cmake")
  include("/Users/taros/Documents/DE424MP/emdw_de424/devel/emdw/build/src/utils-informationtheory/cmake_install.cmake")
  include("/Users/taros/Documents/DE424MP/emdw_de424/devel/emdw/build/src/utils-matrix/cmake_install.cmake")
  include("/Users/taros/Documents/DE424MP/emdw_de424/devel/emdw/build/src/utils-misc/cmake_install.cmake")
  include("/Users/taros/Documents/DE424MP/emdw_de424/devel/emdw/build/src/utils-sigmapoints/cmake_install.cmake")
  include("/Users/taros/Documents/DE424MP/emdw_de424/devel/emdw/build/src/utils-vector/cmake_install.cmake")
  include("/Users/taros/Documents/DE424MP/emdw_de424/devel/emdw/build/src/vmp/cmake_install.cmake")
  include("/Users/taros/Documents/DE424MP/emdw_de424/devel/emdw/build/src/wip/cmake_install.cmake")
  include("/Users/taros/Documents/DE424MP/emdw_de424/devel/emdw/build/src/bin/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/taros/Documents/DE424MP/emdw_de424/devel/emdw/build/src/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
