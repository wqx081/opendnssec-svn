/*
 * $Id$
 *
 * Copyright (c) 2009 NLNet Labs. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 *
 * Parsing configuration files.
 */

#include "parser/confparser.h"
#include "shared/allocator.h"
#include "shared/log.h"
#include "shared/status.h"

#include <libxml/xpath.h>
#include <libxml/relaxng.h>
#include <libxml/xmlreader.h>
#include <string.h>
#include <stdlib.h>

static const char* parser_str = "parser";


/**
 * Parse elements from the configuration file.
 *
 */
ods_status
parse_file_check(const char* cfgfile, const char* rngfile)
{
    xmlDocPtr doc = NULL;
    xmlDocPtr rngdoc = NULL;
    xmlRelaxNGParserCtxtPtr rngpctx = NULL;
    xmlRelaxNGValidCtxtPtr rngctx = NULL;
    xmlRelaxNGPtr schema = NULL;

    if (!cfgfile || !rngfile) {
        ods_log_error("[%s] no cfgfile or rngfile", parser_str);
        return ODS_STATUS_ASSERT_ERR;
    }
    ods_log_assert(cfgfile);
    ods_log_assert(rngfile);
    ods_log_debug("[%s] check cfgfile %s with rngfile %s", parser_str,
        cfgfile, rngfile);

    /* Load XML document */
    doc = xmlParseFile(cfgfile);
    if (doc == NULL) {
        ods_log_error("[%s] unable to read cfgfile %s", parser_str,
            cfgfile);
        return ODS_STATUS_XML_ERR;
    }
    /* Load rng document */
    rngdoc = xmlParseFile(rngfile);
    if (rngdoc == NULL) {
        ods_log_error("[%s] unable to read rngfile %s", parser_str,
            rngfile);
        xmlFreeDoc(doc);
        return ODS_STATUS_XML_ERR;
    }
    /* Create an XML RelaxNGs parser context for the relax-ng document. */
    rngpctx = xmlRelaxNGNewDocParserCtxt(rngdoc);
    if (rngpctx == NULL) {
        xmlFreeDoc(rngdoc);
        xmlFreeDoc(doc);
        ods_log_error("[%s] unable to create XML RelaxNGs parser context",
           parser_str);
        return ODS_STATUS_XML_ERR;
    }
    /* Parse a schema definition resource and
     * build an internal XML schema structure.
     */
    schema = xmlRelaxNGParse(rngpctx);
    if (schema == NULL) {
        ods_log_error("[%s] unable to parse a schema definition resource",
            parser_str);
        xmlRelaxNGFreeParserCtxt(rngpctx);
        xmlFreeDoc(rngdoc);
        xmlFreeDoc(doc);
        return ODS_STATUS_PARSE_ERR;
    }
    /* Create an XML RelaxNGs validation context. */
    rngctx = xmlRelaxNGNewValidCtxt(schema);
    if (rngctx == NULL) {
        ods_log_error("[%s] unable to create RelaxNGs validation context",
            parser_str);
        xmlRelaxNGFree(schema);
        xmlRelaxNGFreeParserCtxt(rngpctx);
        xmlFreeDoc(rngdoc);
        xmlFreeDoc(doc);
        return ODS_STATUS_RNG_ERR;
    }
    /* Validate a document tree in memory. */
/*
    better not check: if not correct, this will segfault.
    status = xmlRelaxNGValidateDoc(rngctx,doc);
    if (status != 0) {
        ods_log_error("[%s] cfgfile validation failed %s", parser_str,
            cfgfile);
        xmlRelaxNGFreeValidCtxt(rngctx);
        xmlRelaxNGFree(schema);
        xmlRelaxNGFreeParserCtxt(rngpctx);
        xmlFreeDoc(rngdoc);
        xmlFreeDoc(doc);
        return ODS_STATUS_RNG_ERR;
    }
*/
    xmlRelaxNGFreeValidCtxt(rngctx);
    xmlRelaxNGFree(schema);
    xmlRelaxNGFreeParserCtxt(rngpctx);
    xmlFreeDoc(rngdoc);
    xmlFreeDoc(doc);
    return ODS_STATUS_OK;
}

