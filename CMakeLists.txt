cmake_minimum_required (VERSION 2.8.8)
project (hbr C)
set (VERSION_MAJOR 0)
set (VERSION_MINOR 0)

# xxd command to build hex array of the XML Schema
find_program(XXD xxd)
add_custom_command(
	OUTPUT xsd.h
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/hbr.xsd hbr.xsd
	COMMAND ${XXD} -i hbr.xsd xsd.h
	DEPENDS hbr.xsd
	COMMENT "Converting XML Schema (hbr.xsd) to static array (xsd.h)")

# libxml-2.0 depends
find_package(LibXml2)
include_directories(${LIBXML2_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# hbr - main executable
add_executable(hbr hbr.c xml.c out_options.c hb_options.c gen_xml.c xsd.h)
target_link_libraries(hbr ${LIBXML2_LIBRARIES})

# CUnit dependency via pkg-config
find_package(PkgConfig)
pkg_check_modules(PC_CUNIT cunit)
find_path(CUNIT_INCLUDE_DIR NAMES CUnit/CUnit.h HINTS ${PC_CUNIT_INCLUDE_DIRS})
find_library(CUNIT_LIBRARY NAMES cunit HINTS ${PC_CUNIT_LIBRARY_DIRS})
include_directories(${CUNIT_INCLUDE_DIR})

# test_hbr - test executable
add_executable(test_hbr EXCLUDE_FROM_ALL test_hbr.c xml.c out_options.c hb_options.c gen_xml.c xsd.h)
target_link_libraries(test_hbr ${LIBXML2_LIBRARIES} ${CUNIT_LIBRARY})

# documentation build
find_program(MARKDOWN markdown)
add_custom_command( 
	OUTPUT README.md
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/README 
	COMMAND ${MARKDOWN} ${CMAKE_CURRENT_SOURCE_DIR}/README > README.md
	)

find_program(DOXYGEN doxygen)
add_custom_target ( doc
	DEPENDS README.md
	BYPRODUCTS doxyfile.inc html/*
	COMMAND echo INPUT = ./ > doxyfile.inc
	COMMAND echo PROJECT_NAME = ${PROJECT_NAME} >> doxyfile.inc
	COMMAND echo FILE_PATTERNS = *.h *.c *.md >> doxyfile.inc
	COMMAND ${DOXYGEN} ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.mk
	)

# run tests 
add_custom_target (
	# Test generate XML is valid
	test_gen
	DEPENDS hbr
	COMMAND ./hbr -g 2 -p series -y 1900 > .test-gen.xml
	COMMAND ${LIBXML2_XMLLINT_EXECUTABLE} --schema hbr.xsd .test-gen.xml -o /dev/null
	COMMAND rm .test-gen.xml
	)
add_custom_target (
	# Run unit test executable
	unit_test
	DEPENDS test_hbr
	COMMAND ./test_hbr
	)
add_custom_target (
	tests
	DEPENDS unit_test test_gen
	)
