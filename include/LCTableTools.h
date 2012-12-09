/*

  Copyright (c) 2010, ARCES - University of Bologna
  All rights reserved.

*/

#ifndef LCTABLE_TOOLS_H
#define LCTABLE_TOOLS_H

#include "sib_operations.h"


#define s_scheduler_item  struct SCHEDULER_ITEM


      /*AD-ARCES*/
      /* ARCES-Protection compatibility check for
       * the op coming from the i_queque list
       *
       * */


      //AR=Access Restrinction
      #define AR_PROPERTY  "http://ProtectionOntology.org#Has_Access_Restriction"
      #define AR_OWNER     "http://ProtectionOntology.org#Has_Owner"
      #define AR_TARGET    "http://ProtectionOntology.org#Has_Target"


      void ProtectionCompatibilityFilter( s_scheduler_item* op);

      void printOpContent( s_scheduler_item*  op);

      void println_GSList(GSList* list);

      int checkProtectionOnPredicate_GSList(GSList* list);

      #define OP_NORMAL      1
      #define OP_PROTECTION  2


      typedef int boolean;

      #ifndef	FALSE
      #define	FALSE	(0)
      #endif

      #ifndef	TRUE
      #define	TRUE	(!FALSE)
      #endif



      /* *****************************************************
       * Check the predicate value of every triple to discover
       * conflict with protection stuff properties
       * return the type of the op
       * OP_NORMAL:     no protection property found
       * OP_PROTECTION: one or more protection property found
       *
       */
      int check_op_type_on_predicate( s_scheduler_item*  op);


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
      boolean isInsertRemoveOperationConsistent( s_scheduler_item*  op);


      typedef struct {
        /* Pi */
        char* Pi;
        /* NEXT ITEM */
        void* next;
      }PiItem;


      typedef struct {
        /* id of joined kp*/
        char* owner;
        /* I */
        char* I;
        /* P */
        char* P;
        /* List of property to protect (Pi) */
        PiItem* PiList;
      } ProtectionDescriptor;


      void PD_updateP( ProtectionDescriptor *pd , char* P );


      /**
       * Safe free-memory method for a ProtetionDesciptor object
       **/
      void PD_free( ProtectionDescriptor *pd );

      /**
       * Update every occurence of the P value into the op structure
       **/
      void scheduler_item_updateP(  s_scheduler_item*  op , char* P_from_table );



      /* *****************************************************
       * Return a protection descriptor made by:
       * OWNER
       * I
       * P
       * Pi[]
       *
       */

       ProtectionDescriptor* getProtectionDescriptor_GSList(GSList* list);


       /**
          ALGORITMO !!!

         accedere con I-Pi[i] alla tabella
         faccio la query
         controllo owner della riga con in richiedente.
         Se sono == vado avanti...(incremento indice C )(recupero il P della tabella)
         se sono differenti, fermo tutto -> op->header->tr_type=M3_PROTECTION_FAULT;
         se non ho valori vado avanti....

         alla fine, se non mi sono fermato prima,
         se C è uguale a ZERO
         posso aggiungere alla tabella tutto il pd

         se C != ZERO, e ho impostato il P (preso dalla tabella)
         sostituisco in op il P con il P preso dalla tabella
         sostituisco in pd il P con il P preso dalla tabella

         posso aggiungere alla tabella tutto il pd

        **
        * Return TRUE if the protection insert/remove query is LC table consistent
        *  otherwise, FALSE
        * In case of TRUE, the LC table will be modified as well as the op structure
        * In case of FALSE, only the op structure will be modified as follow:
        *   op->header->tr_type=M3_PROTECTION_FAULT;
        */
       boolean isProtectionRequestConsistent( s_scheduler_item*  op);






       /*-----------------------------*\

           THE LCTable implementation

       \*-----------------------------*/



       /******************************************************\
        * TEST LCTable library
       \******************************************************/

       void STOP(void);
       void LCTable_Test( void );

       #define LCTABLE_DIM  10

       char *strdump (const char *s);


       /**
        * A single LCTable line
        **/
       typedef struct {
               /* I */
               char* I;
               /* P */
               char* P;
               /* Pi */
               char* Pi;
               /* KP ID or OWNER */
               char* KP;

               /* Previouse line in the table*/
               void* prev;
               /* Next line in the table*/
               void* next;
             } LCLine;

        //----------------------------------------------------------
        void LCLine_free(LCLine* lcl);

        //----------------------------------------------------------
        LCLine* LCLine_new(char* I,char* P,char* Pi,char* KP);


        /**
         * Print the line "lcl"
         **/
        void LCLine_print( LCLine *lcl );


        /**
         * LC Table
         **/
        typedef struct {
          //The table
          LCLine* line;
          //Table size
          int size;
        } LCTable;


        /**
         * Safe free-memory method for a ProtetionDesciptor object
         **/
        void LCTable_free( LCTable *lct );

        /**
         * Allocate memory for a new table
         **/

        LCTable* LCTable_new(void);


        /**
         * Print the table "lct"
         **/
        void LCTable_print( LCTable *lct );

        /**
         * Add a new line at the table T
         * Return the lines ammount
         **/

        int LCTable_addLine(LCTable* t,LCLine* l);


        /**
         * Add a new line to the table T if and only if
         * a line with the same [I,Pi] does not exist yet
         * Return the lines ammount
         **/

        int LCTable_addLineIfDoesNotExistOnIPi( LCTable* t,LCLine* l );

        /**
         *
         **/
        LCLine* LCTable_getLine( LCTable* t, LCLine* l );


        /**
         * Remove a new line from the table T
         * Return the lines ammount
         **/

        int LCTable_removeLine(LCTable* t,LCLine* l);


        /**
         *
         **/
        LCLine* LCTable_getLCLineByIPi( LCTable* t, char* I, char* Pi  );


        /**
         *
         **/
        LCLine* LCTable_getLCLineByIP( LCTable* t, char* I, char* P  );

        /**
         *
         **/
        LCLine* LCTable_getLCLineByIKP( LCTable* t, char* I, char* KP  );

        /**
         *
         * Return the lines ammount
         **/
        int LCTable_addLinesByDescriptor(LCTable* t, ProtectionDescriptor *pd);


        /**
         *
         * Return the lines ammount
         **/
        int LCTable_removeLinesByDescriptor(LCTable* t, ProtectionDescriptor *pd);


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
        GSList* removeGraphSafeProtectionRemove( GSList* list );


        /* Bstr = braced string, e.g. {abcde} */
        //#define braces_strcmp(str,Bstr)  (strncmp(   (Bstr)+1, (str), (strlen((Bstr))-2)   ))
	#define braces_strcmp(str,Bstr)  (strcmp(  Bstr, str ))

#endif
