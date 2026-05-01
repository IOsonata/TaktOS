# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/hoan/swdev/private/I-SYST/TaktOS_Dev/KVB/Targets/nRF54L15-DK/Zephyr_nRF54L15-DK_KvbSuite")
  file(MAKE_DIRECTORY "/Users/hoan/swdev/private/I-SYST/TaktOS_Dev/KVB/Targets/nRF54L15-DK/Zephyr_nRF54L15-DK_KvbSuite")
endif()
file(MAKE_DIRECTORY
  "/Users/hoan/swdev/private/I-SYST/TaktOS_Dev/KVB/Targets/nRF54L15-DK/Zephyr_nRF54L15-DK_KvbSuite/build_832/Zephyr_nRF54L15-DK_KvbSuite"
  "/Users/hoan/swdev/private/I-SYST/TaktOS_Dev/KVB/Targets/nRF54L15-DK/Zephyr_nRF54L15-DK_KvbSuite/build_832/_sysbuild/sysbuild/images/Zephyr_nRF54L15-DK_KvbSuite-prefix"
  "/Users/hoan/swdev/private/I-SYST/TaktOS_Dev/KVB/Targets/nRF54L15-DK/Zephyr_nRF54L15-DK_KvbSuite/build_832/_sysbuild/sysbuild/images/Zephyr_nRF54L15-DK_KvbSuite-prefix/tmp"
  "/Users/hoan/swdev/private/I-SYST/TaktOS_Dev/KVB/Targets/nRF54L15-DK/Zephyr_nRF54L15-DK_KvbSuite/build_832/_sysbuild/sysbuild/images/Zephyr_nRF54L15-DK_KvbSuite-prefix/src/Zephyr_nRF54L15-DK_KvbSuite-stamp"
  "/Users/hoan/swdev/private/I-SYST/TaktOS_Dev/KVB/Targets/nRF54L15-DK/Zephyr_nRF54L15-DK_KvbSuite/build_832/_sysbuild/sysbuild/images/Zephyr_nRF54L15-DK_KvbSuite-prefix/src"
  "/Users/hoan/swdev/private/I-SYST/TaktOS_Dev/KVB/Targets/nRF54L15-DK/Zephyr_nRF54L15-DK_KvbSuite/build_832/_sysbuild/sysbuild/images/Zephyr_nRF54L15-DK_KvbSuite-prefix/src/Zephyr_nRF54L15-DK_KvbSuite-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/hoan/swdev/private/I-SYST/TaktOS_Dev/KVB/Targets/nRF54L15-DK/Zephyr_nRF54L15-DK_KvbSuite/build_832/_sysbuild/sysbuild/images/Zephyr_nRF54L15-DK_KvbSuite-prefix/src/Zephyr_nRF54L15-DK_KvbSuite-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/hoan/swdev/private/I-SYST/TaktOS_Dev/KVB/Targets/nRF54L15-DK/Zephyr_nRF54L15-DK_KvbSuite/build_832/_sysbuild/sysbuild/images/Zephyr_nRF54L15-DK_KvbSuite-prefix/src/Zephyr_nRF54L15-DK_KvbSuite-stamp${cfgdir}") # cfgdir has leading slash
endif()
