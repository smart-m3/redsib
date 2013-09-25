#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <redland.h>

#include <core_utilities.h>

extern charStr SIB_TRIPLELIST;

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

gboolean *file_exists(gchar *filename)
{
    FILE * file;
    if (file = fopen(filename, "r"))
    {
        fclose(file);
        return 1;
    }
    return 0;
}
gboolean *file_remove(gchar *filename)
{
	int status = remove(filename);
	if (status == 0)
		return 1;
	else
		return 0;

}


void initialize_storage_model(sib_data_structure* sd)
{
	//MAIN TRIPLESTORE
	if (sd->mem_volatile)
	{
		sd->RDF_storage=librdf_new_storage(sd->RDF_world,  NULL, NULL, NULL);
	}
	else if (sd->mem_volatile_hash)
	{
	    sd->RDF_storage=librdf_new_storage(sd->RDF_world, "hashes", NULL, "hash-type='memory'");
	}
	else
	{
		if (sd->sqlite)
		{
			if ((file_exists(sd->ss_name)) && (sd->enable_rdf_pp == FALSE))
			{
				//printf("Found a local sqlite db, re-charging.. \n");
				sd->RDF_storage=librdf_new_storage(sd->RDF_world, "sqlite", sd->ss_name, NULL);
			}
			else
			{	//printf("Creating a new Sqlite db.. \n");
			    sd->RDF_storage=librdf_new_storage(sd->RDF_world, "sqlite", sd->ss_name, "new='yes'");
			}
		}
		else
		{
		    if (sd->virtuoso)
		    {
                librdf_hash * options= NULL;
                librdf_node * virtuoso_context = NULL;
                gchar*  uri_db_name = g_strdup_printf("http://sib#%s", sd->ss_name);
                sd->virtuoso_context=librdf_new_node_from_uri_string(sd->RDF_world, (const unsigned char *) uri_db_name);

                options=librdf_new_hash(sd->RDF_world, NULL);
                librdf_hash_open(options, NULL, 0, 1, 1, NULL);
                librdf_hash_put_strings(options, "contexts", "yes");
                librdf_hash_from_string(options, sd->virtuoso_param);
                sd->RDF_storage=librdf_new_storage_with_options(sd->RDF_world, "virtuoso", uri_db_name, options);
                printf ("Virtuoso storage module started\n");

		    }
		    else
		    {
                //BDB default
                gchar* sp2o;
                gchar* po2s;
                gchar* so2p;

                sp2o= g_strdup_printf("%s%s",sd->ss_name,"-sp2o.db");
                so2p= g_strdup_printf("%s%s",sd->ss_name,"-so2p.db");
                po2s= g_strdup_printf("%s%s",sd->ss_name,"-po2s.db");


                if (
                        (
                        (file_exists( sp2o)) &&
                        (file_exists( so2p)) &&
                        (file_exists( po2s))
                        ) &&
                        (sd->enable_rdf_pp == FALSE)
                        )
                {
                    //printf("Found a BDB local file, re-charging.. \n");
                    sd->RDF_storage=librdf_new_storage(sd->RDF_world, "hashes", sd->ss_name, "new='no',hash-type='bdb',dir='.'");
                }
                else
                {
                    sd->RDF_storage=librdf_new_storage(sd->RDF_world, "hashes", sd->ss_name, "new='yes',hash-type='bdb',dir='.'");
                }

                g_free(sp2o);
                g_free(po2s);
                g_free(so2p);
		    }

		}
	}
	sd->RDF_model=librdf_new_model(sd->RDF_world, sd->RDF_storage , NULL);

}
void reset_storage_model(sib_data_structure* sd)
{


	//MAIN TRIPLESTORE
	if (sd->mem_volatile)
	{
		// Cleaning ////////////////////////
		librdf_free_model(sd->RDF_model);
		librdf_free_storage(sd->RDF_storage);
		////////////////////////////////////

		sd->RDF_storage=librdf_new_storage(sd->RDF_world,  NULL, NULL, NULL);
		sd->RDF_model=librdf_new_model(sd->RDF_world, sd->RDF_storage , NULL);
	}
	else if (sd->mem_volatile_hash)
	{
		// Cleaning ////////////////////////
		librdf_free_model(sd->RDF_model);
		librdf_free_storage(sd->RDF_storage);
		////////////////////////////////////

	    sd->RDF_storage=librdf_new_storage(sd->RDF_world, "hashes", NULL, "hash-type='memory'");
		sd->RDF_model=librdf_new_model(sd->RDF_world, sd->RDF_storage , NULL);
	}
	else
	{
		if (sd->sqlite)
		{
			// Cleaning ////////////////////////
			librdf_free_model(sd->RDF_model);
			librdf_free_storage(sd->RDF_storage);
			////////////////////////////////////
			sd->RDF_storage=librdf_new_storage(sd->RDF_world, "sqlite", sd->ss_name, "new='yes'");
			sd->RDF_model=librdf_new_model(sd->RDF_world, sd->RDF_storage , NULL);
		}
		else
		{
		    if (sd->virtuoso)
		    {
		    	librdf_model_context_remove_statements(sd->RDF_model, sd->virtuoso_context);
		    }
		    else
		    {

				//BDB default

		    	// Cleaning ////////////////////////
		    	librdf_free_model(sd->RDF_model);
		    	librdf_free_storage(sd->RDF_storage);
		    	////////////////////////////////////

				gchar* sp2o;
				gchar* po2s;
				gchar* so2p;

				sp2o= g_strdup_printf("%s%s",sd->ss_name,"-sp2o.db");
				so2p= g_strdup_printf("%s%s",sd->ss_name,"-so2p.db");
				po2s= g_strdup_printf("%s%s",sd->ss_name,"-po2s.db");


				sd->RDF_storage=librdf_new_storage(sd->RDF_world, "hashes", sd->ss_name, "new='yes',hash-type='bdb',dir='.'");


				g_free(sp2o);
				g_free(po2s);
				g_free(so2p);

				sd->RDF_model=librdf_new_model(sd->RDF_world, sd->RDF_storage , NULL);
		    }
		}
	}

}

