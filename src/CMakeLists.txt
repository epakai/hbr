# hbr - main executable
add_executable(hbr hbr.c keyfile.c out_options.c hb_options.c)

# install hbr executable
install(
    PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hbr ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hbr-gen
	DESTINATION bin
	)

# CUnit dependency via pkg-config
find_package(PkgConfig)
pkg_check_modules(GLIB_PKG glib-2.0)

if (NOT GLIB_PKG_FOUND)
    message(WARNING "glib-2.0 not found")
endif()
include_directories(${GLIB_PKG_INCLUDE_DIRS})

pkg_check_modules(PC_CUNIT cunit)

if(NOT PKG_CONFIG_FOUND OR NOT PC_CUNIT_FOUND)
	message(WARNING "pkg-config or CUnit not found: tests target disabled")
endif()

if(PC_CUNIT_FOUND)
	find_path(CUNIT_INCLUDE_DIR NAMES CUnit/CUnit.h HINTS ${PC_CUNIT_INCLUDE_DIRS})
	find_library(CUNIT_LIBRARY NAMES cunit HINTS ${PC_CUNIT_LIBRARY_DIRS})
	include_directories(${CUNIT_INCLUDE_DIR})

	# test_hbr - test executable
	add_executable(test_hbr EXCLUDE_FROM_ALL test_hbr.c keyfile.c out_options.c hb_options.c)
	target_link_libraries(test_hbr ${CUNIT_LIBRARY})

	# run tests 
	add_custom_target (
		# Run unit test executable
		unit_test
		DEPENDS test_hbr
		COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_hbr
		)
	add_custom_target (
		tests
		DEPENDS unit_test test_gen
		)
endif()
