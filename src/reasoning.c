#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <sibdefs.h>
#include <reasoning.h>
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

//BOOLEAN CONST PROPERTIES
#define true  1
#define false 0
#define TRUE 1
#define FALSE 0

ssTriple_t* trasform_statement_expand(ssTriple_t* stat)
{

 gchar* subject;
 gchar* predicate;
 gchar* object;

 subject=control_string_expand(stat->subject);
 if (subject!=NULL)
 {
	 g_free(stat->subject);
	 stat->subject=(gchar*)g_strdup(subject);
 }

 predicate=control_string_expand(stat->predicate);
 if (predicate!=NULL)
 {
	 g_free(stat->predicate);
	 stat->predicate=(gchar*)g_strdup(predicate);
 }

 object=control_string_expand(stat->object);
 if (object!=NULL)
 {
	 g_free(stat->object);
	 stat->object=(gchar*)g_strdup(object);
 }

 return stat;  
}

ssTriple_t* trasform_statement_contract(ssTriple_t* stat)
{

 gchar* subject;
 gchar* predicate;
 gchar* object;

 subject=control_string_contract(stat->subject);
 if (subject!=NULL)
 {
	 g_free(stat->subject);
	 stat->subject=subject;
 }

 predicate=control_string_contract(stat->predicate);
 if (predicate!=NULL)
 {
	 g_free(stat->predicate);
	 stat->predicate=predicate;
 }

 object=control_string_contract(stat->object);
 if (object!=NULL)
 {
	 g_free(stat->object);
	 stat->object=object;
 }

 return stat;  

}



gchar* control_string_contract(ssElement_t content)
{ 

 gchar* str_output=NULL;

 gchar** split_result_array;
 gchar** pointer_char_arr;

 if      (g_str_has_prefix (content, rdf_ex))
 {
	split_result_array=g_strsplit(content, rdf_ex , 0);
	pointer_char_arr = split_result_array;

	++pointer_char_arr;
	//printf("string prefixed: %s \n", *pointer_char_arr );
	str_output=g_strdup_printf("%s%s",rdf_c,*pointer_char_arr);
	g_strfreev (split_result_array);
	return str_output;

 }
 else if (g_str_has_prefix (content, fn_ex))
 {
	split_result_array=g_strsplit(content, fn_ex , 0);
	pointer_char_arr = split_result_array;

	++pointer_char_arr;
	//printf("string prefixed: %s \n", *pointer_char_arr );
	str_output=g_strdup_printf("%s%s",fn_c_2,*pointer_char_arr);
	g_strfreev (split_result_array);
	return str_output;

 }
 else if (g_str_has_prefix (content, xsd_ex))
 {
	split_result_array=g_strsplit(content, xsd_ex , 0);
	pointer_char_arr = split_result_array;
	++pointer_char_arr;
	//printf("string prefixed: %s \n", *pointer_char_arr );
	str_output=g_strdup_printf("%s%s",xsd_c,*pointer_char_arr);
	g_strfreev (split_result_array);
	return str_output;

 }
 else if (g_str_has_prefix (content, rdfs_ex))
 {
	split_result_array=g_strsplit(content, rdfs_ex , 0);
	pointer_char_arr = split_result_array;
	++pointer_char_arr;
	//printf("string prefixed: %s \n", *pointer_char_arr );
	str_output=g_strdup_printf("%s%s",rdfs_c,*pointer_char_arr);
	g_strfreev (split_result_array);
	return str_output;


 }

 //NULL CASE!
 return str_output; 



}

gchar* control_string_expand(ssElement_t content)
{ 
 gchar* str_output=NULL;

 gchar** split_result_array;
 gchar** pointer_char_arr;

 if      (g_str_has_prefix (content, rdf_c))
 {
	split_result_array=g_strsplit(content, rdf_c , 0);
	pointer_char_arr = split_result_array;
	++pointer_char_arr;
	//printf("expanding: %s \n", *pointer_char_arr );
	str_output=g_strdup_printf("%s%s",rdf_ex,*pointer_char_arr);
	g_strfreev (split_result_array);
	return str_output;

 }
 else if (g_str_has_prefix (content, fn_c_2))
 {
	split_result_array=g_strsplit(content, fn_c_2 , 0);
	pointer_char_arr = split_result_array;
	++pointer_char_arr;
	//printf("expanding: %s \n", *pointer_char_arr );
	str_output=g_strdup_printf("%s%s",fn_ex,*pointer_char_arr);
	g_strfreev (split_result_array);
	return str_output;

 }
 else if (g_str_has_prefix (content, xsd_c))
 {
	split_result_array=g_strsplit(content, xsd_c , 0);
	pointer_char_arr = split_result_array;
	++pointer_char_arr;
	//printf("expanding: %s \n", *pointer_char_arr );
	str_output=g_strdup_printf("%s%s",xsd_ex,*pointer_char_arr);
	g_strfreev (split_result_array);
	return str_output;

 }
 else if (g_str_has_prefix (content, rdfs_c))
 {
	split_result_array=g_strsplit(content, rdfs_c , 0);
	pointer_char_arr = split_result_array;
	++pointer_char_arr;
	//printf("expanding: %s \n", *pointer_char_arr );
	str_output=g_strdup_printf("%s%s",rdfs_ex,*pointer_char_arr);
	g_strfreev (split_result_array);
	return str_output;
 }

 //NULL CASE!!
 return str_output; 


}