////////SPARQL UTILITIES

gchar *redland_XML_no_preamble(gchar *pstr)
{
	int xml_preamble_offset=39;
	gchar *pnew;
	pnew=strndup(pstr+xml_preamble_offset,strlen(pstr)-1-xml_preamble_offset);
	g_free(pstr);
	return pnew;
}

gchar *space_deleter(gchar *punt)
{
    gchar *app;
    int ind;
    int ind2;
    int flag;
    int flag_lit;

    char ref_str[]="literal";
    int count_pos;


    flag=0;
    flag_lit=0;
    ind=0;
    ind2=0;
    app=g_malloc(strlen(punt)-1);

    while (punt[ind]!='\0')
    {
        if (punt[ind]!='\n')
        {
            if (punt[ind]=='<')
            {
                flag=1;
                count_pos=1;

                flag_lit=1;
                while ( count_pos < strlen(ref_str))
                {
                    if ((punt[ind+count_pos]!=ref_str[count_pos-1]) || (punt[ind+count_pos]=="\0"))
                    {
                        flag_lit=0;
                        break;
                    }
                    count_pos++;
                }
            }
            else if (punt[ind]=='>')
                flag=0;

            if (	(punt[ind]!=' ') || (flag_lit==1)  || (flag==1)	)
            {
                app[ind2]=punt[ind];
                ind2=ind2+1;
            }


        }
        ind=ind+1;
    } 

    app[ind2]='\0';

    g_free(punt);
    return app;
}


gchar *space_deleter_construct(gchar *punt)
{
    char *app;
 int ind;
 int ind2;
 int flag;
 int flag_obj;
 int flag3;
 int cont;
 flag=0;
 flag_obj=0;
 ind=0;
 ind2=0;
 cont=0;
 flag3=0;
 char ref_str[]="rdf:Description";
 int count_pos;

 app=g_malloc(strlen(punt)-1);
 while (punt[ind]!='\0')
    {
    if (punt[ind]!='\n')
       { 
		if (punt[ind]=='<')
		   {
		    if (cont==1)
		        cont=cont+1;

		    flag=1;
		    count_pos=1;
		    flag_obj=1;

		    cont++;	
		    while ( count_pos < strlen(ref_str))
			{
			if ((punt[ind+count_pos]!=ref_str[count_pos-1]) || (punt[ind+count_pos]=="\0"))
				{
					flag_obj=0;
					cont=cont-1;
					break;
				}
			count_pos++;
			}
	 
		    if (flag3==1)
		       {
		        flag_obj=0;
		        cont=0;
		        flag3=0;
		        }
		    }
		else if (punt[ind]=='>')
		    flag=0;


		if (punt[ind]!=' ')
		  {
		   app[ind2]=punt[ind];
		   ind2=ind2+1;
		   }
		else 
		   {
		     if (flag==1)
		       {
		        app[ind2]=punt[ind];
		        ind2=ind2+1;
		        }
		     else if (flag_obj==1 && cont==2)
		       {
		        app[ind2]=punt[ind];
		        ind2=ind2+1;
		        flag3=1;
		        }
		    }
        }
    ind=ind+1;
    } 
 app[ind2]='\0';
 g_free(punt);
 return app;
 }

#define SPACES_LENGTH 80
static const char spaces[SPACES_LENGTH + 1] = "                                                                                ";

unsigned char * load_sparql_on_dedicate_sub_ts(unsigned char* sparql_query_str, librdf_world * world, librdf_storage * storage, librdf_model *model)
{

	unsigned char * 		final_result;

	gchar*					result_sparql_query;
	gchar*					XML_no_pre;
	gchar*					XML_no_pre_nospace;
	librdf_query *			query;
	librdf_query_results*	results;
	librdf_stream * 		stream2;

	int length;

	gchar * sparql_query_str_timed = convert_sib_time_calls(sparql_query_str);

	//query=librdf_new_query(world, "sparql", NULL, sparql_query_str, NULL);
	query=librdf_new_query(world, "sparql", NULL, sparql_query_str_timed, NULL);
	results=librdf_model_query_execute(model, query);

	g_free(sparql_query_str_timed);

	if (results == NULL)
	{
		//printf("SPARQL syntax error! \n");
		librdf_free_query_results(results);
		librdf_free_query(query);
		final_result=g_strdup_printf("");

	}
	else if (librdf_query_results_is_graph(results))
	{
		//RESULT GRAPH CASE
		librdf_serializer* serializer;
		serializer = librdf_new_serializer( world, NULL, NULL, NULL);

		stream2=librdf_query_results_as_stream(results);

		result_sparql_query = librdf_serializer_serialize_stream_to_counted_string(serializer,NULL, stream2, &length);
		librdf_free_stream(stream2);
		XML_no_pre= redland_XML_no_preamble(result_sparql_query);
		final_result=XML_no_pre;

		librdf_free_query_results(results);
		librdf_free_query(query);


		//librdf_free_serializer(serializer);
		//Redland bug
	}
	else
	{
		//RESULT XML CASE
		result_sparql_query= librdf_query_results_to_string(results ,NULL,NULL);
		//printf("RESULT XML CASE\n%s\n",result_sparql_query);
		XML_no_pre= redland_XML_no_preamble((gchar *)result_sparql_query);
		final_result=space_deleter(XML_no_pre);

		librdf_free_query_results(results);
		librdf_free_query(query);
	}

	return final_result;

}

