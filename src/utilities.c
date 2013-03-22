#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <utilities.h>
#include <redland.h>



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



static void navigator_graph_pattern_load(rasqal_graph_pattern *gp, int gp_index,int indent, GSList** template_query_)
{
  int triple_index = 0;

  raptor_sequence *seq;
  rasqal_variable * var;
  rasqal_triple* t;
  rasqal_literal_type type;

  GSList* template_query= *template_query_;

  indent += 1;

  /* look for triples */

  while(1) {

    t = rasqal_graph_pattern_get_triple(gp, triple_index);
    if(!t)
      break;

    /*
    rasqal_literal_type type;
    printf ("triple\n");
    type=rasqal_literal_get_rdf_term_type(t->subject);
    printf ("S type %d val %s\n", type, rasqal_literal_as_string (t->subject)) ;
    type=rasqal_literal_get_rdf_term_type(t->predicate);
    printf ("P type %d val %s\n", type, rasqal_literal_as_string (t->predicate)) ;
    type=rasqal_literal_get_rdf_term_type(t->object);
    printf ("O type %d val %s\n", type, rasqal_literal_as_string (t->object)) ;
	*/
    //ssTriple_t *ttemp = (ssTriple_t *)g_new0(ssTriple_t,1);


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
    	rasqal_free_variable (var);

    	ttemp->subject=g_strdup_printf("%s",wildcard1);
    }
    else
    {    	ttemp->subject=g_strdup(rasqal_literal_as_string (t->subject));
    		ttemp->subject_var=NULL;
    }

    type=rasqal_literal_get_rdf_term_type(t->predicate);
    if ((type==RASQAL_LITERAL_UNKNOWN) && (rasqal_literal_as_string(t->predicate) == NULL))
    {   //variable , needs a wildcard

    	var=rasqal_literal_as_variable(t->predicate);
    	ttemp->predicate_var=g_strdup(var->name);
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

    ttemp->gp_index=gp_index;
    ttemp->indent=indent;

    //printf("SPARQL Binding triple is: \t%s\t%s\t%s , obj_Type %d \n", ttemp->subject, ttemp->predicate, ttemp->object, ttemp->objType);
    //printf("				     Vars: \t%s\t%s\t%s , gp_index %d indent %d\n", ttemp->subject_var, ttemp->predicate_var, ttemp->object_var,ttemp->gp_index,ttemp->indent);

    template_query= g_slist_prepend(template_query, ttemp);

    triple_index++;
  }

  /* look for sub-graph patterns */
  seq = rasqal_graph_pattern_get_sub_graph_pattern_sequence(gp);
  if(seq && raptor_sequence_size(seq) > 0) {

    gp_index = 0;
    while(1)
    {
      rasqal_graph_pattern* sgp;
      sgp = rasqal_graph_pattern_get_sub_graph_pattern(gp, gp_index);
      if(!sgp)
    	  break;

      navigator_graph_pattern_load(sgp, gp_index, indent + 1, &template_query);
      gp_index++;
    }


  }

  indent -= 1;
  *template_query_=template_query;

}


gboolean *check_sparql_result_is_real(unsigned char * sparql_result)
{
	gboolean someresult=0;

	//printf ("all sparql %s\n", sparql_result);

	//if (g_strcmp0(sparql_result,"<results></results>")>0)
	//{
	//	printf("emptyresults \n");
	//	someresult=1;
	//}
	//else
	//{
		gchar** split_result_array;
		gchar** pointer_char_arr;

		split_result_array=	g_strsplit(sparql_result, end_binding	 , 0);
		pointer_char_arr = split_result_array;
		++pointer_char_arr;
		while ((*pointer_char_arr != NULL) && (someresult==0))
		{
			//printf( "part %s \n",*(pointer_char_arr-1) );
			if (g_str_has_suffix (*(pointer_char_arr-1), unbound)==true)
			{	//<unbound/> Not present
				someresult=0;
			}
			else
			{
				someresult=1;
			}
			++pointer_char_arr;
		}
		g_strfreev (split_result_array);

	//}
	//printf("someresult %d \n",someresult);
	return someresult;
}

