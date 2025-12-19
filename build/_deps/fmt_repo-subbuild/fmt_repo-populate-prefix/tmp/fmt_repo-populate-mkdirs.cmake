# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/sachintrivedi/HFT_ProjectV2/build/_deps/fmt_repo-src")
  file(MAKE_DIRECTORY "/Users/sachintrivedi/HFT_ProjectV2/build/_deps/fmt_repo-src")
endif()
file(MAKE_DIRECTORY
  "/Users/sachintrivedi/HFT_ProjectV2/build/_deps/fmt_repo-build"
  "/Users/sachintrivedi/HFT_ProjectV2/build/_deps/fmt_repo-subbuild/fmt_repo-populate-prefix"
  "/Users/sachintrivedi/HFT_ProjectV2/build/_deps/fmt_repo-subbuild/fmt_repo-populate-prefix/tmp"
  "/Users/sachintrivedi/HFT_ProjectV2/build/_deps/fmt_repo-subbuild/fmt_repo-populate-prefix/src/fmt_repo-populate-stamp"
  "/Users/sachintrivedi/HFT_ProjectV2/build/_deps/fmt_repo-subbuild/fmt_repo-populate-prefix/src"
  "/Users/sachintrivedi/HFT_ProjectV2/build/_deps/fmt_repo-subbuild/fmt_repo-populate-prefix/src/fmt_repo-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/sachintrivedi/HFT_ProjectV2/build/_deps/fmt_repo-subbuild/fmt_repo-populate-prefix/src/fmt_repo-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/sachintrivedi/HFT_ProjectV2/build/_deps/fmt_repo-subbuild/fmt_repo-populate-prefix/src/fmt_repo-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
