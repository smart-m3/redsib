#include <sparql_booster.h>

//SPARQL BOOSTER FUNCTIONS///

gint get_gp_num_max(GSList * query_list_)
{
    GSList * query_list;
    ssTriple_t_sparql* tq;
    query_list= query_list_;

    int gp_max_val =0;
    while (query_list != NULL)
    {
        tq = (ssTriple_t_sparql*)query_list->data;
        if (tq->gp_index > gp_max_val)
        {
             gp_max_val=tq->gp_index;
        }
        query_list = query_list->next;
    }
    return gp_max_val;
}


gint find_min_indent_in_gp(int gp_index, GSList* query_list_)
{
    GSList * query_list;
    ssTriple_t_sparql* tq;
    query_list= query_list_;

    gint indent_min =-2;

    int counter =0;

    //printf("gp_index  %d \n", gp_index );

    while (query_list != NULL)
    {

        tq = (ssTriple_t_sparql*)query_list->data;
        //printf("cycling on gp %d \n",tq->gp_index  );

        if (tq->gp_index == gp_index)
        {
            if (counter == 0)
            {
                indent_min=tq->indent;
                //printf("first min indent %d \n", indent_min );
                counter++;
            }
            else
            {
                if (tq->indent < indent_min)
                {
                    indent_min=tq->indent;
                    //printf("new min indent %d \n", indent_min );
                }
            }
        }

        query_list = query_list->next;
    }

    //printf("\nINDENT MIN %d\n",indent_min );
    return indent_min;
}

void find_variable_list_group_indent(GSList **variable_list_, GSList * query_list, gint  gp_index, gint indent)
{
    GSList * variable_list= *variable_list_;
    GSList * variable_list_pointer;
    ssTriple_t_sparql* tq;
    gboolean found;
    gchar * var_tmp;

    variable_list=NULL;

    //printf("gp_index %d indent %d \n", gp_index, indent );

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
            if ((found ==0) && (tq->subject_var!=NULL))
            {
                gchar * new_var= g_strdup(tq->subject_var);
                variable_list=g_slist_prepend(variable_list, new_var);
                //printf("variable added  %s \n", new_var);
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
            if ((found ==0) && (tq->predicate_var!=NULL))
            {
                gchar * new_var= g_strdup(tq->predicate_var);
                variable_list=g_slist_prepend(variable_list, new_var);
                //printf("variable added  %s \n", new_var);
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
            if ((found ==0) && (tq->object_var!=NULL))
            {
                gchar * new_var= g_strdup(tq->object_var);
                variable_list=g_slist_prepend(variable_list, new_var);
                //printf("variable added  %s \n", new_var);
            }

        }
        query_list = query_list->next;
    }

    *variable_list_=variable_list;
}



void free_sparql_result_boost(sparql_binding_struct  * pointer_binding)
{
    if (pointer_binding->string)
        g_free(pointer_binding->string);
    if (pointer_binding->type)
        pointer_binding->type=NULL;
    if (pointer_binding->var_name)
        g_free(pointer_binding->var_name);

    g_free(pointer_binding);
}

void clean_sparql_booster_results(GSList ** results)
{
    GSList * pointer_results_list = *results;
    GSList * pointer_result_list =NULL;
    sparql_binding_struct * pointer_binding =NULL;

    while (pointer_results_list  !=NULL)
    {
        pointer_result_list=pointer_results_list->data;

        while (pointer_result_list !=NULL)
        {
            pointer_binding=pointer_result_list->data;
            free_sparql_result_boost(pointer_binding);

            pointer_result_list=pointer_result_list->next;
        }

        pointer_result_list=pointer_results_list->data;
        g_slist_free(pointer_result_list);

        pointer_results_list =pointer_results_list->next;
    }

    pointer_results_list = *results;
    g_slist_free(pointer_results_list);

    *results=NULL;
}


