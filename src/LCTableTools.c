/*

  Copyright (c) 2010, ARCES - University of Bologna
  All rights reserved.

*/
#include <whiteboard_util.h>
#include <whiteboard_log.h>
#include "LCTableTools.h"


/*
 * the LCTable
 */

LCTable *LCT=NULL;


     /*AD-ARCES*/
      /* ARCES-Protection compatibility check for
       * the op coming from the i_queque list
       *
       * */


      /* *****************************************************
       *
       */
       void ProtectionCompatibilityFilter( s_scheduler_item*  op)
       {

           whiteboard_log_debug("---->:int ProtectionCompatibilityCheck( s_scheduler_item*  op)\n");
       	   printOpContent(op);

           //op->header->tr_type=M3_PROTECTION_FAULT;
           //printf("*** OP protection policy incompatible ***\n");
           int op_type;

           switch(op_type = check_op_type_on_predicate(op))
           {case OP_NORMAL:     
				whiteboard_log_debug("*** NORMAL OP RECOGNIZED!\n");
                                if( !isInsertRemoveOperationConsistent(op) )
                                	{op->header->tr_type=M3_PROTECTION_FAULT;
                                	 whiteboard_log_debug("*** ProtectionCompatibilityFilter():\n\tPROTECTION FAULT: insert or remove operation NOT protection consistent!\n");
                                	 //
                                	 op->rsp->status = ss_OperationFailed;
                                	}
                                break;

            case OP_PROTECTION: whiteboard_log_debug("*** PROTECTION OP RECOGNIZED!\n");

                                if( isProtectionRequestConsistent(op) == FALSE )
                                {	
				    whiteboard_log_debug("*** ProtectionCompatibilityFilter():\n\tPROTECTION FAULT! Operation NOT protection consistent! LC Table UNCHANGED!\n");
                                    //
                                    op->rsp->status = ss_OperationFailed;
                                }//if( isProtectionRequestConsistent(op) == FALSE )
                                else   whiteboard_log_debug("*** LC Table UPDATED!\n");

                                break;

            default:whiteboard_log_debug("*** OP NOT RECOGNIZED!\n");
           }//switch(op_type)


           LCTable_print(LCT);

       }//ProtectionCompatibilityCheck( s_scheduler_item*  op)


      /* *****************************************************
       *
       */
       void printOpContent( s_scheduler_item*  op)
       {char* query_type_name[8]={
		  "QueryTypeInvalid",
		  "QueryTypeTemplate",
		  "QueryTypeWQLValues",
		  "QueryTypeWQLNodeTypes",
		  "QueryTypeWQLRelated",
		  "QueryTypeWQLIsType",
		  "QueryTypeWQLIsSubType",
		  "QueryTypeSPARQL"};

        char* encoding_type_name[3]={
		  "EncodingInvalid",
		  "EncodingM3XML",
		  "EncodingRDFXML"};


           whiteboard_log_debug("------------------------------------\n");
           whiteboard_log_debug("OP COntent:\n");
           whiteboard_log_debug("KP ID:%s\n",(char*)op->header->kp_id);

           whiteboard_log_debug("credentials:%s\n",(char*)op->req->credentials);
           whiteboard_log_debug("encoding:%s\n",encoding_type_name[op->req->encoding]);

           whiteboard_log_debug("INSERT_GRAPH\n");println_GSList(op->req->insert_graph);
           whiteboard_log_debug("REMOVE_GRAPH\n");println_GSList(op->req->remove_graph);

           //whiteboard_log_debug("insert_str:%s\n",(char*)op->req->insert_str);
           //whiteboard_log_debug("remove_str:%s\n",(char*)op->req->remove_str);
           whiteboard_log_debug("query_type:%s\n",       query_type_name[op->req->type]);
           whiteboard_log_debug("query_str:%s\n",(char*)op->req->query_str);

           whiteboard_log_debug("------------------------------------\n");

	       return;
       }//PrintOpContent(op)


    /* *****************************************************
	 *
	 */
     void println_GSList(GSList* list)
     { char* ssElementType_t_name[4]={
		"ssElement_TYPE_URI",
        "ssElement_TYPE_LIT",
        "ssElement_TYPE_BNODE",
        "ssElement_TYPE_eot"};

	    ssTriple_t *triple;

	    while( list!=NULL )
	    {
		  triple=list->data;

		  whiteboard_log_debug("    -------------------\n");
		  whiteboard_log_debug("     subject   :%s [%s]\n",triple->subject,ssElementType_t_name[triple->subjType]);
		  whiteboard_log_debug("     predicate :%s\n"     ,triple->predicate);
		  whiteboard_log_debug("     object    :%s [%s]\n",triple->object,ssElementType_t_name[triple->objType]);

		  list=list->next;
	     }//while( list!=NULL )

      }//void println_GList(GList* list)



 	/* *****************************************************
 	 * Check the predicate value of every triple to discover
 	 * conflict with protection stuff properties
 	 * return the type of the op
 	 * OP_NORMAL:     no protection property found
 	 * OP_PROTECTION: one or more protection property found
 	 *
 	 */
 	int check_op_type_on_predicate( s_scheduler_item*  op)
 	{
 		whiteboard_log_debug("CHECK INSERT_GRAPH\n");
 		if(checkProtectionOnPredicate_GSList(op->req->insert_graph))return OP_PROTECTION;
 		whiteboard_log_debug("CHECK REMOVE_GRAPH\n");
 		if(checkProtectionOnPredicate_GSList(op->req->remove_graph))return OP_PROTECTION;

 		return OP_NORMAL;
 	}//int check_op_type_on_predicate( s_scheduler_item*  op)



    /* *****************************************************
	 * Retunr 1 (true) if any protection predicate was found
	 * otherwise, 0 (false)
	 */
     int checkProtectionOnPredicate_GSList(GSList* list)
     { ssTriple_t *triple;

	    while( list!=NULL )
	    {
		  triple=list->data;

		  if(    strcmp(triple->predicate,AR_PROPERTY)==0
		      || strcmp(triple->predicate,AR_OWNER)==0
              || strcmp(triple->predicate,AR_TARGET)==0 )
			return 1;


		  list=list->next;
	     }//while( list!=NULL )

	    return 0;
      }//int println_GList(GList* list)




     /**
      * Checks if all the operations (insert,remove) are protection consistent
      * In details, it checks every couple SUB-PRE or PRE-OBJ on the LC table.
      * It looks for a line on the LC table.
      * The owner of every line found is compared to the owner of the operation sent.
      * If , at least, one owner in the LC table does not match the owner of
      * the operation sent (KP ID) a protection incensistency fault is found.
      * In this case return FALSE (0).
      * Otherwise, if ALL the owners in the LC tablematch the KP ID, returns TRUE (1)
      *
      */
     boolean isInsertRemoveOperationConsistent( s_scheduler_item*  op)
     {
       //whiteboard_log_debug("*** UNDER CONSTRUCTION!!! boolean isInsertRemoveOperationConsistent( s_scheduler_item*  op)\n");
       if(LCT==NULL) LCT=LCTable_new();

   	   GSList *list[2];

   	   list[0]=op->req->insert_graph;
   	   list[1]=op->req->remove_graph;

   	   ssTriple_t *triple;

   	   int ig=0;
   	   for(;ig<2;ig++)
   	     while( list[ig]!=NULL )
   	     {
   	    	triple=list[ig]->data;

            LCLine *l=LCTable_getLCLineByIPi(LCT,triple->subject , triple->predicate);


            if(l!=NULL)
            	if( braces_strcmp(l->KP,(char*)op->header->kp_id) != 0)
            		{whiteboard_log_debug("*** PROTECTION FAULT: isInsertRemoveOperationConsistent():\n\t KP id differ from KP's line (line found by I-Pi, sub-pred)\n");
            		 whiteboard_log_debug("\tl->KP=%s,op->header->kp_id=%s\n",l->KP,op->header->kp_id);
            		 return FALSE;
            		}

            l=LCTable_getLCLineByIPi(LCT,triple->object  , triple->predicate);

            if(l!=NULL)
            	if( braces_strcmp(l->KP,(char*)op->header->kp_id) != 0)
            		{whiteboard_log_debug("*** PROTECTION FAULT: isInsertRemoveOperationConsistent():\n\t KP id differ from KP's line (line found by I-Pi, obj-pred)\n");
           		     whiteboard_log_debug("\tl->KP=%s,op->header->kp_id=%s\n",l->KP,op->header->kp_id);
            		 return FALSE;
            		}


	  	    list[ig]=list[ig]->next;
   	     }//while( list[ig]!=NULL )

    	 return TRUE;
     }//boolean isInsertRemoveOperationConsistent( s_scheduler_item*  op)



     /* *****************************************************
      * Return a protection descriptor made by:
      * OWNER
      * I
      * P
      * Pi[]
      *
      */
          ProtectionDescriptor* getProtectionDescriptor_GSList(GSList* list)
          {
     	    ssTriple_t *triple;

     	    ProtectionDescriptor* pd=malloc(sizeof(ProtectionDescriptor));
            pd->PiList=NULL;
            pd->I=NULL;
            pd->P=NULL;
            pd->owner=NULL;


     	    while( list!=NULL )
     	    {
     		  triple=list->data;

     		  if( strcmp(triple->predicate,AR_PROPERTY)==0 )
     		  {   pd->I=malloc(strlen(triple->subject)+1);
     		      strcpy(pd->I,triple->subject);

     		      pd->P=malloc(strlen(triple->object)+1);
     		      strcpy(pd->P,triple->object);
     		  }//if( strcmp(triple->predicate,AR_PROPERTY)==0 )

     		  if( strcmp(triple->predicate,AR_OWNER)==0    )
     		  {   pd->owner=malloc(strlen(triple->object)+1);
     		      strcpy(pd->owner,triple->object);
     		  }//if( strcmp(triple->predicate,AR_OWNER)==0    )

     		  if( strcmp(triple->predicate,AR_TARGET)==0   )
     		  {
     			 PiItem *pi=malloc(sizeof(PiItem));

     		     pi->Pi=malloc(strlen(triple->object)+1);
    		     strcpy(pi->Pi,triple->object);

    		     pi->next=pd->PiList;

    		     pd->PiList=pi;

     		  }//if( strcmp(triple->predicate,AR_TARGET)==0   )


     		  list=list->next;
     	     }//while( list!=NULL )

     	    return pd;
           }//int println_GList(GList* list)




          /**
             ALGORITMO !!!

            +accedere con I-Pi[i] alla tabella
            +faccio la query
            +controllo owner della riga con in richiedente.
            +Se sono == vado avanti...(incremento indice C )(recupero il P della tabella)
            +se sono differenti, fermo tutto -> op->header->tr_type=M3_PROTECTION_FAULT;
            +se non ho valori vado avanti....

            +alla fine, se non mi sono fermato prima,
            +se C è uguale a ZERO o non ho recuerato nessun P dalla tabella!!,
            +posso aggiungere alla tabella tutto il pd

            +se C != ZERO, e ho impostato il P (preso dalla tabella)
            +sostituisco in op il P con il P preso dalla tabella
            +sostituisco in pd il P con il P preso dalla tabella

            +posso aggiungere alla tabella tutto il pd

           **
           * Return TRUE if the protection insert/remove query is LC table consistent
           *  otherwise, FALSE
           * In case of TRUE, the LC table will be modified as well as the op structure
           * In case of FALSE, only the op structure will be modified as follow:
           *   op->header->tr_type=M3_PROTECTION_FAULT;
           */
          boolean isProtectionRequestConsistent( s_scheduler_item*  op)
          {
              whiteboard_log_debug("*** boolean isProtectionRequestConsistent( s_scheduler_item*  op)\n");

        	  ProtectionDescriptor* pd_insert = getProtectionDescriptor_GSList(op->req->insert_graph);

              PiItem *pii=pd_insert->PiList;
              char* P_from_table=NULL;


              if(LCT==NULL) LCT=LCTable_new();


              whiteboard_log_debug("+++ START WHILE\n");
              while(pii!=NULL)
              {
                 LCLine *line = LCTable_getLCLineByIPi( LCT, pd_insert->I, pii->Pi  );

                 if(line!=NULL)
                 {
                	LCLine_print(line);

                	//whiteboard_log_debug("+++ Insert Owner:%s vs KP's line:%s\n",pd_insert->owner,line->KP);
                    if(strcmp(pd_insert->owner,line->KP)!=0)
                     {
                	    /*GENERATE FAULT*/
                    	op->header->tr_type=M3_PROTECTION_FAULT;
                    	whiteboard_log_debug("*** PROTECTION FAULT: isProtectionRequestConsistent():\n\t insert owner NOT equal to KP's line\n");
               		    whiteboard_log_debug("\t pd_insert->owner=%s,line->KP=%s\n",pd_insert->owner,line->KP);

                        /*FREE MEMORY*/
                    	PD_free(pd_insert);
                    	if(P_from_table!=NULL)free(P_from_table);
                    	//line

                    	return FALSE;
                     }//if(strcmp(pd_insert->owner,line->owner)!=0)
                    else
                     {
                    	if(P_from_table==NULL)
                    	  {P_from_table=malloc(strlen(line->P)+1);
                    	   strcpy(P_from_table,line->P);
                    	   whiteboard_log_debug("+++ P_FROM_TABLE:%s\n",P_from_table);
                    	  }//if(P_from_table==NULL)

                     }//if(strcmp(pd_insert->owner,line->owner)!=0)...else...


                    /*FREE MEMORY*/

                 }//if(line!=NULL)
                 else
                 { whiteboard_log_debug("+++ PARAMETRI pd_insert->I=%s, pii->Pi=%s\n",pd_insert->I, pii->Pi);
             	   if(P_from_table==NULL)
             	     {
             		  LCLine *l=LCTable_getLCLineByIKP( LCT, pd_insert->I, pd_insert->owner  );

             		  if(l!=NULL)
             		    {
             		      P_from_table=malloc(strlen(l->P)+1);
             	          strcpy(P_from_table,l->P);
             	          whiteboard_log_debug("+++ P_FROM_TABLE:%s\n",P_from_table);

             		    }//if(l!=NULL)

             	     }//if(P_from_table==NULL)

                 }//if(line!=NULL)...else...

                 pii=pii->next;
              }//while(pii!=NULL)

              whiteboard_log_debug("+++ END WHILE\n");

              if(P_from_table!=NULL)
              {
            	  /**
            	   *  MODIFY OP and the descriptor with the P
            	   *  retrived from the LCTable
            	   * */
            	  PD_updateP( pd_insert , P_from_table );
            	  scheduler_item_updateP( op , P_from_table );
            	  free(P_from_table);

              }//if(P_from_table!=NULL)
              else whiteboard_log_debug("+++ P_FROM_TABLE is NULL\n");

              /**
               * May be it is usefull to check here if an error has
               *  been occurred during the table update process!!!
               *
               *  ############################
               *    puo la pd_insert andare a buon fine e modificare la tabella,
               *    mentre la remove FALLISCE???? o vice versa???
               *    NO!!!!
               *    QUINDI FARE UPDATE DELLA TABELLA SOLO ALLA FINE DI TUTTO!!!
               *  ############################
               *
               **/
              LCTable_addLinesByDescriptor(LCT, pd_insert);


              /* FREE MEMORY FOR: pd_insert */
               PD_free(pd_insert);



              /* AD-ARCES
               * INSERIRE QUI IL CASO DI RIMOZIONE DELLA PROTEZIONE*/
              ProtectionDescriptor *pd_remove = getProtectionDescriptor_GSList(op->req->remove_graph);

               /*PiItem* */ pii=pd_remove->PiList;
               /*char* */   P_from_table=NULL;


               if(LCT==NULL) LCT=LCTable_new();


               while(pii!=NULL)
               {
                  LCLine *line = LCTable_getLCLineByIPi( LCT, pd_remove->I, pii->Pi  );

                  if(line!=NULL)
                  {
                     if(strcmp(pd_remove->owner,line->KP)!=0)
                      {
                 	    /*GENERATE FAULT*/
                     	op->header->tr_type=M3_PROTECTION_FAULT;
                     	whiteboard_log_debug("*** PROTECTION FAULT: isProtectionRequestConsistent():\n\t remove owner NOT equal to KP's line\n");
                     	whiteboard_log_debug("\t pd_remove->owner=%s,line->KP=%s\n",pd_remove->owner,line->KP);

                        /*FREE MEMORY*/
                    	PD_free(pd_remove);
                    	if(P_from_table!=NULL)free(P_from_table);
                    	//line

                     	return FALSE;
                      }//if(strcmp(pd_insert->owner,line->KP)!=0)
                     else
                      {
                     	if(P_from_table==NULL)
                     	  {P_from_table=malloc(strlen(line->P)+1);
                     	   strcpy(P_from_table,line->P);
                     	  }//if(P_from_table==NULL)

                      }//if(strcmp(pd_insert->owner,line->KP)!=0)...else...

                     /*FREE MEMORY*/
                 	//line

                  }//if(line!=NULL)

                  pii=pii->next;
               }//while(pii!=NULL)


               if(P_from_table!=NULL)
               {
             	  /**
             	   *  MODIFY OP and the descriptor with the P
             	   *  retrived from the LCTable
             	   * */
             	  PD_updateP( pd_remove , P_from_table );
             	  scheduler_item_updateP( op , P_from_table );
             	  free(P_from_table);

               }//if(P_from_table!=NULL)


               /**
                * May be it is usefull to check here if an error has
                *  been occurred during the table update process!!!
                *
                *  ############################
                *    puo la pd_insert andare a buon fine e modificare la tabella,
                *    mentre la remove FALLISCE???? o vice versa???
                *    NO!!!!
                *    QUINDI FARE UPDATE DELLA TABELLA SOLO ALLA FINE DI TUTTO!!!
                *  ############################
                *
                **/
               LCTable_removeLinesByDescriptor(LCT, pd_remove);


               /*
                * Check if this KP protects some other I's property
                */
               if( LCTable_getLCLineByIP( LCT , pd_remove->I , pd_remove->P) != NULL )
                {
                   whiteboard_log_debug("*** Some properties are still under protection...\n");
                   op->req->remove_graph = removeGraphSafeProtectionRemove( op->req->remove_graph );

                }//if( LCTable_getLCLineByIP(pd_remove->I , pd_remove->P) != NULL )

               /* FREE MEMORY FOR: pd_insert */
                PD_free(pd_remove);


        	  return TRUE;
          }//int isProtectionQueryConsistent( s_scheduler_item*  op)




          /**
           * Update every occurence of the P value into the op structure
           **/
          void scheduler_item_updateP(  s_scheduler_item*  op , char*  P_from_table )
          {
        	whiteboard_log_debug("*** void scheduler_item_updateP(  s_scheduler_item*  op , char*  P_from_table )\n");

        	  GSList *list[2];

        	  list[0]=op->req->insert_graph;
        	  list[1]=op->req->remove_graph;

        	  ssTriple_t *triple;

        	  int ig=0;
        	  for(;ig<2;ig++)
        	  while( list[ig]!=NULL )
        	  	{
        	     triple=list[ig]->data;

        	     if( strcmp(triple->predicate,AR_PROPERTY)==0 )
        	       {whiteboard_log_debug("*** scheduler_item_updateP:\n\t OBJ:UPDATE P from: %s to:%s\n",triple->object,P_from_table);
        	    	free(triple->object);
        	        triple->object=malloc(strlen(P_from_table)+1);
        	    	strcpy(triple->object,P_from_table);
        	       }//if( strcmp(triple->predicate,AR_PROPERTY)==0 )


        	     if(   strcmp(triple->predicate,AR_OWNER)==0
        	         ||strcmp(triple->predicate,AR_TARGET)==0 )
      	           {whiteboard_log_debug("*** scheduler_item_updateP:\n\t SUB:UPDATE P from: %s to:%s\n",triple->subject,P_from_table);
        	    	free(triple->subject);
      	            triple->subject=malloc(strlen(P_from_table)+1);
      	    	    strcpy(triple->subject,P_from_table);
      	           }// if(   strcmp(triple->predicate,AR_OWNER)==0 ...

        	  		  list[ig]=list[ig]->next;
        	  	 }//while( list!=NULL )


        	  whiteboard_log_debug("\n############################################\n");
        	  whiteboard_log_debug("START OP############################################\n");

        	  printOpContent(op);
        	  whiteboard_log_debug("END OP############################################\n");

          }//void updatePofOP(  s_scheduler_item*  op , P_from_table )





          //-----------------------

          void PD_updateP( ProtectionDescriptor *pd , char* P )
          {
            if(P==NULL)return;
            if(pd==NULL)return;
            if(pd->P!=NULL)free(pd->P);
            pd->P=malloc(strlen(P)+1);
            strcpy(pd->P,P);
          }//void PD_updateP( pd_insert , P_from_table )


          /**
           * Safe free-memory method for a ProtetionDesciptor object
           **/
          void PD_free( ProtectionDescriptor *pd )
          {if(pd==NULL)return;
           PiItem *i=pd->PiList,*n;

           if(i!=NULL)
           do
           {n=i->next;
            free(i);
            i=n;
           }while(i!=NULL);//do

           free(pd);

          }//void PD_free()





           /*-----------------------------*\

               THE LCTable implementation

           \*-----------------------------*/


          /******************************************************\
           * TEST LCTable library
          \******************************************************/

          void STOP(void) {whiteboard_log_debug("\nPress enter...\n");getchar();}

          void LCTable_Test( void )
          {
        	 static boolean test_done=FALSE; if(test_done)return; test_done=TRUE;


        	 LCTable *LCT=NULL;
        	 char I[]={"-I-"},P[]={"-P-"},Pi[]={"-Pi-X-"},KP[]={"-KP-X-"};
        	 LCLine *l, *l2=NULL, *l_tmp;

        	 whiteboard_log_debug("\n/* * * * * * * * * * * * * *\\\n");
        	 whiteboard_log_debug("   LC-TABLE FEASIBILITY TEST\n");
             whiteboard_log_debug("\\* * * * * * * * * * * * * * */\n\n\n");

        	 whiteboard_log_debug("Print content of a NULL LCTable:\n");
        	 LCTable_print(LCT);
        	 whiteboard_log_debug("Print a NEW LCTable content:\n");
        	 LCT=LCTable_new();
        	 LCTable_print(LCT);

        	 whiteboard_log_debug("Create a new line:\n");
        	 l = LCLine_new(I,P,Pi,KP);

        	 whiteboard_log_debug("Print a line:\n");
        	 LCLine_print(l);

        	 whiteboard_log_debug("Add one line to the LCTable (ammount:%d)\n", LCTable_addLine(LCT, l ) );
        	 whiteboard_log_debug("Print a LCTable content:\n");
        	 LCTable_print(LCT);



        	 /*****************/
        	 l_tmp=LCLine_new("-I-","-P-","-Pi-X-","-KP-X-");
        	 l2=LCTable_getLine(LCT, l_tmp );

        	 whiteboard_log_debug("/*-----------*/");
        	 whiteboard_log_debug("\"getLine\" from LCTable:%s\n", (l2==NULL?"NOT FOUND":"FOUND" ));
        	 if(l2!=NULL)
        	    {whiteboard_log_debug("Print the founded line:\n");  LCLine_print(l2);
        	     whiteboard_log_debug("Free a line (l_tmp):\n");     LCLine_free(l_tmp);
        	    }//if(l2!=NULL)


        	 whiteboard_log_debug("/*-----------*/");
        	 l2=LCTable_getLCLineByIP(LCT, "-I-","-P-" );
        	 whiteboard_log_debug("\"getLCLineByIP\" from LCTable:%s\n", (l2==NULL?"NOT FOUND":"FOUND" ));
        	 if(l2!=NULL){whiteboard_log_debug("Print the founded line:\n");  LCLine_print(l2);}//if(l2!=NULL)


        	 whiteboard_log_debug("/*-----------*/");
        	 l2=LCTable_getLCLineByIPi(LCT, "-I-","-Pi-X-" );
        	 whiteboard_log_debug("\"getLCLineByIPi\" from LCTable:%s\n", (l2==NULL?"NOT FOUND":"FOUND" ));
        	 if(l2!=NULL){whiteboard_log_debug("Print the founded line:\n");  LCLine_print(l2);}//if(l2!=NULL)


        	 whiteboard_log_debug("/*-----------*/");
        	 l2=LCTable_getLCLineByIPi(LCT, "-I-","-???-" );
        	 whiteboard_log_debug("\"getLCLineByIPi\" from LCTable(should be NOT):%s\n", (l2==NULL?"NOT FOUND":"FOUND" ));
        	 if(l2!=NULL){whiteboard_log_debug("Print the founded line:\n");  LCLine_print(l2);}//if(l2!=NULL)


        	 whiteboard_log_debug("Remove LCLine...\n");
        	 whiteboard_log_debug("Remove LCLine from table(ammount:%d)\n", LCTable_removeLine( LCT,l )  );
        	 whiteboard_log_debug("Print a LCTable content:\n");
        	 LCTable_print(LCT);


        	 whiteboard_log_debug("\n\nAdd lines to the LCTable:\n");


        	 int i;
        	 for(i=0;i<20;i++)
        	 {char str_Pi[60];

        	   sprintf(str_Pi,"Pi-%02d",i);
        	   whiteboard_log_debug("LCTable ammount:%d\n", LCTable_addLine(LCT,LCLine_new(I,P,str_Pi,KP)) );

        	 }//for(;i<20;i++)

        	 //STOP();

        	 whiteboard_log_debug("Print LCTable\n");
        	 LCTable_print(LCT);


        	 /*----------------------------*/
        	 whiteboard_log_debug("Create a new line:\n");              l = LCLine_new(I,P,"Pi-0",KP);
        	 whiteboard_log_debug("Print a line:\n");     	          LCLine_print(l);
        	 whiteboard_log_debug("Remove LCTable line (ammount:%d)\n", LCTable_removeLine(LCT,l) );
        	 whiteboard_log_debug("Print a LCTable content:\n");        LCTable_print(LCT);
        	 whiteboard_log_debug("Free line (l):\n");                  LCLine_free(l);
        	 /*----------------------------*/
        	 whiteboard_log_debug("Create a new line:\n");              l = LCLine_new(I,P,"Pi-00",KP);
        	 whiteboard_log_debug("Print a line:\n");     	          LCLine_print(l);
        	 whiteboard_log_debug("Remove LCTable line (ammount:%d)\n", LCTable_removeLine(LCT,l) );
        	 whiteboard_log_debug("Print a LCTable content:\n");        LCTable_print(LCT);
        	 whiteboard_log_debug("Free line (l):\n");                  LCLine_free(l);
        	 /*----------------------------*/
        	 whiteboard_log_debug("Create a new line:\n");                l = LCLine_new(I,P,"Pi-10",KP);
        	 whiteboard_log_debug("Print a line:\n");     	              LCLine_print(l);
        	 whiteboard_log_debug("Remove LCTable line (ammount:%d)\n", LCTable_removeLine(LCT,l) );
        	 whiteboard_log_debug("Free line (l):\n");                  LCLine_free(l);
        	 whiteboard_log_debug("Print a LCTable content:\n");        LCTable_print(LCT);
        	 /*----------------------------*/
        	 whiteboard_log_debug("Create a new line:\n");                l = LCLine_new(I,P,"Pi-15",KP);
        	 whiteboard_log_debug("Print a line:\n");     	              LCLine_print(l);
        	 whiteboard_log_debug("Remove LCTable line (ammount:%d)\n", LCTable_removeLine(LCT,l) );
        	 whiteboard_log_debug("Print a LCTable content:\n");        LCTable_print(LCT);
        	 whiteboard_log_debug("Free line (l):\n");                  LCLine_free(l);
        	 /*----------------------------*/
        	 whiteboard_log_debug("Create a new line:\n");                l = LCLine_new(I,P,"Pi-14",KP);
        	 whiteboard_log_debug("Print a line:\n");     	              LCLine_print(l);
        	 whiteboard_log_debug("Remove LCTable line (ammount:%d)\n", LCTable_removeLine(LCT,l) );
        	 whiteboard_log_debug("Print a LCTable content:\n");        LCTable_print(LCT);
        	 whiteboard_log_debug("Free line (l):\n");                  LCLine_free(l);
        	 /*----------------------------*/
        	 whiteboard_log_debug("Create a new line:\n");                l = LCLine_new(I,P,"Pi-19",KP);
        	 whiteboard_log_debug("Print a line:\n");     	              LCLine_print(l);
        	 whiteboard_log_debug("Remove LCTable line (ammount:%d)\n", LCTable_removeLine(LCT,l) );
        	 whiteboard_log_debug("Print a LCTable content:\n");        LCTable_print(LCT);
        	 whiteboard_log_debug("Free line (l):\n");                  LCLine_free(l);
        	 /*----------------------------*/
        	 whiteboard_log_debug("Create a new line:\n");                l = LCLine_new(I,P,"Pi-90",KP);
        	 whiteboard_log_debug("Print a line:\n");     	              LCLine_print(l);
        	 whiteboard_log_debug("Remove LCTable line (ammount:%d)\n", LCTable_removeLine(LCT,l) );
        	 whiteboard_log_debug("Print a LCTable content:\n");        LCTable_print(LCT);
        	 whiteboard_log_debug("Free line (l):\n");                  LCLine_free(l);


        	 whiteboard_log_debug("\n\nFree table:\n");                 LCTable_free(LCT);    LCT=NULL;
        	 whiteboard_log_debug("Print a LCTable content:\n");        LCTable_print(LCT);


        	 whiteboard_log_debug("\n/* * * * * * * * * * * * * *\\\n");
        	 whiteboard_log_debug("     TEST CONCLUDED\n");
        	 whiteboard_log_debug(" (This test runs just one time)\n");
             whiteboard_log_debug("\\* * * * * * * * * * * * * * */\n\n\n");

          }//LCTable_Test()



           char *strdump (const char *s)
           {
               char *d = (char *)(malloc (strlen (s) + 1));
               if (d == NULL) return NULL;
               strcpy (d,s);
               return d;
           }//char *strdup (const char *s)



            //----------------------------------------------------------
            void LCLine_free(LCLine* lcl)
            {
            	if(lcl==NULL)return;

            	free(lcl->I);
            	free(lcl->P);
            	free(lcl->Pi);
            	free(lcl->KP);
            	free(lcl);
            }//LCLine_free()


            //----------------------------------------------------------
            LCLine* LCLine_new(char* I,char* P,char* Pi,char* KP)
            { LCLine* lcl=(LCLine*)malloc(sizeof(LCLine));

              lcl->I    = strdump(I);
        	  lcl->P    = strdump(P);
        	  lcl->Pi   = strdump(Pi);
        	  lcl->KP   = strdump(KP);
        	  lcl->prev = NULL;
        	  lcl->next = NULL;

        	  return lcl;
            }//LCLine_new()



            /**
             * Safe free-memory method for a ProtetionDesciptor object
             **/
            void LCTable_free( LCTable *lct )
            {
             if(lct==NULL)return;

             LCLine *i=lct->line,*n;

             if(i!=NULL)
             do
             {n=i->next;
              LCLine_free(i);
              i=n;
             }while(i!=NULL);//do

             free(lct);

            }//void PD_free()


            /**
             * Allocate memory for a new table
             **/

            LCTable* LCTable_new()
            {LCTable* t=(LCTable*)malloc(sizeof(LCTable));

             t->size=0;
             t->line=NULL;
             return t;
            }//LCTable* LCTable_new()


            /**
             * Print the line "lcl"
             **/
            void LCLine_print( LCLine *lcl )
            {
                whiteboard_log_debug("|%31s|%31s|%31s|%31s| [p:%s n:%s]\n",lcl->I,lcl->P,lcl->Pi,lcl->KP,(lcl->prev==NULL?"NULL":"a-line"),(lcl->next==NULL?"NULL":"a-line"));
            }//void LCLine_print( LCLine *lcl )



            /**
             * Print the table "lct"
             **/
            void LCTable_print( LCTable *lct )
            { if(lct==NULL)return;

              LCLine *i=lct->line;
              int il=0;

              if(i==NULL)return;

              whiteboard_log_debug("/%31s|%31s|%31s|%31s\\\n","-","-","-","-");
              whiteboard_log_debug("|%31s|%31s|%31s|%31s|\n"," I"," P"," Pi"," KP");
              whiteboard_log_debug("|%31s|%31s|%31s|%31s|\n","-","-","-","-");
              do
               {LCLine_print(i);
                i=i->next;    il++;
                if(il>lct->size && i!=NULL){whiteboard_log_debug("\n*** ERROR:LCTable_print:while:inconsistent value for 'size':il=%d\n",il);break;}
               }while(i!=NULL);//do
              whiteboard_log_debug("\\%31s|%31s|%31s|%31s/\n","-","-","-","-");

            }//void LCTable_print( LCTable *lct )



            /**
             * Add a new line to the table T
             * Return the lines ammount
             **/

            int LCTable_addLine(LCTable* t,LCLine* l)
            {
            	l->next=t->line;
            	if(t->line!=NULL)t->line->prev=l;
            	t->line=l;

            	return ++t->size;
            }//LCTable_addLine()


            /**
             * Add a new line to the table T if and only if
             * a line with the same [I,Pi] does not exist yet
             * Return the lines ammount
             **/

            int LCTable_addLineIfDoesNotExistOnIPi( LCTable* t,LCLine* l )
            {
            	if( LCTable_getLCLineByIPi( t, l->I, l->Pi  ) == NULL)
            		/*The line does not exist, add the line*/
            	   return LCTable_addLine(t,l);
            	return t==NULL?0:t->size;
            }//LCTable_addLine()


            /**
             *
             **/
            LCLine* LCTable_getLine( LCTable* t, LCLine* l )
            {
                if(l==NULL)return NULL;

                LCLine *i=t->line;
                int il=0;

                //whiteboard_log_debug("LCTable_getLine:\n");
                if(i!=NULL)
                do
                {
                 //whiteboard_log_debug("LCTable_getLine:line content:il=%d\n",il);
                 //LCLine_print(i);

                 if(    strcmp(i->I,l->I)==0
                     && strcmp(i->P,l->P)==0
                     && strcmp(i->Pi,l->Pi)==0
                     && strcmp(i->KP,l->KP)==0
                    )
                    return i;

                 i=i->next; il++;
                 if(il>t->size && i!=NULL){whiteboard_log_debug("\n*** ERROR:LCTable_getLine:while:inconsistent value for 'size':il=%d\n",il);break;}
                }while(i!=NULL);//do

            	return NULL;
            }//LCLine* LCTable_getLCLine( LCTable* t, LCLine* l )



            /**
             * Remove a new line from the table T
             * Return the lines ammount
             **/

            int LCTable_removeLine(LCTable* t,LCLine* user_l)
            {
            	LCLine *p,*n,*l;

            	//printf("removeLine:Line to remove:\n");
            	//LCLine_print(user_l);

            	//printf("removeLine:Find line on the table:\n");
            	/*Search the table to get the exact line specified by the user*/
            	l=LCTable_getLine(t,user_l);

            	//printf("removeLine:getLine result:%s\n",(l==NULL?"NULL":"OK"));

            	/*  NULL value for "l" means "line not found" */
            	if(l==NULL)return t->size;

            	//printf("removeLine:Line found:Print line content:\n");
            	//LCLine_print(l);


            	p=l->prev;
            	n=l->next;
            	if(p!=NULL)p->next=n;
            	if(n!=NULL)n->prev=p;

            	//printf("removeLine:Free Line:\n");
            	//LCLine_free(l);

            	/*if it was the first line of the table*/
            	if(p==NULL) t->line=n;

            	return --t->size;
            }//LCTable_removeLine()



            /**
             *
             **/
            LCLine* LCTable_getLCLineByIPi( LCTable* t, char* I, char* Pi  )
            {
            	//printf("*** UNDER CONSTRUCTION!!! LCLine* LCTable_getLCLineByIPi( pd_insert->I, pii  )\n");
                if(t==NULL)return NULL;
                if(I==NULL)return NULL;
                if(Pi==NULL)return NULL;

                LCLine *i=t->line;

                if(i!=NULL)
                do
                {
                 if( strcmp(i->I,I)==0 && strcmp(i->Pi,Pi)==0 )
                    return i;

                 i=i->next;
                }while(i!=NULL);//do

            	return NULL;
            }//LCTable_getLCLineByIPi( pd_insert->I, pii  )


            /**
             *
             **/
            LCLine* LCTable_getLCLineByIP( LCTable* t, char* I, char* P  )
            {
                if(t==NULL)return NULL;
                if(I==NULL)return NULL;
                if(P==NULL)return NULL;

                LCLine *i=t->line;

                if(i!=NULL)
                do
                {
                 if( strcmp(i->I,I)==0 && strcmp(i->P,P)==0 )
                    return i;

                 i=i->next;
                }while(i!=NULL);//do

            	return NULL;
            }//LCTable_getLCLineByIP( pd_insert->I, pii  )



            /**
             *
             **/
            LCLine* LCTable_getLCLineByIKP( LCTable* t, char* I, char* KP  )
            {
                if(t==NULL)return NULL;
                if(I==NULL)return NULL;
                if(KP==NULL)return NULL;

                LCLine *i=t->line;

                if(i!=NULL)
                do
                {
                 if( strcmp(i->I,I)==0 && strcmp(i->KP,KP)==0 )
                    return i;

                 i=i->next;
                }while(i!=NULL);//do

            	return NULL;
            }//LCTable_getLCLineByIP( pd_insert->I, pii  )




            /**
             *
             * Return the lines ammount
             **/
            int LCTable_addLinesByDescriptor(LCTable* t, ProtectionDescriptor *pd)
            {
            	if(t==NULL)return -1;
            	if(pd==NULL)return t->size;
            	PiItem *i=pd->PiList;

            	if(i!=NULL)
            	do
            	{
                 //LCTable_addLine(t, LCLine_new( pd->I,pd->P,i->Pi,pd->owner) );
            	 LCTable_addLineIfDoesNotExistOnIPi(t, LCLine_new( pd->I,pd->P,i->Pi,pd->owner) );
            	 i=i->next;
            	}while(i!=NULL);//do

            	return t->size;
            }//LCTable_addLinesByDescriptor(pd_insert)




            /**
             *
             * Return the lines ammount
             **/
            int LCTable_removeLinesByDescriptor(LCTable* t, ProtectionDescriptor *pd)
            {
            	if(t==NULL)return -1;
            	if(pd==NULL)return t->size;
            	PiItem *i=pd->PiList;

            	if(i!=NULL)
            	do
            	{
                 LCTable_removeLine(t, LCTable_getLCLineByIPi( LCT , pd->I, i->Pi) );
            	 i=i->next;
            	}while(i!=NULL);//do

            	return t->size;
            }//LCTable_getLCLineByIPi( pd_insert->I, pii  )



            /**
             * It contributes to the overall consistency avoiding unwanted data deletion.
             * It is necessary because the KP doesn't know the Protection entity instance
             * and doesn't know if it is still protecting other properties of the same instance
             *
             * It removes the triples with AR_Property and AR_Owner from the remove graph
             * Return the new valid remove graph
             *
             * rg = Remove Graph
             **/
            GSList* removeGraphSafeProtectionRemove( GSList* list )
            {
            	ssTriple_t *triple;
            	GSList* l=list;

            	whiteboard_log_debug("############################################\n");
            	whiteboard_log_debug("removeGraphSafeProtectionRemove()\n");
            	whiteboard_log_debug("list content ##############################\n");
            	println_GSList(list);

                while( l!=NULL )
            	{
            	  triple=l->data;

            	  if(    strcmp(triple->predicate,AR_PROPERTY)==0
            	      || strcmp(triple->predicate,AR_OWNER)==0    )
            		  {
            		   l = g_slist_remove( list, triple );
            		  }
            	  else
            	      l=l->next;

            	}//while( list!=NULL )


            	whiteboard_log_debug("list content ##############################\n");
            	println_GSList(list);


            	whiteboard_log_debug("end content ##############################\n");
            	whiteboard_log_debug("############################################\n");

              return list;

            }//void removeGraphSafeProtectionRemove( GSList* rg, ProtectionDescriptor *pd )