static void navigator_graph_pattern_load(rasqal_graph_pattern *gp, int gp_index,int indent, GSList** template_query_, GSList** var_list_ , sib_data_structure* p )
{
  int triple_index = 0;

  raptor_sequence *seq;
  rasqal_variable * var;
  rasqal_triple* t;
  rasqal_literal_type type;

  rasqal_graph_pattern_operator op;

  GSList* template_query= *template_query_;
  GSList* var_list=*var_list_;

  indent += 1;

  int idx;
  idx = rasqal_graph_pattern_get_index(gp);
  op = rasqal_graph_pattern_get_operator(gp);
  gchar * operator = g_strdup(rasqal_graph_pattern_operator_as_string (op));
  //printf("%s : %d, [%d]\n",operator, op, idx);
  g_free(operator);

  /* look for triples */

  while(1) {

    t = rasqal_graph_pattern_get_triple(gp, triple_index);
    if(!t)
      break;


    ssTriple_t_sparql *ttemp = (ssTriple_t_sparql *)g_new0(ssTriple_t_sparql,1);
    type=rasqal_literal_get_rdf_term_type(t->subject);

    //var=rasqal_literal_as_variable(t->subject);
    //rasqal_variable_print(var,    stdout);

    //printf("\n");
    //printf("%s\n", var->name);
    //rasqal_free_variable (var);

    if ((type==RASQAL_LITERAL_UNKNOWN) && (rasqal_literal_as_string(t->subject) == NULL))
    {   //variable , needs a wildcard

    	var=rasqal_literal_as_variable(t->subject);
    	ttemp->subject_var=g_strdup(var->name);
    	if (g_slist_find_custom(var_list,var->name,strcmp) ==NULL)
    	{
    	        var_list=g_slist_prepend(var_list, g_strdup(var->name));
    	}
    	rasqal_free_variable (var);

    	ttemp->subject=g_strdup_printf("%s",wildcard1);
    }
    else
    {
        ttemp->subject=g_strdup(rasqal_literal_as_string (t->subject));
    	ttemp->subject_var=NULL;
    }

    type=rasqal_literal_get_rdf_term_type(t->predicate);
    if ((type==RASQAL_LITERAL_UNKNOWN) && (rasqal_literal_as_string(t->predicate) == NULL))
    {   //variable , needs a wildcard

    	var=rasqal_literal_as_variable(t->predicate);
    	ttemp->predicate_var=g_strdup(var->name);
    	if (g_slist_find_custom(var_list,var->name,strcmp) ==NULL)
    	{
    	        var_list=g_slist_prepend(var_list, g_strdup(var->name));
    	}
    	rasqal_free_variable (var);

    	ttemp->predicate=g_strdup_printf("%s",wildcard1);
    }
    else
    {    	ttemp->predicate=g_strdup(rasqal_literal_as_string (t->predicate));
    		ttemp->predicate_var=NULL;
    }

    type=rasqal_literal_get_rdf_term_type(t->object);
    if ((type==RASQAL_LITERAL_UNKNOWN) && (rasqal_literal_as_string(t->object) == NULL))
    {    	//variable , needs a wildcard

    	var=rasqal_literal_as_variable(t->object);
    	ttemp->object_var=g_strdup(var->name);

    	if (g_slist_find_custom(var_list,var->name,strcmp) == NULL)
    	{
    	        var_list=g_slist_prepend(var_list, g_strdup(var->name));
    	}
    	rasqal_free_variable (var);

    	ttemp->object=g_strdup_printf("%s",wildcard1);
    }
    else
    {
    	if (type==RASQAL_LITERAL_STRING)
    	{	ttemp->object=g_strdup(rasqal_literal_as_string (t->object));
    		ttemp->objType = ssElement_TYPE_LIT;
    	}
    	else
    	{	ttemp->object=g_strdup(rasqal_literal_as_string (t->object));
            ttemp->objType = ssElement_TYPE_URI;
    	}
    	ttemp->object_var=NULL;
    }

    //ttemp->gp_index=gp_index+2;
    ttemp->gp_index=idx;

    ttemp->indent=indent;

    //printf("SPARQL Binding triple is: \t%s\t%s\t%s , obj_Type %d \n", ttemp->subject, ttemp->predicate, ttemp->object, ttemp->objType);
    //printf("				     Vars: \t%s\t%s\t%s , gp_idx %d indent %d\n", ttemp->subject_var, ttemp->predicate_var, ttemp->object_var,ttemp->gp_index,ttemp->indent);
    template_query= g_slist_prepend(template_query, ttemp);

    triple_index++;
  }

  /* look for sub-graph patterns */
  seq = rasqal_graph_pattern_get_sub_graph_pattern_sequence(gp);
  if(seq && raptor_sequence_size(seq) > 0)
  {

    gp_index = 0;
    while(1)
    {
      rasqal_graph_pattern* sgp;
      sgp = rasqal_graph_pattern_get_sub_graph_pattern(gp, gp_index);
      if(!sgp)
      {
      	break;
      }
      else
      {
          navigator_graph_pattern_load(sgp, gp_index, indent + 1, &template_query, &var_list, p);
      }

      gp_index++;
    }

  }

  /*
  rasqal_expression* expr;
  expr = rasqal_graph_pattern_get_filter_expression(gp);

  if(expr) {
	printf("\n");
    rasqal_expression_print(expr, stdout);
    printf("\n");
  }
  */

  indent -= 1;

  *var_list_=var_list;
  *template_query_=template_query;

}



