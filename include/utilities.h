#include <stdio.h>
#include <string.h>
#include "sib_operations.h"
//RDF DEFINE
#define wildcard1 	"sib:any"
#define wildcard2 	"http://www.nokia.com/NRC/M3/sib#any"

#define fn_ex 		"http://www.w3.org/2005/xpath-functions#"
#define fn_c_1 		"fn#"
#define fn_c_2 		"fn:"

#define rdf_ex 		"http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define rdf_c 		"rdf:"

#define xsd_ex		"http://www.w3.org/2001/XMLSchema#"
#define xsd_c 		"xsd:"

#define rdfs_ex		"http://www.w3.org/2000/01/rdf-schema#"
#define rdfs_c		"rdfs:"
////////////////////////////////////////////////////////////////////

#define results_tag			"<results>"
#define results_endtag		"</results>"

#define result_tag			"<result>"
#define result_endtag		"</result>"

#define sparql_end			"</sparql>"

#define end_binding			"</binding>"
#define noresults			"<results></results>"
#define unbound				"<unbound/>"

gboolean *file_exists(gchar *filename);
gchar *redland_XML_no_preamble(gchar *pstr);
gchar *space_deleter(gchar *punt);
gchar *space_deleter_construct(gchar *punt);
ssStatus_t parseM3_sparql_bindings(GSList** template_query_, unsigned char  * query_str, sib_data_structure* p);
gboolean *file_exists(gchar *filename);
gboolean *file_remove(gchar *filename);
void  *sparql_string_added_removed(gchar **added_, gchar **removed_,gchar *new_res , gchar *old_res);
gboolean *check_sparql_result_is_real(unsigned char * sparql_result);