///////////////////////////////////////////// Reasoning////////////////////////////////////////////////

void reasoning(sib_data_structure* param, ssTriple_t* t, gboolean* enable_real_reasoning)
{
	 librdf_statement* statement;

	 //Insert object as class
	 if(strcmp(t->predicate,rdf_ex "type")==0 && strcmp(t->object,rdfs_ex "Class")!=0)
	 {

	    statement=librdf_new_statement(param->RDF_world);
	    librdf_statement_set_subject(statement,librdf_new_node_from_uri_string(param->RDF_world, t->object));	  
	    librdf_statement_set_predicate(statement,
				               librdf_new_node_from_uri_string(param->RDF_world,  rdf_ex "type"));

	    librdf_statement_set_object(statement, 
						librdf_new_node_from_uri_string(param->RDF_world, rdfs_ex "Class"));

	    librdf_model_add_statement(param->RDF_model, statement);

	    //FOR SUBSCRIBE
            librdf_model_add_statement(param->RDF_model_insert, statement);

	    librdf_free_statement(statement);

	    if (enable_real_reasoning)
	    //////////////////////////////////////////////
	    {
	   	 check_subtype_and_add(param, t->subject,t->object);
	    }
	    ///////////////////////////////////////////////
	 } 

	 //Insert subject and object as class
	 else if(strcmp(t->predicate,rdfs_ex "subClassOf")==0)
	 {

	    statement=librdf_new_statement(param->RDF_world);
	    librdf_statement_set_subject(statement,librdf_new_node_from_uri_string(param->RDF_world,  t->subject));	  
	    librdf_statement_set_predicate(statement,
				               librdf_new_node_from_uri_string(param->RDF_world, rdf_ex "type"));

	    librdf_statement_set_object(statement, 
						librdf_new_node_from_uri_string(param->RDF_world, rdfs_ex "Class"));

	    librdf_model_add_statement(param->RDF_model, statement);

	    //FOR SUBSCRIBE
            librdf_model_add_statement(param->RDF_model_insert, statement);

	    librdf_free_statement(statement);

	    statement=librdf_new_statement(param->RDF_world);
	    librdf_statement_set_subject(statement,librdf_new_node_from_uri_string(param->RDF_world,  t->object));	  
	    librdf_statement_set_predicate(statement,
				               librdf_new_node_from_uri_string(param->RDF_world, rdf_ex "type"));

	    librdf_statement_set_object(statement, 
						librdf_new_node_from_uri_string(param->RDF_world, rdfs_ex "Class"));

	    librdf_model_add_statement(param->RDF_model, statement);

	    //FOR SUBSCRIBE
            librdf_model_add_statement(param->RDF_model_insert, statement);

	    librdf_free_statement(statement);

	    //////////////////////////////////////////////
	    if (enable_real_reasoning)
	    {
	   	 add_sub_type(param, t->subject,t->object);
	    }
	    ///////////////////////////////////////////////
	 } 

	 //Insert subject and object as property
	 else if(strcmp(t->predicate,rdfs_ex "subPropertyOf")==0)
	   {

	    statement=librdf_new_statement(param->RDF_world);
	    librdf_statement_set_subject(statement,librdf_new_node_from_uri_string(param->RDF_world,  t->subject));	  
	    librdf_statement_set_predicate(statement,
				               librdf_new_node_from_uri_string(param->RDF_world, rdf_ex "type"));

	    librdf_statement_set_object(statement, 
						librdf_new_node_from_uri_string(param->RDF_world,  rdf_ex "Property"));

	    librdf_model_add_statement(param->RDF_model, statement);

	    //FOR SUBSCRIBE
            librdf_model_add_statement(param->RDF_model_insert, statement);
	    librdf_free_statement(statement);
	    

	    statement=librdf_new_statement(param->RDF_world);
	    librdf_statement_set_subject(statement,librdf_new_node_from_uri_string(param->RDF_world,  t->object));	  
	    librdf_statement_set_predicate(statement,
				               librdf_new_node_from_uri_string(param->RDF_world, rdf_ex "type"));

	    librdf_statement_set_object(statement, 
						librdf_new_node_from_uri_string(param->RDF_world,  rdf_ex "Property"));

	    librdf_model_add_statement(param->RDF_model, statement);

	    //FOR SUBSCRIBE
            librdf_model_add_statement(param->RDF_model_insert, statement);

	    librdf_free_statement(statement);

	    ////////////////////////////////
	    if (enable_real_reasoning)
	    {
	 	 add_sub_properties(param, t->subject, t->object);
	    }
	    ////////////////////////////////	    
	 } 
	 
	 //Insert subject and object as property and class(domain and range)
	 else if(strcmp(t->predicate,rdfs_ex "domain")==0 )
	 {

	    statement=librdf_new_statement(param->RDF_world);
	    librdf_statement_set_subject(statement,librdf_new_node_from_uri_string(param->RDF_world,  t->subject));	  
	    librdf_statement_set_predicate(statement,
				               librdf_new_node_from_uri_string(param->RDF_world, rdf_ex "type"));

	    librdf_statement_set_object(statement, 
						librdf_new_node_from_uri_string(param->RDF_world,  rdf_ex "Property"));

	    librdf_model_add_statement(param->RDF_model, statement);
	    //FOR SUBSCRIBE
            librdf_model_add_statement(param->RDF_model_insert, statement);
	    librdf_free_statement(statement);

	 
	    statement=librdf_new_statement(param->RDF_world);
	    librdf_statement_set_subject(statement,librdf_new_node_from_uri_string(param->RDF_world,  t->object));	  
	    librdf_statement_set_predicate(statement,
				               librdf_new_node_from_uri_string(param->RDF_world, rdf_ex "type"));

	    librdf_statement_set_object(statement, 
						librdf_new_node_from_uri_string(param->RDF_world,  rdfs_ex "Class"));

	    librdf_model_add_statement(param->RDF_model, statement);
	    //FOR SUBSCRIBE
            librdf_model_add_statement(param->RDF_model_insert, statement);
	    librdf_free_statement(statement);

	    //////////////////////////////////////////////////////////////////////////////////////
	    if (enable_real_reasoning)
	    {
	    	 add_properties_domain(param, t->subject,  t->object);
	    }
	    //////////////////////////////////////////////////////////////////////////////////////
	 } 

	 else if(strcmp(t->predicate,rdfs_ex "range")==0)
	    {

	    statement=librdf_new_statement(param->RDF_world);
	    librdf_statement_set_subject(statement,librdf_new_node_from_uri_string(param->RDF_world,  t->subject));	  
	    librdf_statement_set_predicate(statement,
				               librdf_new_node_from_uri_string(param->RDF_world, rdf_ex "type"));

	    librdf_statement_set_object(statement, 
						librdf_new_node_from_uri_string(param->RDF_world,  rdf_ex "Property"));

	    librdf_model_add_statement(param->RDF_model, statement);
	    //FOR SUBSCRIBE
            librdf_model_add_statement(param->RDF_model_insert, statement);
	    librdf_free_statement(statement);



	 
	    statement=librdf_new_statement(param->RDF_world);
	    librdf_statement_set_subject(statement,librdf_new_node_from_uri_string(param->RDF_world,  t->object));	  
	    librdf_statement_set_predicate(statement,
				               librdf_new_node_from_uri_string(param->RDF_world, rdf_ex "type"));

	    librdf_statement_set_object(statement, 
						librdf_new_node_from_uri_string(param->RDF_world,  rdfs_ex "Class"));

	    librdf_model_add_statement(param->RDF_model, statement);
	    //FOR SUBSCRIBE
            librdf_model_add_statement(param->RDF_model_insert, statement);
	    librdf_free_statement(statement);


	    //////////////////////////////////////////////////////////////////////////////////////
	    if (enable_real_reasoning)
	    {
	    	add_properties_range(param, t->subject,  t->object);
	    }
	    //////////////////////////////////////////////////////////////////////////////////////

	 }
	   
	 // All other cases!
	 // Insert predicate as property (eventually check for subproperties)
	 else if(strcmp(t->predicate,rdf_ex "type")!=0)
	 {

		statement=librdf_new_statement(param->RDF_world);
		librdf_statement_set_subject(statement, 
					       librdf_new_node_from_uri_string(param->RDF_world, t->predicate));
		  
		librdf_statement_set_predicate(statement,
					       librdf_new_node_from_uri_string(param->RDF_world,  rdf_ex "type"));

		librdf_statement_set_object(statement, 
						librdf_new_node_from_uri_string(param->RDF_world, rdf_ex "Property"));

		librdf_model_add_statement(param->RDF_model, statement);
		//FOR SUBSCRIBE
		librdf_model_add_statement(param->RDF_model_insert, statement);

		librdf_free_statement(statement);

 		//////////////////////////////////////////////
	        if (enable_real_reasoning)
	    	{
			check_if_predicate_subproperty_and_add(param,t->subject,t->predicate,t->object);
			check_if_property_in_domain_and_add_subject_type(param, t->subject, t->predicate);
			check_if_property_in_range_and_add_object_type(param, t->object, t->predicate);
		}
		//////////////////////////////////////////////
	 }



 }

