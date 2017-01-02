#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#include "gen_xml.h"
#include "hb_options.h"
#include "out_options.h"
#include "xml.h"

int init_gen_xml(void) { return 0; }
int clean_gen_xml(void) { return 0; }
void test_parse_gen_opt(void) {}
void test_read_episode_list(void) {}
void test_free_episode_array(void) {}
void test_gen_xml(void) {}

int init_hb_options(void) { return 0; }
int clean_hb_options(void) { return 0; }
void test_hb_options_string(void) {}
void test_get_format(void) {}
void test_get_input_basedir(void) {}
void test_hb_format(void) {}
void test_hb_video_encoder(void) {}
void test_hb_video_quality(void) {}
void test_hb_audio_encoder(void) {}
void test_hb_audio_quality(void) {}
void test_hb_audio_bitrate(void) {}
void test_hb_crop(void) {}
void test_hb_markers(void) {}
void test_hb_anamorphic(void) {}
void test_hb_deinterlace(void) {}
void test_hb_decomb(void) {}
void test_hb_denoise(void) {}
void test_valid_bit_rate(void) {}

int init_out_options(void) { return 0; }
int clean_out_options(void) { return 0; }
void test_out_options_string(void) {}
void test_validate_file_string(void) {}
void test_out_series_output(void) {}
void test_out_movie_output(void) {}
void test_out_input(void) {}
void test_out_dvdtitle(void) {}
void test_out_crop(void) {}
void test_out_chapters(void) {}
void test_out_audio(void) {}
void test_out_subtitle(void) {}

int init_xml(void) { return 0; }
int clean_xml(void) { return 0; }
void test_parse_xml(void) {}
void test_xpath_get_object(void) {}
void test_get_outfile_child_content(void) {}
void test_get_outfile(void) {}
void test_get_outfile_line_number(void) {}
void test_get_outfile_from_episode(void) {}

int main (int argc, char **argv) {
	if (CU_initialize_registry() != CUE_SUCCESS ) {
		fprintf(stderr, "CUnit registry failed to initialize\n");
		return CU_get_error();
	}

	CU_pSuite gen_xml_suite = CU_add_suite("gen_xml", init_gen_xml, clean_gen_xml);
	CU_add_test(gen_xml_suite, "test of gen_xml.c: parse_gen_opt()", test_parse_gen_opt); 
	CU_add_test(gen_xml_suite, "test of gen_xml.c: read_episode_list()", test_read_episode_list);
	CU_add_test(gen_xml_suite, "test of gen_xml.c: free_episode_array()", test_free_episode_array); 
	CU_add_test(gen_xml_suite, "test of gen_xml.c: gen_xml()", test_gen_xml); 

	CU_pSuite hb_options_suite = CU_add_suite("hb_options", init_hb_options, clean_hb_options);
	CU_add_test(hb_options_suite, "test of hb_options.c: hb_options_string()", test_hb_options_string);
	CU_add_test(hb_options_suite, "test of hb_options.c: get_format()", test_get_format);
	CU_add_test(hb_options_suite, "test of hb_options.c: get_input_basedir()", test_get_input_basedir);
	CU_add_test(hb_options_suite, "test of hb_options.c: hb_format()", test_hb_format);
	CU_add_test(hb_options_suite, "test of hb_options.c: hb_video_encoder()", test_hb_video_encoder);
	CU_add_test(hb_options_suite, "test of hb_options.c: hb_video_quality()", test_hb_video_quality);
	CU_add_test(hb_options_suite, "test of hb_options.c: hb_audio_encoder()", test_hb_audio_encoder);
	CU_add_test(hb_options_suite, "test of hb_options.c: hb_audio_quality()", test_hb_audio_quality);
	CU_add_test(hb_options_suite, "test of hb_options.c: hb_audio_bitrate()", test_hb_audio_bitrate);
	CU_add_test(hb_options_suite, "test of hb_options.c: hb_crop()", test_hb_crop);
	CU_add_test(hb_options_suite, "test of hb_options.c: hb_markers()", test_hb_markers);
	CU_add_test(hb_options_suite, "test of hb_options.c: hb_anamorphic()", test_hb_anamorphic);
	CU_add_test(hb_options_suite, "test of hb_options.c: hb_deinterlace()", test_hb_deinterlace);
	CU_add_test(hb_options_suite, "test of hb_options.c: hb_decomb()", test_hb_decomb);
	CU_add_test(hb_options_suite, "test of hb_options.c: hb_denoise()", test_hb_denoise);
	CU_add_test(hb_options_suite, "test of hb_options.c: valid_bit_rate()", test_valid_bit_rate);


	CU_pSuite out_options_suite = CU_add_suite("out_options", init_out_options, clean_out_options);
	CU_add_test(out_options_suite, "test of out_options.c: out_options_string", test_out_options_string);
	CU_add_test(out_options_suite, "test of out_options.c: validate_file_string", test_validate_file_string);
	CU_add_test(out_options_suite, "test of out_options.c: out_series_output", test_out_series_output);
	CU_add_test(out_options_suite, "test of out_options.c: out_movie_output", test_out_movie_output);
	CU_add_test(out_options_suite, "test of out_options.c: out_input", test_out_input);
	CU_add_test(out_options_suite, "test of out_options.c: out_dvdtitle", test_out_dvdtitle);
	CU_add_test(out_options_suite, "test of out_options.c: out_crop", test_out_crop);
	CU_add_test(out_options_suite, "test of out_options.c: out_chapters", test_out_chapters);
	CU_add_test(out_options_suite, "test of out_options.c: out_audio", test_out_audio);
	CU_add_test(out_options_suite, "test of out_options.c: out_subtitle", test_out_subtitle);


	CU_pSuite xml_suite = CU_add_suite("xml", init_xml, clean_xml);
	CU_add_test(xml_suite, "test of xml.c: parse_xml", test_parse_xml);
	CU_add_test(xml_suite, "test of xml.c: xpath_get_object", test_xpath_get_object);
	CU_add_test(xml_suite, "test of xml.c: get_outfile_child_content", test_get_outfile_child_content);
	CU_add_test(xml_suite, "test of xml.c: get_outfile", test_get_outfile);
	CU_add_test(xml_suite, "test of xml.c: get_outfile_line_number", test_get_outfile_line_number);
	CU_add_test(xml_suite, "test of xml.c: get_outfile_from_episode", test_get_outfile_from_episode);

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();

	return CU_get_error();
}