ssStatus_t parseM3_sparql_bindings(GSList** template_query_, GSList** var_list_,GSList** required_var_list_, unsigned char * query_str,  sib_data_structure* p)
{
	GSList* template_query =NULL;
	GSList* var_list =NULL;
    GSList* required_var_list =NULL;


	rasqal_query* rq;
	rasqal_graph_pattern* gp;

    gchar*		prefix_query_str;
    gchar*		prefix_str;

    //printf("RECEIVED SPARQL IN\n");
    //printf("q: %s\n",op->req->query_str);

    //prefix_str= g_strdup_printf("PREFIX %s <%s> PREFIX %s <%s> PREFIX %s <%s> PREFIX %s <%s>  ",rdf_c ,rdf_ex ,fn_c_2,fn_ex,rdfs_c,rdfs_ex,xsd_c , xsd_ex);
    //prefix_query_str=g_strdup_printf("%s %s", prefix_str, query_str);

	rq = rasqal_new_query(p->sparql_preprocessing_world, "sparql",NULL);
	//printf ("pre parsing\n");

	if(rasqal_query_prepare(rq, query_str, NULL)) {

		rasqal_free_query(rq);
		rq = NULL;
		*template_query_ =template_query;

		return ss_GeneralError;
	}


	////////////// CHECK FOR REALLY REQUIRED VARS////////////////////
	raptor_sequence *seq;
	int i;
	seq = rasqal_query_get_bound_variable_sequence(rq);
	if(seq && raptor_sequence_size(seq) > 0) {
	    i = 0;
	    while(1)
	    {
	        rasqal_variable* v = (rasqal_variable*)raptor_sequence_get_at(seq, i);
	        if(!v)
	            break;

	        gchar * temp_var_name= g_strdup((const char*)v->name);
	        //printf("Required var: %s\n", temp_var_name);
	        required_var_list=g_slist_prepend(required_var_list,temp_var_name);
	        i++;
	    }
	}
    ////////////// END CHECK FOR REALLY REQUIRED VARS////////////////////

	//printf ("query prepared\n");
	gp = rasqal_query_get_query_graph_pattern(rq);
	navigator_graph_pattern_load(gp, -1,  0, &template_query, &var_list, p);

    ////////////////// MARK IDENTICAL QUERY TEMPLATES
	////////////////// for speeding up matching operations

	GSList* query_list = template_query;
	while (query_list)
	{
	    ssTriple_t_sparql* tq = query_list->data;
	    tq->replicated=FALSE;

        GSList* temp_query_list = template_query;
        int * tq_done =0;
        while (temp_query_list != query_list)
        {
            ssTriple_t* tq_previous =temp_query_list->data;
            if (
                    (g_strcmp0(tq_previous->subject, tq->subject)==0 ) &&
                    (g_strcmp0(tq_previous->predicate, tq->predicate)==0 ) &&
                    (g_strcmp0(tq_previous->object, tq->object)==0 ) &&
                    (tq_previous->objType == tq->objType)
            )
            {
                tq_done =1;
                break;
            }
            temp_query_list=temp_query_list->next;
        }
        if (tq_done ==1)
        {
            tq->replicated=TRUE;
            query_list=query_list->next;
            continue;
        }

        query_list=query_list->next;
	}
    //////////////////END MARK IDENTICAL QUERY TEMPLATES///////////////////////////////////////////////////////////

	*template_query_ =template_query;
	*var_list_ =var_list;
	*required_var_list_ =required_var_list;

	g_free( prefix_str);
	g_free( prefix_query_str);

	rasqal_free_query(rq);

	return ss_StatusOK;
}

gchar* sparql_update_prefix(gchar * query_str, sib_data_structure* p)
{


	int i;
	int idx;

	char* pref_array_np[] = { rdfs_c_np, fn_c_2_np, rdf_c_np, xsd_c_np};
	char* pref_array[] = { rdfs_c, fn_c_2, rdf_c, xsd_c};
	char* pref_array_extended[] = { rdf_ex, fn_ex, rdf_ex, xsd_ex};

	rasqal_query* rq;

	gchar * additional_prefix_query = g_strdup(query_str);
	gchar * additional_prefix_query_tmp;

	rq = rasqal_new_query(p->sparql_preprocessing_world, "sparql", NULL);
	if(rasqal_query_prepare(rq, query_str, NULL))
	{
		rasqal_free_query(rq);
		//Trying prefix complete interpretation.
		i=0;
		while (i < 4)
		{
			    additional_prefix_query_tmp= g_strdup_printf("PREFIX %s<%s> %s", pref_array[i], pref_array_extended[i] ,additional_prefix_query );
			    g_free( additional_prefix_query);
			    additional_prefix_query=g_strdup(additional_prefix_query_tmp);
			    g_free( additional_prefix_query_tmp);
			i++;
		}

		rq = rasqal_new_query(p->sparql_preprocessing_world, "sparql", NULL);
		if(rasqal_query_prepare(rq, additional_prefix_query, NULL))
		{
		    //Not working again.. syntax error, nothing to do..
			rasqal_free_query(rq);
			rq = NULL;
			return NULL;
		}
		else
		{
		    //printf("%s\n", additional_prefix_query);
		    rasqal_free_query(rq);
		    return additional_prefix_query;
		}
	}


	rasqal_free_query(rq);
	return additional_prefix_query;

	/*
	//all fine
	i =0;
	idx=0;

	gboolean prefix_found = FALSE;

	while (i < 4)
	{
	    idx=0;
	    prefix_found = FALSE;
		rasqal_prefix *   prefix =  rasqal_query_get_prefix   (rq,    idx);

		while (prefix)
		{
		    //printf("%s - %s\n", prefix->prefix , pref_array_np[i]);
			if (strcmp(prefix->prefix, pref_array_np[i]) == 0)
			{
			    prefix_found = TRUE;
			    break;
			}
			idx++;
			prefix =  rasqal_query_get_prefix   (rq,    idx);
		}

		if (prefix_found == FALSE)
		{
		    additional_prefix_query_tmp= g_strdup_printf("PREFIX %s<%s> %s", pref_array[i], pref_array_extended[i] ,additional_prefix_query );
		    g_free( additional_prefix_query);
		    additional_prefix_query=g_strdup(additional_prefix_query_tmp);
		    g_free( additional_prefix_query_tmp);
		}

		i++;
	}

	printf("%s\n", additional_prefix_query);
	rasqal_free_query(rq);
	*/

}

void m3_additional_free_triple_sparql_list(GSList** triple_list_)
{
	GSList* triple_list = *triple_list_;

	ssTriple_t_sparql *t;
	while (NULL != triple_list)
	{
		t = triple_list->data;
		//printf("m3_free_triple_list: freeing triple %s %s %s\n",(char*)t->subject,(char*)t->predicate,(char*)t->object);

		if (t->subject_var !=NULL)
			g_free(t->subject_var);
		if (t->predicate_var!=NULL)
			g_free(t->predicate_var);
		if (t->object_var!=NULL)
			g_free(t->object_var);

		triple_list = triple_list->next;
	}
	*triple_list_ = triple_list;
}




