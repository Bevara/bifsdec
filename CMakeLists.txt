cmake_minimum_required(VERSION 3.7)
project(Filters)

include(filters.cmake)

add_definitions(-fpic)

SET(BIFSDEC_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/dec_bifs.c
)

SET(BIFSDEC_INC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_filter(bifdec
        "${BIFSDEC_SRC}"
        ""
        []
        ""
        "${BIFSDEC_INC}"
        ""
        "1")
