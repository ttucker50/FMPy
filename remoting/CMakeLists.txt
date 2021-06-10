cmake_minimum_required (VERSION 3.2)

project (remoting)

# set(CMAKE_BUILD_TYPE Debug)

set(RPCLIB "" CACHE STRING "RPCLIB installation directory")

if (MSVC)
  # link statically against the the Visual C runtime
  set(variables
    CMAKE_C_FLAGS_DEBUG
    CMAKE_C_FLAGS_MINSIZEREL
    CMAKE_C_FLAGS_RELEASE
    CMAKE_C_FLAGS_RELWITHDEBINFO
    CMAKE_CXX_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_MINSIZEREL
    CMAKE_CXX_FLAGS_RELEASE
    CMAKE_CXX_FLAGS_RELWITHDEBINFO
  )

  foreach(variable ${variables})
    if(${variable} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
    endif()
  endforeach()

  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

endif ()

# server
add_executable(server
  ../fmpy/c-code/fmi2Functions.h
  ../fmpy/c-code/fmi2FunctionTypes.h
  ../fmpy/c-code/fmi2TypesPlatform.h
  remoting.h
  server.cpp
)

target_include_directories(server PUBLIC
  ..
  ../fmpy/c-code
  "${RPCLIB}/include"
)

#if (UNIX)
    target_compile_options(server PRIVATE "-pthread")
#endif ()

if (WIN32)
    target_link_libraries(server
      "${RPCLIB}/lib/rpc.lib"
    )
else ()
    target_link_libraries(server
      "${RPCLIB}/lib/librpc.a"
      ${CMAKE_DL_LIBS}
      pthread
    )
endif ()

# add_custom_command(TARGET server POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
#  "$<TARGET_FILE:server>"
#  "${CMAKE_CURRENT_SOURCE_DIR}/../../fmpy/remoting"
# )


set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

# client
add_library(client SHARED
  ../fmpy/c-code/fmi2Functions.h
  ../fmpy/c-code/fmi2FunctionTypes.h
  ../fmpy/c-code/fmi2TypesPlatform.h
  remoting.h
  client.cpp
)

set_property(TARGET client PROPERTY CXX_STANDARD 11)
set_target_properties(client PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(client PROPERTIES LINK_SEARCH_END_STATIC 1)

set_target_properties(client PROPERTIES
    PREFIX ""
)

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
find_package(Threads REQUIRED)

target_include_directories(client PRIVATE
  "${RPCLIB}/include"
  ..
  ../fmpy/c-code
)

if (WIN32)
    target_link_libraries(client
      "${RPCLIB}/lib/rpc.lib"
    )
else ()
    target_link_libraries(client
      "${RPCLIB}/lib/librpc.a"
      Threads::Threads
    )
endif ()

# client test
add_executable(client_test
  client_test.cpp
)

target_include_directories(client_test PUBLIC
  ../fmpy/c-code
)

target_link_libraries(client_test
  ${CMAKE_DL_LIBS}
)