ssStatus_t parseM3_sparql_bindings(GSList** template_query_, unsigned char * query_str, sib_data_structure* p)
{
	GSList* template_query =NULL;
	librdf_world * world;
	rasqal_query* rq;
	rasqal_graph_pattern* gp;

    gchar*		prefix_query_str;
    gchar*		prefix_str;

    //printf("RECEIVED SPARQL IN\n");
    //printf("q: %s\n",op->req->query_str);


    prefix_str= g_strdup_printf("PREFIX %s <%s> PREFIX %s <%s> PREFIX %s <%s> PREFIX %s <%s>  ",rdf_c ,rdf_ex ,fn_c_2,fn_ex,rdfs_c,rdfs_ex,xsd_c , xsd_ex);
    prefix_query_str=g_strdup_printf("%s %s", prefix_str, query_str);

	rq = rasqal_new_query(p->sparql_preprocessing_world, "sparql",NULL);
	//printf ("pre parsing\n");

	if(rasqal_query_prepare(rq, prefix_query_str, NULL)) {

		rasqal_free_query(rq);
		rq = NULL;
		*template_query_ =template_query;

		g_free( prefix_str);
		g_free( prefix_query_str);

		rasqal_free_world(world);
		return ss_GeneralError;
	}

	//printf ("query prepared\n");
	gp = rasqal_query_get_query_graph_pattern(rq);

	navigator_graph_pattern_load(gp, -1,  0, &template_query);

	*template_query_ =template_query;

	g_free( prefix_str);
	g_free( prefix_query_str);

	rasqal_free_query(rq);

	return ss_StatusOK;
}




