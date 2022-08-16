include(ExternalProject)

if(NOT DEFINED EJDB2_URL)
  set(EJDB2_URL
      https://github.com/Softmotions/ejdb/archive/refs/heads/master.zip)
endif()

set(BYPRODUCT
    "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/libejdb2-2.a"
    "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/libiwnet-1.a"
    "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/libiowow-1.a")

set(CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}
    -DINSTALL_FLAT_SUBPROJECT_INCLUDES=ON
    -DBUILD_SHARED_LIBS=OFF
    -DBUILD_EXAMPLES=OFF)

set(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} ${CMAKE_INSTALL_PREFIX})

# In order to properly pass owner project CMAKE variables than contains
# semicolons, we used a specific separator for 'ExternalProject_Add', using the
# LIST_SEPARATOR parameter. This allows building fat binaries on macOS, etc.
# (ie: -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64") Otherwise, semicolons get
# replaced with spaces and CMake isn't aware of a multi-arch setup.
set(SSUB "^^")

foreach(
  extra
  ANDROID_ABI
  ANDROID_PLATFORM
  ARCHS
  CMAKE_C_COMPILER
  CMAKE_FIND_ROOT_PATH
  CMAKE_OSX_ARCHITECTURES
  CMAKE_TOOLCHAIN_FILE
  ENABLE_ARC
  ENABLE_BITCODE
  ENABLE_STRICT_TRY_COMPILE
  ENABLE_VISIBILITY
  PLATFORM
  TEST_TOOL_CMD)
  if(DEFINED ${extra})
    # Replace occurences of ";" with our custom separator and append to
    # CMAKE_ARGS
    string(REPLACE ";" "${SSUB}" extra_sub "${${extra}}")
    list(APPEND CMAKE_ARGS "-D${extra}=${extra_sub}")
  endif()
endforeach()

message("EJDB CMAKE_ARGS: ${CMAKE_ARGS}")

ExternalProject_Add(
  extern_ejdb2
  URL ${EJDB2_URL}
  DOWNLOAD_NAME ejdb2.zip
  TIMEOUT 360
  PREFIX ${CMAKE_BINARY_DIR}
  BUILD_IN_SOURCE OFF
  #DOWNLOAD_EXTRACT_TIMESTAMP ON
  UPDATE_COMMAND ""
  LIST_SEPARATOR "${SSUB}"
  CMAKE_ARGS ${CMAKE_ARGS}
  BUILD_BYPRODUCTS ${BYPRODUCT})

add_library(IOWOW::static STATIC IMPORTED GLOBAL)
set_target_properties(
  IOWOW::static
  PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C"
             IMPORTED_LOCATION
             "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/libiowow-1.a"
             IMPORTED_LINK_INTERFACE_LIBRARIES "Threads::Threads;m")

add_library(IWNET::static STATIC IMPORTED GLOBAL)
set_target_properties(
  IWNET::static
  PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C"
             IMPORTED_LOCATION
             "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/libiwnet-1.a"
             IMPORTED_LINK_INTERFACE_LIBRARIES "IOWOW::static")

add_library(EJDB2::static STATIC IMPORTED GLOBAL)
set_target_properties(
  EJDB2::static
  PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C"
             IMPORTED_LOCATION
             "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/libejdb2-2.a"
             IMPORTED_LINK_INTERFACE_LIBRARIES "IWNET::static")

add_dependencies(IOWOW::static extern_ejdb2)
add_dependencies(IWNET::static extern_ejdb2)
add_dependencies(EJDB2::static extern_ejdb2)
