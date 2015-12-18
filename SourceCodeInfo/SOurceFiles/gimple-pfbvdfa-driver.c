/*
     gdfo 1.0
     Copyright (C) 2008 GCC Resource Center, Department of Computer Science and Engineering,
     Indian Institute of Technology Bombay.
     License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
     This is free software: you are free to change and redistribute it.
     There is NO WARRANTY, to the extent permitted by law.
*/
#include "assert.h"
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "rtl.h"
#include "tm_p.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "output.h"
#include "errors.h"
#include "flags.h"
#include "function.h"
#include "expr.h"
#include "ggc.h"
#include "langhooks.h"
#include "diagnostic.h"
#include "tree-flow.h"
#include "timevar.h"
#include "tree-dump.h"
#include "tree-pass.h"
#include "toplev.h"
#include "except.h"
#include "cfgloop.h"
#include "cfglayout.h"
#include "hashtab.h"
#include "gimple-pfbvdfa.h"

#define ASSERT(condition)        assert(condition);
/*@Non-separable : START*/
#define IS_NONSEPARABLE(dfa_spec)       (IGNORE_ENTITY_DEPENDENCE != dfa_spec.dependent_gen	          \
                                  ||    IGNORE_ENTITY_DEPENDENCE  != dfa_spec.dependent_kill) 

#define IS_NODE_CONSTANT(node)          (TREE_CODE(node) == INTEGER_CST || TREE_CODE(node) == REAL_CST    \
                                  ||    TREE_CODE(node) == FIXED_CST   || TREE_CODE(node) == COMPLEX_CST  \
                                  ||    TREE_CODE(node) == VECTOR_CST  || TREE_CODE(node) == STRING_CST) 
#define ENTITY_INDEX(node) (node).common.base.index
/*@Non-separable : END*/

/***    Generic Bit Vector Data Flow Analyzer for Gimple IR with 
        example instantiations for several bit vector frameworks 

        ( Abbreviations used in the code

         pf = "Per Function", wp = "Whole Program", 
         bv = "Bit Vector"
         dfa = "Data Flow Analysis"
         dfa = "Generic Data Flow Analyzer"
         dfi = "Data Flow Information"

        )

        This data flow analyzer is an intraprocedural (i.e. "per function")
        analyzer and hence restricts itself to local variables, expressions,
        and definitions. More details about this are provided in the associate
        file gimple-pfdfa-support.c which contains the initialization code.

        This driver computes generic data flow equations of the form

        IN(bb) = MEET_over_preds (forward_edge_flow(OUT(pred_bb))
                 MEET
                 backward_node_flow (OUT(bb))

        OUT(bb) = MEET_over_succs (backward_edge_flow(IN(succ_bb))
                  MEET
                  forward_node_flow (IN(bb))

        where the node and edge flow functions are of the usual
          form: f(X) = GEN + (X - KILL).

        In order the instantiate a data flow analysis, one needs to

        - Specify direction of traversal (FORWARD/BACKWARD).

        - Specify confluence operation (INTERSECTION/UNION).

        - Specify the top element of the lattice. This value is used for 
          initialization.

        - Specify the entry_info/exit_info (data flow value associate with 
          the ENTRY/EXIT block of the control flow graph).

        - Specify flow functions and associate them with corresponding
          function pointers. For this purpose, default flow functions
          ("identity", "gen_kill", and "stop_flow") have been also defined. 
          Thus the user has to provide only the non-default flow functions. An
          example of "stop_flow" is the backward node and edge flow functions 
          in available expressions analysis.

        - Specify the semantics of local properties GEN and KILL.
          (Details are given below.)

        - Specify whether the driver should return global data flow values
          only (i.e. IN/OUT), or all data flow values (i.e. IN/OUT/GEN/KILL),
          or no data flow value.

        These values are specified by initializing a structure variable
        ("struct gimple_pfbv_dfa_spec"). This file contains specifications
        for the following bit vector frameworks:

        - Available expressions analysis (ave)
        - Partially available expressions analysis (pav)
        - Anticipable expressions analysis (ant)
        - Live variables analysis (lv)
        - Partial redundancy elimination (pre)

        The following references are recommended for the generic concepts
        of data flow analysis:
        -  Uday P. Khedker, Amitabha Sanyal, and Bageshri Karkare. 
           "Data Flow Analysis: Theory and Practice". CRC Press, USA 
           (In preparation, will be available by May 2009).
        -  Uday P. Khedker. 
           "Data Flow Analysis". 
           In "The Compiler Design Handbook : Optimizations & Machine Code Generation." 
           (1st edition) CRC Press USA. 2002. 
           (This chapter is now being expanded in a full-fledged book and hence is not 
            present in the second edition of the Compiler Design Handbook.)

        This implementation also uses some new concepts related to
        local data flow analysis. In practical optimizers, implementing
        global data flow analyzers is easier compared to implementing
        local data flow analyzers. This is because local data flow
        analysis has to deal with the lower level intricate details of
        the intermediate representation and intermediate representation
        are the most complex data structures in practical compilers.
        Global data flow analyzer are insulated from these lower level
        details; they just need to know control flow graphs in terms
        of basic blocks. Thus most data flow analysis engines require
        leave local property computation to be implemented by the user
        of the engine.

        In this generic data flow analyzer, local properties are
        assumed to be GEN and KILL. For computing these properties,
        rather than writing C code, only their semantics needs to be
        specified in terms of a combination of suitable values of the
        following three attributes:

        1. The entity for which data flow analysis is being performed.
           At the moment, three entities are visualized: Expressions,
           Variables, and Definitions.
        2. The effect of a statement on these entities. These could be
           either the "use" of the entity or its "modification".  We
           have fixed the usual meanings of these for the above entities.
        3. Whether these effects are "upwards exposed", "downwards
           exposed", or "exposition does not matter" in the basic block.  
           We have fixed the usual meanings of these for the above entities.


        As an example, for available expressions analysis, the values of 
        these attributes are 

        For GEN: "entity = expr", "effect = use", and "exposition = downwards". 
        For KILL: "entity = expr", "effect = mod", and "exposition = any_where". 

        At the moment, support for definitions has not be implemented but 
        it is easy to do so. 

        When new entities are supported, two things will have to be done:
        (a) It will have to be examined whether the other two attributes
            (effect and exposition) are sufficient or whether a new 
            attribute needs to be supported. If a new attribute is 
            required, its possible values will have to be identified.
        (b) It will have to be examined if additional values of effect
            and exposition are required.
        
        The main limitation of this approach is that it requires
        independent traversal of a basic block for computing GEN and
        KILL. However, by using a slightly more complicated data
        structure that passes both GEN and KILL to the basic block level
        routines will solve this problem. The other limitation is that
        due to the generality, there are many checks that are done in
        the underlying functions. There are two possible options:

        - This is used as a rapid prototyping tool for a given data flow
          analysis. Once the details are fixed, one could spend time
          writing a more efficient data flow analyzer.
        - Instead of "interpreting" the specification, they are
          compiled into a customized code. In principle, this is
          similar to "compiling" the machine descriptions using the
          "gen..." programs into the actual back end code at the time
          of building GCC.
           
     
        Compile time option "-fdump-tree-all" creates the dump files.
        Initial and final values are printed by the option
        "-fgdfa". Values in each iteration are printed by the option
        "-fgdfa-details".


        We suggest the following extensions to gdfa:

        . Extensions that do not require changing the architecture of gdfa

          - include space and time measurement of analyses.

          - Consider scalar formal parameters for analysis.

          - Support a work list based driver.

          - Extend gdfa to support definitions as entities and specify
            reaching definitions analysis.

          - Extend gdfa to support other entities such as statements
            (eg. for data flow analysis based program slicing), and
            basic blocks (eg. for data flow analysis based dominator
            computation). Both these problems are bit vector problems.

          - Improve the implementation of gdfa to make it more space
            and time efficient. This may require compromising on the
            simplicity of the implementation but generality should not
            be compromised.

        . Extensions that may require minor changes to the architecture of gdfa.

          – Implement incremental data flow analysis and measure its
            effectiveness by invoking in just before gimple is expanded
            into RTL. This would require a variant of a work list based
            driver.

          – Explore the possibility of extending gdfa to the data flow
            frameworks where data flow information can be represented
            using bit vectors but the frameworks are not bit vector
            frameworks because they are nonseparable (eg. faint
            variables analysis, possibly undefined variables, analysis,
            strongly live variables analysis).

            This would require changing the local data flow analysis.
            One possible option is using matrix based local property
            computation [See Bageshri Karkare's Ph.D. Thesis]. The other
            option is to treat a statement as an independent basic
            block.

      . Extensions that may require major changes to the architecture of gdfa.

          - Extend gdfa to non-separable frameworks in which data flow
            information cannot be represented by bit vectors (eg.
            constant propagation, signs analysis, points-to analysis,
            alias analysis, heap reference analysis etc. Although the
            main driver would remain same, this would require making
            fundamental changes to the architecture.

          - Extend gdfa to support some variant of context and flow
            sensitive interprocedural data flow analysis.
--------------------------------------------------------------------------
        This code has been implemented by Uday Khedker
        (www.cse.iitb.ac.in/~uday:uday@cse.iitb.ac.in) with active
        suggestions from Bageshri Karkare (bageshri@gmail.com). The main
        data structures in this code, as also the support functions
        interfacing with GIMPLE IR (in file gimple-pfdfa-support.c) have
        been adapted from the original code implemented by Seema
        Ravandale (ravandaless@gmail.com) when she was working at IIT
        Bombay under IITB Research Fellowship.
--------------------------------------------------------------------------

***/


/************ Generic bit vector data flow analysis driver **************/


