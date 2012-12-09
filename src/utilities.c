#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <utilities.h>



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