gchar * turn_sparql_booster_results_to_str(GSList * results, GSList *sparql_total_vars)
{
    GSList * pointer_results_list = results;
    GSList * pointer_result_list =NULL;
    GSList * pointer_bindings_list =NULL;
    GSList * sparql_total_vars_pointer= NULL;

    gchar * point_var = NULL;
    sparql_binding_struct * pointer_binding = NULL;

    gchar * result = NULL;


    if( results == NULL)
    {
        result =g_strdup_printf("");
        return result;
    }

    //printf ("\nTurn_sparql_booster_results_to_str\n");

    //Building header
    gchar *header =g_strdup(head_tag);
    gchar *header_tmp =NULL;

    sparql_total_vars_pointer=sparql_total_vars;
    while (sparql_total_vars_pointer !=NULL)
    {
        point_var=sparql_total_vars_pointer->data;
        header_tmp=g_strdup_printf("%s<variable name=\"%s\"/>", header, point_var);

        g_free(header);
        header=g_strdup(header_tmp);
        g_free(header_tmp);

        sparql_total_vars_pointer=sparql_total_vars_pointer->next;
    }


    header_tmp=g_strdup_printf("%s%s",header,head_endtag);
    g_free(header);
    header=g_strdup(header_tmp);
    g_free(header_tmp);

    //printf ("head: \n%s\n",header );

    //Building Body
    gboolean * founded_var=0;

    gchar *body =g_strdup(results_tag);
    gchar *body_tmp =NULL;


    while (pointer_results_list !=NULL)
    {
        //printf ("result cycle\n\n");

        pointer_result_list=pointer_results_list->data;

        body_tmp=g_strdup_printf("%s%s", body,result_tag);
        g_free(body);
        body=g_strdup(body_tmp);
        g_free(body_tmp);

        sparql_total_vars_pointer=sparql_total_vars;

        while (sparql_total_vars_pointer !=NULL)
        {
            //pointer_result_list=pointer_results_list->data;

            pointer_bindings_list=pointer_result_list;
            point_var=sparql_total_vars_pointer->data;

            //printf("var cycle : %s\n\n" ,point_var );
            founded_var=0;

            while (pointer_bindings_list !=NULL)
            {
                pointer_binding = pointer_bindings_list->data;
                //printf ("bindings cycle\n\n");

                //printf("bind->var_name: %s\n",pointer_binding->var_name);
                //printf("bind->string: %s\n",pointer_binding->string);
                //printf("bind->type: %d\n",pointer_binding->type);

                if (g_strcmp0(pointer_binding->var_name, point_var) == 0)
                {
                    //printf("found\n");

                    if (g_strcmp0(pointer_binding->string ,"")==0)
                    {
                        founded_var=0;
                        break;
                    }
                    else
                    {
                        founded_var=1;
                        if (pointer_binding->type==0)
                        {
                            body_tmp=g_strdup_printf("%s<binding name=\"%s\"><uri>%s</uri></binding>", body, pointer_binding->var_name, pointer_binding->string );
                            g_free(body);
                            body=g_strdup(body_tmp);
                            g_free(body_tmp);
                        }
                        else
                        {
                            body_tmp=g_strdup_printf("%s<binding name=\"%s\"><literal>%s</literal></binding>", body, pointer_binding->var_name, pointer_binding->string );
                            g_free(body);
                            body=g_strdup(body_tmp);
                            g_free(body_tmp);
                        }
                        break;
                    }
                }


                //printf ("end binding\n\n");
                pointer_bindings_list=pointer_bindings_list->next;
            }

            if (founded_var == 0)
            {
                body_tmp=g_strdup_printf("%s<binding name=\"%s\"><unbound/></binding>", body, point_var);
                g_free(body);
                body=g_strdup (body_tmp);
                g_free(body_tmp);
            }

            sparql_total_vars_pointer=sparql_total_vars_pointer->next;
        }

        body_tmp=g_strdup_printf("%s%s", body, result_endtag);
        g_free(body);
        body=g_strdup (body_tmp);
        g_free(body_tmp);

        pointer_results_list=pointer_results_list->next;
    }

    body_tmp=g_strdup_printf("%s%s", body, results_endtag);
    g_free(body);
    body=g_strdup (body_tmp);
    g_free(body_tmp);

    //printf("\nHEADER\n%s\n",header);
    //printf("\nBODY\n%s\n",body);

    result=g_strdup_printf("%s%s%s%s",sparql_tag, header, body, sparql_endtag);

    g_free(header);
    g_free(body);

    //printf("\n%s\n", result);


    return result;

}

//1 non present (ok), 0 present
gboolean * sparql_result_not_found_in_results(GSList * target_result, GSList * results)
{
    GSList * pointer_result_lists  = NULL;
    GSList * pointer_target_result = NULL;
    GSList * pointer_result = NULL;

    int equal;

    sparql_binding_struct * pointer_target_binding =NULL;
    sparql_binding_struct * pointer_bindings_list =NULL;

    pointer_result_lists  = results;
    while (pointer_result_lists !=NULL)
    {
        pointer_result=pointer_result_lists->data;

        equal=1;

        pointer_target_result=target_result;
        while ((pointer_target_result !=NULL) && (pointer_result !=NULL))
        {
            pointer_bindings_list=pointer_result->data;
            pointer_target_binding=pointer_target_result->data;

            if (
                    (g_strcmp0(pointer_target_binding->string,  pointer_bindings_list->string)==0) &&
                    (g_strcmp0(pointer_target_binding->var_name,  pointer_bindings_list->var_name)==0) &&
                    (pointer_target_binding->type == pointer_bindings_list->type)
                )
            { }
            else
            {
                equal=0;
                break;
            }

            pointer_target_result=pointer_target_result->next;
            pointer_result=pointer_result->next;
        }

        if (equal==1)
        {
            //the result is in the queue
            return false;
        }

        pointer_result_lists=pointer_result_lists->next;
    }
    return true;

}

