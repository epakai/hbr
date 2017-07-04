cmake_minimum_required (VERSION 2.8.8)
project (hbr C)
set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_subdirectory(src)

# documentation build
find_program(MARKDOWN markdown)
add_custom_command( 
	OUTPUT README.md
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/README 
	COMMAND ${MARKDOWN} ${CMAKE_CURRENT_SOURCE_DIR}/README > README.md
	)
add_custom_target(readme_md
	DEPENDS README.md
	)

find_program(DOXYGEN doxygen)
if(${DOXYGEN} STREQUAL "DOXYGEN-NOTFOUND")
	message(WARNING "${DOXYGEN}: documentation cannot be generated (install doxygen)")
else()
	if(${DOT} STREQUAL "DOT-NOTFOUND")
		file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/doxyfile.inc "HAVE_DOT = NO\n")
		message(WARNING "${DOT}: documentation will not include graphs (install graphviz)")
	else()
		file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/doxyfile.inc "HAVE_DOT = YES\n")
	endif()
	if(MARKDOWN)
		ADD_DEPENDENCIES(doc readme_md)
	endif()
	if(${MARKDOWN} STREQUAL "MARKDOWN-NOTFOUND")
		message(WARNING "${MARKDOWN}: documentation will not include README (install markdown)")
	endif()
endif()
find_program(DOT dot)
add_custom_target ( doc
	COMMAND echo INPUT = ${CMAKE_CURRENT_SOURCE_DIR}/src/ ${CMAKE_CURRENT_BINARY_DIR}/ ${CMAKE_CURRENT_BINARY_DIR}/src/ >> doxyfile.inc
	COMMAND echo PROJECT_NAME = ${PROJECT_NAME} >> doxyfile.inc
	COMMAND echo FILE_PATTERNS = *.h *.c *.md >> doxyfile.inc
	COMMAND ${DOXYGEN} ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.mk
	)
ADD_DEPENDENCIES(doc hbr_xml_schema_h)

# remove docs on clean command
set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES 
	"html"
	"doxyfile.inc"
	)
