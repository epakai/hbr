/*
 * hbr - handbrake runner
 * Copyright (C) 2016 Joshua Honeycutt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "xml.h"

xmlDocPtr parse_xml(char *infile)
{
	xmlParserCtxtPtr ctxt; // the parser context
	xmlDocPtr doc; // the resulting document tree

	// create a parser context
	ctxt = xmlNewParserCtxt();
	if (ctxt == NULL) {
		fprintf(stderr, "Failed to allocate parser context\n");
		exit(1);
	}
	// parse the file, activating the DTD validation option
	doc = xmlCtxtReadFile(ctxt, infile, NULL, XML_PARSE_DTDVALID);
	// check if parsing suceeded
	if (doc == NULL) {
		fprintf(stderr, "Failed to parse %s\n", infile);
		xmlFreeParserCtxt(ctxt);
		exit(1);
	} else {
		// check if validation suceeded
		if (ctxt->valid == 0) {
			fprintf(stderr, "Failed to validate %s\n", infile);
			xmlFreeParserCtxt(ctxt);
			exit(1);
		}
	}
	// free up the parser context
	xmlFreeParserCtxt(ctxt);
	return doc;
}
xmlXPathObjectPtr xpath_get_object(xmlDocPtr doc, xmlChar *search_xpath)
{
	xmlXPathContextPtr xpath_context = xmlXPathNewContext(doc);
	if (xpath_context == NULL) {
		fprintf(stderr, "Unable to create XPath context\n");
		return NULL;
	} else {
		xmlXPathObjectPtr outfile_obj = xmlXPathEvalExpression(search_xpath, xpath_context);
		if (outfile_obj == NULL) {
			fprintf(stderr, "Unable to evaluate xpath expression \"%s\"\n", search_xpath);
			xmlXPathFreeContext(xpath_context);
			xmlFree(search_xpath);
			return NULL;
		}
		xmlXPathFreeContext(xpath_context);
		return outfile_obj;
	}
}

xmlChar* xpath_get_outfile_child_content(xmlDocPtr doc, int out_count, xmlChar *child)
{
	// build xpath like: "/handbrake_encode/outfile[3]/child"
	xmlChar *outfile_xpath = xmlCharStrdup("/handbrake_encode/outfile[");
	xmlChar out_count_string[35];
	snprintf((char *) out_count_string, 33, "%d", out_count);
	outfile_xpath = xmlStrncat(outfile_xpath, out_count_string, xmlStrlen(out_count_string));
	outfile_xpath = xmlStrncat(outfile_xpath, (const xmlChar *) "]/", 2);
	outfile_xpath = xmlStrncat(outfile_xpath, child, xmlStrlen(child));

	xmlXPathObjectPtr outfile_obj = xpath_get_object(doc, outfile_xpath);
	xmlFree(outfile_xpath);
	if (outfile_obj != NULL) {
		// get child's content
		xmlChar *temp = xmlNodeGetContent((xmlNode *) outfile_obj->nodesetval->nodeTab[0]);
		// copy content so we can free the XPath Object
		xmlChar *content = xmlStrdup(temp);
		xmlFree(temp);
		xmlXPathFreeObject(outfile_obj);
		return content;
	} else {
		return NULL;
	}
}

xmlNode* xpath_get_outfile(xmlDocPtr doc, int out_count)
{
	// build xpath like: "/handbrake_encode/outfile[3]"
	xmlChar *outfile_xpath = xmlCharStrdup("/handbrake_encode/outfile[");
	xmlChar out_count_string[33];
	snprintf((char *) out_count_string, 33, "%d", out_count);
	outfile_xpath = xmlStrncat(outfile_xpath, out_count_string, xmlStrlen(out_count_string));
	outfile_xpath = xmlStrncat(outfile_xpath, (const xmlChar *) "]", 1);
	xmlXPathObjectPtr outfile_obj = xpath_get_object(doc, outfile_xpath);
	// free other libxml structures
	xmlFree(outfile_xpath);
	if (outfile_obj != NULL) {
		xmlNodePtr outfile_node = outfile_obj->nodesetval->nodeTab[0];
		xmlXPathFreeObject(outfile_obj);
		return outfile_node;
	} else {
		return NULL;
	}
}

long int xpath_get_outfile_line_number(xmlDocPtr doc, int out_count, xmlChar *child)
{
	xmlChar *outfile_xpath = xmlCharStrdup("/handbrake_encode/outfile[");
	xmlChar out_count_string[33];
	snprintf((char *) out_count_string, 33, "%d", out_count);
	outfile_xpath = xmlStrncat(outfile_xpath, out_count_string, xmlStrlen(out_count_string));
	outfile_xpath = xmlStrncat(outfile_xpath, (const xmlChar *) "]/", 2);
	outfile_xpath = xmlStrncat(outfile_xpath, child, xmlStrlen(child));

	xmlXPathObjectPtr outfile_obj = xpath_get_object(doc, outfile_xpath);
	xmlFree(outfile_xpath);
	if (outfile_obj != NULL) {
		int	line_number = xmlGetLineNo(outfile_obj->nodesetval->nodeTab[0]);
		xmlXPathFreeObject(outfile_obj);
		return line_number;
	} else {
		return -1;
	}
}

int outfile_count(xmlDocPtr doc)
{
	xmlChar *outfile_xpath = xmlCharStrdup("/handbrake_encode/outfile");
	xmlXPathObjectPtr outfile_obj = xpath_get_object(doc, outfile_xpath);
	xmlFree(outfile_xpath);
	if (outfile_obj != NULL) {
		int count = outfile_obj->nodesetval->nodeNr;
		xmlXPathFreeObject(outfile_obj);
		return count;
	} else {
		return -1;
	}
}

int get_outfile_from_episode(xmlDocPtr doc, int episode_number)
{
	int out_count = outfile_count(doc);
	xmlXPathContextPtr xpath_context = xmlXPathNewContext(doc);
	if (xpath_context == NULL) {
		fprintf(stderr, "get_outfile_from_episode() Unable to create XPath context\n");
		return -1;
	} else {
		xmlChar *outfile_xpath = xmlCharStrdup("/handbrake_encode/outfile");
		xmlXPathObjectPtr outfile_obj = xmlXPathEvalExpression(outfile_xpath, xpath_context);
		if (outfile_obj == NULL) {
			fprintf(stderr, "Unable to evaluate xpath expression \"%s\"\n", outfile_xpath);
			xmlFree(outfile_xpath);
			xmlXPathFreeContext(xpath_context);
			return -1;
		}
		int i;
		for (i = 1; i <= out_count; i++) {
			xmlChar *out_episode_num =
				xpath_get_outfile_child_content(doc, i, (xmlChar *) "episode_number");
			int e = strtol((const char *) out_episode_num, NULL, 10);
			if ( e == episode_number) {
				xmlFree(out_episode_num);
				xmlFree(outfile_xpath);
				xmlXPathFreeContext(xpath_context);
				return i;
			}
		}
		fprintf(stderr, "Unable to find episode matching number: %d in file \"%s\"\n",
				episode_number, outfile_xpath);
		xmlFree(outfile_xpath);
		xmlXPathFreeContext(xpath_context);
	}
	return -1;
}