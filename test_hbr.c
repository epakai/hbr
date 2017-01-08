#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#include "gen_xml.h"
#include "hb_options.h"
#include "out_options.h"
#include "xml.h"

int init_gen_xml(void) { return 0; }
int clean_gen_xml(void) { return 0; }
void test_parse_gen_opt(void) { CU_ASSERT(0); }
void test_read_episode_list(void) { CU_ASSERT(0); }
void test_free_episode_array(void) { CU_ASSERT(0); }
void test_gen_xml(void) { CU_ASSERT(0); }

int init_hb_options(void) { return 0; }
int clean_hb_options(void) { return 0; }
void test_hb_options_string(void) { CU_ASSERT(0); }
void test_get_format(void) { CU_ASSERT(0); }
void test_get_input_basedir(void) { CU_ASSERT(0); }
void test_hb_format(void) { CU_ASSERT(0); }
void test_hb_video_encoder(void) { CU_ASSERT(0); }
void test_hb_video_quality(void) { CU_ASSERT(0); }
void test_hb_audio_encoder(void) { CU_ASSERT(0); }
void test_hb_audio_quality(void) { CU_ASSERT(0); }
void test_hb_audio_bitrate(void) { CU_ASSERT(0); }
void test_hb_crop(void) { CU_ASSERT(0); }
void test_hb_markers(void) { CU_ASSERT(0); }
void test_hb_anamorphic(void) { CU_ASSERT(0); }
void test_hb_deinterlace(void) { CU_ASSERT(0); }
void test_hb_decomb(void) { CU_ASSERT(0); }
void test_hb_denoise(void) { CU_ASSERT(0); }
void test_valid_bit_rate(void) { CU_ASSERT(0); }

int init_out_options(void) { return 0; }
int clean_out_options(void) { return 0; }
void test_out_options_string(void) { CU_ASSERT(0); }
void test_validate_file_string(void) { CU_ASSERT(0); }
void test_out_series_output(void) { CU_ASSERT(0); }
void test_out_movie_output(void) { CU_ASSERT(0); }
void test_out_input(void) { CU_ASSERT(0); }
void test_out_dvdtitle(void) { CU_ASSERT(0); }
void test_out_crop(void) { CU_ASSERT(0); }
void test_out_chapters(void) { CU_ASSERT(0); }
void test_out_audio(void) { CU_ASSERT(0); }
void test_out_subtitle(void) { CU_ASSERT(0); }

int init_xml(void) { return 0; }
int clean_xml(void) { return 0; }
void test_parse_xml(void) { CU_ASSERT(0); }
void test_xpath_get_object(void) { CU_ASSERT(0); }
void test_get_outfile_child_content(void) { CU_ASSERT(0); }
void test_get_outfile(void) { CU_ASSERT(0); }
void test_get_outfile_line_number(void) { CU_ASSERT(0); }
void test_get_outfile_from_episode(void) { CU_ASSERT(0); }