void m3_free_triple_list_simple(GSList** triple_list_)
{
	GSList* triple_list = *triple_list_;
	GSList* triple_list_pointer = *triple_list_;

	ssTriple_t *t;

	while (NULL != triple_list)
	{
		t = triple_list->data;
		//printf("m3_free_triple_list: freeing triple %s %s %s\n",(char*)t->subject,(char*)t->predicate,(char*)t->object);
		//triple_list = g_slist_remove(triple_list, t);
		ssFreeTriple(t);
		triple_list = triple_list->next;
	}
	g_slist_free (triple_list_pointer);


	*triple_list_ = NULL;

}

ssTriple_t * ssTripleHardCopy (ssTriple_t * triple_in)
{
    ssTriple_t * triple_out = g_new0(ssTriple_t,1);

    triple_out->subject=g_strdup(triple_in->subject);
    triple_out->predicate=g_strdup(triple_in->predicate);
    triple_out->object=g_strdup(triple_in->object);
    triple_out->objType=triple_in->objType;

    return triple_out;
}

gchar* m3_gen_triple_string(GSList* triples)
{
	ssStatus_t status;
	ssBufDesc_t *bd = ssBufDesc_new();
	ssTriple_t *t;
	gchar* retval;

	status = addXML_start(bd, &SIB_TRIPLELIST, NULL, NULL, 0);
	while (status == ss_StatusOK &&
			NULL != triples)
	{
		t = (ssTriple_t*)triples->data;
		if (NULL == t)
		{
			status = ss_OperationFailed;
			whiteboard_log_debug("m3_gen_triple_string(): triple was NULL\n");
			goto end;
		}

		////////////////////////////////////////////////////////////////
		// Expand Prefix 		//////////////////////////////////////
		//t=trasform_statement_expand(t);
		////////////////////////////////////////////////////////////////

		status = addXML_templateTriple(t, NULL, (gpointer)bd);
		//whiteboard_log_debug("Added triple\n%s\n%s\n%s\n to result set\n", t->subject, t->predicate, t->object);
		triples = triples->next;
	}
	end:
	addXML_end(bd, &SIB_TRIPLELIST);
	retval = g_strdup(ssBufDesc_GetMessage(bd));
	ssBufDesc_free(&bd);
	return retval;
}



void statement_in_tl(GSList ** list_, librdf_statement * statement)
{
    GSList * list=*list_;

    ssTriple_t * triple;
    ssTriple_t * triple_pointer;

    librdf_node* sub_node ;
    librdf_node* pred_node;
    librdf_node* obj_node ;

    gboolean found =0;

    //printf("Statement\n");
    //librdf_statement_print(statement , stdout);
    //printf("\n");

    sub_node  = librdf_statement_get_subject(statement);
    pred_node = librdf_statement_get_predicate(statement);
    obj_node  = librdf_statement_get_object(statement);

    triple  =(ssTriple_t *) g_new0(ssTriple_t,1);

    triple->subject=    g_strdup(librdf_uri_as_string( librdf_node_get_uri(sub_node)));
    triple->predicate = g_strdup(librdf_uri_as_string( librdf_node_get_uri(pred_node)));

    if (librdf_node_is_literal(obj_node))
    {
    	triple->object= NULL;
    	triple->object =  g_strdup(librdf_node_get_literal_value(obj_node));
        triple->objType = ssElement_TYPE_LIT;
    }
    else
    {
    	triple->object= NULL;
    	triple->object =  g_strdup(librdf_uri_as_string( librdf_node_get_uri(obj_node)));
        triple->objType = ssElement_TYPE_URI;
    }

    //printf("obj_in_string\n");
    //printf ("%s\n",triple->object);
    //printf("\n");


    while (list != NULL)
    {
        triple_pointer=list->data;

    	if  (
            (g_strcmp0(triple->subject,triple_pointer->subject)==0) &
            (g_strcmp0(triple->predicate,triple_pointer->predicate)==0) &
            (g_strcmp0(triple->object,triple_pointer->object)==0) &
            (triple->objType==triple_pointer->objType)
            )
        {
            found =1;
            break;
        }
        list= list->next;

    }

    list=*list_;

    if(found == 0)
    {
        //printf ("statement to TL %s_%s_%s_%d\n", (char *)triple->subject, (char *)triple->predicate, (char *)triple->object, (bool *)triple->objType);
        list=g_slist_prepend(list, triple);
        ///////////////////////////////////////////////////////////
    }
    else
    {
        ssFreeTriple(triple);
    }

    *list_=list;

}



///////////////////////////////////////////////////////////////////////////////////////////////////

//void add_triplelist_in_model( sib_data_structure* p, librdf_model * model, GSList * triplelist)
void add_triplelist_in_submodel( sib_data_structure* p, ssap_sib_message * rsp, GSList * triplelist)

{

    librdf_statement * statement;
    ssTriple_t * t;

    while (triplelist !=NULL)
    {
        t=triplelist->data;
        statement=librdf_new_statement(rsp->subworld);

        librdf_statement_set_subject(statement,
                librdf_new_node_from_uri_string(rsp->subworld,  t->subject));

        librdf_statement_set_predicate(statement,
                librdf_new_node_from_uri_string(rsp->subworld, t->predicate));

        if (! t->objType) {
            //URI:  t->objType==0
            librdf_statement_set_object(statement,
                    librdf_new_node_from_uri_string(rsp->subworld,  t->object));
        }
        else
        {
            //Literal:      t->objType==1
            librdf_statement_set_object(statement,
                    librdf_new_node_from_literal(rsp->subworld, t->object, NULL, 0));

        }

        //printf("add_triplelist_in_submodel ADDING\n");
        //librdf_statement_print(statement, stdout);
        //printf("\n");

        librdf_model_add_statement(rsp->submodel, statement);
        librdf_free_statement(statement);

        triplelist=triplelist->next;
    }
}