pfbv_dfi ** gdfa_driver(struct gimple_pfbv_dfa_spec dfa_spec);
static void perform_pfbvdfa(void);
static bool compute_in_info(basic_block bb);
static bool compute_out_info(basic_block bb);
static dfvalue combined_forward_edge_flow(basic_block bb);
static dfvalue combined_backward_edge_flow(basic_block bb);
static void preserve_dfi(dfi_to_be_preserved preserve);
static void create_dfi_space(int);
static void initialise_special_values(struct gimple_pfbv_dfa_spec dfa_spec);
static int find_entity_size(struct gimple_pfbv_dfa_spec dfa_spec);

dfvalue (*forward_edge_flow)(basic_block src, basic_block dest);
dfvalue (*backward_edge_flow)(basic_block src, basic_block dest);
dfvalue (*forward_node_flow)(basic_block bb);
dfvalue (*backward_node_flow)(basic_block bb);


/********** Default node and edge flow functions   *************/

dfvalue identity_forward_edge_flow(basic_block src, basic_block dest);
dfvalue identity_backward_edge_flow(basic_block src, basic_block dest);
dfvalue identity_forward_node_flow(basic_block bb);
dfvalue identity_backward_node_flow(basic_block bb);
dfvalue stop_flow_along_node(basic_block bb);
dfvalue stop_flow_along_edge(basic_block src, basic_block dest);
dfvalue forward_gen_kill_node_flow(basic_block bb);
dfvalue backward_gen_kill_node_flow(basic_block bb);

/********* dfvalue support functions for flow functions ********/

static dfvalue combine (dfvalue value1, dfvalue value2);
static bool is_new_info(dfvalue prev_info,dfvalue new_info);

/***************  Specification Driven Local Property Computation ***************/

static void local_dfa(struct gimple_pfbv_dfa_spec dfa_spec);
static dfvalue local_dfa_of_bb(lp_specs lps_given, basic_block bb);
static dfvalue effect_of_a_statement(lp_specs lps_given, tree stmt, dfvalue accumulated_entities);
static dfvalue exprs_in_statement(tree stmt, lp_specs lps);
static dfvalue vars_in_statement(tree stmt, lp_specs lps);
static dfvalue defn_in_statement(tree stmt, lp_specs lps);
/*@Non-separable : START*/
static void    local_dfa_nonseparable(struct gimple_pfbv_dfa_spec dfa_spec);
static dfvalue local_dfa_of_stmt(lp_specs_nonseparable lps_given, tree bb);
static dfvalue var_in_statement_nonseparable(tree stmt, lp_specs_nonseparable lps, dfvalue globaldf);
static dfvalue update_gen_of_stmt(tree, lp_specs_nonseparable, dfvalue);
static dfvalue update_kill_of_stmt(tree, lp_specs_nonseparable, dfvalue);
/*@Non-separable : END*/
        
/************ End of specification driven local property computation ***********/

/************ Top level functions to print the result of data flow analysis   ***********/

static void print_entity_info(void);
static void print_initial_dfi(void);
static void print_final_dfi(int count);
static void print_per_iteration_dfi(int iteration);

/************ Lower level functions to print the result of data flow analysis   ***********/

static void dump_dfi(FILE * file, bool in_iterations);
static void dump_basic_block_info(FILE * file, basic_block bb);
static void dump_entity_list(FILE * file, dfvalue value);
static void dump_entity_mapping(FILE * file);


/******************* End of the generic dfa driver    *******************/


/**************** miscellaneous   ******************/
static void verify_allocation_of_dfi(pfbv_dfi **dfi);

extern expr_index_list **exprs_of_vars;
extern defn_index_list **defns_of_vars;
extern expr_template **local_expr;
extern int local_expr_count;
extern tree * local_var_list;
extern tree * local_defn_list;
extern int local_var_count;
extern int local_defn_count;
extern int number_of_nodes;
extern varray_type dfs_ordered_basic_blocks; 
/*@Non-separable : START*/
extern tree * local_stmt_list;
extern int local_stmt_count;
extern int find_index_of_local_stmt(tree stmt);
extern void assign_indices_to_stmt(void);
/*@Non-separable : END*/

static traversal_direction traversal_order;
static meet_operation confluence;

static initial_value top_value_spec;
static dfvalue value_top = NULL;
static dfvalue entry_info = NULL;
static dfvalue exit_info = NULL;

int relevant_pfbv_entity_count = 0;
static entity_name relevant_pfbv_entity;

pfbv_dfi ** current_pfbv_dfi = NULL;
/*@Non-separable : START*/
pfbv_dfi ** current_pfbv_dfi_of_stmt = NULL;
static bool is_nonseparable = false; 
static lp_specs_nonseparable gen_lps, kill_lps;
static change_at_in_out_of_stmt; 
/*@Non-separable : END*/


extern FILE * dump_file;


static void debug_statement_expr(void);
static void print_dfi(FILE*);
pfbv_dfi ** 
gdfa_driver(struct gimple_pfbv_dfa_spec dfa_spec)
{
        if (find_entity_size(dfa_spec) == 0)
                return NULL;

	initialise_special_values(dfa_spec);

        if(IS_NONSEPARABLE(dfa_spec))
        {
                is_nonseparable = true;

                gen_lps.entity               = dfa_spec.entity;
                gen_lps.stmt_effect          = dfa_spec.gen_effect;
                gen_lps.read_or_use_stmt     = dfa_spec.constgen_statement_type;
                gen_lps.precondition         = dfa_spec.constgen_precondition;
                gen_lps.dependence           = dfa_spec.dependent_gen;

                kill_lps.entity              = dfa_spec.entity;
                kill_lps.stmt_effect         = dfa_spec.kill_effect;
                kill_lps.read_or_use_stmt    = dfa_spec.constkill_statement_type;
                kill_lps.precondition        = dfa_spec.constkill_precondition;
                kill_lps.dependence          = dfa_spec.dependent_kill;


                create_dfi_space(local_stmt_count);
                current_pfbv_dfi_of_stmt = current_pfbv_dfi;

                create_dfi_space(number_of_nodes);

                local_dfa_nonseparable(dfa_spec);
		/*debug_statement_expr();*/
        } else {
                create_dfi_space(number_of_nodes); 
                local_dfa(dfa_spec); 
        }

        traversal_order = dfa_spec.traversal_order; 
        confluence = dfa_spec.confluence;

        forward_edge_flow = dfa_spec.forward_edge_flow;
        backward_edge_flow = dfa_spec.backward_edge_flow;
        forward_node_flow = dfa_spec.forward_node_flow;
        backward_node_flow = dfa_spec.backward_node_flow;

        perform_pfbvdfa();

        preserve_dfi(dfa_spec.preserved_dfi); 

        if(is_nonseparable) 
        {
                return current_pfbv_dfi_of_stmt;
                is_nonseparable = false;
        } else {
                return current_pfbv_dfi; 
        }
}

static void
initialise_special_values(struct gimple_pfbv_dfa_spec dfa_spec)
{
        top_value_spec = dfa_spec.top_value_spec;

        value_top = make_initialised_dfvalue(dfa_spec.top_value_spec);
        entry_info = make_initialised_dfvalue(dfa_spec.entry_info);
        exit_info = make_initialised_dfvalue(dfa_spec.exit_info); 

}

static int
find_entity_size(struct gimple_pfbv_dfa_spec dfa_spec)
{
        switch (dfa_spec.entity)
        {        
                case entity_expr:
                        relevant_pfbv_entity = entity_expr;
                        relevant_pfbv_entity_count = local_expr_count;
                        break;
                case entity_var:
                        relevant_pfbv_entity = entity_var;
                        relevant_pfbv_entity_count = local_var_count;
                        break;
                case entity_defn:
                        relevant_pfbv_entity = entity_defn;
                        relevant_pfbv_entity_count = local_defn_count;
                        break;
                default:
                        report_dfa_spec_error ("Wrong choice of entity (Function gdfa_driver)");
                        break;
        }

	print_entity_info();

        return relevant_pfbv_entity_count;
}


static void 
perform_pfbvdfa(void)
{
        int visit_bb=0, iteration_number=0;
        basic_block bb;
        bool change, change_at_in, change_at_out;
 
	print_initial_dfi(); 

        do{        
                iteration_number++;
                change = false;
                FOR_EACH_BB_IN_SPECIFIED_TRAVERSAL_ORDER  
                {         
                        bb = VARRAY_BB(dfs_ordered_basic_blocks,visit_bb);
                        if(bb)
                        {        
                                if (traversal_order == FORWARD)
                                {
                                        change_at_in = compute_in_info(bb);
                                        change_at_out = compute_out_info(bb);
                                        change = change || change_at_out || change_at_in;
                                }
                                else if ((traversal_order == BACKWARD) || (traversal_order == BIDIRECTIONAL))
                                {
                                        change_at_out = compute_out_info(bb);
                                        change_at_in = compute_in_info(bb);
                                        change = change || change_at_in || change_at_out;
                                }
                                else 
                                         report_dfa_spec_error ("Direction can only be FORWARD, BACKWARD, or BIDIRECTIONAL (Function perform_pfbvdfa)");
                        }
                }
		print_per_iteration_dfi(iteration_number); 
        } while(change);

	print_final_dfi(iteration_number);

}

static bool 
compute_in_info(basic_block bb)
{        
        bool change;
        dfvalue temp, old;

        temp = make_uninitialised_dfvalue();
        change_at_in_out_of_stmt = false;

        if (!bb->preds) 
                temp = combine(entry_info, backward_node_flow(bb));
        else 
                temp = combine(combined_forward_edge_flow(bb),
                                     backward_node_flow(bb));

        /*DEBUG*/
        //if(is_nonseparable ) { 
        //    fprintf(stdout, "in compute_in_info");  
        //    dump_dfvalue(stdout, temp);
        //}
                            
        old = CURRENT_IN(bb);
        change = is_new_info(temp,old);
        if (change)
        {
                CURRENT_IN(bb) = temp;
                if (old)
                        free_dfvalue_space(old);
        }
        return change || change_at_in_out_of_stmt ;
}

