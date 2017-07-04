# xxd command to build hex array of the XML Schema
find_program(XXD xxd)
add_custom_command(
	OUTPUT hbr_xml_schema.h
	DEPENDS hbr.xsd
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/hbr.xsd hbr.xsd
	COMMAND ${XXD} -i hbr.xsd hbr_xml_schema.h
	COMMENT "Converting XML Schema (hbr.xsd) to static array (hbr_xml_schema.h)")
add_custom_target(hbr_xml_schema_h
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/hbr_xml_schema.h
	)

# libxml-2.0 depends
find_package(LibXml2)
include_directories(${LIBXML2_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# hbr - main executable
add_executable(hbr hbr.c xml.c out_options.c hb_options.c gen_xml.c hbr_xml_schema.h)
target_link_libraries(hbr ${LIBXML2_LIBRARIES})

# install hbr executable
install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hbr
	DESTINATION bin
	)

# CUnit dependency via pkg-config
find_package(PkgConfig)
pkg_check_modules(PC_CUNIT cunit)
find_path(CUNIT_INCLUDE_DIR NAMES CUnit/CUnit.h HINTS ${PC_CUNIT_INCLUDE_DIRS})
find_library(CUNIT_LIBRARY NAMES cunit HINTS ${PC_CUNIT_LIBRARY_DIRS})
include_directories(${CUNIT_INCLUDE_DIR})

# test_hbr - test executable
add_executable(test_hbr EXCLUDE_FROM_ALL test_hbr.c xml.c out_options.c hb_options.c gen_xml.c hbr_xml_schema.h)
target_link_libraries(test_hbr ${LIBXML2_LIBRARIES} ${CUNIT_LIBRARY})

# run tests 
add_custom_target (
	# Test generate XML is valid
	test_gen
	DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hbr
	COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hbr -g 2 -p series -y 1900 > .test-gen.xml
	COMMAND ${LIBXML2_XMLLINT_EXECUTABLE} --schema hbr.xsd .test-gen.xml -o /dev/null
	COMMAND rm .test-gen.xml
	)
add_custom_target (
	# Run unit test executable
	unit_test
	DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_hbr
	COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_hbr
	)
add_custom_target (
	tests
	DEPENDS unit_test test_gen
	)

set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
	"hbr.xsd"
	)
