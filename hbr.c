#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>
#include <unistd.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

/***************/
/* PROTOTYPES  */
/***************/
xmlDocPtr parse_xml(char *); 
void gen_xml(int outfiles_count, int title, int season, int video_type, char *source, char *year, char *crop, char *name); 
static error_t parse_gen_opt(int , char *, struct argp_state *);
static error_t parse_enc_opt (int , char *, struct argp_state *);

const char *argp_program_version = "hbr 1.0";
const char *argp_program_bug_address = "<joshua.honeycutt@gmail.com>";
/***************/


/*************************************************/
/* OPTION HANDLING                               */
/*************************************************/
static char gen_doc[] = "handbrake runner -- generates a xml template with [NUM] outfile sections";
static char enc_doc[] = "handbrake runner -- runs handbrake with setting from an [XML FILE] with all encoded files placed in [OUTPUT PATH]";
static char gen_args_doc[] = "-g NUM";
static char enc_args_doc[] = "[XML FILE] [OUTPUT PATH]";


static struct argp_option gen_options[] = {
	{"generate", 'g', "NUM",  0, "Generate xml file with NUM outfile sections" },
	{"title",  't', "NUM", 0, "DVD Title number (for discs with a single title)"},
	{"type",   'p', "series|movie", 0, "Type of video"},
	{"source", 's', "FILE", 0, "Source filename (DVD iso file)"},
	{"year",   'y', "YEAR", 0, "Movie Release year"},
	{"crop",   'c', "T:B:L:R", 0, "Pixels to crop, top:bottom:left:right"},
	{"name",   'n', "Name", 0, "Movie or series name"},
	{"season", 'e', "NUM", 0, "Movie season"},
	{"help",   '?', 0, 0, "Give this help list"},
	{ 0 }
};

static struct argp_option enc_options[] = {
	{"in",      'i', "FILE", 0, "handbrake_encode XML File" },
	{"out",     'o', "PATH", 0, "directory for output files" },
	{"episode", 'e', "NUM",  0, "only encodes for entry with matching episode number" },
	{"verbose", 'v', 0,      0, "Produce verbose output" },
	{"help", '?', 0,      OPTION_HIDDEN, "Produce verbose output" },
	{ 0 }
};

struct gen_arguments {
	int generate, title, season, help, video_type;
	char *source, *year, *crop, *name; 
};

struct enc_arguments {
	char *args[2];
	int episode, verbose;
};

static struct argp gen_argp = { gen_options, parse_gen_opt, gen_args_doc, gen_doc};
static struct argp enc_argp = { enc_options, parse_enc_opt, enc_args_doc, enc_doc };
/*************************************************/



int main(int argc, char * argv[]) {
	struct enc_arguments enc_arguments;
	struct gen_arguments gen_arguments;
	gen_arguments.generate = -1;
	gen_arguments.help = 0;
	enc_arguments.verbose = 0;
	enc_arguments.episode = -1;
	gen_arguments.title = 0;
	gen_arguments.season = 0;
	gen_arguments.video_type = -1;
	gen_arguments.source = NULL;
	gen_arguments.year = NULL;
	gen_arguments.crop = NULL; 
	gen_arguments.name = NULL;
	// check for request to generate new xml file
	argp_parse (&gen_argp, argc, argv, ARGP_NO_ARGS|ARGP_IN_ORDER|ARGP_NO_EXIT|ARGP_NO_HELP, 0, &gen_arguments);
	if ( gen_arguments.generate > 0 ) {
		gen_xml(gen_arguments.generate, gen_arguments.title ?: 1, gen_arguments.season ?: 1, gen_arguments.video_type,  \
				gen_arguments.source ?: "", gen_arguments.year ?: "", gen_arguments.crop ?: "0:0:0:0", gen_arguments.name ?: ""); 
	} else if (gen_arguments.help == 0) {
		// parse normal options
		argp_parse (&enc_argp, argc, argv, ARGP_NO_HELP, 0, &enc_arguments);
		xmlDocPtr hb_encode = parse_xml(enc_arguments.args[0]);
		xmlNode *root_element = xmlDocGetRootElement(hb_encode);


		xmlFreeDoc(hb_encode);
	}

	return 0;
}