static bool 
compute_out_info(basic_block bb)
{        
        bool change;
        dfvalue temp, old;

        temp = make_uninitialised_dfvalue();
        change_at_in_out_of_stmt = false;

        if (!bb->succs)
                temp = combine(exit_info, forward_node_flow(bb));
        else
                temp = combine(combined_backward_edge_flow(bb),
                                     forward_node_flow(bb));
                       
        /*DEBUG*/
        //if(is_nonseparable ) { 
        //    fprintf(stdout, "in compute_out_info");  
        //    dump_dfvalue(stdout, temp);
        //}

        old = CURRENT_OUT(bb);
        change = is_new_info(temp,old);
        if (change)
        {
                CURRENT_OUT(bb) = temp;
                if (old)
                        free_dfvalue_space(old);
        }
        return change || change_at_in_out_of_stmt;
}


static dfvalue
combined_forward_edge_flow(basic_block bb)
{                
        dfvalue temp, new;
        VEC(edge, gc) *edge_vec;
        edge e;
        edge_iterator ei;
        basic_block pred_bb;

        edge_vec = bb->preds;
        temp = make_initialised_dfvalue(top_value_spec);

        if (forward_edge_flow == &stop_flow_along_edge)
                return temp;
        
        FOR_EACH_EDGE(e,ei,edge_vec)
        {
                pred_bb = e->src;
                new = combine(temp,forward_edge_flow(pred_bb,bb));
                if (temp)
                        free_dfvalue_space(temp);
                temp = new;        
        }
        return temp;
}

static dfvalue
combined_backward_edge_flow(basic_block bb)
{                
        dfvalue temp, new;
        VEC(edge, gc) *edge_vec;
        edge e;
        edge_iterator ei;
        basic_block succ_bb;

        edge_vec = bb->succs;
        temp = make_initialised_dfvalue(top_value_spec);

        if (backward_edge_flow == &stop_flow_along_edge)
                return temp;

        FOR_EACH_EDGE(e,ei,edge_vec)
        {
                succ_bb = e->dest;
                new = combine(temp,backward_edge_flow(bb,succ_bb));
                if (temp)
                        free_dfvalue_space(temp);
                temp = new;        
        }                
        return temp;
}

static void
preserve_dfi(dfi_to_be_preserved preserve)
{
        int iter;
        
        switch (preserve)
        {        
                case no_value:
                        for (iter=0; iter < number_of_nodes; iter++)
                        {                         
                                if (GEN_nid(current_pfbv_dfi,iter))
                                {
                                        free_dfvalue_space(GEN_nid(current_pfbv_dfi,iter));
                                        GEN_nid(current_pfbv_dfi,iter) =  NULL;
                                }
                                if (KILL_nid(current_pfbv_dfi,iter))
                                {
                                        free_dfvalue_space(KILL_nid(current_pfbv_dfi,iter));
                                        KILL_nid(current_pfbv_dfi,iter) = NULL;
                                }
                                if (IN_nid(current_pfbv_dfi,iter))
                                {
                                        free_dfvalue_space(IN_nid(current_pfbv_dfi,iter));
                                        IN_nid(current_pfbv_dfi,iter) = NULL;
                                }
                                if (OUT_nid(current_pfbv_dfi,iter))
                                {
                                        free_dfvalue_space(OUT_nid(current_pfbv_dfi,iter));
                                        OUT_nid(current_pfbv_dfi,iter) = NULL;
                                }
                        }
                        if (current_pfbv_dfi)
                                ggc_free(current_pfbv_dfi);
                        current_pfbv_dfi = NULL;

                        if(is_nonseparable) 
                        {
                                for (iter=0; iter < local_stmt_count; iter++)
                                {
                                        if (GEN_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter))
                                        {
                                                free_dfvalue_space(GEN_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter));
                                                GEN_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter) = NULL; 
                                        }
                                        if (KILL_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter))
                                        {
                                                free_dfvalue_space(KILL_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter));
                                                KILL_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter) = NULL; 
                                        }
                                        if (IN_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter))
                                        {
                                                free_dfvalue_space(IN_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter));
                                                IN_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter) = NULL; 
                                        }
                                        if (OUT_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter))
                                        {
                                                free_dfvalue_space(OUT_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter));
                                                OUT_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter) = NULL; 
                                        }
                                }
                                if (current_pfbv_dfi_of_stmt)
                                        ggc_free(current_pfbv_dfi_of_stmt);
                                current_pfbv_dfi_of_stmt = NULL;
                        }
                        break;
                case global_only:
                        for (iter=0; iter < number_of_nodes; iter++)
                        {                         
                                if (GEN_nid(current_pfbv_dfi,iter))
                                {
                                        free_dfvalue_space(GEN_nid(current_pfbv_dfi,iter));
                                        GEN_nid(current_pfbv_dfi,iter) =  NULL;
                                }
                                if (KILL_nid(current_pfbv_dfi,iter))
                                {
                                        free_dfvalue_space(KILL_nid(current_pfbv_dfi,iter));
                                        KILL_nid(current_pfbv_dfi,iter) = NULL;
                                }
                        }
                        if(is_nonseparable) 
                        {
                                for (iter=0; iter < local_stmt_count; iter++)
                                {
                                        if (GEN_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter))
                                        {
                                                free_dfvalue_space(GEN_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter));
                                                GEN_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter) = NULL; 
                                        }
                                        if (KILL_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter))
                                        {
                                                free_dfvalue_space(KILL_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter));
                                                KILL_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter) = NULL; 
                                        }
                                }
                        }
                        break;

                case all:
                        break;
                
                default:
                        report_dfa_spec_error("Wrong choice of values to be preserved (Function preserve_dfi)");
                        break;
        }
}


static void
create_dfi_space(int number_of_nodes)
{
        int iter;


        current_pfbv_dfi = (pfbv_dfi **)ggc_alloc_cleared(sizeof(pfbv_dfi*)*number_of_nodes);

        for (iter=0; iter < number_of_nodes; iter++)
        {         

                /* We use nid to access DFI because for nid 0 and 1, bb is NULL */

                DFI_nid(current_pfbv_dfi,iter) =  (pfbv_dfi *)ggc_alloc_cleared(sizeof(pfbv_dfi));

                IN_nid(current_pfbv_dfi,iter) =  make_initialised_dfvalue(top_value_spec);
                OUT_nid(current_pfbv_dfi,iter) =  make_initialised_dfvalue(top_value_spec);

                /* Allocation and initialisation for local properties is
                   done independently.
                */
                GEN_nid(current_pfbv_dfi,iter) =  NULL;
                KILL_nid(current_pfbv_dfi,iter) =  NULL;

        }
}


/********** Default node and edge flow functions   *************/

dfvalue
identity_forward_edge_flow(basic_block src, basic_block dest)
{
        return CURRENT_OUT(src);
}

dfvalue
identity_backward_edge_flow(basic_block src, basic_block dest)
{
        return CURRENT_IN(dest);
}

dfvalue
identity_forward_node_flow(basic_block bb)
{
        return CURRENT_IN(bb);
}

dfvalue
identity_backward_node_flow(basic_block bb)
{
        return CURRENT_OUT(bb);
}

dfvalue
stop_flow_along_node(basic_block bb)
{
        return value_top;
}

dfvalue
stop_flow_along_edge(basic_block src, basic_block dest)
{
        return value_top;
}

dfvalue
forward_gen_kill_node_flow(basic_block bb)
{
        dfvalue temp, gen_of_stmt, kill_of_stmt, old_in_dfvalue, old_out_dfvalue, new_in_dfvalue, new_out_dfvalue;
        block_stmt_iterator bsi;
        tree stmt = NULL;
        bool change_at_in, change_at_out;

        temp = make_uninitialised_dfvalue();
        new_out_dfvalue = CURRENT_IN(bb); 

        if(true == is_nonseparable) 
        {
                new_in_dfvalue = CURRENT_IN(bb);  
                FOR_EACH_STMT_FWD
                {
                        stmt = bsi_stmt(bsi); 

                        /*DEBUG*/ 
                        //fprintf(dump_file,"\n");
                        //print_generic_stmt(dump_file, stmt,0);

                        if(-1 == find_index_of_local_stmt(stmt))  continue;                        
                                               
                        old_in_dfvalue = IN_OF_STMT(current_pfbv_dfi_of_stmt, stmt); 
                        change_at_in = is_new_info(new_in_dfvalue, old_in_dfvalue);
                        if(change_at_in)
                        {
                                IN_OF_STMT(current_pfbv_dfi_of_stmt,stmt) = new_in_dfvalue;
                                if(old_in_dfvalue) free_dfvalue_space(old_in_dfvalue); 
                        }

                        gen_of_stmt  = update_gen_of_stmt(stmt, gen_lps,  new_in_dfvalue);
                        kill_of_stmt = update_kill_of_stmt(stmt, kill_lps, new_in_dfvalue);

                        new_out_dfvalue = a_plus_b_minus_c(gen_of_stmt, new_in_dfvalue, kill_of_stmt); 

                        old_out_dfvalue = OUT_OF_STMT(current_pfbv_dfi_of_stmt, stmt);
                        change_at_out = is_new_info(new_out_dfvalue, old_out_dfvalue);
                        if (change_at_out)
                        {
                                OUT_OF_STMT(current_pfbv_dfi_of_stmt,stmt) = new_out_dfvalue;
                                if (old_out_dfvalue) free_dfvalue_space(old_out_dfvalue);
                        }

                        change_at_in_out_of_stmt = change_at_in_out_of_stmt || change_at_out || change_at_in;
                        new_in_dfvalue = new_out_dfvalue;
                }
                temp = new_out_dfvalue;
        } else {
	        temp = a_plus_b_minus_c(CURRENT_GEN(bb), CURRENT_IN(bb), CURRENT_KILL(bb));
        }
        return temp;
}