void add_sub_type(sib_data_structure* param, gchar* sub_class, gchar* class)
{
	GSList* upper_classes=NULL;
	GSList* upper_classes_pointer;
	gchar* temp_class;
	upper_classes = g_hash_table_lookup(param->subClasses, sub_class);

	if (upper_classes!=NULL)
	{
		upper_classes_pointer=upper_classes;
		for ( ; upper_classes_pointer != NULL ; upper_classes_pointer = upper_classes_pointer->next)
		{	temp_class=upper_classes_pointer->data;
			if (strcmp(class,temp_class) ==0)
			{
				//Upper Class still present
				return;
			}	
		}
		//printf("Inserting more-than one subClass %s upperClass %s \n",sub_class, class); 
		upper_classes = g_slist_prepend(upper_classes, g_strdup(class));

		g_hash_table_remove(param->subClasses, sub_class);
		g_hash_table_insert(param->subClasses, g_strdup(sub_class), upper_classes);	
	}
	else
	{
		//printf("Inserting to subClass %s upperClass %s \n",sub_class, class);
		upper_classes = g_slist_prepend(upper_classes, g_strdup(class));
		g_hash_table_insert(param->subClasses, g_strdup(sub_class), upper_classes);
	}	
}

void check_subtype_and_add(sib_data_structure* param, gchar* uri, gchar* class_or_subclass)
{
	GSList* upper_classes=NULL;
	GSList* upper_classes_pointer;
	gchar* temp_class;
	librdf_statement* statement;

	upper_classes = g_hash_table_lookup(param->subClasses, class_or_subclass);

	//printf("DEBUG_check_subtype  %s\n",class_or_subclass);
	if (upper_classes!=NULL)
	{
		//printf("DEBUG %s Is_subclass\n",class_or_subclass);
		upper_classes_pointer=upper_classes;
		for ( ; upper_classes_pointer != NULL ; upper_classes_pointer = upper_classes_pointer->next)
		{	
			temp_class=upper_classes_pointer->data;

			//printf("Upper Class Of %s : %s\n",uri,temp_class);

			statement=librdf_new_statement(param->RDF_world);
		        librdf_statement_set_subject(statement,librdf_new_node_from_uri_string(param->RDF_world, uri));
		        librdf_statement_set_predicate(statement,librdf_new_node_from_uri_string(param->RDF_world, rdf_ex "type"));
		        librdf_statement_set_object(statement,librdf_new_node_from_uri_string(param->RDF_world, temp_class));

			librdf_model_add_statement(param->RDF_model, statement);

			//FOR SUBSCRIBE
			librdf_model_add_statement(param->RDF_model_insert, statement);
			librdf_free_statement(statement);

			//RECOURSIVE
			check_subtype_and_add(param,uri,temp_class);
	
		}
	}
}