static error_t parse_gen_opt(int key, char *arg, struct argp_state *state) {
	struct gen_arguments *gen_arguments = state->input;
	switch (key) {
		case '?':
			gen_arguments->help = 1;
			argp_help(&enc_argp, stdout, ARGP_HELP_SHORT_USAGE, "hbr");
			argp_help(&enc_argp, stdout, ARGP_HELP_PRE_DOC, "hbr");
			argp_help(&enc_argp, stdout, ARGP_HELP_LONG, "hbr");

			printf("\nUsage: hbr -g NUM [OPTION...]\n");
			argp_state_help(state, stdout, ARGP_HELP_PRE_DOC);
			argp_state_help(state, stdout, ARGP_HELP_LONG);
			break;
		case 'g':
			if ( atoi(arg) > 0 )
				gen_arguments->generate = atoi(arg);
			break;
		case 's':
			gen_arguments->source = arg;
			break;
		case 'p':
			if (strncmp(arg, "movie", 5) == 0)
				gen_arguments->video_type = 0;
			if (strncmp(arg, "series", 6) == 0)
				gen_arguments->video_type = 1;
			break;
		case 't':
			if ( atoi(arg) > 0 )
				gen_arguments->title = atoi(arg);
			break;
		case 'y':
			gen_arguments->year = arg;
			break;
		case 'c':
			gen_arguments->crop = arg;
			break;
		case 'n':
			gen_arguments->name = arg;
			break;
		case 'e':
			if ( atoi(arg) > 0 )
				gen_arguments->season = atoi(arg);
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static error_t parse_enc_opt (int key, char *arg, struct argp_state *state) {
	struct enc_arguments *enc_arguments = state->input;

	switch (key) {
		case 'v':
			enc_arguments->verbose = 1;
			break;
		case 'e':
			if ( atoi(arg) > 0 )
				enc_arguments->episode =  atoi(arg);
			break;
		case '?':
			break;
		case ARGP_KEY_ARG:
			if (state->arg_num >= 2)
				/* Too many arguments. */
				argp_usage (state);
			enc_arguments->args[state->arg_num] = arg;
			break;
		case ARGP_KEY_END:
			if (state->arg_num < 2)
				/* Not enough arguments. */
				argp_usage (state);
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

xmlDocPtr parse_xml(char *infile) {
	xmlParserCtxtPtr ctxt; /* the parser context */
	xmlDocPtr doc; /* the resulting document tree */

	/* create a parser context */
	ctxt = xmlNewParserCtxt();
	if (ctxt == NULL) {
		fprintf(stderr, "Failed to allocate parser context\n");
		exit(1);
	}
	/* parse the file, activating the DTD validation option */
	doc = xmlCtxtReadFile(ctxt, infile, NULL, XML_PARSE_DTDVALID);
	/* check if parsing suceeded */
	if (doc == NULL) {
		fprintf(stderr, "Failed to parse %s\n", infile);
		exit(1);
	} else {
		/* check if validation suceeded */
		if (ctxt->valid == 0) {
			fprintf(stderr, "Failed to validate %s\n", infile);
			exit(1);
		}
	}
	/* free up the parser context */
	xmlFreeParserCtxt(ctxt);
	return doc;
}

void gen_xml(int outfiles_count, int title, int season, int video_type, char *source, char *year, char *crop, char *name) {
	printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	printf("<!DOCTYPE handbrake_encode SYSTEM \"handbrake_encode.dtd\">\n");
	printf("\t<handbrake_encode>\n <handbrake_options \n" \
			"\t\t\tvideo_encoder=\"x264\"\n \t\t\tquality=\"18\"\n \t\t\taudio_encoder=\"fdk_aac\" \n \t\t\taudio_bitrate=\"192\" \n" \
			"\t\t\taudio_quality=\"\"\n \t\t\tcrop=\"%s\"\n \t\t\tanamorphic=\"loose\"\n \t\t\tdeinterlace=\"none\"\n" \
			"\t\t\tdecomb=\"default\"\n \t\t\tdenoise=\"none\"\n />", crop);
	int i;
	for (i = 0; i< outfiles_count; i++) {
		if ( video_type == 0) {
			printf("\t<outfile>\n \t\t<type>movie</type>");
		} else if ( video_type == 1) {
			printf("\t<outfile>\n \t\t<type>series</type>");
		} else {
			printf("\t<outfile>\n \t\t<type></type>");
		}
		if(i == 0) {
			printf("<!-- type may be series or movie -->");
		}
		printf("\n\t\t<source>\n \t\t\t<filename>%s</filename>\n", source);
		printf("\t\t\t<dvdtitle>%d</dvdtitle>\n \t\t</source>\n", title);
		if(i == 0) {
			printf("\t\t<!-- filename depends on outfile type -->\n" \
					"\t\t<!-- series filename: \"<name> - s<season>e<episode_number> - <episode_name>\" -->\n" \
					"\t\t<!-- movie filename: \"<name> (<year>)\" -->\n");
		}
		printf("\t\t<name>%s</name>\n \t\t<year>%s</year>\n \t\t<season>%d</season>\n" \
				"\t\t<episode_number></episode_number>\n \t\t<episode_name></episode_name>\n", name, year, season);

		printf("\t\t<chapters></chapters>");
		if(i == 0) {
			printf(" <!-- specified as a range \"1-3\" or single chapter \"3\" -->");
		}
		printf("\n\t\t<audio></audio>");
		if(i == 0) {
			printf(" <!-- comma separated list of audio tracks -->");
		}
		printf("\n\t\t<audio_names></audio_names>");
		if(i == 0) {
			printf(" <!-- comma separated list of track names if defaults are bad -->");
		}
		printf("\n\t\t<subtitle></subtitle>");
		if(i == 0) {
			printf(" <!-- comma separated list of subtitle tracks -->");
		}
		printf("\n\t</outfile>\n");
	} 
	printf("</handbrake_encode>\n");
	return;
}
