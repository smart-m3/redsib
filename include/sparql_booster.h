#include <stdio.h>
#include <string.h>
#include <redland.h>
#include <rasqal.h>
#include <core_utilities.h>
#include "sib_operations.h"
#include <glib.h>

//RDF DEFINE
#define wildcard1   "sib:any"
#define wildcard2   "http://www.nokia.com/NRC/M3/sib#any"

#define fn_ex       "http://www.w3.org/2005/xpath-functions#"
#define fn_c_1      "fn#"
#define fn_c_2      "fn:"

#define rdf_ex      "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define rdf_c       "rdf:"

#define xsd_ex      "http://www.w3.org/2001/XMLSchema#"
#define xsd_c       "xsd:"

#define rdfs_ex     "http://www.w3.org/2000/01/rdf-schema#"
#define rdfs_c      "rdfs:"
////////////////////////////////////////////////////////////////////

#define sparql_tag          "<sparql xmlns=\"http://www.w3.org/2005/sparql-results#\">"
#define sparql_endtag       "</sparql>"

#define head_tag            "<head>"
#define head_endtag         "</head>"

#define results_tag         "<results>"
#define results_endtag      "</results>"

#define result_tag          "<result>"
#define result_endtag       "</result>"


#define end_binding         "</binding>"
#define noresults           "<results></results>"
#define unbound             "<unbound/>"

#define nulluri             "<http://null#null>"

#define where_uc            "WHERE"
#define where_lc            "where"


void clean_sparql_booster_results(GSList ** results);
