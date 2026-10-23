#------------------------------------------------------------------------------------------------------------
#
# Automate-VCPKG by Andre Taulien
# ===============================
#
# Project Repository: https://github.com/REGoth-project/Automate-VCPKG
# License ..........: MIT, see end of file.
#
# Based on: https://github.com/sutambe/cpptruths/blob/vcpkg_cmake_blog/cpp0x/vcpkg_test/CMakeLists.txt
#
# 
# While [Vcpkg](https://github.com/microsoft/vcpkg) on it's own is awesome, it does add 
# a little bit of complexity to getting a project to build. Even more if the one trying 
# to compile your application is not too fond of the commandline. Additionally, CMake 
# commands tend to get rather long with the toolchain path. 
# 
# To keep things simple for new users who just want to get the project to build, this 
# script offers a solution.
# 
# Lets assume your main `CMakelists.txt` looks something like this:
#
#     cmake_minimum_required (VERSION 3.12.0)
#     project (MyProject)
#     
#     add_executable(MyExecutable main.c)
# 
# To integrate Vcpkg into that `CMakelists.txt`, simple put the following lines before the 
# call to `project(MyProject)`:
# 
#     include(cmake/automate-vcpkg.cmake)
#     
#     vcpkg_bootstrap()
#     vcpkg_install_packages(libsquish physfs)
# 
# The call to `vcpkg_bootstrap()` will clone the official Vcpkg repository and bootstrap it. 
# If it detected an existing environment variable defining a valid `VCPKG_ROOT`, it will 
# update the existing installation of Vcpkg.
# 
# Arguments to `vcpkg_install_packages()` are the packages you want to install using Vcpkg.
# 
# If you want to keep the possibility for users to chose their own copy of Vcpkg, you can 
# simply not run the code snippet mentioned above, something like this will work:
# 
#     option(SKIP_AUTOMATE_VCPKG "When ON, you will need to built the packages 
#      required by MyProject on your own or supply your own vcpkg toolchain.")
#     
#     if (NOT SKIP_AUTOMATE_VCPKG)
#       include(cmake/automate-vcpkg.cmake)
#     
#       vcpkg_bootstrap()
#       vcpkg_install_packages(libsquish physfs)
#     endif()
#  
# Then, the user has to supply the packages on their own, be it through Vcpkg or manually 
# specifying their locations.
#------------------------------------------------------------------------------------------------------------

cmake_minimum_required (VERSION 3.12)

if(WIN32)
    set(VCPKG_FALLBACK_ROOT ${CMAKE_CURRENT_BINARY_DIR}/vcpkg CACHE STRING "vcpkg configuration directory to use if vcpkg was not installed on the system before")
else()
    set(VCPKG_FALLBACK_ROOT ${CMAKE_CURRENT_BINARY_DIR}/.vcpkg CACHE STRING  "vcpkg configuration directory to use if vcpkg was not installed on the system before")
endif()

# On Windows, Vcpkg defaults to x86, even on x64 systems. If we're 
# doing a 64-bit build, we need to fix that.
if (WIN32)

    # Since the compiler checks haven't run yet, we need to figure
    # out the value of CMAKE_SIZEOF_VOID_P ourselfs

    include(CheckTypeSize)
    enable_language(C)
    check_type_size("void*" SIZEOF_VOID_P BUILTIN_TYPES_ONLY)
    
    if (SIZEOF_VOID_P EQUAL 8)
        message(STATUS "Using Vcpkg triplet 'x64-windows'")
        
        set(VCPKG_TRIPLET x64-windows)
    endif()
endif()

if(NOT DEFINED VCPKG_ROOT)
    if(NOT DEFINED ENV{VCPKG_ROOT})
	    set(VCPKG_ROOT ${VCPKG_FALLBACK_ROOT})
    else()
        set(VCPKG_ROOT $ENV{VCPKG_ROOT})
    endif()
endif()

