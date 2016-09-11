#include <stdio.h>
#include <stdlib.h>
#include <argp.h>
#include <unistd.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

/***************/
/* PROTOTYPES  */
/***************/
xmlDocPtr parse_xml(char *); 
static error_t parse_gen_opt(int , char *, struct argp_state *);
static error_t parse_enc_opt (int , char *, struct argp_state *);

const char *argp_program_version = "hbr 1.0";
const char *argp_program_bug_address = "<joshua.honeycutt@gmail.com>";



static char gen_doc[] = "handbrake runner -- generates a xml template with [NUM] outfile sections";
static char enc_doc[] = "handbrake runner -- runs handbrake with setting from an [XML FILE] with all encoded files placed in [OUTPUT PATH]";
static char gen_args_doc[] = "-g NUM";
static char enc_args_doc[] = "[XML FILE] [OUTPUT PATH]";


static struct argp_option gen_options[] = {
	{"generate", 'g', "NUM",  0, "Generate xml file with NUM outfile sections" },
	{"title", 't', "NUM", 0, "DVD Title number (for discs with a single title)"},
	{"source", 's', "FILE", 0, "Source filename (DVD iso file)"},
	{"year", 'y', "YEAR", 0, "Movie Release year"},
	{"crop", 'c', "T:B:L:R", 0, "Pixels to crop, top:bottom:left:right"},
	{"name", 'n', "Name", 0, "Movie or series name"},
	{"season", 'e', "NUM", 0, "Movie season"},
	{"help", '?', 0, 0, "Give this help list"},
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
	int generate, title, season, help;
	char *source, *year, *crop, *name; 
};

struct enc_arguments {
	char *args[2];
	int episode, verbose;
};

static struct argp gen_argp = { gen_options, parse_gen_opt, gen_args_doc, gen_doc};
static struct argp enc_argp = { enc_options, parse_enc_opt, enc_args_doc, enc_doc };
	



int main(int argc, char * argv[]) {
	struct enc_arguments enc_arguments;
	struct gen_arguments gen_arguments;
	gen_arguments.generate = -1;
	gen_arguments.help = 0;
	enc_arguments.verbose = 0;
	enc_arguments.episode = -1;
	// check for request to generate new xml file
	argp_parse (&gen_argp, argc, argv, ARGP_NO_ARGS|ARGP_IN_ORDER|ARGP_NO_EXIT|ARGP_NO_HELP, 0, &gen_arguments);
	if ( gen_arguments.generate > 0 ) {
		//TODO: generate xml files
		printf("Got generate option: %d\n", gen_arguments.generate);
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
