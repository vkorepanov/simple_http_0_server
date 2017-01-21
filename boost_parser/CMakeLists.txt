cmake_minimum_required (VERSION 2.8)

set(BOOST_PARSER_SRCS
    request.hpp
    request_parser.cpp
    request_parser.hpp
    )

add_library(BoostParserLib ${BOOST_PARSER_SRCS})