/* TODO: look how the enforcer reads this now */

/**
 * Parse elements from the configuration file.
 *
 */
const char*
parse_conf_string(const char* cfgfile, const char* expr, int required)
{
    xmlDocPtr doc = NULL;
    xmlXPathContextPtr xpathCtx = NULL;
    xmlXPathObjectPtr xpathObj = NULL;
    xmlChar *xexpr = NULL;
    const char* string = NULL;

    ods_log_assert(expr);
    ods_log_assert(cfgfile);

    /* Load XML document */
    doc = xmlParseFile(cfgfile);
    if (doc == NULL) {
        return NULL;
    }
    /* Create xpath evaluation context */
    xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == NULL) {
        ods_log_error("[%s] unable to create new XPath context for cfgile "
            "%s expr %s", parser_str, cfgfile, (char*) expr);
        xmlFreeDoc(doc);
        return NULL;
    }
    /* Get string */
    xexpr = (unsigned char*) expr;
    xpathObj = xmlXPathEvalExpression(xexpr, xpathCtx);
    if (xpathObj == NULL || xpathObj->nodesetval == NULL ||
        xpathObj->nodesetval->nodeNr <= 0) {
        if (required) {
            ods_log_error("[%s] unable to evaluate required element %s in "
                "cfgfile %s", parser_str, (char*) xexpr, cfgfile);
        }
        xmlXPathFreeContext(xpathCtx);
        if (xpathObj) {
            xmlXPathFreeObject(xpathObj);
        }
        xmlFreeDoc(doc);
        return NULL;
    }
    if (xpathObj->nodesetval != NULL &&
        xpathObj->nodesetval->nodeNr > 0) {
        string = (const char*) xmlXPathCastToString(xpathObj);
        xmlXPathFreeContext(xpathCtx);
        xmlXPathFreeObject(xpathObj);
        xmlFreeDoc(doc);
        return string;
    }
    xmlXPathFreeContext(xpathCtx);
    xmlXPathFreeObject(xpathObj);
    xmlFreeDoc(doc);
    return NULL;
}

const char*
parse_conf_policy_filename(allocator_type* allocator, const char* cfgfile)
{
    const char* dup = NULL;
    const char* str = parse_conf_string(
                                        cfgfile,
                                        "//Configuration/Common/PolicyFile",
                                        1);
    
    if (str) {
        dup = allocator_strdup(allocator, str);
        free((void*)str);
    }
    return dup;
}

const char*
parse_conf_zonelist_filename(allocator_type* allocator, const char* cfgfile)
{
    const char* dup = NULL;
    const char* str = parse_conf_string(
        cfgfile,
        "//Configuration/Common/ZoneListFile",
        1);

    if (str) {
        dup = allocator_strdup(allocator, str);
        free((void*)str);
    }
    return dup;
}


const char*
parse_conf_zonefetch_filename(allocator_type* allocator, const char* cfgfile)
{
    const char* dup = NULL;
    const char* str = parse_conf_string(
        cfgfile,
        "//Configuration/Common/ZoneFetchFile",
        0);

    if (str) {
        dup = allocator_strdup(allocator, str);
        free((void*)str);
    }
    return dup;
}


const char*
parse_conf_log_filename(allocator_type* allocator, const char* cfgfile)
{
    const char* dup = NULL;
    const char* str = parse_conf_string(cfgfile,
        "//Configuration/Common/Logging/Syslog/Facility",
        0);
    if (!str) {
        str = parse_conf_string(cfgfile,
            "//Configuration/Common/Logging/File/Filename",
            0);
    }
    if (str) {
        dup = allocator_strdup(allocator, str);
        free((void*)str);
    }
    return dup; /* NULL, Facility or Filename */
}


const char*
parse_conf_pid_filename(allocator_type* allocator, const char* cfgfile)
{
    const char* dup = NULL;
    const char* str = parse_conf_string(
        cfgfile,
        "//Configuration/Enforcer/PidFile",
        0);

    if (str) {
        dup = allocator_strdup(allocator, str);
        free((void*)str);
    } else {
        dup = allocator_strdup(allocator, OPENDNSSEC_ENFORCER_PIDFILE);
    }
    return dup;
}