dfvalue
backward_gen_kill_node_flow(basic_block bb)
{
        dfvalue temp, gen_of_stmt, kill_of_stmt, old_in_dfvalue, old_out_dfvalue, new_in_dfvalue, new_out_dfvalue;
        block_stmt_iterator bsi;
        tree stmt = NULL;
        bool change_at_in, change_at_out;

        temp = make_uninitialised_dfvalue();
        new_in_dfvalue = CURRENT_OUT(bb); 

        if(true == is_nonseparable) 
        {
                new_out_dfvalue = CURRENT_OUT(bb);  
                FOR_EACH_STMT_BKD
                {
                        stmt = bsi_stmt(bsi); 

                        /*DEBUG*/ 
                        //fprintf(dump_file,"\n");
                        //print_generic_stmt(dump_file, stmt,0);

                        if(-1 == find_index_of_local_stmt(stmt))  continue;                        
                        
                        old_out_dfvalue = OUT_OF_STMT(current_pfbv_dfi_of_stmt, stmt); 
                        change_at_out = is_new_info(new_out_dfvalue, old_out_dfvalue);
                        if(change_at_out)
                        {
                                OUT_OF_STMT(current_pfbv_dfi_of_stmt,stmt) = new_out_dfvalue;
                                if(old_out_dfvalue) free_dfvalue_space(old_out_dfvalue); 
                        }

                        gen_of_stmt  = update_gen_of_stmt(stmt, gen_lps, new_out_dfvalue);
                        kill_of_stmt = update_kill_of_stmt(stmt, kill_lps,new_out_dfvalue);

                        new_in_dfvalue = a_plus_b_minus_c(gen_of_stmt, new_out_dfvalue, kill_of_stmt);

                        old_in_dfvalue = IN_OF_STMT(current_pfbv_dfi_of_stmt, stmt);
                        change_at_in = is_new_info(new_in_dfvalue, old_in_dfvalue);
                        if (change_at_in)
                        {
                                IN_OF_STMT(current_pfbv_dfi_of_stmt,stmt) = new_in_dfvalue;
                                if (old_in_dfvalue) free_dfvalue_space(old_in_dfvalue);
                        }

                        change_at_in_out_of_stmt = change_at_in_out_of_stmt || change_at_out || change_at_in;
                        new_out_dfvalue = new_in_dfvalue;
                }
                temp = new_in_dfvalue;
        } else {
	        temp = a_plus_b_minus_c(CURRENT_GEN(bb), CURRENT_OUT(bb), CURRENT_KILL(bb));
        }

        return temp;
}

static dfvalue
combine (dfvalue value1, dfvalue value2)
{        
        dfvalue temp;


        if (confluence == INTERSECTION)
                temp = intersect_dfvalues(value1, value2);
        else if (confluence == UNION)
                temp = union_dfvalues(value1, value2);
        else 
                 report_dfa_spec_error ("Confluence can only be UNION or INTERSECTION (Function combine)");

        return temp;
}

static bool
is_new_info(dfvalue prev_info,dfvalue new_info)
{
        if(!(is_dfvalue_equal(prev_info,new_info))) 
                return true;
        else return false;
}


/***************  Specification Driven Local Property Computation ***************/

static void
local_dfa_nonseparable(struct gimple_pfbv_dfa_spec dfa_spec)
{
        int iter;
        tree stmt = NULL;

        for (iter=0; iter < local_stmt_count; iter++)
        {
                stmt                                                 = local_stmt_list[iter];
                GEN_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter)       = local_dfa_of_stmt(gen_lps, stmt);
                KILL_OF_STMT_nid(current_pfbv_dfi_of_stmt,iter)      = local_dfa_of_stmt(kill_lps, stmt);
        }       
}        

static dfvalue 
local_dfa_of_stmt(lp_specs_nonseparable lps_given, tree stmt)
{
    dfvalue dfvalue_of_stmt = NULL;
    switch (lps_given.entity)
        {
        case entity_var:
            dfvalue_of_stmt = var_in_statement_nonseparable(stmt,lps_given, NULL);
            break;
        case entity_expr:
            break;
        case entity_defn:
            break;
        default :
            report_dfa_spec_error ("Wrong choice of entity in local property computation (Function effect_of_statement)");
            break;
    }
    return dfvalue_of_stmt;
}

static dfvalue
var_in_statement_nonseparable(tree stmt, lp_specs_nonseparable lps, dfvalue global_dfv)
{
        dfvalue temp_Gen = NULL, temp = NULL;
        tree expr=NULL, left_opd=NULL, right_opd=NULL, lval=NULL;
        int lval_index=-1, left_opd_index=-1, right_opd_index=-1;
        bool operand_is_constant = false;

        if (lps.entity != entity_var)
                report_dfa_spec_error ("Wrong choice of entity in local property computation (Function vars_in_statement)");

        temp_Gen = make_initialised_dfvalue(ZEROS);
        temp = make_initialised_dfvalue(ZEROS);

        /*Extract the lvalue of the stmt*/
        lval = extract_lval(stmt);
        lval_index = find_index_of_local_var(lval);

        /*Extract the expr of the stmt*/
        expr = extract_expr(stmt);

        if (expr)
        {
                left_opd = extract_operand(expr,0);
                right_opd = extract_operand(expr,1);
                left_opd_index = find_index_of_local_var(left_opd);
                right_opd_index = find_index_of_local_var(right_opd);
        } else {
                if(TREE_CODE(stmt) == GIMPLE_MODIFY_STMT || TREE_CODE(stmt) == MODIFY_EXPR)
                {
                        expr = extract_operand(stmt,1);

                        /* Check if the operand is constant, i.e stmts of type a =5.*/
                        operand_is_constant = IS_NODE_CONSTANT(expr) ? true : false;  

                        /* Stmts of type a = b; or a = a; These are represented by left_opd_index > 0 and right_opd_index = -2.*/
                        if(TREE_CODE(expr) == VAR_DECL) 
                        {
                                 left_opd = extract_operand(expr,0);
                                 if(TREE_CODE(left_opd) == IDENTIFIER_NODE) 
                                 {
                                         left_opd_index = find_index_of_local_var(expr); 
                                 }
                        }
                }
        }


        switch (lps.stmt_effect)
        {
                case entity_mod:
                        if(NULL == global_dfv)
                        {
                                switch(lps.precondition) 
                                {
                                        case X_IN_OPERAND: 
                                                if(-1 != lval_index && (lval_index == left_opd_index || lval_index == right_opd_index)) 
                                                {
                                                        SET_BIT(temp_Gen,lval_index);
                                                }
                                                break;
                    			case X_NOT_IN_OPERAND:
                                                if(-1 != lval_index && (lval_index != left_opd_index && lval_index != right_opd_index)) 
                                                {
                                                        SET_BIT(temp_Gen,lval_index);
                                                }
                                                break;
                                        case OPERAND_IS_CONST:
                                                if(true == operand_is_constant) 
                                                {
                                                        if(-1 != lval_index)
                                                            SET_BIT(temp_Gen,lval_index);
                                                }
                                                break;
                                        case OPERAND_ISNOT_CONST:
                                                break;
                                        case IGNORE_PRECONDITION:
                                                break;
                                }
                                switch(lps.read_or_use_stmt)
                                {
                                        case READ_X:
                                                break;
                                        case USE_X:
                                                break;
                                        case IGNORE_STATEMENT_TYPE:
                                                break;
                                }
                        } else {
                                switch(lps.dependence)
                                {
                                        case X_IN_GLOBAL_DATA_FLOW_VALUE:
                                               if(-1 != lval_index) 
                                               {
                                                       SET_BIT(temp,lval_index);
                                                       if(true == sbitmap_any_common_bits(temp, global_dfv))
                                                               SET_BIT(temp_Gen,lval_index);
                                               }
                                               break;
                                        case X_NOT_IN_GLOBAL_DATA_FLOW_VALUE:
                                               if(-1 != lval_index) 
                                               {
                                                       SET_BIT(temp,lval_index);
                                                       if(false == sbitmap_any_common_bits(temp, global_dfv))
                                                               SET_BIT(temp_Gen,lval_index);
                                               }
                                               break;
                                        case OPER_IN_GLOBAL_DATA_FLOW_VALUE:
                                               if(-1 != lval_index) 
                                               {
                                                       if(-1 != left_opd_index)
                                                               SET_BIT(temp,left_opd_index);
                                                       if(-1 != right_opd_index)
                                                               SET_BIT(temp,right_opd_index);
                                                       if(true == sbitmap_any_common_bits(temp, global_dfv))
                                                               SET_BIT(temp_Gen,lval_index);
                                               }
                                               break;
                                        case OPER_NOT_IN_GLOBAL_DATA_FLOW_VALUE:
                                               if(-1 != lval_index) 
                                               {
                                                       if(-1 != left_opd_index)
                                                               SET_BIT(temp,left_opd_index);
                                                       if(-1 != right_opd_index)
                                                               SET_BIT(temp,right_opd_index);
                                                       if(false == sbitmap_any_common_bits(temp, global_dfv))
                                                               SET_BIT(temp_Gen,lval_index);
                                               }
                                               break;
                                        case IGNORE_ENTITY_DEPENDENCE:
                                               break;
                                }
                        }
                        break;
                case entity_use:
                        if(NULL == global_dfv)
                        { 
                                switch(lps.precondition)
                                {
                                        case X_IN_OPERAND:
                                            break;
                                        case X_NOT_IN_OPERAND:
                                            break;
                                        case OPERAND_IS_CONST:
                                            if(true == operand_is_constant) 
                                            {
                                                    if(-1 != lval_index)
                                                            SET_BIT(temp_Gen,lval_index);
                                            }
                                            break;
                                        case OPERAND_ISNOT_CONST:
                                            break;
                                        case IGNORE_PRECONDITION:
                                            break;
                                }
                                switch(lps.read_or_use_stmt)
                                {
                                        case READ_X:
                                            break;
                                        case USE_X:
                                            /*Condition Checking*/
                                            if(-1 == lval_index) 
                                            {
                                                    if(-1 != left_opd_index)
                                                            SET_BIT(temp_Gen,left_opd_index);
                                                    if(-1 != right_opd_index)
                                                            SET_BIT(temp_Gen,right_opd_index);
                                            }           
                                            break;
                                        case IGNORE_STATEMENT_TYPE:
                                            break;
                                }
                        } else {
                                switch(lps.dependence)
                                {
                                        case X_IN_GLOBAL_DATA_FLOW_VALUE:
                                               if(-1 != lval_index) 
                                               {
                                                       SET_BIT(temp,lval_index);
                                                       if(true == sbitmap_any_common_bits(temp, global_dfv))
                                                       {
                                                               if(-1 != left_opd_index)
                                                                       SET_BIT(temp_Gen,left_opd_index);
                                                               if(-1 != right_opd_index)
                                                                       SET_BIT(temp_Gen,right_opd_index);
                                                       }
                                               }
                                            break;
                                        case X_NOT_IN_GLOBAL_DATA_FLOW_VALUE:
                                               if(-1 != lval_index) 
                                               {
                                                       SET_BIT(temp,lval_index);
                                                       if(false == sbitmap_any_common_bits(temp, global_dfv))
                                                       {
                                                               if(-1 != left_opd_index)
                                                                       SET_BIT(temp_Gen,left_opd_index);
                                                               if(-1 != right_opd_index)
                                                                       SET_BIT(temp_Gen,right_opd_index);
                                                       }
                                                }
                                            break;
                                        case OPER_IN_GLOBAL_DATA_FLOW_VALUE:
                                                if(-1 != left_opd_index)     
                                                        SET_BIT(temp,left_opd_index);
                                                if(-1 != right_opd_index)     
                                                        SET_BIT(temp,right_opd_index);
                                                if(true == sbitmap_any_common_bits(temp, global_dfv))
                                                {
                                                        if(-1 != left_opd_index)
                                                                SET_BIT(temp_Gen,left_opd_index);
                                                        if(-1 != right_opd_index)
                                                                SET_BIT(temp_Gen,right_opd_index);
                                                }
                                            break;
                                        case OPER_NOT_IN_GLOBAL_DATA_FLOW_VALUE:
                                                if(-1 != left_opd_index)     
                                                        SET_BIT(temp,left_opd_index);
                                                if(-1 != right_opd_index)     
                                                        SET_BIT(temp,right_opd_index);
                                                if(false == sbitmap_any_common_bits(temp, global_dfv))
                                                {
                                                        if(-1 != left_opd_index)
                                                                SET_BIT(temp_Gen,left_opd_index);
                                                        if(-1 != right_opd_index)
                                                                SET_BIT(temp_Gen,right_opd_index);
                                                }
                                            break;
                                        case IGNORE_ENTITY_DEPENDENCE:
                                            break;
                                }
                                break;
                        }
                        break;
                default:
                        report_dfa_spec_error ("Wrong entity manipulation in local property computation (Function vars_in_statement_nonseparable)");
                        break;
        }
        ASSERT(temp_Gen);
        return temp_Gen;
}

