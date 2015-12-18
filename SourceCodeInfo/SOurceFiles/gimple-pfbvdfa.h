/*
     gdfa 1.0
     Copyright (C) 2008 GCC Resource Center, Department of Computer Science and Engineering,
     Indian Institute of Technology Bombay.
     License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
     This is free software: you are free to change and redistribute it.
     There is NO WARRANTY, to the extent permitted by law.
*/


/*        Macros, types, external variables, and functions for specifying 
          bit vector data flow analysers. Associated files are 
          gimple-pfbvdfa-init.c and gimple-pfbvdfa.c
*/

#define FOR_EACH_BB_FWD(entry_bb)     for(bb=entry_bb->next_bb;bb->next_bb!=NULL;bb=bb->next_bb)
#define FOR_EACH_BB_BKD(exit_bb)      for(bb=exit_bb->prev_bb;bb->prev_bb!=NULL;bb=bb->prev_bb)

#define FOR_EACH_STMT_FWD         for(bsi=bsi_start(bb);!bsi_end_p(bsi);bsi_next(&bsi))
#define FOR_EACH_STMT_BKD         for(bsi=bsi_last(bb);bsi.tsi.ptr!=NULL;bsi_prev(&bsi))




typedef struct expr_index_list
{
        int expr_no;
        struct expr_index_list *next;
}expr_index_list;

typedef struct defn_index_list
{
        int defn_no;
        struct defn_index_list *next;
}defn_index_list;


/*Data structure to store expression template*/
typedef struct expr_template
{
        tree expr;
        int op0_index;
        int op1_index;        /* op = VAR_DECL then index of var
                         else if op = INT_CST then value of INT_CST*/
} expr_template;


#define CURRENT_GEN(bb)   ((current_pfbv_dfi)[find_index_bb(bb)]->gen)
#define CURRENT_KILL(bb)   ((current_pfbv_dfi)[find_index_bb(bb)]->kill)
#define CURRENT_IN(bb)   ((current_pfbv_dfi)[find_index_bb(bb)]->in)
#define CURRENT_OUT(bb)   ((current_pfbv_dfi)[find_index_bb(bb)]->out)

#define DFI_nid(dfi,nid)   (dfi)[nid]

#define GEN_nid(dfi,nid)   ((dfi)[nid]->gen)
#define KILL_nid(dfi,nid)   ((dfi)[nid]->kill)
#define IN_nid(dfi,nid)   ((dfi)[nid]->in)
#define OUT_nid(dfi,nid)   ((dfi)[nid]->out)


#define GEN(dfi,bb)   ((dfi)[find_index_bb(bb)]->gen)
#define KILL(dfi,bb)   ((dfi)[find_index_bb(bb)]->kill)
#define IN(dfi,bb)   ((dfi)[find_index_bb(bb)]->in)
#define OUT(dfi,bb)   ((dfi)[find_index_bb(bb)]->out)
/*@Non-separable : START*/
#define GEN_OF_STMT(dfi,stmt)     ((dfi)[find_index_of_local_stmt(stmt)]->gen)
#define KILL_OF_STMT(dfi,stmt)    ((dfi)[find_index_of_local_stmt(stmt)]->kill)
#define IN_OF_STMT(dfi,stmt)      ((dfi)[find_index_of_local_stmt(stmt)]->in)
#define OUT_OF_STMT(dfi,stmt)     ((dfi)[find_index_of_local_stmt(stmt)]->out)
#define GEN_OF_STMT_nid(dfi,nid)   ((dfi)[nid]->gen)
#define KILL_OF_STMT_nid(dfi,nid)   ((dfi)[nid]->kill)
#define IN_OF_STMT_nid(dfi,nid)   ((dfi)[nid]->in)
#define OUT_OF_STMT_nid(dfi,nid)   ((dfi)[nid]->out)
/*@Non-separable : END*/

#define FOR_EACH_BB_IN_SPECIFIED_TRAVERSAL_ORDER         \
        for( visit_bb = (traversal_order == FORWARD)?  0 :  number_of_nodes -1 ;\
            (traversal_order == FORWARD)? visit_bb < number_of_nodes  -1 \
                                        : visit_bb >=0 ; \
            (traversal_order == FORWARD)? visit_bb++ \
                                        : visit_bb-- \
           )

typedef sbitmap dfvalue;


/* Data structure to hold data flow information bit vectors */

typedef struct pfbv_dfi
{
        dfvalue gen;
        dfvalue kill;
        dfvalue in;
        dfvalue out;
} pfbv_dfi;



typedef enum meet_operation
                {
                        UNION=1,
                        INTERSECTION
                } meet_operation;

typedef enum traversal_direction 
                {
                        FORWARD=1,
                        BACKWARD,
                        BIDIRECTIONAL
                } traversal_direction;

typedef enum initial_value 
                {
                        ONES=1, 
                        ZEROS
                } initial_value;

typedef enum dfi_to_be_preserved 
                { 
                        all=1, 
                        global_only, 
                        no_value 
                } dfi_to_be_preserved;