const char*
parse_conf_delegation_signer_submit_command(allocator_type* allocator, const char* cfgfile)
{
    const char* dup = NULL;
    const char* str = parse_conf_string(
        cfgfile,
        "//Configuration/Enforcer/DelegationSignerSubmitCommand",
        0);

    if (str) {
        dup = allocator_strdup(allocator, str);
        free((void*)str);
    }
    return dup;
}


const char*
parse_conf_clisock_filename(allocator_type* allocator, const char* cfgfile)
{
    const char* dup = NULL;
    const char* str = parse_conf_string(
        cfgfile,
        "//Configuration/Enforcer/SocketFile",
        0);

    if (str) {
        dup = allocator_strdup(allocator, str);
        free((void*)str);
    } else {
        dup = allocator_strdup(allocator, OPENDNSSEC_ENFORCER_SOCKETFILE);
    }
    return dup;
}


const char*
parse_conf_working_dir(allocator_type* allocator, const char* cfgfile)
{
    const char* dup = NULL;
    const char* str = parse_conf_string(
        cfgfile,
        "//Configuration/Enforcer/WorkingDirectory",
        0);

    if (str) {
        dup = allocator_strdup(allocator, str);
        free((void*)str);
    } else {
        dup = allocator_strdup(allocator, OPENDNSSEC_ENFORCER_WORKINGDIR);
    }
    return dup;
}


const char*
parse_conf_username(allocator_type* allocator, const char* cfgfile)
{
    const char* dup = NULL;
    const char* str = parse_conf_string(
        cfgfile,
        "//Configuration/Enforcer/Privileges/User",
        0);

    if (str) {
        dup = allocator_strdup(allocator, str);
        free((void*)str);
    }
    return dup;
}


const char*
parse_conf_group(allocator_type* allocator, const char* cfgfile)
{
    const char* dup = NULL;
    const char* str = parse_conf_string(
        cfgfile,
        "//Configuration/Enforcer/Privileges/Group",
        0);

    if (str) {
        dup = allocator_strdup(allocator, str);
        free((void*)str);
    }
    return dup;
}


const char*
parse_conf_chroot(allocator_type* allocator, const char* cfgfile)
{
    const char* dup = NULL;
    const char* str = parse_conf_string(
        cfgfile,
        "//Configuration/Enforcer/Privileges/Directory",
        0);

    if (str) {
        dup = allocator_strdup(allocator, str);
        free((void*)str);
    }
    return dup;
}

const char*
parse_conf_datastore(allocator_type* allocator, const char* cfgfile)
{
    const char* dup = NULL;
    const char* str = parse_conf_string(
        cfgfile,
        "//Configuration/Enforcer/Datastore/SQLite",
        0);
    
    if (str) {
        dup = allocator_strdup(allocator, str);
        free((void*)str);
    }
    return dup;
}


/**
 * Parse elements from the configuration file.
 *
 */
int
parse_conf_use_syslog(const char* cfgfile)
{
    const char* str = parse_conf_string(cfgfile,
        "//Configuration/Common/Logging/Syslog/Facility",
        0);
    if (str) {
        free((void*)str);
        return 1;
    }
    return 0;
}


int
parse_conf_worker_threads(const char* cfgfile)
{
    int numwt = ODS_SE_WORKERTHREADS;
    const char* str = parse_conf_string(cfgfile,
        "//Configuration/Enforcer/WorkerThreads",
        0);
    if (str) {
        if (strlen(str) > 0) {
            numwt = atoi(str);
        }
        free((void*)str);
    }
    return numwt;
}

int
parse_conf_manual_keygen(const char* cfgfile)
{
    const char* str = parse_conf_string(cfgfile,
        "//Configuration/Enforcer/ManualKeyGeneration",
        0);
    if (str) {
        free((void*)str);
        return 1;
    }
    return 0;
}