gchar * string_substitution (gchar * input_str, gchar * obsolete_substr, gchar * new_substr )
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
    {

        return old_result;
    }

    g_free(old_result);
    return result;
}

gint * load_sparql_morphism(ssap_kp_message *req, ssap_sib_message *rsp,  ssTriple_t_sparql *tq_reference , ssTriple_t_sparql *tir_reference, GSList **variable_list_, int * ins_rm)
{

    GSList * query_list = req->template_query;
    ssTriple_t_sparql* tq = NULL;

    GSList * variable_list = *variable_list_;

    gchar*                  prefix_query_str =NULL;
    gchar*                  temp_question_var=NULL;
    gchar*                  temp_node_value=NULL;

    gchar * before_where_sparql=NULL;
    gchar * after_where_sparql=NULL;
    gchar * final_sparql=NULL;

    gchar * temp_before_where_sparql=NULL;
    gchar * temp_after_where_sparql =NULL;

    gchar** split_result_array;
    gchar** pointer_char_arr;

    librdf_query *  query =NULL;
    librdf_query_results* query_results=NULL;

    int num_involved_block_var=       g_slist_length (variable_list);

    int num_total_sparql_query_vars= g_slist_length (rsp->sparql_vars);
    int num_required_sparql_query_vars= g_slist_length (rsp->required_sparql_vars);

    int substituted_vars=0;
    int substituted_required_vars=0;

    //printf("involved_block_var %d \n",num_involved_block_var);
    //printf("num_total_sparql_query_vars %d \n",num_total_sparql_query_vars);

    int results_generated= 0;

    // Checking uninvolved vars and crating list

    GSList * pointer_all_vars = rsp->sparql_vars;
    GSList * pointer_block_vars = variable_list;
    GSList * uninvolved_vars=NULL;
    GSList * uninvolved_vars_temp=NULL;

    gboolean found_var =FALSE;
    gchar * tempvar=NULL;

	//Time utilities
    struct timeval start, end;
    unsigned long mtime, seconds, useconds;



    while (pointer_all_vars != NULL)
    {
        found_var =FALSE;
        pointer_block_vars = variable_list;

        while  (pointer_block_vars != NULL)
        {
            if (g_strcmp0(pointer_all_vars->data, pointer_block_vars->data)==0)
            {
                found_var =TRUE;
                break;
            }
            pointer_block_vars=pointer_block_vars->next;
        }

        if (found_var==FALSE)
        {
            //printf("uninvolved var: %s\n", pointer_all_vars->data);
            gchar * tempvar =NULL;
            tempvar=g_strdup(pointer_all_vars->data);
            uninvolved_vars=g_slist_prepend(uninvolved_vars, tempvar);
        }

        pointer_all_vars=pointer_all_vars->next;
    }


    //SPLITTING QUERY in WHERE
    split_result_array=g_strsplit( rsp->sparql_query_str, where_uc , 0);
    if (g_strcmp0(*split_result_array, prefix_query_str)==0)
    {
        //not found
        g_strfreev (split_result_array);
        split_result_array=g_strsplit(rsp->sparql_query_str, where_lc , 0);

        if (g_strcmp0(*split_result_array, prefix_query_str)==0)
        {
            g_strfreev (split_result_array);
            printf("ERROR, NO WHERE SPARQL\n");
            exit(-1);
            //ERROR NO WHERE
        }
    }

    pointer_char_arr = split_result_array;
    before_where_sparql=g_strdup_printf("%s",*pointer_char_arr);
    pointer_char_arr++;
    after_where_sparql =g_strdup_printf("%s",*pointer_char_arr);
    g_strfreev (split_result_array);

    //REMOVE * if present

	if  (
		(strstr(rsp->sparql_query_str, " * ") != NULL)
		)
	{
		//if the strings exists

		gchar *involved_var_string=g_strdup("");
		gchar *involved_var_string_tmp=NULL;

        GSList * pointer_require_vars = rsp->required_sparql_vars;
        while (pointer_require_vars)
        {
            gchar * required_var = pointer_require_vars->data;
            involved_var_string_tmp=g_strdup_printf("%s ?%s ", involved_var_string, required_var );
            g_free(involved_var_string);
            involved_var_string=g_strdup(involved_var_string_tmp);
            g_free(involved_var_string_tmp);

            pointer_require_vars=pointer_require_vars->next;
        }

        temp_before_where_sparql= string_substitution (before_where_sparql, " * ", involved_var_string);
        g_free(involved_var_string);
        g_free(before_where_sparql);
        before_where_sparql = g_strdup(temp_before_where_sparql);
        g_free(temp_before_where_sparql);

        //printf("%s\n", before_where_sparql);
	}


    //REMOVE-SUBSTITUTE UNINVOLVED VARS

    if (tq_reference->subject_var != NULL)
    {
        temp_question_var=g_strdup_printf("?%s" ,tq_reference->subject_var);
        temp_before_where_sparql= string_substitution (before_where_sparql, temp_question_var, "");

        g_free(before_where_sparql);
        before_where_sparql = g_strdup(temp_before_where_sparql);
        g_free(temp_before_where_sparql);

        temp_node_value  =g_strdup_printf("<%s>",tir_reference->subject);
        temp_after_where_sparql= string_substitution (after_where_sparql, temp_question_var, temp_node_value);

        g_free(after_where_sparql);
        after_where_sparql = g_strdup(temp_after_where_sparql);
        g_free(temp_after_where_sparql);
        g_free(temp_question_var);
        g_free(temp_node_value);

        substituted_vars++;

        //////////////////////////////////////////////////////////////
        GSList * pointer_require_vars = rsp->required_sparql_vars;
        while (pointer_require_vars)
        {
            gchar * required_var = pointer_require_vars->data;
            if (g_strcmp0(tq_reference->subject_var, required_var) ==0)
            {
                substituted_required_vars++;
            }
            pointer_require_vars=pointer_require_vars->next;
        }
        //////////////////////////////////////////////////////////////
    }
    if (tq_reference->predicate_var != NULL)
    {
        temp_question_var=g_strdup_printf("?%s" ,tq_reference->predicate_var);
        temp_before_where_sparql= string_substitution (before_where_sparql, temp_question_var, "");

        g_free(before_where_sparql);
        before_where_sparql = g_strdup(temp_before_where_sparql);
        g_free(temp_before_where_sparql);

        temp_node_value  =g_strdup_printf("<%s>",tir_reference->predicate);
        temp_after_where_sparql= string_substitution (after_where_sparql, temp_question_var, temp_node_value);

        g_free(after_where_sparql);
        after_where_sparql = g_strdup(temp_after_where_sparql);
        g_free(temp_after_where_sparql);
        g_free(temp_question_var);
        g_free(temp_node_value);

        substituted_vars++;

        //////////////////////////////////////////////////////////////
        GSList * pointer_require_vars = rsp->required_sparql_vars;
        while (pointer_require_vars)
        {
            gchar * required_var = pointer_require_vars->data;
            if (g_strcmp0(tq_reference->predicate_var, required_var) ==0)
            {
                substituted_required_vars++;
            }
            pointer_require_vars=pointer_require_vars->next;
        }
        //////////////////////////////////////////////////////////////
    }
    if (tq_reference->object_var != NULL)
    {
        temp_question_var=g_strdup_printf("?%s" ,tq_reference->object_var);
        temp_before_where_sparql= string_substitution (before_where_sparql, temp_question_var, "");
        g_free(before_where_sparql);
        before_where_sparql = g_strdup(temp_before_where_sparql);
        g_free(temp_before_where_sparql);

        if (tir_reference->objType ==0) //URI
        {   temp_node_value  =g_strdup_printf("<%s>",tir_reference->object);
        }
        else                            //Literal
        {   temp_node_value  =g_strdup_printf("\"%s\"",tir_reference->object);
        }

        temp_after_where_sparql= string_substitution (after_where_sparql, temp_question_var, temp_node_value);

        g_free(after_where_sparql);
        after_where_sparql = g_strdup(temp_after_where_sparql);
        g_free(temp_after_where_sparql);
        g_free(temp_question_var);
        g_free(temp_node_value);

        substituted_vars++;

        //////////////////////////////////////////////////////////////
        GSList * pointer_require_vars = rsp->required_sparql_vars;
        while (pointer_require_vars)
        {
            gchar * required_var = pointer_require_vars->data;
            if (g_strcmp0(tq_reference->object_var, required_var) ==0)
            {
                substituted_required_vars++;
            }
            pointer_require_vars=pointer_require_vars->next;
        }
        //////////////////////////////////////////////////////////////
    }

    //printf ("after_where_sparql %s\n", after_where_sparql );
    //Erasing unwanted vars from after_where_sparql (TODO removing entire block) -- similar performances

    uninvolved_vars_temp = uninvolved_vars;
    while (uninvolved_vars_temp !=NULL)
    {
        gchar *tempchar;
        tempchar=uninvolved_vars_temp->data;

        temp_question_var=g_strdup_printf("?%s" ,tempchar);

        //after where
        temp_after_where_sparql= string_substitution (after_where_sparql, temp_question_var, nulluri);
        if(g_strcmp0(after_where_sparql,temp_after_where_sparql)!=0)  //STRINGS CHANGED
        {

            substituted_vars++;

            g_free(after_where_sparql);
            after_where_sparql = g_strdup(temp_after_where_sparql);
            g_free(temp_after_where_sparql);
        }

        //before where
        temp_before_where_sparql= string_substitution (before_where_sparql, temp_question_var, "");
        if(g_strcmp0(before_where_sparql,temp_before_where_sparql)!=0) //STRINGS CHANGED
        {

            substituted_required_vars++;

            g_free(before_where_sparql);
            before_where_sparql = g_strdup(temp_before_where_sparql);
            g_free(temp_before_where_sparql);

        }
        g_free(temp_question_var);

        uninvolved_vars_temp=uninvolved_vars_temp->next;
    }

    //Deleting ununsed vars list
    uninvolved_vars_temp = uninvolved_vars;
    while (uninvolved_vars_temp !=NULL)
    {
        gchar * uninv_tmp =uninvolved_vars_temp->data;
        g_free(uninv_tmp);
        uninvolved_vars_temp=uninvolved_vars_temp->next;
    }
    g_slist_free(uninvolved_vars);


    //printf ("substituted_required_vars %d - num_required_sparql_query_vars %d \n",substituted_required_vars, num_required_sparql_query_vars);

    //Getting final query
    final_sparql= g_strdup_printf("%s WHERE %s", before_where_sparql, after_where_sparql);

    if      (
            (substituted_vars>=num_total_sparql_query_vars) ||
            (substituted_required_vars>=num_required_sparql_query_vars)
            )
    {
        gchar * temp_final_sparql = NULL;
        temp_final_sparql=  string_substitution (final_sparql, "SELECT", "ASK");
        g_free(final_sparql);
        final_sparql=g_strdup(temp_final_sparql);
        g_free(temp_final_sparql);
    }
    g_free(before_where_sparql);
    g_free(after_where_sparql);

	//TIME MEASUREMENT UTILITY LINES
	//gettimeofday(&start, NULL);
	/////////////////////////////

	gchar * final_sparql_timed = convert_sib_time_calls(final_sparql);
	g_free(final_sparql);
	final_sparql=final_sparql_timed;

    //printf("ACCELERATOR QUERY: \n%s\n",final_sparql);
    query=librdf_new_query( rsp->subworld, "sparql", NULL, final_sparql , NULL);
    query_results=librdf_model_query_execute(rsp->submodel, query);
    g_free(final_sparql);

	///////////////////////////////////////
	//gettimeofday(&end, NULL);
	//seconds  = end.tv_sec  - start.tv_sec;
	//useconds = end.tv_usec - start.tv_usec;
	//fprintf(stderr,"Morphism Pre-Query Time [us,sub_id] : %ld \n",  seconds * 1000000 + useconds);
	///////////////////////////////////////

	//TIME MEASUREMENT UTILITY LINES
	//gettimeofday(&start, NULL);
	/////////////////////////////
    if (query_results == NULL)
    {
        //printf("SPARQL syntax error! \n");
        librdf_free_query_results(query_results);
        librdf_free_query(query);

    	///////////////////////////////////////
    	//gettimeofday(&end, NULL);
    	//seconds  = end.tv_sec  - start.tv_sec;
    	//useconds = end.tv_usec - start.tv_usec;
    	//fprintf(stderr,"Morphism Post-Query no result Time [us] : %ld \n",  seconds * 1000000 + useconds);
    	///////////////////////////////////////

        return FALSE;
    }
    else
    {
        if (librdf_query_results_is_boolean(query_results))
        {
        	if (librdf_query_results_get_boolean(query_results))
        	{
				//printf("BOOLEAN RESULT: %d\n",librdf_query_results_get_boolean(query_results));
				GSList * sparql_result = NULL;
				GSList * var_pointer = rsp->required_sparql_vars;
				//GSList * var_pointer = rsp->sparql_vars;
				gchar * varchar =NULL;

				//Some results found
				results_generated=1;

				while (var_pointer != NULL)
				{
					varchar=var_pointer->data;

					//printf("varchar %s \n", varchar);
					//printf("subject_var %s \n", tq_reference->subject_var);
					//printf("predicate_var %s \n", tq_reference->predicate_var);
					//printf("object_var %s \n", tq_reference->object_var);

					if      (g_strcmp0(tq_reference->subject_var, varchar )==0)
					{
						//printf("sub found \n");

						sparql_binding_struct * sparql_binding = (sparql_binding_struct *)g_new0(sparql_binding_struct,1);

						sparql_binding->var_name=g_strdup(varchar);
						sparql_binding->string=g_strdup(tir_reference->subject);
						sparql_binding->type=0;

						sparql_result =  g_slist_prepend(sparql_result, sparql_binding);
					}
					else if (g_strcmp0(tq_reference->predicate_var, varchar )==0)
					{
						//printf("pred found \n");

						sparql_binding_struct * sparql_binding = (sparql_binding_struct *)g_new0(sparql_binding_struct,1);

						sparql_binding->var_name=g_strdup(varchar);
						sparql_binding->string=g_strdup(tir_reference->predicate);
						sparql_binding->type=0;

						sparql_result =  g_slist_prepend(sparql_result, sparql_binding);
					}
					else if (g_strcmp0(tq_reference->object_var, varchar )==0)
					{
						//printf("obj found \n");

						sparql_binding_struct * sparql_binding = (sparql_binding_struct *)g_new0(sparql_binding_struct,1);

						sparql_binding->var_name=g_strdup(varchar);
						sparql_binding->string=g_strdup(tir_reference->object);

						if (tir_reference->objType)
							sparql_binding->type=1;
						else
							sparql_binding->type=0;

						sparql_result = g_slist_prepend(sparql_result, sparql_binding);
					}
					else
					{
						//UNBOUND
						sparql_binding_struct * sparql_binding = (sparql_binding_struct *)g_new0(sparql_binding_struct,1);
						sparql_binding->var_name=g_strdup(varchar);
						sparql_binding->string=g_strdup_printf("");
						sparql_binding->type=NULL;
						sparql_result = g_slist_prepend(sparql_result, sparql_binding);
					}

					var_pointer=var_pointer->next;
				}

				if (ins_rm == 0)
				{
					//printf("Check on Added :\n");
					if ((sparql_result_not_found_in_results(sparql_result, rsp->added_booster)))
					{
						//printf("Result added because not present\n");
						rsp->added_booster=g_slist_prepend( rsp->added_booster, sparql_result );
					}
					else
					{
						NULL;
						//printf("Result NOT added because present\n");
					}
				}
				else
				{
					//printf("Check on Removed :\n");
					if (sparql_result_not_found_in_results(sparql_result, rsp->removed_booster))
					{
						//printf("Result added because not present\n");
						rsp->removed_booster=g_slist_prepend( rsp->removed_booster, sparql_result );
					}
					else
					{
						NULL;
						//printf("Result NOT added because present\n");
					}
				}
        	}
            else
            {
            	///////////////////////////////////////
            	//gettimeofday(&end, NULL);
            	//seconds  = end.tv_sec  - start.tv_sec;
            	//useconds = end.tv_usec - start.tv_usec;
            	//fprintf(stderr,"Morphism False_bool Post-Query Time [us,sub_id] : %ld \n",  seconds * 1000000 + useconds);
            	///////////////////////////////////////

            	//TODO CONSTRUCT
                librdf_free_query_results(query_results);
                librdf_free_query(query);
                return FALSE;
            }
        }
        else if (librdf_query_results_is_bindings(query_results))
        {

            //printf("Binding result/s found \n");
            //printf("%s\n",librdf_query_results_to_string(query_results,NULL,NULL));

            while(!librdf_query_results_finished(query_results))
            {

              GSList * sparql_result = NULL;
              GSList * var_pointer = rsp->required_sparql_vars;

              while (var_pointer != NULL)
              {
            	  gchar * varchar=var_pointer->data;
            	  if   (g_strcmp0(tq_reference->subject_var, varchar )==0)
            	  {
            		  sparql_binding_struct * sparql_binding = (sparql_binding_struct *)g_new0(sparql_binding_struct,1);
            		  sparql_binding->var_name=g_strdup(varchar);
            		  sparql_binding->string=g_strdup(tir_reference->subject);
            		  sparql_binding->type=0;
            		  sparql_result =  g_slist_prepend(sparql_result, sparql_binding);
            	  }
            	  else if (g_strcmp0(tq_reference->predicate_var, varchar )==0)
            	  {

            		  sparql_binding_struct * sparql_binding = (sparql_binding_struct *)g_new0(sparql_binding_struct,1);
            		  sparql_binding->var_name=g_strdup(varchar);
            		  sparql_binding->string=g_strdup(tir_reference->predicate);
            		  sparql_binding->type=0;
            		  sparql_result =  g_slist_prepend(sparql_result, sparql_binding);

            	  }
            	  else if (g_strcmp0(tq_reference->object_var, varchar )==0)
            	  {
            		  sparql_binding_struct * sparql_binding = (sparql_binding_struct *)g_new0(sparql_binding_struct,1);

            		  sparql_binding->var_name=g_strdup(varchar);
            		  sparql_binding->string=g_strdup(tir_reference->object);

            		  if (tir_reference->objType)
            			  sparql_binding->type=1;
            		  else
            			  sparql_binding->type=0;

            		  sparql_result = g_slist_prepend(sparql_result, sparql_binding);
            	  }
            	  else
            	  {

            	      librdf_node *tmp_node = NULL;
            		  tmp_node = librdf_query_results_get_binding_value_by_name(query_results, varchar);
            		  if (tmp_node)
            		  {
            			  sparql_binding_struct * sparql_binding = (sparql_binding_struct *)g_new0(sparql_binding_struct,1);

            			  sparql_binding->var_name=g_strdup(varchar);

            			  if (librdf_node_is_literal(tmp_node))
            			  {
            				  sparql_binding->string =  g_strdup(librdf_node_get_literal_value(tmp_node));
            				  sparql_binding->type=1;
            			  }
            			  else
            			  {
            				  sparql_binding->string =  g_strdup(librdf_uri_as_string( librdf_node_get_uri(tmp_node)));
            				  sparql_binding->type=0;
            			  }

            			  sparql_result = g_slist_prepend(sparql_result, sparql_binding);

            		  }
            		  else
            		  {
            			  sparql_binding_struct * sparql_binding = (sparql_binding_struct *)g_new0(sparql_binding_struct,1);
            			  sparql_binding->var_name=g_strdup(varchar);
            			  sparql_binding->string=g_strdup_printf("");
            			  sparql_binding->type=NULL;

            			  sparql_result = g_slist_prepend(sparql_result, sparql_binding);
            		  }

            		  librdf_free_node(tmp_node);

            	  }
            	  var_pointer=var_pointer->next;
              }

              if (ins_rm == 0)
              {
                  //printf("Check on Added :\n");
                  if (sparql_result_not_found_in_results(sparql_result, rsp->added_booster))
                  {
                      //printf("Result added because not present\n");
                      rsp->added_booster=g_slist_prepend(rsp->added_booster, sparql_result );
                  }
                  else
                  {
                      NULL;
                      //printf("Result NOT added because present\n");
                  }
              }
              else
              {
                  //printf("Check on Removed :\n");
                  if (sparql_result_not_found_in_results(sparql_result, rsp->removed_booster))
                  {
                      //printf("Result added because not present\n");
                      rsp->removed_booster=g_slist_prepend( rsp->removed_booster, sparql_result );
                  }
                  else
                  {
                      NULL;
                      //printf("Result NOT added because present\n");
                  }
              }

              librdf_query_results_next(query_results);
            }

        }
        else
        {
        	///////////////////////////////////////
        	//gettimeofday(&end, NULL);
        	//seconds  = end.tv_sec  - start.tv_sec;
        	//useconds = end.tv_usec - start.tv_usec;
        	//fprintf(stderr,"Morphism Post-Query Construct Time [us,sub_id] : %ld \n",  seconds * 1000000 + useconds);
        	///////////////////////////////////////

        	//TODO CONSTRUCT
            librdf_free_query_results(query_results);
            librdf_free_query(query);
            return FALSE;
        }
    }

    librdf_free_query_results(query_results);
    librdf_free_query(query);

	///////////////////////////////////////
	//gettimeofday(&end, NULL);
	//seconds  = end.tv_sec  - start.tv_sec;
	//useconds = end.tv_usec - start.tv_usec;
	//fprintf(stderr,"Morphism Post-Query Time [us,sub_id] : %ld \n",  seconds * 1000000 + useconds);
	///////////////////////////////////////



    //printf("load_sparql_morphism ended :\n");

    return results_generated;
}


