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

/**
 * @brief Parses the xml file. Validates xml against the DTD.
 *
 * @param infile path for xml file.
 *
 * @return Document tree object pointer. NULL on failure. Must be freed by caller.
 */
xmlDocPtr parse_xml(char *infile)
{
	xmlParserCtxtPtr ctxt; // the parser context
	xmlDocPtr doc; // the resulting document tree

	// create a parser context
	ctxt = xmlNewParserCtxt();
	if (ctxt == NULL) {
		fprintf(stderr, "Failed to allocate parser context\n");
		return NULL;
	}
	// parse the file, activating the DTD validation option
	doc = xmlCtxtReadFile(ctxt, infile, NULL, XML_PARSE_DTDVALID);
	// check if parsing succeeded
	if (doc == NULL) {
		fprintf(stderr, "Failed to parse %s\n", infile);
		xmlFreeParserCtxt(ctxt);
		return NULL;
	} else {
		// check if validation succeeded
		if (ctxt->valid == 0) {
			fprintf(stderr, "Failed to validate %s\n", infile);
			xmlFreeParserCtxt(ctxt);
			return NULL;
		}
	}
	// free up the parser context
	xmlFreeParserCtxt(ctxt);
	return doc;
}


/**
 * @brief Evaluates XPath expression.
 *
 * @param doc The document to be evaluated against.
 * @param xpath_expr XPath 1.0 expression.
 *
 * @return  Result of evaluation. NULL if no match. Must be freed by caller.
 */
xmlXPathObjectPtr xpath_get_object(xmlDocPtr doc, xmlChar *xpath_expr)
{
	xmlXPathContextPtr xpath_context = xmlXPathNewContext(doc);
	if (xpath_context == NULL) {
		fprintf(stderr, "Unable to create XPath context\n");
		return NULL;
	} else {
		xmlXPathObjectPtr outfile_obj = xmlXPathEvalExpression(xpath_expr, xpath_context);
		if (outfile_obj == NULL) {
			fprintf(stderr, "Unable to evaluate xpath expression \"%s\"\n", xpath_expr);
			xmlXPathFreeContext(xpath_context);
			xmlFree(xpath_expr);
			return NULL;
		}
		xmlXPathFreeContext(xpath_context);
		return outfile_obj;
	}
}

/**
 * @brief Gets the content of an outfile's child element.
 *
 * @param doc The document to be searched.
 * @param out_count Outfile to match (1-indexed).
 * @param child Element to return contents of.
 *
 * @return Contents of child element. NULL if outfile or child is invalid. Must be freed by caller.
 */
xmlChar* get_outfile_child_content(xmlDocPtr doc, int out_count, xmlChar *child)
{
	// build xpath like: "/handbrake_encode/outfile[3]/child"
	xmlChar *outfile_xpath = xmlCharStrdup("/handbrake_encode/outfile[");
	xmlChar out_count_string[35];
	snprintf((char *) out_count_string, 33, "%d", out_count);
	outfile_xpath = xmlStrncat(outfile_xpath, out_count_string, xmlStrlen(out_count_string));
	outfile_xpath = xmlStrncat(outfile_xpath, BAD_CAST "]/", 2);
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

/**
 * @brief Gets Nth outfile.
 *
 * @param doc The document to be searched.
 * @param out_count Outfile to return (1-indexed).
 *
 * @return Outfile object. NULL if not found. Must be freed by caller.
 */
xmlNode* get_outfile(xmlDocPtr doc, int out_count)
{
	// build xpath like: "/handbrake_encode/outfile[3]"
	xmlChar *outfile_xpath = xmlCharStrdup("/handbrake_encode/outfile[");
	xmlChar out_count_string[33];
	snprintf((char *) out_count_string, 33, "%d", out_count);
	outfile_xpath = xmlStrncat(outfile_xpath, out_count_string, xmlStrlen(out_count_string));
	outfile_xpath = xmlStrncat(outfile_xpath, BAD_CAST "]", 1);

	xmlXPathObjectPtr outfile_obj = xpath_get_object(doc, outfile_xpath);
	xmlFree(outfile_xpath);
	if (outfile_obj != NULL) {
		xmlNodePtr outfile_node = outfile_obj->nodesetval->nodeTab[0];
		xmlXPathFreeObject(outfile_obj);
		return outfile_node;
	} else {
		return NULL;
	}
}

/**
 * @brief Finds the line number for the Nth outfile's child element.
 *
 * @param doc The document to be searched.
 * @param out_count Outfile to return (1-indexed).
 * @param child Element to return line number of.
 *
 * @return Line number of matching element. Returns -1 if no match found.
 */
long int get_outfile_line_number(xmlDocPtr doc, int out_count, xmlChar *child)
{
	xmlChar *outfile_xpath = xmlCharStrdup("/handbrake_encode/outfile[");
	xmlChar out_count_string[33];
	snprintf((char *) out_count_string, 33, "%d", out_count);
	outfile_xpath = xmlStrncat(outfile_xpath, out_count_string, xmlStrlen(out_count_string));
	outfile_xpath = xmlStrncat(outfile_xpath, BAD_CAST "]/", 2);
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

/**
 * @brief Counts number of outfile sections in the document.
 *
 * @param doc The document to be searched.
 *
 * @return Number of outfile sections in the document.
 */
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

/**
 * @brief Find first outfile with matching episode number.
 *
 * @param doc The document to be searched.
 * @param episode_number Episode number to match.
 *
 * @return Index of the first matching outfile. Returns -1 if no match.
 */
int get_outfile_from_episode(xmlDocPtr doc, int episode_number)
{
	int out_count = outfile_count(doc);
	xmlChar *outfile_xpath = xmlCharStrdup("/handbrake_encode/outfile");
	xmlXPathObjectPtr outfile_obj = xpath_get_object(doc, outfile_xpath);
	xmlFree(outfile_xpath);
	if (outfile_obj != NULL) {
		int i;
		for (i = 1; i <= out_count; i++) {
			xmlChar *out_episode_num =
				get_outfile_child_content(doc, i, (xmlChar *) "episode_number");
			int e = strtol((const char *) out_episode_num, NULL, 10);
			if ( e == episode_number) {
				xmlFree(out_episode_num);
				return i;
			}
		}
		fprintf(stderr, "Unable to find episode matching number: %d in file \"%s\"\n",
				episode_number, outfile_xpath);
		xmlXPathFreeObject(outfile_obj);
		return -1;
	} else {
		xmlXPathFreeObject(outfile_obj);
		return -1;
	}
}