void empty_list_elements(gchar * sub_id, GSList * sub_data_)
{
    GSList * sub_data = sub_data_;
    gchar * point_var;
    //printf("key cleaning %s\n ",sub_id);

    while (sub_data !=NULL)
    {
        point_var=sub_data->data;
        //printf("cleaning %s\n ", point_var);
        g_free(point_var);
        sub_data= sub_data->next;
    }
    g_slist_free (sub_data_);
    g_free(sub_id);
}

// NO Wildcard!
//void remove_triplelist_in_model( sib_data_structure* p, librdf_model * model, GSList * triplelist)
void remove_triplelist_in_submodel( sib_data_structure* p, ssap_sib_message * rsp, GSList * triplelist)
{

    librdf_statement * statement;
    ssTriple_t * t;

    while (triplelist !=NULL)
    {

        t=triplelist->data;
        statement=librdf_new_statement(rsp->subworld);

        librdf_statement_set_subject(statement,
                librdf_new_node_from_uri_string(rsp->subworld,  t->subject));

        librdf_statement_set_predicate(statement,
                librdf_new_node_from_uri_string(rsp->subworld, t->predicate));

        if (! t->objType) {
            //URI:  t->objType==0
            librdf_statement_set_object(statement,
                    librdf_new_node_from_uri_string(rsp->subworld,  t->object));
        }
        else
        {
            //Literal:      t->objType==1
            librdf_statement_set_object(statement,
                    librdf_new_node_from_literal(rsp->subworld, t->object, NULL, 0));

        }

        //printf("remove_triplelist_in_submodel REMOVING\n");
        //librdf_statement_print(statement, stdout);
        //printf("\n");

        librdf_model_remove_statement(rsp->submodel, statement);
        librdf_free_statement(statement);

        triplelist=triplelist->next;
    }
}


//////////////////////////
//SPARQL UPDATE UTILITIES
//////////////////////////

gchar * string_substitution_ (gchar * input_str, gchar * obsolete_substr, gchar * new_substr )
{
    gchar** split_result_array;
    gchar** pointer_char_arr;
    gchar * result = NULL;
    gchar * old_result;
    int one_al_least_found=0;

    split_result_array=g_strsplit(input_str, obsolete_substr , 0);
    pointer_char_arr = split_result_array;

    old_result=g_strdup(*pointer_char_arr);
    pointer_char_arr++;

    while (*pointer_char_arr)
    {
        one_al_least_found=1;

        g_free(result);
        result=g_strdup_printf("%s%s%s",old_result,new_substr,*pointer_char_arr);
        pointer_char_arr++;

        g_free(old_result);
        old_result=g_strdup(result);
    }
    g_strfreev (split_result_array);

    if (one_al_least_found==0)
          return old_result;

    g_free(old_result);
    return result;
}

gboolean check_sparql_is_update (char * sparql_query)
{
	gchar ** split_result_array;
	gchar ** pointer_before_brace;

    split_result_array=g_strsplit( sparql_query, "{" , 0);
    pointer_before_brace = split_result_array;
    //pointer_char_arr++;

	if  (
		(strstr(*pointer_before_brace, "INSERT") != NULL) ||
		(strstr(*pointer_before_brace, "insert") != NULL) ||
		(strstr(*pointer_before_brace, "DELETE") != NULL) ||
		(strstr(*pointer_before_brace, "delete") != NULL)
		)
	{

		g_strfreev (split_result_array);
		return true;
	}

	g_strfreev (split_result_array);
	return false;

}