int main (int argc, char **argv) {
	if (CU_initialize_registry() != CUE_SUCCESS ) {
		fprintf(stderr, "CUnit registry failed to initialize\n");
		return CU_get_error();
	}
	// Create test suites
	CU_pSuite xml_suite = CU_add_suite("xml", init_xml, clean_xml);
	CU_pSuite gen_xml_suite = CU_add_suite("gen_xml", init_gen_xml, clean_gen_xml);
	CU_pSuite hb_options_suite = CU_add_suite("hb_options", init_hb_options, clean_hb_options);
	CU_pSuite out_options_suite = CU_add_suite("out_options", init_out_options, clean_out_options);
	if ( gen_xml_suite == NULL || hb_options_suite == NULL ||
			xml_suite == NULL || out_options_suite == NULL ) {
		fprintf(stderr, "Failing adding a tets suite\n");
		goto exit;
	}

	// Add tests to xml_suite
	if ( NULL == CU_add_test(xml_suite, "test of xml.c: parse_xml", test_parse_xml) ||
			NULL == CU_add_test(xml_suite, "test of xml.c: xpath_get_object", test_xpath_get_object) ||
			NULL == CU_add_test(xml_suite, "test of xml.c: get_outfile_child_content", test_get_outfile_child_content) ||
			NULL == CU_add_test(xml_suite, "test of xml.c: get_outfile", test_get_outfile) ||
			NULL == CU_add_test(xml_suite, "test of xml.c: get_outfile_line_number", test_get_outfile_line_number) ||
			NULL == CU_add_test(xml_suite, "test of xml.c: get_outfile_from_episode", test_get_outfile_from_episode) ) {
		fprintf(stderr, "Failed adding test to xml_suite\n");
		goto exit;
	}

	// Add tests to gen_xml_suite
	if ( NULL == CU_add_test(gen_xml_suite, "test of gen_xml.c: parse_gen_opt()", test_parse_gen_opt) || 
			NULL == CU_add_test(gen_xml_suite, "test of gen_xml.c: read_episode_list()", test_read_episode_list) ||
			NULL == CU_add_test(gen_xml_suite, "test of gen_xml.c: free_episode_array()", test_free_episode_array) || 
			NULL == CU_add_test(gen_xml_suite, "test of gen_xml.c: gen_xml()", test_gen_xml) ) {
		fprintf(stderr, "Failed adding test to gen_xml_suite\n");
		goto exit;
	}

	// Add tests to hb_options_suite
	if ( NULL == CU_add_test(hb_options_suite, "test of hb_options.c: hb_options_string()", test_hb_options_string) ||
			NULL == CU_add_test(hb_options_suite, "test of hb_options.c: get_format()", test_get_format) ||
			NULL == CU_add_test(hb_options_suite, "test of hb_options.c: get_input_basedir()", test_get_input_basedir) ||
			NULL == CU_add_test(hb_options_suite, "test of hb_options.c: hb_format()", test_hb_format) ||
			NULL == CU_add_test(hb_options_suite, "test of hb_options.c: hb_video_encoder()", test_hb_video_encoder) ||
			NULL == CU_add_test(hb_options_suite, "test of hb_options.c: hb_video_quality()", test_hb_video_quality) ||
			NULL == CU_add_test(hb_options_suite, "test of hb_options.c: hb_audio_encoder()", test_hb_audio_encoder) ||
			NULL == CU_add_test(hb_options_suite, "test of hb_options.c: hb_audio_quality()", test_hb_audio_quality) ||
			NULL == CU_add_test(hb_options_suite, "test of hb_options.c: hb_audio_bitrate()", test_hb_audio_bitrate) ||
			NULL == CU_add_test(hb_options_suite, "test of hb_options.c: hb_crop()", test_hb_crop) ||
			NULL == CU_add_test(hb_options_suite, "test of hb_options.c: hb_markers()", test_hb_markers) ||
			NULL == CU_add_test(hb_options_suite, "test of hb_options.c: hb_anamorphic()", test_hb_anamorphic) ||
			NULL == CU_add_test(hb_options_suite, "test of hb_options.c: hb_deinterlace()", test_hb_deinterlace) ||
			NULL == CU_add_test(hb_options_suite, "test of hb_options.c: hb_decomb()", test_hb_decomb) ||
			NULL == CU_add_test(hb_options_suite, "test of hb_options.c: hb_denoise()", test_hb_denoise) ||
			NULL == CU_add_test(hb_options_suite, "test of hb_options.c: valid_bit_rate()", test_valid_bit_rate) ) {
		fprintf(stderr, "Failed adding test to hb_options_suite\n");
		goto exit;
	}

	// Add tests to out_options_suite
	if ( NULL == CU_add_test(out_options_suite, "test of out_options.c: out_options_string", test_out_options_string) ||
			NULL == CU_add_test(out_options_suite, "test of out_options.c: validate_file_string", test_validate_file_string) ||
			NULL == CU_add_test(out_options_suite, "test of out_options.c: out_series_output", test_out_series_output) ||
			NULL == CU_add_test(out_options_suite, "test of out_options.c: out_movie_output", test_out_movie_output) ||
			NULL == CU_add_test(out_options_suite, "test of out_options.c: out_input", test_out_input) ||
			NULL == CU_add_test(out_options_suite, "test of out_options.c: out_dvdtitle", test_out_dvdtitle) ||
			NULL == CU_add_test(out_options_suite, "test of out_options.c: out_crop", test_out_crop) ||
			NULL == CU_add_test(out_options_suite, "test of out_options.c: out_chapters", test_out_chapters) ||
			NULL == CU_add_test(out_options_suite, "test of out_options.c: out_audio", test_out_audio) ||
			NULL == CU_add_test(out_options_suite, "test of out_options.c: out_subtitle", test_out_subtitle) ) {
		fprintf(stderr, "Failed adding test to out_options_suite\n");
		goto exit;
	}

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
exit:	CU_cleanup_registry();

	return CU_get_error();
}



