cmake_minimum_required (VERSION 3.2)

project (remoting)

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

if (WIN32)
    target_link_libraries(server
      "${RPCLIB}/lib/rpc.lib"
    )
else ()
    target_link_libraries(server
      "${RPCLIB}/lib/librpc.a"
      ${CMAKE_DL_LIBS}
    )
endif ()

add_custom_command(TARGET server POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
  "$<TARGET_FILE:server>"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../fmpy/remoting"
)

# client
add_library(client SHARED
  ../fmpy/c-code/fmi2Functions.h
  ../fmpy/c-code/fmi2FunctionTypes.h
  ../fmpy/c-code/fmi2TypesPlatform.h
  remoting.h
  client.cpp
)

target_include_directories(client PRIVATE
  "${RPCLIB}/include"
  ..
  ../fmpy/c-code
)

target_link_libraries(client
  "${RPCLIB}/lib/rpc.lib"
)

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