void *sparql_string_added_removed(gchar **added_, gchar **removed_,gchar *new_res , gchar *old_res)
{

	gchar* added = *added_;
	gchar* removed = *removed_;

	////////////////////////////////////////////////////////////////////////////////
	gchar** split_result_array;

	gchar** split_result_new;
	gchar** split_result_old;

	gchar** pointer_char_arr;

	gchar** pointer_result_new;
	gchar** pointer_result_old;

	gchar * tempstr1;
	gchar * tempstr2;

	int present ;

	////////////////////////////////////////////////////////////////////////////////

	gchar *preamble_sparql;
	gchar *temp_str;

	gchar *results_new_res_str;
	gchar *results_old_res_str;

	//printf("new_res %s \n", new_res);
	//printf("old_res %s \n", old_res);


	split_result_array=g_strsplit(new_res, results_tag	 , 0);
	pointer_char_arr = split_result_array;

	preamble_sparql=g_strdup_printf("%s",*pointer_char_arr);
	++pointer_char_arr;
	temp_str=		g_strdup_printf("%s",*pointer_char_arr);

	g_strfreev (split_result_array);

	//printf("temp %s \n", temp_str);
	//printf("preamble sparql %s \n", preamble_sparql);

	split_result_array=	g_strsplit(temp_str, results_endtag	 , 0);
	results_new_res_str=g_strdup(*split_result_array);

	g_strfreev (split_result_array);
	g_free(temp_str);

	//printf("results_new_res_str %s \n", results_new_res_str);


	split_result_array=g_strsplit(old_res, results_tag	 , 0);
	pointer_char_arr = split_result_array;
	++pointer_char_arr;
	temp_str=		g_strdup(*pointer_char_arr);

	g_strfreev (split_result_array);

	split_result_array=	g_strsplit(temp_str, results_endtag	 , 0);
	results_old_res_str=g_strdup(*split_result_array);

	g_strfreev (split_result_array);
	g_free(temp_str);



	/////////////////////////////////////////////////////////////
	// got results_new_res_str  and results_old_res_str
	//


	split_result_new=g_strsplit(results_new_res_str, result_tag	 , 0);
	split_result_old=g_strsplit(results_old_res_str, result_tag	 , 0);


	pointer_result_new = split_result_new;
	pointer_result_old = split_result_old;


	tempstr1=g_strdup_printf("%s%s",preamble_sparql,results_tag);

	while (*pointer_result_new != NULL)
	{

		if (g_strcmp0(*pointer_result_new,"")==0)
		{	++pointer_result_new;	}
		else
		{
			//printf("cycling on new values\n");
			//////////////////////////////////////////////////////////////////////
			present =0;
			pointer_result_old = split_result_old;
			while (*pointer_result_old != NULL)
			{
				if (g_strcmp0(*pointer_result_old,"")==0)
				{	++pointer_result_old;
				}
				else
				{

					//printf("comparing %s   %s\n", *pointer_result_new, *pointer_result_old);
					if (g_strcmp0(*pointer_result_new,*pointer_result_old)==0)
						present =1;


					if (present ==1)
					{
						break;
					}
					++pointer_result_old;

				}

			}
			//////////////////////////////////////////////////////////////////////

			if (present == 0)
			{
				//New result , appending!
				tempstr2=g_strdup_printf("%s%s%s",tempstr1, result_tag	,*pointer_result_new );
				g_free(tempstr1);
				tempstr1=g_strdup(tempstr2);
				g_free(tempstr2);

				//printf ("new result: %s%s\n",result_tag,*pointer_result_new);
			}
			//printf ("new result: %s%s\n",result_tag,*pointer_result_new);
			++pointer_result_new;
		}
	}

	added=g_strdup_printf("%s%s%s",tempstr1,results_endtag,sparql_end);
	g_free(tempstr1);
	//printf ("Added final sparql values %s\n", added);


	pointer_result_new = split_result_new;
	pointer_result_old = split_result_old;


	tempstr1=g_strdup_printf("%s%s",preamble_sparql,results_tag);

	while (*pointer_result_old != NULL)
	{

		if (g_strcmp0(*pointer_result_old,"")==0)
		{	++pointer_result_old;	}
		else
		{

			//////////////////////////////////////////////////////////////////////
			present =0;
			pointer_result_new = split_result_new;
			while (*pointer_result_new != NULL)
			{
				if (g_strcmp0(*pointer_result_new,"")==0)
				{	++pointer_result_new; }
				else
				{

					//printf("comparing %s   %s\n", *pointer_result_old, *pointer_result_new);
					if (g_strcmp0(*pointer_result_old,*pointer_result_new)==0)
						present =1;

					if (present ==1)
					{
						break;
					}
					++pointer_result_new;
				}

			}
			//////////////////////////////////////////////////////////////////////

			if (present == 0)
			{
				//New result , appending!
				tempstr2=g_strdup_printf("%s%s%s",tempstr1, result_tag	,*pointer_result_old );
				g_free(tempstr1);
				tempstr1=g_strdup(tempstr2);
				g_free(tempstr2);

				//printf ("old result: %s%s\n",result_tag,*pointer_result_old);
			}
			//printf ("old result: %s%s\n",result_tag,*pointer_result_old);
			++pointer_result_old;
		}
	}

	removed=g_strdup_printf("%s%s%s",tempstr1,results_endtag,sparql_end);
	g_free(tempstr1);
	//printf ("Obsolete final sparql values %s\n", removed);


	g_strfreev (split_result_new);
	g_strfreev (split_result_old);

	g_free(results_new_res_str);
	g_free(results_old_res_str);


	*added_= added ;
	*removed_ =removed;

}


