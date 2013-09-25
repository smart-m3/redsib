#include <stdio.h>
#include <string.h>
#include <sibmsg.h>
#include <whiteboard_util.h>
#include <whiteboard_log.h>
#include <glib.h>
#include <string.h>
#include <redland.h>



#include "sib_operations.h"
//RDF DEFINE
#define wildcard1 	"sib:any"
#define wildcard2 	"http://www.nokia.com/NRC/M3/sib#any"

#define fn_ex 		"http://www.w3.org/2005/xpath-functions#"
#define fn_c_1 		"fn#"
#define fn_c_2 		"fn:"
#define fn_c_2_np  "fn"


#define rdf_ex 		"http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define rdf_c 		"rdf:"
#define rdf_c_np   "rdf"

#define xsd_ex		"http://www.w3.org/2001/XMLSchema#"
#define xsd_c 		"xsd:"
#define xsd_c_np   "xsd"


#define rdfs_ex		"http://www.w3.org/2000/01/rdf-schema#"
#define rdfs_c		"rdfs:"
#define rdfs_c_np  "rdfs"

////////////////////////////////////////////////////////////////////
#define prefix_uc			"PREFIX"
#define prefix_lc			"prefix"

#define sparql_tag			"<sparql xmlns=\"http://www.w3.org/2005/sparql-results#\">"
#define sparql_endtag		"</sparql>"

#define head_tag			"<head>"
#define head_endtag			"</head>"

#define results_tag			"<results>"
#define results_endtag		"</results>"

#define result_tag			"<result>"
#define result_endtag		"</result>"


#define end_binding			"</binding>"
#define noresults			"<results></results>"
#define unbound				"<unbound/>"

#define nulluri				"<http://null#null>"

#define where_uc			"WHERE"
#define where_lc			"where"

#define sparql_upd_resp_true  	"<sparql xmlns=\"http://www.w3.org/2005/sparql-results#\"><head></head><boolean>true</boolean></sparql>"
#define sparql_upd_resp_false 	"<sparql xmlns=\"http://www.w3.org/2005/sparql-results#\"><head></head><boolean>false</boolean></sparql>"

#define sparql_seq_separator 	" >> "

////////////////////////////////////////////////////// SIB Time //////////////////////////////////////////////////////

#define sched_SIB_t			"scheduled_time:"
#define get_sib_time		"get_sib_time()"

#define scheduler_upd_resp_true  "<sparql xmlns=\"http://sib.time.managment/udpate-scheduled#\"><head></head><boolean>true</boolean></sparql>"
#define scheduler_upd_resp_false "<sparql xmlns=\"http://sib.time.managment/update-scheduled#\"><head></head><boolean>false</boolean></sparql>"


gboolean *file_exists(gchar *filename);
gchar *redland_XML_no_preamble(gchar *pstr);
gchar *space_deleter(gchar *punt);
gchar *space_deleter_construct(gchar *punt);
ssStatus_t parseM3_sparql_bindings(GSList** template_query_,GSList** var_query_, GSList** required_var_query_,unsigned char  * query_str, sib_data_structure* p);
gboolean *file_exists(gchar *filename);
gboolean *file_remove(gchar *filename);
void  *sparql_string_added_removed(gchar **added_, gchar **removed_,gchar *new_res , gchar *old_res);
gboolean *check_sparql_result_is_real(unsigned char * sparql_result);
gchar * turn_sparql_booster_results_to_str(GSList * results, GSList *sparql_total_vars);
unsigned char * load_sparql_on_dedicate_sub_ts(unsigned char* sparql_query_str, librdf_world * world, librdf_storage * storage, librdf_model *model);
gchar * sparql_update_prefix(gchar * query_str, sib_data_structure* p);
void m3_free_triple_list_simple(GSList** triple_list_);
gchar* m3_gen_triple_string(GSList* triples);
ssTriple_t * ssTripleHardCopy (ssTriple_t * triple_in);


gboolean check_sparql_is_update (char * sparql_query);
gchar * get_remove_construct(gchar * sparql_query);
gchar * get_insert_construct(gchar * sparql_query);

gchar * get_select_all_from_sparql_upd(gchar * sparql_query);
gchar * get_construct_all_from_sparql_upd(gchar * sparql_query);

double get_sparql_update_scheduled_time (gchar * sparql_query);
gchar * convert_sib_time_calls(gchar * sparql_query);