void add_sub_properties(sib_data_structure* param, gchar* sub_property, gchar* property)
{
	GSList* upper_properties=NULL;
	GSList* upper_properties_pointer;
	gchar*  temp_property;

	upper_properties = g_hash_table_lookup(param->subProperties, sub_property);
	
	if (upper_properties!=NULL)
	{
		
		upper_properties_pointer=upper_properties;
		for ( ; upper_properties_pointer != NULL ; upper_properties_pointer = upper_properties_pointer->next)
		{	temp_property=upper_properties_pointer->data;
			if (strcmp(property,temp_property) ==0)
			{
				//Upper property still present
				return;
			}	
		}
		//printf("Inserting more-than one subPropery %s upperProperty %s \n",sub_property, property); 
		upper_properties = g_slist_prepend(upper_properties, g_strdup(property));	

		g_hash_table_remove(param->subProperties, sub_property);
		g_hash_table_insert(param->subProperties, g_strdup(sub_property), upper_properties);		
	}
	else
	{
		//printf("Inserting Sub property array in hash \n");
		upper_properties = g_slist_prepend(upper_properties, g_strdup(property));
		g_hash_table_insert(param->subProperties, g_strdup(sub_property), upper_properties);
	}	
}


void check_if_predicate_subproperty_and_add(sib_data_structure* param, gchar* uri, gchar* property ,gchar* object)
{
	GSList* upper_properties=NULL;
	GSList* upper_properties_pointer;
	gchar*  temp_property;
	librdf_statement* statement;

	upper_properties = g_hash_table_lookup(param->subProperties, property);

	//printf("DEBUG_check_subproperty  %s\n",property);
	if (upper_properties!=NULL)
	{
		//printf("DEBUG %s is subproperty \n",property);
		upper_properties_pointer=upper_properties;
		for ( ; upper_properties_pointer != NULL ; upper_properties_pointer = upper_properties_pointer->next)
		{	
			temp_property=upper_properties_pointer->data;

			statement=librdf_new_statement(param->RDF_world);

		        librdf_statement_set_subject(statement,librdf_new_node_from_uri_string(param->RDF_world, uri));
		        librdf_statement_set_predicate(statement,librdf_new_node_from_uri_string(param->RDF_world, temp_property));
		        librdf_statement_set_object(statement,librdf_new_node_from_uri_string(param->RDF_world, object));

			librdf_model_add_statement(param->RDF_model, statement);

			//FOR SUBSCRIBE
			librdf_model_add_statement(param->RDF_model_insert, statement);
			librdf_free_statement(statement);

			//RECOURSIVE
			check_if_predicate_subproperty_and_add(param,uri,temp_property,object);
	
		}
	}
}