gchar * get_remove_construct(gchar * sparql_query)
{

	gchar ** split_result_array;
	gchar ** pointer_split_result_array;

	gchar * sparql_before_where;
	gchar * sparql_after_where;
	gchar * sparql_after_delete_before_where;
	gchar * sparql_before_delete;

	gchar * remove_triples_str;
	gchar * final_construct;

	gchar * sparql_before_insert;
	gchar * sparql_after_insert;
	gchar * sparql_after_insert_endbrace;

	gboolean no_where=FALSE;

	if  (
		(strstr(sparql_query, "DELETE") != NULL) ||
		(strstr(sparql_query, "delete") != NULL)
		)
	{
		//if the strings exists
	}
	else
		return NULL;


	//printf ("SPARQL UPDATE: DELETE\n");

    split_result_array=g_strsplit( sparql_query, where_uc , 0);
    if (g_strcmp0(*split_result_array, sparql_query)==0)
    {
        //not found
        g_strfreev (split_result_array);
        split_result_array=g_strsplit(sparql_query, where_lc , 0);

        if (g_strcmp0(*split_result_array, sparql_query)==0)
        {
        	g_strfreev (split_result_array);
            no_where=TRUE;
        }
    }

    if (no_where == FALSE)
    {
		pointer_split_result_array= split_result_array;
		sparql_before_where = g_strdup(*pointer_split_result_array);
		pointer_split_result_array ++;
		sparql_after_where = g_strdup(*pointer_split_result_array);
		g_strfreev (split_result_array);
    }
    else
    {

    	if  (strstr(sparql_query, " DATA ") != NULL)
    		sparql_before_where=string_substitution_(sparql_query," DATA ", "");
    	else if (strstr(sparql_query, " data ") != NULL)
    		sparql_before_where=string_substitution_(sparql_query," data ", "");
    	else
    		sparql_before_where=g_strdup(sparql_query);


    	sparql_after_where= g_strdup_printf(" {}");
    }




    ///////////////// TAKING AWAY EVENTUAL INSERT PART


    gboolean insert_found = TRUE;

    split_result_array=g_strsplit( sparql_before_where, "INSERT" , 0);
    if (g_strcmp0(*split_result_array, sparql_before_where)==0)
    {
        //not found
        g_strfreev (split_result_array);
        split_result_array=g_strsplit(sparql_before_where, "insert"  , 0);

        if (g_strcmp0(*split_result_array, sparql_before_where)==0)
        {
        	insert_found = FALSE;
        }
    }

    if (insert_found)
    {
    	//printf ("even insert found\n");
    	sparql_before_insert= g_strdup(*split_result_array);
    	gchar ** pointer_char_arr=split_result_array;
    	pointer_char_arr++;
    	sparql_after_insert= g_strdup(*pointer_char_arr);

    	g_strfreev (split_result_array);

        split_result_array=g_strsplit( sparql_after_insert, "}" , 0);
        if (g_strcmp0(*split_result_array, sparql_after_insert)==0)
        {
            g_free(sparql_before_where);
        	g_free(sparql_after_where);
            g_free(sparql_before_insert);
        	g_free(sparql_after_insert);


        	g_strfreev (split_result_array);
        	return NULL;
        }
    	pointer_char_arr=split_result_array;
    	pointer_char_arr++;
    	sparql_after_insert_endbrace= g_strdup(*pointer_char_arr);
    	g_strfreev (split_result_array);

    	gchar * temp_sparql_before_where= g_strdup_printf("%s %s }", sparql_before_insert ,sparql_after_insert_endbrace);
    	g_free(sparql_before_where);
    	sparql_before_where=temp_sparql_before_where;

    	g_free(sparql_after_insert_endbrace);
    	g_free(sparql_before_insert);
    	g_free(sparql_after_insert);

    }
    else
    {
    	g_strfreev (split_result_array);
    }

    ////////////////////////////////////////////////

    split_result_array=g_strsplit( sparql_before_where, "delete" , 0);
    if (g_strcmp0(*split_result_array, sparql_before_where)==0)
    {
        //not found
        g_strfreev (split_result_array);
        split_result_array=g_strsplit(sparql_before_where, "DELETE" , 0);

        if (g_strcmp0(*split_result_array, sparql_before_where)==0)
        {
            g_strfreev (split_result_array);

            g_free(sparql_before_where);
        	g_free(sparql_after_where);
            return NULL;
            //ERROR NO WHERE
        }
    }

    pointer_split_result_array= split_result_array;
    sparql_before_delete = g_strdup(*pointer_split_result_array);
    pointer_split_result_array ++;
    sparql_after_delete_before_where = g_strdup(*pointer_split_result_array);
    g_strfreev (split_result_array);


    split_result_array=g_strsplit( sparql_after_delete_before_where, "}" , 0);
    if (g_strcmp0(*split_result_array, sparql_after_delete_before_where)==0)
    {
        //not found
    	g_free(sparql_after_where);
    	g_free(sparql_before_delete);
    	g_free(sparql_after_delete_before_where);
    	g_free(sparql_before_where);

        g_strfreev (split_result_array);
        return NULL;
    }

    remove_triples_str = g_strdup(*split_result_array);
    g_strfreev (split_result_array);

    //OPTIMIZATION : No vars -> No query

    if  (g_strrstr(remove_triples_str, " ?") != NULL)
    		final_construct=g_strdup_printf("%s CONSTRUCT %s } WHERE %s", sparql_before_delete, remove_triples_str, sparql_after_where);
    else
    		final_construct=g_strdup_printf("%s CONSTRUCT %s } WHERE {}", sparql_before_delete, remove_triples_str);


    g_free(sparql_before_where);
	g_free(sparql_after_where);
	g_free(sparql_after_delete_before_where);
	g_free(sparql_before_delete);
	g_free(remove_triples_str);


	//printf ("%s\n", final_construct);
    return final_construct;
}


