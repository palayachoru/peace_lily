cmake_minimum_required(VERSION 3.31)

project(PEACELILY VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_COMPILER "clang")         # Set the compiler to use
set(CMAKE_C_STANDARD 11)              # Set the c standard
set(CMAKE_C_STANDARD_REQUIRED ON)     # Strict adherence to chosen standard
set(CMAKE_C_EXTENSIONS OFF)           # Stick strictly to ISO C rules

# Force cmake to generate compile_commands.json (contain entire compilation stages)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Check for CUnit testing library installation
find_library(CUNIT_LIB cunit)
if(NOT CUNIT_LIB)
  message(FATAL_ERROR "CUnit library not found! Please install it.")
endif()


# Declare the target library (source files will be added later)
add_library(plily STATIC)

# Locate the header files
target_include_directories(plily
  PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src         # for internal headers
)


# Specify the ASan (AddressSanitizer) option
option(PL_ASAN "Enable Address/Undefined sanitizers" OFF)
if (PL_ASAN AND NOT MSVC)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=address,undefined)
endif()


# Specify the compiler flags
if(MSVC)
  target_compile_options(plily PRIVATE /W4 /permissive-)
else()
  target_compile_options(plily PRIVATE
    -Wall               # Enable all standard warning
    -Wextra             # Enable extra useful warning
    -Wpedantic          # Warn if C standard is not followed
    -Wshadow            # Warn about variable shadowing
    -Wpointer-arith     # Warn about invalid pointer arithmetic
    -Wundef             # Warn if undefined identifier is evaluated
    -Wcast-align        # Warn if any unsafe pointer casting happens
    -Wstrict-prototypes # Warn if function declared without specifying arg types
    -Wconversion        # Warn if implicite convertion will lose pricision
    -Wunreachable-code  # Warn dead code
    -Werror             # All warnings will be treated as error

    -funsigned-char           # Treat char as unsigned char
    -fno-strict-overflow      # Detect integer overflow
    -fstack-protector-strong  # Enable Stack based buffer overflow checks at Runtime
  )
endif()


# Take all data structures under src
add_subdirectory(src)


enable_testing()
add_subdirectory(tests)



# NOTES: To run with ASan options
# cmake -B build_asan -DPL_ASAN=ON -DCMAKE_BUILD_TYPE=Debug  # generate build files
# cmake --build build_asan                                   # compile
# ctest --test-dir build_asan --output-on-failure            # run test with ASan

# Need symbolizer to map -> hex offset to source code file line number
# sudo dnf install llvm-symbolizer
