include(ExternalProject)

function(fetch_jemalloc jemlloc_path)
  if(NOT EXISTS ${CMAKE_BINARY_DIR}/deps/jemalloc)
    set(JEMALLOC_SOURCE_DIR ${jemlloc_path})
    execute_process(
      COMMAND ${JEMALLOC_SOURCE_DIR}/autogen.sh
      WORKING_DIRECTORY ${JEMALLOC_SOURCE_DIR}
    )

    execute_process(
      COMMAND ${JEMALLOC_SOURCE_DIR}/configure
        --prefix=${CMAKE_BINARY_DIR}/deps/jemalloc 
        --with-jemalloc-prefix=je_
      WORKING_DIRECTORY ${JEMALLOC_SOURCE_DIR}
    )

    execute_process(
      COMMAND make -j4
      WORKING_DIRECTORY ${JEMALLOC_SOURCE_DIR}
    )

    execute_process(
      COMMAND make install
      WORKING_DIRECTORY ${JEMALLOC_SOURCE_DIR}
    )

    execute_process(
      COMMAND git clean -xdf
      WORKING_DIRECTORY ${JEMALLOC_SOURCE_DIR}
    )

    set(JEMALLOC_INCLUDE_DIR ${CMAKE_BINARY_DIR}/deps/jemalloc/include)
    set(JEMALLOC_LIBRARY_DIR ${CMAKE_BINARY_DIR}/deps/jemalloc/lib)
  endif()
endfunction()
