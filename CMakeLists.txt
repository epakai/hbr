cmake_minimum_required (VERSION 2.8.8)
project (hbr C)
set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_subdirectory(src)

find_program(DOXYGEN doxygen)
find_program(DOT dot)
add_custom_target ( doc
	COMMAND echo INPUT = ${CMAKE_CURRENT_SOURCE_DIR}/src/ ${CMAKE_CURRENT_BINARY_DIR}/ > doxyfile.inc
	COMMAND echo PROJECT_NAME = ${PROJECT_NAME} >> doxyfile.inc
	COMMAND echo FILE_PATTERNS = *.h *.c *.md >> doxyfile.inc
	COMMAND ${DOXYGEN} ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.mk
	COMMAND ${CMAKE_COMMAND} -E remove doxyfile.inc
	)
# remove docs on clean command
set_directory_properties(
	PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "html"
	)

ADD_DEPENDENCIES(doc readme_md)