static dfvalue 
update_gen_of_stmt(tree stmt, lp_specs_nonseparable lps_given, dfvalue globaldfv)
{
    dfvalue dep_dfvalue, const_dfvalue;
    switch (lps_given.entity)
    {
        case entity_var:
            dep_dfvalue = var_in_statement_nonseparable(stmt,lps_given, globaldfv);
                        /*DEBUG*/
                        //fprintf(dump_file,"depGen : ");
                        //dump_entity_list(dump_file,dep_dfvalue);
                        
            const_dfvalue = GEN_OF_STMT(current_pfbv_dfi_of_stmt,stmt);
            return union_dfvalues(dep_dfvalue, const_dfvalue);
        case entity_expr:
            /*To be implemented*/ 
            break;
        case entity_defn:
            /*To be implemented*/ 
            break;
        default :
            report_dfa_spec_error ("Wrong choice of entity in local property computation (Function effect_of_statement)");
            break;
    }
}

static dfvalue
update_kill_of_stmt(tree stmt, lp_specs_nonseparable lps_given, dfvalue globaldfv)
{
    dfvalue dep_dfvalue, const_dfvalue;
    switch (lps_given.entity)
        {
        case entity_var:
            dep_dfvalue = var_in_statement_nonseparable(stmt,lps_given, globaldfv);
                        /*DEBUG*/
                        //fprintf(dump_file,"depKill : ");
                        //dump_entity_list(dump_file,dep_dfvalue);
                        
            const_dfvalue = KILL_OF_STMT(current_pfbv_dfi_of_stmt,stmt);
            return union_dfvalues(dep_dfvalue, const_dfvalue);
        case entity_expr:
            /*To be implemented*/ 
            break;
        case entity_defn:
            /*To be implemented*/ 
            break;
        default :
            report_dfa_spec_error ("Wrong choice of entity in local property computation (Function effect_of_statement)");
            break;
    }
}


static void
local_dfa(struct gimple_pfbv_dfa_spec dfa_spec)
{
        basic_block bb;
        int iter;
        lp_specs gen_lps, kill_lps;

        gen_lps.entity = dfa_spec.entity;
        gen_lps.stmt_effect = dfa_spec.gen_effect;
        gen_lps.exposition = dfa_spec.gen_exposition;

        kill_lps.entity = dfa_spec.entity;
        kill_lps.stmt_effect = dfa_spec.kill_effect;
        kill_lps.exposition = dfa_spec.kill_exposition;

        for (iter=0; iter < number_of_nodes; iter++)
        {         
                bb = VARRAY_BB(dfs_ordered_basic_blocks,iter);
                if (bb)
                {        
                         GEN(current_pfbv_dfi,bb) = local_dfa_of_bb(gen_lps, bb);
                         KILL(current_pfbv_dfi,bb) = local_dfa_of_bb(kill_lps, bb);
                }
                else
                {        
                         GEN_nid(current_pfbv_dfi,iter) = make_initialised_dfvalue(ZEROS);
                         KILL_nid(current_pfbv_dfi,iter) = make_initialised_dfvalue(ZEROS);
                }
        }
}


static dfvalue
local_dfa_of_bb(lp_specs lps_given, basic_block bb)
{
        block_stmt_iterator bsi;
        tree stmt;
        dfvalue accumulated_entities = NULL;

        accumulated_entities = make_initialised_dfvalue(ZEROS); 

        switch (lps_given.exposition)
        {       
                case down_exp:
                case any_where:
                        FOR_EACH_STMT_FWD         
                        {       
                                stmt = bsi_stmt(bsi);
                                accumulated_entities = effect_of_a_statement(lps_given, stmt, accumulated_entities);
                        }
                        break;
                case up_exp:
                        FOR_EACH_STMT_BKD         
                        {       
                                stmt = bsi_stmt(bsi);
                                accumulated_entities = effect_of_a_statement(lps_given, stmt, accumulated_entities);
                        }
                        break;
                default :
                        report_dfa_spec_error ("Wrong choice of exposition in local property computation (Function local_dfa_of_bb)");
                        break;
        }
        ASSERT (accumulated_entities)
        return accumulated_entities ; 
}


static dfvalue
effect_of_a_statement(lp_specs lps_given, tree stmt, dfvalue accumulated_entities)
{
        dfvalue add_entities=NULL, remove_entities=NULL;
        lp_specs lps_temp;        

        switch (lps_given.entity)
        {        
                 case entity_expr:
                        add_entities = exprs_in_statement(stmt, lps_given);
                        if (lps_given.stmt_effect == entity_use)
                        {       
                                lps_temp.entity = lps_given.entity;
                                lps_temp.exposition = lps_given.exposition;
                                lps_temp.stmt_effect = entity_mod;
                                remove_entities = exprs_in_statement(stmt, lps_temp);
                        }
                        else 
                        {
                                remove_entities = make_initialised_dfvalue(ZEROS); 
                        }
                        accumulated_entities=a_plus_b_minus_c(add_entities, accumulated_entities, remove_entities);
                        break;
                case entity_var:
                        add_entities = vars_in_statement(stmt, lps_given);
                        if (lps_given.stmt_effect == entity_use)
                        {       
                                lps_temp.entity = lps_given.entity;
                                lps_temp.exposition = lps_given.exposition;
                                lps_temp.stmt_effect = entity_mod;
                                remove_entities = vars_in_statement(stmt, lps_temp);
                        }
                        else 
                        {
                                remove_entities = make_initialised_dfvalue(ZEROS); 
                        }
                        accumulated_entities=a_plus_b_minus_c(add_entities, accumulated_entities, remove_entities);
                        break;
                case entity_defn:
                        add_entities = defn_in_statement(stmt, lps_given);
			if (lps_given.stmt_effect == entity_use)
                        {       
                                lps_temp.entity = lps_given.entity;
                                lps_temp.exposition = lps_given.exposition;
                                lps_temp.stmt_effect = entity_mod;
                                remove_entities = defn_in_statement(stmt, lps_temp);
                        }
                        else 
                        {
                                remove_entities = make_initialised_dfvalue(ZEROS); 
                        }
			accumulated_entities=a_plus_b_minus_c(add_entities, accumulated_entities, remove_entities);
                        break;        
                default :
                        report_dfa_spec_error ("Wrong choice of entity in local property computation (Function effect_of_statement)");
                        break;
        }
        ASSERT (accumulated_entities)
        if (add_entities)
                free_dfvalue_space(add_entities);
        if (remove_entities)
                free_dfvalue_space(remove_entities);
        return accumulated_entities;
}

