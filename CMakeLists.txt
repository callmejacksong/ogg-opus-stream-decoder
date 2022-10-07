PROJECT(opus_stream_decoder)
CMAKE_MINIMUM_REQUIRED(VERSION 3.15)


# ADD_SUBDIRECTORY(src)
INCLUDE_DIRECTORIES(src libogg-1.3.5/include opus-1.3.1/include opusfile-0.12/include opusfile-0.12/src )
set(source_file src/main.cc src/stream_opus_decoder.cc opusfile-0.12/src/http.c opusfile-0.12/src/info.c opusfile-0.12/src/internal.c
opusfile-0.12/src/opusfile.c
opusfile-0.12/src/stream.c
opusfile-0.12/src/wincerts.c
)

LINK_DIRECTORIES(/usr/local/lib)
set(third_libs 
-lopus
-logg
)
add_compile_options(-g)

ADD_EXECUTABLE(main ${source_file})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})


set_property(TARGET main PROPERTY CXX_STANDARD 11)

TARGET_LINK_LIBRARIES(main ${third_libs})
