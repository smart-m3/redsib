#include "sib_operations.h"

gchar* control_string_expand(ssElement_t thing);
ssTriple_t* trasform_statement_expand(ssTriple_t* stat);

gchar* control_string_contract(ssElement_t thing);
ssTriple_t* trasform_statement_contract(ssTriple_t* stat);

void reasoning(sib_data_structure* param, ssTriple_t* t, gboolean* enable_real_reasoning);