# Installs a new copy of Vcpkg or updates an existing one
macro(vcpkg_bootstrap)
    _install_or_update_vcpkg()

    # Find out whether the user supplied their own VCPKG toolchain file
    if(NOT DEFINED ${CMAKE_TOOLCHAIN_FILE})
        # We know this wasn't set before so we need point the toolchain file to the newly found VCPKG_ROOT
        set(CMAKE_TOOLCHAIN_FILE ${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake CACHE STRING "")
    
        # Just setting vcpkg.cmake as toolchain file does not seem to actually pull in the code
        include(${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
    
        set(AUTOMATE_VCPKG_USE_SYSTEM_VCPKG OFF)
    else()
        # VCPKG_ROOT has been defined by the toolchain file already
        set(AUTOMATE_VCPKG_USE_SYSTEM_VCPKG ON)
    endif()
    
    message(STATUS "Automate VCPKG status:")
    message(STATUS "  VCPKG_ROOT.....: ${VCPKG_ROOT}")
    message(STATUS "  VCPKG_EXEC.....: ${VCPKG_EXEC}")
    message(STATUS "  VCPKG_BOOTSTRAP: ${VCPKG_BOOTSTRAP}")
endmacro()

# MK64 Online fork: pin the vcpkg TOOL clone (upstream clones HEAD and `git pull`s on reuse).
# Default pin = vcpkg.json "builtin-baseline", so tool and port baseline come from one vcpkg commit.
# -DVCPKG_PIN_COMMIT= (empty) restores the upstream behaviour (clone HEAD, git pull on reuse).
set(_AUTOMATE_VCPKG_BASELINE 2e58bb35ff7a3a037920d959ce20cb4d8c22319a)  # fallback = vcpkg.json:46 at base 5073d2e
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19 AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/../vcpkg.json")
    file(READ "${CMAKE_CURRENT_LIST_DIR}/../vcpkg.json" _automate_vcpkg_manifest)
    string(JSON _automate_vcpkg_bl ERROR_VARIABLE _automate_vcpkg_json_error GET "${_automate_vcpkg_manifest}" builtin-baseline)
    if(NOT _automate_vcpkg_json_error)
        set(_AUTOMATE_VCPKG_BASELINE ${_automate_vcpkg_bl})
    endif()
endif()
if(NOT DEFINED VCPKG_GIT_URL)
    set(VCPKG_GIT_URL https://github.com/Microsoft/vcpkg.git CACHE STRING "vcpkg repository to clone")
endif()
if(NOT DEFINED VCPKG_PIN_COMMIT)
    set(VCPKG_PIN_COMMIT ${_AUTOMATE_VCPKG_BASELINE} CACHE STRING
        "vcpkg tool commit to check out (empty = upstream behaviour: HEAD + git pull)")
endif()

function(_automate_vcpkg_checkout_pin)
    string(LENGTH "${VCPKG_PIN_COMMIT}" _len)
    if(NOT _len EQUAL 40 OR NOT VCPKG_PIN_COMMIT MATCHES "^[0-9a-f]+$")
        message(FATAL_ERROR "***** FATAL ERROR: VCPKG_PIN_COMMIT must be a full 40-hex commit, got '${VCPKG_PIN_COMMIT}' *****")
    endif()
    if(NOT VCPKG_PIN_COMMIT STREQUAL _AUTOMATE_VCPKG_BASELINE)
        message(WARNING "VCPKG_PIN_COMMIT ${VCPKG_PIN_COMMIT} differs from vcpkg.json builtin-baseline ${_AUTOMATE_VCPKG_BASELINE}")
    endif()
    execute_process(COMMAND git -C "${VCPKG_ROOT}" rev-parse HEAD
                    OUTPUT_VARIABLE _head OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(_head STREQUAL VCPKG_PIN_COMMIT)
        message(STATUS "vcpkg already at pinned commit ${VCPKG_PIN_COMMIT}")
        return()
    endif()
    execute_process(COMMAND git -C "${VCPKG_ROOT}" cat-file -e "${VCPKG_PIN_COMMIT}^{commit}"
                    RESULT_VARIABLE _have ERROR_QUIET)
    if(NOT _have EQUAL 0)
        message(STATUS "Fetching vcpkg ${VCPKG_PIN_COMMIT} from ${VCPKG_GIT_URL}")
        execute_process(COMMAND git -C "${VCPKG_ROOT}" fetch --quiet "${VCPKG_GIT_URL}" "${VCPKG_PIN_COMMIT}"
                        RESULT_VARIABLE _fetch_rc ERROR_QUIET)
        if(NOT _fetch_rc EQUAL 0)  # server refused fetch-by-SHA: fetch its default branch instead
            execute_process(COMMAND git -C "${VCPKG_ROOT}" fetch --quiet "${VCPKG_GIT_URL}")
        endif()
    endif()
    message(STATUS "Pinning vcpkg in ${VCPKG_ROOT} to ${VCPKG_PIN_COMMIT}")
    execute_process(COMMAND git -C "${VCPKG_ROOT}" -c advice.detachedHead=false
                            checkout --quiet --detach "${VCPKG_PIN_COMMIT}"
                    RESULT_VARIABLE _co_rc)
    execute_process(COMMAND git -C "${VCPKG_ROOT}" rev-parse HEAD
                    OUTPUT_VARIABLE _head OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(NOT _co_rc EQUAL 0 OR NOT _head STREQUAL VCPKG_PIN_COMMIT)
        message(FATAL_ERROR "***** FATAL ERROR: could not check out vcpkg ${VCPKG_PIN_COMMIT} in ${VCPKG_ROOT} (HEAD=${_head}) *****")
    endif()
    # HEAD moved: the tool binary belongs to the old commit, force bootstrap to rebuild it.
    file(REMOVE "${VCPKG_ROOT}/vcpkg" "${VCPKG_ROOT}/vcpkg.exe")
endfunction()

macro(_install_or_update_vcpkg)
    if(NOT EXISTS ${VCPKG_ROOT})
        message(STATUS "Cloning vcpkg in ${VCPKG_ROOT}")
        execute_process(COMMAND git clone ${VCPKG_GIT_URL} ${VCPKG_ROOT})
        if(NOT VCPKG_PIN_COMMIT STREQUAL "" AND EXISTS ${VCPKG_ROOT}/README.md)
            _automate_vcpkg_checkout_pin()
        endif()
    elseif(VCPKG_PIN_COMMIT STREQUAL "")
        # The following command has no effect if the vcpkg repository is in a detached head state.
        message(STATUS "Auto-updating vcpkg in ${VCPKG_ROOT}")
        execute_process(COMMAND git pull WORKING_DIRECTORY ${VCPKG_ROOT})
    else()
        _automate_vcpkg_checkout_pin()
    endif()

    if(NOT EXISTS ${VCPKG_ROOT}/README.md)
        message(FATAL_ERROR "***** FATAL ERROR: Could not clone vcpkg *****")
    endif()

    if(WIN32)
        set(VCPKG_EXEC ${VCPKG_ROOT}/vcpkg.exe)
        set(VCPKG_BOOTSTRAP ${VCPKG_ROOT}/bootstrap-vcpkg.bat)
    else()
        set(VCPKG_EXEC ${VCPKG_ROOT}/vcpkg)
        set(VCPKG_BOOTSTRAP ${VCPKG_ROOT}/bootstrap-vcpkg.sh)
    endif()

    if(NOT EXISTS ${VCPKG_EXEC})
        message("Bootstrapping vcpkg in ${VCPKG_ROOT}")
        execute_process(COMMAND ${VCPKG_BOOTSTRAP} WORKING_DIRECTORY ${VCPKG_ROOT})
    endif()

    if(NOT EXISTS ${VCPKG_EXEC})
        message(FATAL_ERROR "***** FATAL ERROR: Could not bootstrap vcpkg *****")
    endif()
   
endmacro()

# Installs the list of packages given as parameters using Vcpkg
macro(vcpkg_install_packages)
    
    # Need the given list to be space-separated
    #string (REPLACE ";" " " PACKAGES_LIST_STR "${ARGN}")

    message(STATUS "Installing/Updating the following vcpkg-packages: ${PACKAGES_LIST_STR}")

    if (VCPKG_TRIPLET)
        set(ENV{VCPKG_DEFAULT_TRIPLET} "${VCPKG_TRIPLET}")
    endif()

    execute_process(
        COMMAND ${VCPKG_EXEC} install ${ARGN}
        WORKING_DIRECTORY ${VCPKG_ROOT}
        )
endmacro()
    
# MIT License
# 
# Copyright (c) 2019 REGoth-project
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
