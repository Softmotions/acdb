if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} \
  -std=gnu11 \
  -fPIC \
  -Wall \
  -Wextra \
  -Wfatal-errors \
  -Wno-implicit-fallthrough \
  -Wno-missing-braces \
  -Wno-missing-field-initializers \
  -Wno-sign-compare \
  -Wno-unknown-pragmas \
  -Wno-unused-function \
  -Wno-unused-parameter")

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG
    "-O0 -g -ggdb \
    -Werror \
    -Wno-unused-variable \
    -DDEBUG -D_DEBUG -UNDEBUG")

set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,-s")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} -g")
set(CMAKE_C_FLAGS_RELEASEWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})

find_package(Threads REQUIRED CMAKE_THREAD_PREFER_PTHREAD)

include(AddIWNET)
set(LINK_LIBS IWNET::static)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/include)
add_definitions(-D_LARGEFILE_SOURCE)

file(GLOB ALL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
               ${CMAKE_BINARY_DIR}/include/config.h)

add_executable(acdb ${ALL_SRC})
target_link_libraries(acdb ${LINK_LIBS})
set_target_properties(acdb PROPERTIES COMPILE_FLAGS "-DIW_EXEC")

add_dependencies(acdb generated)

install(
  TARGETS acdb
  FRAMEWORK DESTINATION ${FRAMEWORK_INSTALL_DIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

if(BUILD_TESTS)
  add_library(acdb_s ${ALL_SRC})
  target_link_libraries(acdb_s PUBLIC ${LINK_LIBS})
  add_subdirectory(tests)
endif()