/**

Find out those expressions in a given statement that satisfy the local property specification

**/

static dfvalue
exprs_in_statement(tree stmt, lp_specs lps)
{       
        dfvalue temp_Gen=NULL;
        tree expr=NULL,lval=NULL;

        int expr_index=-1, lval_index=-1;
        
        if (lps.entity != entity_expr)
                 report_dfa_spec_error ("Wrong choice of entity in local property computation (Function exprs_in_statement)");

        temp_Gen = make_initialised_dfvalue(ZEROS);


        /* Find the expression occurring in stmt */

        expr = extract_expr(stmt); 
        expr_index = find_index_of_local_expr(expr);

        /* Find out the l-value of this statement */
  
        lval = extract_lval(stmt);
        lval_index = find_index_of_local_var(lval);
        
#if TEST_LOCAL_ANALYSIS
        printf ("\n\nGiven Statement is ");
        print_generic_stmt(stdout,stmt,0);
        printf ("\tRequired Exposition is ");

        switch (lps.exposition)
        {      
               case down_exp:
                        printf ("Downwards\n");
                       break;
               case any_where:
                        printf ("Anywhere\n");
                        break;
               case up_exp:
                        printf ("Upwards\n");
                       break;
        }
#endif

        switch (lps.stmt_effect)
        {        
               case entity_use:
                        if (expr_index != -1)
                        {       
                                SET_BIT(temp_Gen,expr_index);

                                /* Reset the bits of the expressions if its operand
                                      is modified by this statement */
 
                                if(lval_index >=0 && lval_index < local_var_count)
                                {
                                        expr_index_list * temp= NULL; 
                                        for(temp = exprs_of_vars[lval_index];temp;temp=temp->next)
                                        {
                                                if (temp->expr_no == expr_index)
                                                {        
                                                        switch (lps.exposition)
                                                        {       
                                                                case down_exp:
                                                                case any_where:
                                                                        RESET_BIT(temp_Gen,temp->expr_no);
                                                                        break;
                                                                case up_exp:
                                                                        break;
                                                                default:
                                                                        report_dfa_spec_error ("Wrong choice of occurrence in local property computation (Function exprs_in_statement)");
                                                                        break;
                                                        }
                                                }
                                        }
                                }
                        }
#if TEST_LOCAL_ANALYSIS
                        printf ("entity Use. Used entity is \t");
                        dump_entity_list(stdout,temp_Gen);        
#endif
                        break;
                case entity_mod:
                        if(lval_index >=0 && lval_index < local_var_count) 
                        {
                                expr_index_list * temp= NULL; 
                                for(temp = exprs_of_vars[lval_index];temp;temp=temp->next)
                                {
                                        if (temp->expr_no != expr_index)
                                                /* These expressions do not appear in this
                                                   statement but are modified by this 
                                                   statement.
                                                */
                                                SET_BIT(temp_Gen,temp->expr_no);
                                        else
                                        {        /* Expression appearing in the statement
                                                   is included only if we are looking for
                                                   upwards exposed expressions.
                                                */
                                                switch (lps.exposition)
                                                {        
                                                        case down_exp:
                                                        case any_where:
                                                                SET_BIT(temp_Gen,temp->expr_no);
                                                                break;
                                                    
                                                        case up_exp:
                                                                break;
                                                        default:
                                                                report_dfa_spec_error ("Wrong choice of occurrence in local property computation (Function exprs_in_statement)");
                                                                break;
                                                }
                                        }
                                }
                        }
#if TEST_LOCAL_ANALYSIS
                        printf ("entity Modification. Modified entity is \t");
                        dump_entity_list(stdout,temp_Gen);        
#endif
                        break;
                default: 
                        report_dfa_spec_error ("Wrong entity manipulation in local property computation (Function exprs_in_statement)");
                        break;
        }
        ASSERT (temp_Gen)
        return temp_Gen;
}

static dfvalue
defn_in_statement(tree stmt, lp_specs lps)
{       
        dfvalue temp_Gen=NULL;
        tree defn=NULL,lval=NULL;

        int defn_index=-1, lval_index=-1;
        
        if (lps.entity != entity_defn)
                 report_dfa_spec_error ("Wrong choice of entity in local property computation (Function defn_in_statement)");

        temp_Gen = make_initialised_dfvalue(ZEROS);


        /* Find the definition number if statement is definition */

        defn_index = find_index_of_local_defn(stmt);

        /* Find out the l-value of this statement */
	  
        lval = extract_lval(stmt);
        lval_index = find_index_of_local_var(lval);
        
        switch (lps.stmt_effect)
        {        
               case entity_use:
                        if (defn_index >= 0 && defn_index < local_defn_count)
                        {       
                                SET_BIT(temp_Gen,defn_index);

                                /* Reset the bits of the other definitions of lval */ 
 
                                if(lval_index >=0 && lval_index < local_var_count)
                                {
                                        defn_index_list * temp= NULL; 
                                        for(temp = defns_of_vars[lval_index];temp;temp=temp->next)
                                        {
                                                if (temp->defn_no != defn_index && temp->defn_no != -1) 
                                                {    
						        
                                                        switch (lps.exposition)
                                                        {       
                                                                case down_exp:
                                                                case any_where:
                                                                        RESET_BIT(temp_Gen,temp->defn_no);
                                                                        break;
                                                                case up_exp:
                                                                        break;
                                                                default:
                                                                        report_dfa_spec_error ("Wrong choice of occurrence in local property computation (Function defn_in_statement)");
                                                                        break;
                                                        }
						    
						}
					}
                                }
                        }
	
                        break;

                case entity_mod:
                        if(lval_index >=0 && lval_index < local_var_count) 
                        {
                                defn_index_list * temp= NULL; 
                                for(temp = defns_of_vars[lval_index];temp;temp=temp->next)
                                {
                                        if (temp->defn_no != defn_index && temp->defn_no != -1)
                                                SET_BIT(temp_Gen,temp->defn_no);
                                 
                                }
                        }
                        break;
                default: 
                        report_dfa_spec_error ("Wrong entity manipulation in local property computation (Function defn_in_statement)");
                        break;
        }
        ASSERT (temp_Gen)
        return temp_Gen;
}

static dfvalue
vars_in_statement(tree stmt, lp_specs lps)
{        
        dfvalue temp_Gen=NULL;
        tree expr=NULL, left_opd=NULL, right_opd=NULL, lval=NULL;

        int lval_index=-1, left_opd_index=-1, right_opd_index=-1;
        
        if (lps.entity != entity_var)
                 report_dfa_spec_error ("Wrong choice of entity in local property computation (Function vars_in_statement)");

        temp_Gen = make_initialised_dfvalue(ZEROS);


        /* Find out the l-value of this statement */
  
        lval = extract_lval(stmt);
        lval_index = find_index_of_local_var(lval);


        /* Find the expression and its variables occurring in stmt */
        expr = extract_expr(stmt); 
        if (expr)
        {
                left_opd = extract_operand(expr,0);
                right_opd = extract_operand(expr,1);

                        left_opd_index = find_index_of_local_var(left_opd);
                        right_opd_index = find_index_of_local_var(right_opd);
        } 

#if TEST_LOCAL_ANALYSIS
        printf ("\n\nGiven Statement is ");
        print_generic_stmt(stdout,stmt,0);
        printf ("\tRequired Exposition is ");

        switch (lps.exposition)
        {      
               case down_exp:
                        printf ("Downwards\n");
                       break;
               case any_where:
                        printf ("Anywhere\n");
                        break;
               case up_exp:
                        printf ("Upwards\n");
                       break;
        }
        printf ("\t Expression is ");
        print_generic_expr(stdout,expr,0);
        printf ("\t Its operands are :");
        print_generic_expr(stdout,left_opd,0);
        printf ("\t and :");
        print_generic_expr(stdout,right_opd,0);
#endif

        
        switch (lps.stmt_effect)
        {        
               case entity_use:
                        if (left_opd_index != -1)
                                SET_BIT(temp_Gen,left_opd_index);
                        if (right_opd_index != -1)
                                SET_BIT(temp_Gen,right_opd_index);
                        switch (lps.exposition)
                        {       
                                case down_exp:
                                case any_where:
                                        if (lval_index != -1)
                                                RESET_BIT(temp_Gen,lval_index);
                                break;
                                case up_exp:
                                        break;
                                default:
                                report_dfa_spec_error ("Wrong choice of occurrence in local property computation (Function vars_in_statement)");
                                        break;
                        }
                        break;
                case entity_mod:
                        if (lval_index != -1)
                                SET_BIT(temp_Gen,lval_index);
                        switch (lps.exposition)
                        {        
                                case down_exp:
                                case any_where:
                                        break;
                                case up_exp:
                                        if (left_opd_index != -1)
                                                RESET_BIT(temp_Gen,left_opd_index);
                                        if (right_opd_index != -1)
                                                RESET_BIT(temp_Gen,right_opd_index);
                                        break;
                                default:
                                        report_dfa_spec_error ("Wrong choice of occurrence in local property computation (Function vars_in_statement)");
                                        break;
                        }
                        break;
                default: 
                        report_dfa_spec_error ("Wrong entity manipulation in local property computation (Function vars_in_statement)");
                        break;
        }
#if TEST_LOCAL_ANALYSIS
        printf ("\t The identified entities are :");
        dump_entity_list(stdout,temp_Gen);
#endif
        ASSERT (temp_Gen)
        return temp_Gen;
}

       
/************ End of specification driven local property computation ***********/