typedef enum entity_occurrence  
               { 
                        up_exp=1, 
                        down_exp, 
                        any_where
               } entity_occurrence;

typedef enum entity_manipulation 
               { 
                        entity_use=1, 
                        entity_mod 
               } entity_manipulation;

typedef enum entity_name 
               { 
                        entity_expr=1, 
                        entity_var, 
                        entity_defn
               } entity_name;

typedef struct lp_specs 
               {
                        entity_name entity;
                        entity_manipulation stmt_effect;
                        entity_occurrence exposition;
               } lp_specs;

/*@Non-separable : START*/
/*The statements revevent to datatflow analysis are:
**    (A) Assignment statements x =e where x E Var, e E Expr.
**    (B) Input Statements read(x), which assigns new value to x.
**    (C) Use Statemets use(x) which includes condition checking, printing, parameter passing.
**    (D) other Statemets.
*/
typedef enum statement_type
               {
                        READ_X = 1,
                        USE_X,
                        IGNORE_STATEMENT_TYPE
               } statement_type;


/*For the statement x =e, 'precond' are the conditions need to be checked before
**cosidering the data flow value.
*/
typedef enum precondition
               {
                        X_IN_OPERAND = 1,
                        X_NOT_IN_OPERAND,
                        OPERAND_IS_CONST,
                        OPERAND_ISNOT_CONST,
                        IGNORE_PRECONDITION
               } precondition;


/*For the statement x = e, the corresponding dataflow value may depend on the 
**global dataflow value ae well.(This is true for non-separable frameworks)
*/
typedef enum entity_dependence
               {
                        X_IN_GLOBAL_DATA_FLOW_VALUE = 1,
                        X_NOT_IN_GLOBAL_DATA_FLOW_VALUE,
                        OPER_IN_GLOBAL_DATA_FLOW_VALUE,
                        OPER_NOT_IN_GLOBAL_DATA_FLOW_VALUE,
                        IGNORE_ENTITY_DEPENDENCE
               } entity_dependence;


typedef struct lp_specs_nonseparable 
               {
                        entity_name         entity;
                        entity_manipulation stmt_effect;
                        statement_type      read_or_use_stmt;
                        precondition        precondition;
                        entity_dependence   dependence;
               } lp_specs_nonseparable;

/*@Non-separable : END*/

/* The main data structure for specifying data flow analysis */

struct gimple_pfbv_dfa_spec 
{
        entity_name               entity;
        initial_value             top_value_spec;
        initial_value             entry_info;
        initial_value             exit_info;
        traversal_direction       traversal_order;
        meet_operation            confluence;
        entity_manipulation       gen_effect;
        entity_occurrence         gen_exposition;
        entity_manipulation       kill_effect;
        entity_occurrence         kill_exposition;
        dfi_to_be_preserved       preserved_dfi; 

        dfvalue (*forward_edge_flow)(basic_block src, basic_block dest);
        dfvalue (*backward_edge_flow)(basic_block src, basic_block dest);
        dfvalue (*forward_node_flow)(basic_block bb);
        dfvalue (*backward_node_flow)(basic_block bb);
        /*@Non-separable : START*/
        statement_type            constgen_statement_type;
        precondition              constgen_precondition;
        statement_type            constkill_statement_type;
        precondition              constkill_precondition;
        entity_dependence         dependent_gen;
        entity_dependence         dependent_kill;
        /*@Non-separable : END*/
};


/* Main driver function */


pfbv_dfi ** pfbvdfa_driver(struct gimple_pfbv_dfa_spec dfa_spec);

/* Default edge flow functions */

dfvalue identity_forward_edge_flow(basic_block src, basic_block dest);
dfvalue identity_backward_edge_flow(basic_block src, basic_block dest);
dfvalue stop_flow_along_edge(basic_block src, basic_block dest);

/* Default node flow functions */

dfvalue stop_flow_along_node(basic_block bb);
dfvalue forward_gen_kill_node_flow(basic_block bb);
dfvalue backward_gen_kill_node_flow(basic_block bb);
dfvalue identity_forward_node_flow(basic_block bb);
dfvalue identity_backward_node_flow(basic_block bb);

/* dfvalue interface  */

bool is_dfvalue_equal(dfvalue value1, dfvalue value2);
void free_dfvalue_space(dfvalue value);
dfvalue intersect_dfvalues (dfvalue value1, dfvalue value2);
dfvalue union_dfvalues (dfvalue value1, dfvalue value2);
dfvalue a_plus_b_minus_c(dfvalue v_a, dfvalue v_b, dfvalue v_c);
dfvalue make_initialised_dfvalue(initial_value value);
dfvalue make_uninitialised_dfvalue(void);
void dump_dfvalue (FILE * file, dfvalue value);

/* helper functions */

int find_index_bb(basic_block bb);
void report_dfa_spec_error(const char * mesg);
tree extract_expr(tree stmt);
tree extract_lval(tree stmt);
tree extract_operand(tree expr,int op_num);
int find_index_of_local_var(tree var);
int find_index_of_local_expr(tree expr);

/**  End of helper functions **/

extern pfbv_dfi ** current_pfbv_dfi ;