gint * check_group_value (gint * gp_index, ssap_kp_message *req, ssap_sib_message *rsp, int * ins_rm)
{

    gint * result = 0;
    gint * result_tmp = 0;

    GSList * query_list=req->template_query;
    GSList * variable_list;

    GSList * pointer =NULL;

    ssTriple_t_sparql* tq;
    ssTriple_t_sparql* tir;

    //printf ("Load Group Value with gp index %d \n\n",gp_index);
    int min_indent_in_gp = find_min_indent_in_gp(gp_index, query_list);

    //printf ("Min Indent %d \n\n",min_indent_in_gp);
    find_variable_list_group_indent(&(variable_list), query_list,  gp_index, min_indent_in_gp);


	//Time utilities
    //struct timeval start, end;
    //unsigned long mtime, seconds, useconds, global_t =0;



    query_list = req->template_query;
    while ((query_list != NULL) )
    {
        tq = (ssTriple_t_sparql*)query_list->data;
        if (tq->gp_index == gp_index )
        {
            if      (ins_rm == 0)
                pointer=rsp->added;
            else if (ins_rm == 1)
                pointer=rsp->removed;

            while ((pointer != NULL) )
            {
                tir= (ssTriple_t_sparql*)pointer->data;

                if
                (
                        (
                                (g_strcmp0(tir->subject,tq->subject) ==0) ||
                                (g_strcmp0(tq->subject,wildcard1) ==0) ||
                                (g_strcmp0(tq->subject,wildcard2) ==0)
                        )
                        &&
                        (
                                (g_strcmp0(tir->predicate,tq->predicate) ==0) ||
                                (g_strcmp0(tq->predicate,wildcard1) ==0) ||
                                (g_strcmp0(tq->predicate,wildcard2) ==0)
                        )
                        &&
                        (
                                (g_strcmp0(tir->object,tq->object) ==0) && (tir->objType==tq->objType) ||
                                (g_strcmp0(tq->object,wildcard1) ==0) ||
                                (g_strcmp0(tq->object,wildcard2) ==0)
                        )
                )
                {
                    //printf ("\nMatching\n");
                    //printf("Triple Incoming:  \t%s\t%s\t%s , obj_Type %d \n", tir->subject, tir->predicate, tir->object, tir->objType);
                    //printf("Triple Query:         \t%s\t%s\t%s , obj_Type %d \n", tq->subject , tq->predicate, tq->object, tq->objType);

					//TIME MEASUREMENT UTILITY LINES
					//gettimeofday(&start, NULL);
					/////////////////////////////

                    result_tmp = load_sparql_morphism( req, rsp, tq, tir, &(variable_list), ins_rm);

					///////////////////////////////////////
					//gettimeofday(&end, NULL);
					//seconds  = end.tv_sec  - start.tv_sec;
					//useconds = end.tv_usec - start.tv_usec;
					//fprintf(stderr,"Morphism Time [us,sub_id] : %ld \n",  seconds * 1000000 + useconds);
					///////////////////////////////////////


                    if (result_tmp > 0)
                        result++;
                }
                pointer=pointer->next;
            }
        }
        query_list = query_list->next;
    }

    g_slist_free_full(variable_list, g_free );

    return result;

}

gint * group_exist (gint * group_counter, GSList * query_list)
{
    gint * result = 0;
    ssTriple_t_sparql * pointer;

    while (query_list != NULL)
    {
        pointer= query_list->data;
        if (pointer->gp_index == group_counter)
        {
            //printf("GROUP %d EXIST\n", group_counter);
            result = 1;
            return result;
        }

        query_list=query_list->next;
    }
    //printf("GROUP %d NOT EXIST\n", group_counter);
    return result;
}

gint * sparql_booster(ssap_kp_message *req, ssap_sib_message *rsp,  int * ins_rm)
{
    GSList * query_list=req->template_query;

    int number_of_groups;
    int group_counter;

    gint * result = 0;
    gint * temp_result = 0;

    number_of_groups=get_gp_num_max(query_list);
    //printf("Number of groups %d \n",number_of_groups );

    group_counter=0;
    while ((group_counter<= number_of_groups))
    {
        if (group_exist (group_counter, query_list))
        {
            //printf("group exist %d \n",group_counter  );
            temp_result= check_group_value(group_counter, req , rsp,  ins_rm);
            if (temp_result > 0)
                result++;
        }
        group_counter++;
    }

    //printf("VERY FINAL RESULT %d \n",result);
    return result;
}