/************ Functions to print the result of data flow analysis   ***********/
static void
dump_dfi(FILE * file, bool in_iterations)
{
        basic_block bb;
        block_stmt_iterator bsi;
        tree stmt = NULL; 

        FOR_EACH_BB(bb)
        {         
                dump_basic_block_info(file, bb);

                if(true == is_nonseparable) 
                {
                        FOR_EACH_STMT_FWD {
                                stmt = bsi_stmt(bsi);

                                if(-1 == find_index_of_local_stmt(stmt))  continue;                        
                                
                                /*DEBUG*/
                                //fprintf(file,"\n %d of %d --> ", find_index_of_local_stmt(stmt), local_stmt_count);
                                fprintf(file,"\n");
                                print_generic_stmt(file,stmt,0);
                                if (! in_iterations)
                                {

                                        if (GEN_OF_STMT(current_pfbv_dfi_of_stmt,stmt) == NULL) {
                                               fprintf (stderr, "\nStmt  %d: Null Gen value\n", find_index_of_local_stmt(stmt));
                                        } else {
                                               fprintf (file, "\n\t----------------------------");
                                               fprintf (file, "\n\tGEN Bit Vector: ");
                                               dump_dfvalue(file,GEN_OF_STMT(current_pfbv_dfi_of_stmt,stmt));
                                               fprintf (file, "\tGEN Entities:     ");
                                               dump_entity_list(file,GEN_OF_STMT(current_pfbv_dfi_of_stmt,stmt));
                                        }
                                        if (KILL_OF_STMT(current_pfbv_dfi_of_stmt,stmt) == NULL) {
                                               fprintf (stderr, "\nStmt  %d: Null Kill value\n", find_index_of_local_stmt(stmt));
                                        } else {
                                               fprintf (file, "\n\tKILL Bit Vector:");
                                               dump_dfvalue(file,KILL_OF_STMT(current_pfbv_dfi_of_stmt,stmt));
                                               fprintf (file, "\tKILL Entities:    ");
                                               dump_entity_list(file,KILL_OF_STMT(current_pfbv_dfi_of_stmt,stmt));
                                        }
                                }

                                if (IN_OF_STMT(current_pfbv_dfi_of_stmt,stmt) == NULL)
                                        fprintf (stderr, "\nStmt %d: Null In value\n", find_index_of_local_stmt(stmt));
                                else
                                {
                                        fprintf (file, "\n\tIN Bit Vector:  ");
                                        dump_dfvalue(file,IN_OF_STMT(current_pfbv_dfi_of_stmt,stmt));
                                        fprintf (file, "\tIN Entities:      ");
                                        dump_entity_list(file,IN_OF_STMT(current_pfbv_dfi_of_stmt,stmt));
                                }

                                if (OUT_OF_STMT(current_pfbv_dfi_of_stmt,stmt) == NULL)
                                        fprintf (stderr, "\nStmt %d: Null Out value\n", find_index_of_local_stmt(stmt));
                                else
                                {
                                        fprintf (file, "\n\tOUT Bit Vector: ");
                                        dump_dfvalue(file,OUT_OF_STMT(current_pfbv_dfi_of_stmt,stmt));
                                        fprintf (file, "\tOUT Entities:     ");
                                        dump_entity_list(file,OUT_OF_STMT(current_pfbv_dfi_of_stmt,stmt));
                                        fprintf (file, "\n\t------------------------------");
                                }
                        }
                } else { 
                        if (! in_iterations)
                        {
                                if (GEN(current_pfbv_dfi,bb) == NULL)
                                        fprintf (stderr, "\nBasic Block %d: Null Gen value\n", find_index_bb(bb));
                                else         
                                {       
                                        fprintf (file, "\n\t----------------------------");
                                        fprintf (file, "\n\tGEN Bit Vector: ");
                                        dump_dfvalue(file,GEN(current_pfbv_dfi,bb));
                                        fprintf (file, "\tGEN Entities:     ");
                                        dump_entity_list(file,GEN(current_pfbv_dfi,bb));
                                }
        
                                if (KILL(current_pfbv_dfi,bb) == NULL)
                                        fprintf (stderr, "\nBasic Block %d: Null Kill value\n", find_index_bb(bb));
                                else 
                                {       
                                        fprintf (file, "\n\t------------------------------");
                                        fprintf (file, "\n\tKILL Bit Vector:");
                                        dump_dfvalue(file,KILL(current_pfbv_dfi,bb));
                                        fprintf (file, "\tKILL Entities:    ");
                                        dump_entity_list(file,KILL(current_pfbv_dfi,bb));
                                }
                        }        

                        if (IN(current_pfbv_dfi,bb) == NULL)
                                fprintf (stderr, "\nBasic Block %d: Null In value\n", find_index_bb(bb));
                        else 
                        {       
                                fprintf (file, "\n\t------------------------------");
                                fprintf (file, "\n\tIN Bit Vector:  ");
                                dump_dfvalue(file,IN(current_pfbv_dfi,bb));
                                fprintf (file, "\tIN Entities:      ");
                                dump_entity_list(file,IN(current_pfbv_dfi,bb));
                        }

                        if (OUT(current_pfbv_dfi,bb) == NULL)
                                fprintf (stderr, "\nBasic Block %d: Null Out value\n", find_index_bb(bb));
                        else 
                        {               
                                fprintf (file, "\n\t------------------------------");
                                fprintf (file, "\n\tOUT Bit Vector: ");
                                dump_dfvalue(file,OUT(current_pfbv_dfi,bb));
                                fprintf (file, "\tOUT Entities:     ");
                                dump_entity_list(file,OUT(current_pfbv_dfi,bb));
                                fprintf (file, "\n\t------------------------------");
                        }
                }
        }
}


static void
dump_basic_block_info(FILE * file, basic_block bb)
{
        VEC(edge, gc) *edge_vec;
        edge e;
        edge_iterator ei;
        basic_block pred_bb, succ_bb;

        fprintf (file, "\nBasic Block %d. Preds: ",find_index_bb(bb));

        edge_vec = bb->preds;
        if (!edge_vec)
                fprintf (file, "None ");
                
        else
        {         
                FOR_EACH_EDGE(e,ei,edge_vec)
                {
                        pred_bb = e->src;
                        if (pred_bb->index == 0)
                                fprintf (file, " ENTRY");
                        else
                        fprintf (file, " %d",find_index_bb(pred_bb));
                }        
        }
        fprintf (file, ". Succs: ");
        edge_vec = bb->succs;
        if (!edge_vec)
                fprintf (file, "None ");
                
        else
        {         
                FOR_EACH_EDGE(e,ei,edge_vec)
                {
                        succ_bb = e->dest;
                        if (succ_bb->index == 1)
                                fprintf (file, " EXIT");
                        else
                                fprintf (file, " %d",find_index_bb(succ_bb));
                }        
        }
        
}
static void
dump_entity_list(FILE * file, dfvalue value)
{        
        tree expr = NULL;
        int i;
        bool at_least_one = false;
        
        for (i=0; i<relevant_pfbv_entity_count ; i++)
        {         
                if (TEST_BIT(value,i))
                {        
                        switch (relevant_pfbv_entity)
                        {       
                                case entity_expr:
                                        expr = local_expr[i]->expr;
                                        break;
                                case entity_var:
                                        expr = local_var_list[i];
                                        break;
				case entity_defn:
					expr = local_defn_list[i];
					break;
                                default:
                                        report_dfa_spec_error("Only expressions,variables,definitions are supported at the moment (Function dump_entity_list)");
                                        break;
                        }
                        if (expr)
                        {
                                if (at_least_one)
                                        fprintf (file, ",(");
                                else
                                        fprintf (file, "(");
                                print_generic_expr(file, expr,0);
                                fprintf (file, ")");
                                at_least_one = true;
                        }
                }
        }
}

static void
dump_entity_mapping(FILE * file)
{        
        tree expr=NULL;
        int i;

        for (i=0; i<relevant_pfbv_entity_count ; i++)
        {
                switch (relevant_pfbv_entity)
                {       
                        case entity_expr:
                                expr = local_expr[i]->expr;
                                break;
                        case entity_var:
                                expr = local_var_list[i];
                                break;
			case entity_defn:
				expr = local_defn_list[i];
				break;
                        default:
                                report_dfa_spec_error("Only expressions,variables and definitions are supported at the moment (Function dump_entity_mapping)");
                                break;
                }
                if (expr)
                {
                        if (i)
                                fprintf (file, ",%d:(",i);
                        else
                                fprintf (file, "%d:(",i);
                        print_generic_expr(file, expr,0);
                        fprintf (file, ")");
                }
        }
}

static void 
print_entity_info(void)
{
        if (flag_gdfa || flag_gdfa_details)
        {
                fprintf(dump_file, "\nNumber of relevant entities: %d\n\t",relevant_pfbv_entity_count);
                if (relevant_pfbv_entity_count != 0)
                {
                        fprintf(dump_file, "\n Bit position and entity mapping is  **************************************\n\t");
                        dump_entity_mapping(dump_file);
                        fprintf(dump_file, "\n ");
                }
        }

}

static void 
print_initial_dfi(void)
{
        if (flag_gdfa || flag_gdfa_details)
        {
                fprintf(dump_file, "\n Initial values ************************\n");
                dump_dfi(dump_file, false);
        }

}

static void 
print_final_dfi(int count)
{
        if (flag_gdfa || flag_gdfa_details)
        {
                fprintf(dump_file, "\n Total Number of Iterations = %d *******\n",count);
                fprintf(dump_file, "\n Final values **************************\n");
                dump_dfi(dump_file, false);
        }


}

static void 
print_per_iteration_dfi(int iteration)
{
        if (flag_gdfa_details)    
        {
               fprintf(dump_file, "\n Values after iteration %d *************\n",iteration);
               dump_dfi(dump_file, true);
        }


}