gchar * get_insert_construct(gchar * sparql_query)
{

	gchar ** split_result_array;
	gchar ** pointer_split_result_array;

	gchar * sparql_before_where;
	gchar * sparql_after_where;
	gchar * sparql_after_insert_before_where;
	gchar * sparql_before_insert;

	gchar * insert_triples_str;
	gchar * final_construct;

	gchar * sparql_before_delete;
	gchar * sparql_after_delete;
	gchar * sparql_after_delete_endbrace;

	gboolean * no_where = FALSE;

	if  (
		(strstr(sparql_query, "INSERT") != NULL) ||
		(strstr(sparql_query, "insert") != NULL)
		)
	{}
	else
		return NULL;

	//printf ("SPARQL UPDATE: INSERT\n");

    split_result_array=g_strsplit( sparql_query, where_uc , 0);
    if (g_strcmp0(*split_result_array, sparql_query)==0)
    {
        //not found
        g_strfreev (split_result_array);
        split_result_array=g_strsplit(sparql_query, where_lc , 0);

        if (g_strcmp0(*split_result_array, sparql_query)==0)
        {
            g_strfreev (split_result_array);
            no_where=TRUE;

        }
    }

	if (no_where == FALSE)
	{
		pointer_split_result_array= split_result_array;
		sparql_before_where = g_strdup(*pointer_split_result_array);
		pointer_split_result_array ++;
		sparql_after_where = g_strdup(*pointer_split_result_array);
		g_strfreev (split_result_array);
	}
    else
    {
    	if  (strstr(sparql_query, " DATA ") != NULL)
    	{
    		sparql_before_where=string_substitution_(sparql_query," DATA ", "");
    	}
    	else if (strstr(sparql_query, " data ") != NULL)
    	{
    		sparql_before_where=string_substitution_(sparql_query," data ", "");
    	}
    	else
    	{
    		sparql_before_where=g_strdup(sparql_query);
    	}

    	sparql_after_where= g_strdup_printf(" {}");
    }


    ///////////////// TAKING AWAY EVENTUAL DELETE PART


    gboolean delete_found = TRUE;

    split_result_array=g_strsplit( sparql_before_where, "DELETE" , 0);
    if (g_strcmp0(*split_result_array, sparql_before_where)==0)
    {
        //not found
        g_strfreev (split_result_array);
        split_result_array=g_strsplit(sparql_before_where, "delete"  , 0);

        if (g_strcmp0(*split_result_array, sparql_before_where)==0)
        {
        	delete_found = FALSE;
            //ERROR NO WHERE
        }
    }

    if (delete_found)
    {
    	//printf ("even delete found\n");
    	sparql_before_delete= g_strdup(*split_result_array);
    	gchar ** pointer_char_arr=split_result_array;
    	pointer_char_arr++;
    	sparql_after_delete= g_strdup(*pointer_char_arr);

    	g_strfreev (split_result_array);

        split_result_array=g_strsplit( sparql_after_delete, "}" , 0);
        if (g_strcmp0(*split_result_array, sparql_after_delete)==0)
        {
            g_free(sparql_before_where);
        	g_free(sparql_after_where);
            g_free(sparql_before_delete);
        	g_free(sparql_after_delete);


        	g_strfreev (split_result_array);
        	return NULL;
        }
    	pointer_char_arr=split_result_array;
    	pointer_char_arr++;
    	sparql_after_delete_endbrace= g_strdup(*pointer_char_arr);
    	g_strfreev (split_result_array);

    	gchar * temp_sparql_before_where= g_strdup_printf("%s %s }", sparql_before_delete ,sparql_after_delete_endbrace);
    	g_free(sparql_before_where);
    	sparql_before_where=temp_sparql_before_where;

    	g_free(sparql_after_delete_endbrace);
    	g_free(sparql_before_delete);
    	g_free(sparql_after_delete);

    }
    else
    {
    	g_strfreev (split_result_array);
    }


    ////////////////////////////////////////////////


    split_result_array=g_strsplit( sparql_before_where, "insert" , 0);
    if (g_strcmp0(*split_result_array, sparql_before_where)==0)
    {
        //not found
        g_strfreev (split_result_array);
        split_result_array=g_strsplit(sparql_before_where, "INSERT" , 0);

        if (g_strcmp0(*split_result_array, sparql_before_where)==0)
        {
            g_strfreev (split_result_array);

            g_free(sparql_before_where);
        	g_free(sparql_after_where);
            return NULL;
            //ERROR NO WHERE
        }
    }

    pointer_split_result_array= split_result_array;

    sparql_before_insert = g_strdup(*pointer_split_result_array);
    pointer_split_result_array ++;
    sparql_after_insert_before_where = g_strdup(*pointer_split_result_array);
    g_strfreev (split_result_array);


    split_result_array=g_strsplit( sparql_after_insert_before_where, "}" , 0);
    if (g_strcmp0(*split_result_array, sparql_after_insert_before_where)==0)
    {
        //not found
    	g_free(sparql_after_where);
    	g_free(sparql_before_insert);
    	g_free(sparql_after_insert_before_where);
    	g_free(sparql_before_where);

        g_strfreev (split_result_array);
        return NULL;
    }


    insert_triples_str = g_strdup(*split_result_array);
    g_strfreev (split_result_array);


    //OPTIMIZATION : No vars -> No query
    if  (g_strrstr(insert_triples_str, " ?") != NULL)
    		final_construct=g_strdup_printf("%s CONSTRUCT %s } WHERE %s", sparql_before_insert, insert_triples_str, sparql_after_where);
    else
    		final_construct=g_strdup_printf("%s CONSTRUCT %s } WHERE {}", sparql_before_insert, insert_triples_str);


    g_free(sparql_before_where);
	g_free(sparql_after_where);
	g_free(sparql_after_insert_before_where);
	g_free(sparql_before_insert);
	g_free(insert_triples_str);

	//printf ("%s\n", final_construct);
    return final_construct;
}

/////////////////////// TIME UTILITIES ////////////////////////////

double get_sparql_update_scheduled_time (char * sparql_query)
{

	gchar ** split_result_array;
	gchar ** pointer_split_result_array;

	gboolean no_where;
	gchar * sparql_before_where;

	gchar * sparql_after_timepref;
	gchar * sparql_time_value;
	gchar * sparql_time_value_temp;


    split_result_array=g_strsplit( sparql_query, where_uc , 0);
    if (g_strcmp0(*split_result_array, sparql_query)==0)
    {
        //not found
        g_strfreev (split_result_array);
        split_result_array=g_strsplit(sparql_query, where_lc , 0);

        if (g_strcmp0(*split_result_array, sparql_query)==0)
        {
            g_strfreev (split_result_array);
            no_where=TRUE;
        }
    }

	if (no_where == FALSE)
	{
		pointer_split_result_array= split_result_array;
		sparql_before_where = g_strdup(*pointer_split_result_array);
		g_strfreev (split_result_array);
	}
	else
	{
		sparql_before_where = g_strdup(sparql_query);
	}


	split_result_array=g_strsplit( sparql_before_where, sched_SIB_t , 0 );

    if (g_strcmp0(*split_result_array, sparql_before_where)==0)
    {
    	g_strfreev(split_result_array);
    	return 0;
    }

	pointer_split_result_array= split_result_array;
	pointer_split_result_array++;
	sparql_after_timepref=g_strdup(*pointer_split_result_array);
	g_strfreev(split_result_array);

	split_result_array=g_strsplit( sparql_after_timepref, ">" , 0 );
	sparql_time_value=g_strdup(*split_result_array);

	g_strfreev(split_result_array);
	g_free(sparql_after_timepref);

	sparql_time_value_temp=string_substitution_(sparql_time_value,"<", "");

	g_free(sparql_time_value);
	sparql_time_value=sparql_time_value_temp;

	double time_v = atof(sparql_time_value);
	g_free(sparql_time_value);

	//printf("%f\n", time_v);

	//struct timeval tv;
	//gettimeofday(&tv, 0);
	//double realtime= (int)tv.tv_sec + ((double)(int)tv.tv_usec / 1000000);
	//printf("%f\n", realtime);

	return time_v;
}

gchar * convert_sib_time_calls(char * sparql_query)
{
	gchar * result;

	struct timeval tv;
	gettimeofday(&tv, 0);
	double realtime= (int)tv.tv_sec + ((double)(int)tv.tv_usec / 1000000);
	gchar *realtime_str = g_strdup_printf("%f",realtime);

	result=string_substitution_(sparql_query, get_sib_time, realtime_str);

	g_free(realtime_str);

	return result;

}