void initialize_storage_model(sib_data_structure* sd)
{
	//MAIN TRIPLESTORE
	if (sd->mem_volatile)
	{
		sd->RDF_storage=librdf_new_storage(sd->RDF_world, "hashes", NULL, "hash-type='memory'");
		//sd->RDF_storage=librdf_new_storage(sd->RDF_world,  NULL, NULL, NULL);
	}
	else
	{
		if (sd->sqlite)
		{
			if ((file_exists(sd->ss_name)) && (sd->enable_rdf_pp == FALSE))
			{
				printf("Found a local sqlite db, re-charging.. \n");
				sd->RDF_storage=librdf_new_storage(sd->RDF_world, "sqlite", sd->ss_name, NULL);
			}
			else
			{	printf("Creating a new Sqlite db.. \n");
			sd->RDF_storage=librdf_new_storage(sd->RDF_world, "sqlite", sd->ss_name, "new='yes'");
			}
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


			if ((
					(file_exists( sp2o)) &&
					(file_exists( so2p)) &&
					(file_exists( po2s))
			) && (sd->enable_rdf_pp == FALSE))
			{
				printf("Found a BDB local file, re-charging.. \n");
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
	sd->RDF_model=librdf_new_model(sd->RDF_world, sd->RDF_storage , NULL);

}
void reset_storage_model(sib_data_structure* sd)
{
	// Cleaning ////////////////////////
	librdf_free_model(sd->RDF_model);
	librdf_free_storage(sd->RDF_storage);
	////////////////////////////////////

	//MAIN TRIPLESTORE
	if (sd->mem_volatile)
	{
		sd->RDF_storage=librdf_new_storage(sd->RDF_world, "hashes", NULL, "hash-type='memory'");
	}
	else
	{
		if (sd->sqlite)
		{
			sd->RDF_storage=librdf_new_storage(sd->RDF_world, "sqlite", sd->ss_name, "new='yes'");
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


			sd->RDF_storage=librdf_new_storage(sd->RDF_world, "hashes", sd->ss_name, "new='yes',hash-type='bdb',dir='.'");


			g_free(sp2o);
			g_free(po2s);
			g_free(so2p);


		}
	}
	sd->RDF_model=librdf_new_model(sd->RDF_world, sd->RDF_storage , NULL);
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

		if (t->gp_index)
			t->gp_index=NULL;
		if (t->indent)
			t->indent=NULL;

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


void statement_in_tl(GSList ** list_, librdf_statement * statement)
{
	GSList * list=*list_;

	ssTriple_t * triple;

	librdf_node* sub_node ;
	librdf_node* pred_node;
	librdf_node* obj_node ;

	sub_node  = librdf_statement_get_subject(statement);
	pred_node = librdf_statement_get_predicate(statement);
	obj_node  = librdf_statement_get_object(statement);

	triple  =(ssTriple_t *) g_new0(ssTriple_t,1);

	triple->subject=	g_strdup(librdf_uri_as_string( librdf_node_get_uri(sub_node)));
	triple->predicate =	g_strdup(librdf_uri_as_string( librdf_node_get_uri(pred_node)));

	if (librdf_node_is_literal(obj_node))
	{	triple->object =  g_strdup(librdf_node_get_literal_value(obj_node));
		triple->objType = ssElement_TYPE_LIT;
	}
	else
	{	triple->object =  g_strdup(librdf_uri_as_string( librdf_node_get_uri(obj_node)));
		triple->objType = ssElement_TYPE_URI;
	}

	//printf ("statement to TL %s_%s_%s_%d\n", (char *)triple->subject, (char *)triple->predicate, (char *)triple->object, (bool *)triple->objType);
	list=g_slist_prepend(list, triple);
	///////////////////////////////////////////////////////////

	*list_=list;

}




///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////SPARQL TURBO SUBSCRIBE UTILITIES///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


gint get_gp_num(GSList * query_list_)
{
	GSList * query_list;
	ssTriple_t_sparql* tq;
	query_list= query_list_;

	int gp_max_num =0;
	while (query_list != NULL)
	{
		tq = (ssTriple_t_sparql*)query_list->data;
		if (tq->gp_index > gp_max_num)
			 gp_max_num=tq->gp_index;
		query_list = query_list->next;
	}
	return gp_max_num;
}


gint find_min_indent_in_gp(int gp_index, GSList * query_list_)
{
	GSList * query_list;
	ssTriple_t_sparql* tq;
	query_list= query_list_;

	int indent_min =-1;

	int counter =0;
	while (query_list != NULL)
	{
		tq = (ssTriple_t_sparql*)query_list->data;
		if (tq->gp_index == gp_index)
		{
			if (counter == 0)
			{
				indent_min=tq->indent;
			}
			else
			{
				if (tq->indent < indent_min)
					indent_min=tq->indent;
			}
		}
		counter++;
		query_list = query_list->next;
	}
	return indent_min;
}

void find_variable_list_group_indent(GSList **variable_list_, GSList * query_list, gint  gp_index, gint indent)
{
	GSList * variable_list= *variable_list_;
	GSList * variable_list_pointer;
	ssTriple_t_sparql* tq;
	gboolean found;
	gchar * var_tmp;

	while (query_list != NULL)
	{
		tq = (ssTriple_t_sparql*)query_list->data;

		if ((tq->gp_index==gp_index) && (tq->indent==indent))
		{

			////////////// subject
			variable_list_pointer=variable_list;
			found=0;
			while (variable_list_pointer != NULL)
			{
				var_tmp= variable_list_pointer->data;
				if (g_strcmp0(var_tmp, tq->subject_var)==0)
				{
					found=1;
				}
				variable_list_pointer=variable_list_pointer->next;
			}
			if (found ==0)
			{
				gchar * new_var= g_strdup(tq->subject_var);
				variable_list=g_slist_prepend(variable_list, new_var);
			}


			////////////// predicate
			variable_list_pointer=variable_list;
			found=0;
			while (variable_list_pointer != NULL)
			{
				var_tmp= variable_list_pointer->data;
				if (g_strcmp0(var_tmp, tq->predicate_var)==0)
				{
					found=1;
				}
				variable_list_pointer=variable_list_pointer->next;
			}
			if (found ==0)
			{
				gchar * new_var= g_strdup(tq->predicate_var);
				variable_list=g_slist_prepend(variable_list, new_var);
			}

			////////////// object
			variable_list_pointer=variable_list;
			found=0;
			while (variable_list_pointer != NULL)
			{
				var_tmp= variable_list_pointer->data;
				if (g_strcmp0(var_tmp, tq->object_var)==0)
				{
					found=1;
				}
				variable_list_pointer=variable_list_pointer->next;
			}
			if (found ==0)
			{
				gchar * new_var= g_strdup(tq->object_var);
				variable_list=g_slist_prepend(variable_list, new_var);
			}


		}
		query_list = query_list->next;
	}

	*variable_list_=variable_list;
}

gboolean check_group_value (gint * gp_index, scheduler_item* op)
{
	gboolean result = false;
	GSList * query_list=op->req->template_query;
	GSList * variable_list;
	ssTriple_t_sparql* tq;

	int min_indent_in_gp = find_min_indent_in_gp( gp_index, query_list);
	find_variable_list_group_indent(&(variable_list), query_list,  gp_index, min_indent_in_gp);


	while (query_list != NULL)
	{
		tq = (ssTriple_t_sparql*)query_list->data;


		query_list = query_list->next;
	}

	return result;

}


gboolean is_sparql_check_necessary(scheduler_item* op)
{
	//result : really make query? true -> yes

	GSList * query_list=op->req->template_query;
	int number_of_groups;
	int group_counter;

	gboolean result = false;
	number_of_groups=get_gp_num(query_list);
	printf("Number of groups %d \n",number_of_groups );

	group_counter=0;
	while ((group_counter<= number_of_groups) || (result!=true))
	{
		result= result + check_group_value(group_counter, op);

		group_counter++;
	}

	return true;
}