/******************* End of the generic dfa driver    *******************/


/*** Just in case this is needed some time :-) ***/

static void
verify_allocation_of_dfi(pfbv_dfi **dfi) 
{
        int iter;

        for (iter=0; iter < number_of_nodes; iter++)
        {         
                if (GEN_nid(dfi,iter) == NULL)
                        fprintf (stderr, "iteration count %d, iter : Null Gen value\n", iter);
                else if (KILL_nid(dfi,iter) == NULL)
                        fprintf (stderr, "iteration count %d, iter : Null Kill value\n", iter);
                else if (IN_nid(dfi,iter) == NULL)
                        fprintf (stderr, "iteration count %d, iter : Null In value\n", iter);
                else if (OUT_nid(dfi,iter) == NULL)
                        fprintf (stderr, "iteration count %d, iter : Null Out value\n", iter);
                else continue;
        }
}

static bool
is_valid_expr(tree lval, tree expr)
{
        bool valid;
	int index;
        tree op0,op1;


        fprintf (dump_file,"\n EXPR_TYPE: ");
        switch(TREE_CODE(expr))
        {
                case MULT_EXPR:
                        fprintf (dump_file,"\n\n MULT_EXPR: %d\n", TREE_CODE(expr));
                case PLUS_EXPR:
                        fprintf (dump_file,"\n\n PLUS_EXPR: %d\n", TREE_CODE(expr));
                case MINUS_EXPR:
                        fprintf (dump_file,"\n\n MINUS_EXPR: %d\n", TREE_CODE(expr));
                case LT_EXPR:
                        fprintf (dump_file,"\n\n LT_EXPR: %d\n", TREE_CODE(expr));
                case LE_EXPR:
                        fprintf (dump_file,"\n\n LE_EXPR: %d\n", TREE_CODE(expr));
                case GT_EXPR:
                        fprintf (dump_file,"\n\n GT_EXPR: %d\n", TREE_CODE(expr));
                case GE_EXPR:
                        fprintf (dump_file,"\n\n GE_EXPR: %d\n", TREE_CODE(expr));
                case NE_EXPR:
                        fprintf (dump_file,"\n\n NE_EXPR: %d\n", TREE_CODE(expr));
                case EQ_EXPR:
                        fprintf (dump_file,"\n\n EQ_EXPR: %d\n", TREE_CODE(expr));

                        fprintf (dump_file,"\n\nexpr type ok\n");
                        op0 = extract_operand(expr,0);
                        op1 = extract_operand(expr,1);
                        if((TREE_CODE(op0) == VAR_DECL || TREE_CODE(op0) == INTEGER_CST) && (TREE_CODE(op1) == VAR_DECL || TREE_CODE(op1) == INTEGER_CST)) {
                                valid = true;
                        } else {
                                fprintf (dump_file,"\n\nexpr type NOT ok 1\n");
                                valid = false;
                        }
                        break;
                case INTEGER_CST:
                        fprintf (dump_file,"\n\n INTEGER_CST: %d\nThe expre is : ", TREE_CODE(expr));
			print_generic_expr(dump_file,expr,0);
                        valid = false;
                        break;
                case REAL_CST:
                        fprintf (dump_file,"\n\n REAL_CST: %d\n", TREE_CODE(expr));
                        valid = false;
                        break;
                case FIXED_CST:
                        fprintf (dump_file,"\n\n FIXED_CST: %d\n", TREE_CODE(expr));
                        valid = false;
                        break;
                case VAR_DECL:
                        fprintf (dump_file," VAR_DECL: %d\n", TREE_CODE(expr));
                        op0 = extract_operand(expr,0);
                        fprintf (dump_file,"\n\n op0 tree code :: %d\n", TREE_CODE(op0));
                        if(TREE_CODE(op0) == IDENTIFIER_NODE ) {
				index = find_index_of_local_var(expr);
                                fprintf (dump_file," op1 index: %d\n", index);
				if(-1 != index) {
                                    print_generic_expr(dump_file,op0,0);
                                }
                        } else {
                               fprintf (dump_file,"Only expr is VAR+DECL\n");
                        }
                        if( TREE_CODE(expr) == VAR_DECL && ENTITY_INDEX(*lval) == ENTITY_INDEX(*expr)) {
                                fprintf(dump_file," CONDITION for a = b; true\n");
                        }
                        valid = true;
                        break;
                default:
                        fprintf (dump_file,"\n\n %d  expr type NOT ok 2\n", TREE_CODE(expr));
                        valid = false;
                        break;

        }
        return valid;
}

static void 
debug_statement_expr(void)
{
    block_stmt_iterator bsi;
    int iter, Lindex, Rindex;
    basic_block bb;
    tree stmt, expr=NULL, lval=NULL, left_opd=NULL, right_opd=NULL;
    for (iter=0; iter < number_of_nodes; iter++) {
        bb = VARRAY_BB(dfs_ordered_basic_blocks,iter);
        if (bb) {
            FOR_EACH_STMT_FWD {
                stmt = bsi_stmt(bsi);
                fprintf (dump_file,"\n\nGiven Statement is ");
                print_generic_stmt(dump_file,stmt,0);
                /*expr = extract_expr(stmt);*/
                fprintf (dump_file,"\n\nStatement Type  is ");
                switch(TREE_CODE(stmt)) {
                    case COND_EXPR:
                        expr = extract_operand(stmt,0);
                        fprintf (dump_file,"\tConditional\n");
                        break;
                    case MODIFY_EXPR:
                        lval = extract_operand(stmt,0);
                        expr = extract_operand(stmt,1);
                        fprintf (dump_file,"\tModify lval %d\n", find_index_of_local_var(lval));
                        break;
                    case GIMPLE_MODIFY_STMT:
                        lval = extract_operand(stmt,0);
                        expr = extract_operand(stmt,1);
                        fprintf (dump_file,"\tGimple Modify lval %d\n", find_index_of_local_var(lval));
                        break;
                    case RETURN_EXPR:
                        fprintf (dump_file,"\n\nexpr type : RETURN\n");
                        expr = NULL;
                        break;
                    default:
                        fprintf (dump_file,"\tIllegal 2\n");
                        expr = NULL;
                        break;
                }
                fprintf(dump_file, "index of expr : %d \n", find_index_of_local_expr(expr));
                if(NULL == expr || false == is_valid_expr(lval, expr))
                    expr = NULL;


                fprintf (dump_file, "\t Expression is ");
                if (expr) {
                    print_generic_expr(dump_file,expr,0);

                    left_opd = extract_operand(expr,0);
                    right_opd = extract_operand(expr,1);
                    Lindex = find_index_of_local_var(left_opd);
                    Rindex = find_index_of_local_var(right_opd);
                   
                    fprintf (dump_file,"\t Its operands are(%d,%d) :", Lindex, Rindex);

                    if(-1 != Lindex) {
                        print_generic_expr(dump_file,left_opd,0);
	            }
                    if(-1 != Rindex) {
                        fprintf (dump_file,"\t and :");
                        print_generic_expr(dump_file,right_opd,0);
                    }
                } else {
                    fprintf (dump_file,"\t NULL\n");
                }
            }
        }
    }
}

static void
print_dfi(FILE * file) 
{
        int iter;
        tree stmt = NULL;

        for (iter=0; iter < local_stmt_count; iter++)
        {
                
                stmt = local_stmt_list[iter];
                fprintf (file, "\nCurrent Stmt : ");
                print_generic_stmt(file,stmt,0);
                if (GEN_OF_STMT(current_pfbv_dfi_of_stmt,stmt) == NULL)
                        fprintf (stderr, "\nStmt  %d: Null Gen value\n", iter);
                else
                {
                        fprintf (file, "\n\t----------------------------");
                        fprintf (file, "\n\tGEN Bit Vector: ");
                        dump_dfvalue(file,GEN_OF_STMT(current_pfbv_dfi_of_stmt,stmt));
                        fprintf (file, "\tGEN Entities:     ");
                        dump_entity_list(file,GEN_OF_STMT(current_pfbv_dfi_of_stmt,stmt));
                }

                if (KILL_OF_STMT(current_pfbv_dfi_of_stmt,stmt) == NULL)
                        fprintf (stderr, "\nStmt  %d: Null Kill value\n", iter);
                else
                {
                        fprintf (file, "\n\tKILL Bit Vector:");
                        dump_dfvalue(file,KILL_OF_STMT(current_pfbv_dfi_of_stmt,stmt));
                        fprintf (file, "\tKILL Entities:    ");
                        dump_entity_list(file,KILL_OF_STMT(current_pfbv_dfi_of_stmt,stmt));
                }

                if (IN_OF_STMT(current_pfbv_dfi_of_stmt,stmt) == NULL)
                        fprintf (stderr, "\nStmt %d: Null In value\n", iter);
                else
                {     
                        fprintf (file, "\n\tIN Bit Vector:  ");
                        dump_dfvalue(file,IN_OF_STMT(current_pfbv_dfi_of_stmt,stmt));
                        fprintf (file, "\tIN Entities:      ");
                        dump_entity_list(file,IN_OF_STMT(current_pfbv_dfi_of_stmt,stmt));
                }

                if (OUT_OF_STMT(current_pfbv_dfi_of_stmt,stmt) == NULL)
                        fprintf (stderr, "\nStmt %d: Null Out value\n", iter);
                else
                {     
                        fprintf (file, "\n\tOUT Bit Vector: ");
                        dump_dfvalue(file,OUT_OF_STMT(current_pfbv_dfi_of_stmt,stmt));
                        fprintf (file, "\tOUT Entities:     ");
                        dump_entity_list(file,OUT_OF_STMT(current_pfbv_dfi_of_stmt,stmt));
                        fprintf (file, "\n\t------------------------------");
                }
        }
}