void  add_properties_domain(sib_data_structure* param, gchar* property, gchar* class)
{
	gchar*  temp_class =NULL;
	temp_class = g_hash_table_lookup(param->propertyDomain, property);
	if (temp_class==NULL)
	{
		g_hash_table_insert(param->propertyDomain, g_strdup(property), g_strdup(class));
	}

}

void  add_properties_range(sib_data_structure* param, gchar* property, gchar* class)
{
	gchar*  temp_class =NULL;
	temp_class = g_hash_table_lookup(param->propertyRange, property);
	if (temp_class==NULL)
	{
		g_hash_table_insert(param->propertyRange, g_strdup(property), g_strdup(class));
	}

}
void check_if_property_in_domain_and_add_subject_type(sib_data_structure* param, gchar* uri, gchar* property)
{	
	gchar*  temp_class =NULL;
	librdf_statement* statement;

	temp_class = g_hash_table_lookup(param->propertyDomain, property);
	if (temp_class!=NULL)
	{
		statement=librdf_new_statement(param->RDF_world);

	        librdf_statement_set_subject(statement,librdf_new_node_from_uri_string(param->RDF_world, uri));
	        librdf_statement_set_predicate(statement,librdf_new_node_from_uri_string(param->RDF_world, rdf_ex "type"));
	        librdf_statement_set_object(statement,librdf_new_node_from_uri_string(param->RDF_world, temp_class));

		librdf_model_add_statement(param->RDF_model, statement);

		//FOR SUBSCRIBE
		librdf_model_add_statement(param->RDF_model_insert, statement);
		librdf_free_statement(statement);

		//////////////////////////////////////////////
		check_subtype_and_add(param, uri, temp_class);
		///////////////////////////////////////////////

	}

	
}

void check_if_property_in_range_and_add_object_type(sib_data_structure* param, gchar* uri, gchar* property)
{	
	gchar*  temp_class =NULL;
	librdf_statement* statement;

	temp_class = g_hash_table_lookup(param->propertyRange, property);
	if (temp_class!=NULL)
	{
		statement=librdf_new_statement(param->RDF_world);

	        librdf_statement_set_subject(statement,librdf_new_node_from_uri_string(param->RDF_world, uri));
	        librdf_statement_set_predicate(statement,librdf_new_node_from_uri_string(param->RDF_world, rdf_ex "type"));
	        librdf_statement_set_object(statement,librdf_new_node_from_uri_string(param->RDF_world, temp_class));

		librdf_model_add_statement(param->RDF_model, statement);

		//FOR SUBSCRIBE
		librdf_model_add_statement(param->RDF_model_insert, statement);
		librdf_free_statement(statement);

		//////////////////////////////////////////////
		check_subtype_and_add(param, uri, temp_class);
		///////////////////////////////////////////////

	}
}

