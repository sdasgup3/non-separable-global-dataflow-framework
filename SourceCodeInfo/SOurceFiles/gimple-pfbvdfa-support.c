/*
     gdfa 1.0
     Copyright (C) 2008 GCC Resource Center, Department of Computer Science and Engineering,
     Indian Institute of Technology Bombay.
     License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
     This is free software: you are free to change and redistribute it.
     There is NO WARRANTY, to the extent permitted by law.
*/

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "vec.h"
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
#include "cgraph.h" 
#include "assert.h" 
#include "gimple-pfbvdfa.h"

/* This file contains the initialization required for performing
   per function (i.e. intraprocedural) data flow analysis on gimple IR
   which has been defined in the file gimple-pfbvdfa-driver.c.
   
   This file contains the following modules:
        - functions for assigning unique indices to local expressions and 
          variables so that these can be used for bit positions in bit vector 
          representation of data flow properties.
        - functions for performing dfs numbering of the control flow graph
        - functions for accessing gimple IR

  For more details about data flow analysis, please see the documentation
  in the file gimple-pfbvdfa-driver.c.

*/

typedef enum eopdT 
                { 
                     op_in_var_decl=1,
                     op_is_int_constant
                } expr_op_type;

typedef enum varT  
                {  
                      locally_scoped_var=1,
                      globally_scoped_var,
                      artificial_var,
                      temporary_var,
                      uninteresting_var_type
                } var_scope;

typedef enum defnT  
                {  
                      locally_scoped_defn=1,
                      globally_scoped_defn,
                      artificial_defn,
                      temporary_defn,
                      uninteresting_defn_type
                } defn_scope;


#define LOCALISED_VERSION(var)  DECL_IGNORED_P ((var)) == 1 && \ 
				DECL_ARTIFICIAL((var)) == 1 && \
                              	DECL_SEEN_IN_BIND_EXPR_P((var)) == 1  && \
                                DECL_NAME((var))

#define GROW_STEP 10

#define ENTITY_INDEX(node) (node).common.base.index

/*initial allocation of memory while storing expressions */
int e_old_size_local=1000,e_new_size_local=100;

/*initial allocation of memory while storing variables */
int v_old_size_local=1000,v_new_size_local=100;

/*initial  allocation of memory while storing definitions */
int d_old_size_local=1000,d_new_size_local=100;	

/*@Non-separable : START*/
/*initial  allocation of memory while storing statements */
int s_old_size_local=1000,s_new_size_local=100;	
/*@Non-separable : END*/

/* Count of local entities */
int local_var_count=0;
int local_expr_count=0;
int local_defn_count=0;
int number_of_nodes=0;
/*@Non-separable : START*/
int local_stmt_count=0;
/*@Non-separable : END*/

expr_template **local_expr=NULL;
expr_index_list **exprs_of_vars=NULL;

 /* local_var_list, we are storing temporarily for a function under 
 * compilation, used to assign indices to local copies of local variable*/

tree * local_var_list=NULL;
tree * local_defn_list=NULL;
/*@Non-separable : START*/
tree * local_stmt_list = NULL;
/*@Non-separable : END*/
defn_index_list **defns_of_vars=NULL;

varray_type dfs_ordered_basic_blocks = NULL; 
basic_block * stack_bb=NULL;

/**  Functions to assign indices to local expressions, variables, and definitions **/

static void assign_indices_to_var(void);
static void assign_indices_to_exprs(void);
static void assign_indices_to_local_expr(tree expr);
static void assign_indices_to_defn();
/*@Non-separable : START*/
static void assign_indices_to_stmt(void);
/*@Non-separable : END*/

static void create_add_node_expr_index_list( int expr_index, expr_index_list **list,int index);
static expr_index_list* create_node_expr_index_list(int expr_index);
static void add_to_list_expr(expr_index_list **list_head,int var,expr_index_list * temp_node);

static void create_add_node_defn_index_list(int defn_index, defn_index_list **list,int index);
static defn_index_list* create_node_defn_index_list(int defn_index);
static void add_to_list_defn(defn_index_list **list_head,int var,defn_index_list * temp_node);

static signed int index_of_operand(tree operand);
static bool is_expr_in_template(expr_template **template, int iter, tree expr, tree op0, tree op1);
static void validate_expr_index_list(void);


/**  Functions to perform depth first numbering of gimple cfg  **/

static varray_type add_to_varray_bb(varray_type to_varray, basic_block bb, int index);
static void dfs_numbering_of_bb(void);
static void dfs_numbering_of_bb_inner(basic_block bb);
static void init_stack(int number_of_elements);
static bool is_empty_stack_bb(void);
static bool is_full_stack_bb(void);
static void push_bb(basic_block bb);
static basic_block pop_bb(void);

/**  Accessor functions for entities in gimple IR **/
static var_scope type_of_var(tree var);
static bool is_local_expr(tree expr);
static bool is_valid_expr(tree expr);
static char * extract_string(tree var);
static defn_scope type_of_defn(tree defn);
static bool is_valid_defn(tree stmt);
static bool is_global_var_grc (tree var);
/*@Non-separable : START*/
static bool is_valid_stmt(tree stmt);
int find_index_of_local_stmt(tree stmt);
/*@Non-separable : END*/

static unsigned int
init_gimple_pfbvdfa_execute (void)
{
        local_var_count=0;
        local_expr_count=0;
	local_defn_count=0;
        number_of_nodes = n_basic_blocks;

        assign_indices_to_var();
        assign_indices_to_exprs();
	assign_indices_to_defn();
        assign_indices_to_stmt();

        dfs_ordered_basic_blocks = NULL; 
        dfs_numbering_of_bb();
        
        return 0;
}

struct tree_opt_pass pass_init_gimple_pfbvdfa =
{
  "init_gimple_pfbvdfa",                   /* name */
  NULL,                                    /* gate */
  init_gimple_pfbvdfa_execute,             /* execute */
  NULL,                                    /* sub */
  NULL,                                    /* next */
  0,                                       /* static_pass_number */
  0,                                       /* tv_id */
  0,                                       /* properties_required */
  0,                                       /* properties_provided */
  0,                                       /* properties_destroyed */
  0,                                       /* todo_flags_start */
  0,                                       /* todo_flags_finish */
  0                                        /* letter */
};


/**  Functions to assign indices to local expressions, variables, and definitions **/


static void 
assign_indices_to_var(void)
{
        tree vars,list;
        char * var_name,*position_ptr=NULL,*var_temp=NULL;
        size_t position=0;

        /*data structure to store all expression templates */
        local_var_list = (tree *)ggc_alloc_cleared(sizeof(tree )*v_old_size_local);

        list = cfun->unexpanded_var_list;
        while (list) 
        {
                if(local_var_count == v_old_size_local)
                {
                        v_old_size_local += v_new_size_local;
                        local_var_list =(tree *) ggc_realloc(local_var_list,sizeof(tree )*v_old_size_local);
                }
                vars = TREE_VALUE (list);

                switch(type_of_var(vars))
                {
                        case locally_scoped_var: /*pure local variable*/
                                        ENTITY_INDEX(vars->decl_minimal) = local_var_count;
                                        local_var_list[local_var_count++] = vars;
                                break;

                        /* Since temporary variables are not reused,
                           there is exactly one use of a temporary
                           variable so the expression using this
                           variable cannot undergo a traditional
                           optimization like common subexpressions
                           elimination. Hence we do not assign indices
                           to temporaries. An artificial variable is a
                           clone of a global variable, or of a local
                           variable that gets its value from outside.
                           If it is a clone of a local variable, its
                           value is copied to the local variable and
                           the expressions using the value use local
                           variables. Thus an artificial variable
                           appears in an expression only when it
                           represents a global variable and we are
                           keeping global variables out of the scope of
                           our data flow analyzers.
                         */
                          
                        case temporary_var: 
                        case artificial_var:
                        case globally_scoped_var:
			case uninteresting_var_type:
                                ENTITY_INDEX(vars->decl_minimal) = -1;
                                break;
                        default:
                                report_dfa_spec_error ("Which type of variable is this? (Function assign_indices_to_var)");
                                       break; 

                }
                list = TREE_CHAIN(list);
        }
}


static void
create_add_node_expr_index_list( int expr_index, expr_index_list **list,int index)
{
        expr_index_list *temp=NULL;
        temp=(expr_index_list*)ggc_alloc_cleared(sizeof(expr_index_list));
        temp->expr_no = expr_index;
        temp->next = NULL;

        if(list[index]==NULL)
                list[index] = temp;
        else
        {
                temp->next = list[index];
                list[index]=temp;
        }

}        

static void
create_add_node_defn_index_list( int defn_index, defn_index_list **list,int index)
{
        defn_index_list *temp=NULL;
        temp=(defn_index_list*)ggc_alloc_cleared(sizeof(defn_index_list));
        temp->defn_no = defn_index;
        temp->next = NULL;

        if(list[index]==NULL)
                list[index] = temp;
        else
        {
                temp->next = list[index];
                list[index]=temp;
        }

}        

static signed int 
index_of_operand(tree operand)
{
        switch(TREE_CODE(operand))
        {
                case VAR_DECL:
                        return ENTITY_INDEX(*(operand));
                case INTEGER_CST:
                        return operand->int_cst.int_cst.low;
                default:
                        return -1;
        }
}

static bool 
is_expr_in_template(expr_template **template, int iter, tree expr, tree op0, tree op1)
{
        bool flag=false;

        int op0_indx = index_of_operand(op0);
        int op1_indx = index_of_operand(op1);

        if(template[iter])
        {
        if((TREE_CODE(template[iter]->expr)== TREE_CODE(expr)) && (template[iter]->op0_index == op0_indx) &&(template[iter]->op1_index == op1_indx))
                flag = true;
        }

        return flag;
}


static void 
assign_indices_to_local_expr(tree expr)
{

        tree op0,op1;


        int iter_local_expr=0;
        signed int op0_index=-1;
        signed int op1_index=-1; 

        op0 = extract_operand(expr,0);
        op1 = extract_operand(expr,1);

        /* First find out if we have already assigned an index to this expr */
        for(iter_local_expr=0;iter_local_expr<local_expr_count;iter_local_expr++)
        {

                bool cmp_expr =  is_expr_in_template( local_expr,iter_local_expr,expr,op0,op1);
                        
                /*if expr is in array local_expr, do nothing*/ 
                if(cmp_expr)
                {
                        ENTITY_INDEX(*expr) = iter_local_expr;
                        return;
                }
         }

        /* This is a new expression that needs to be assigned an index */
        /* We need to store it in the array*/
              
        /* First check if there is a need to dynamically increase the 
           allocated size of the array that remembers expressions. */
        if(local_expr_count == e_old_size_local)
        {
                e_old_size_local += e_new_size_local;
                local_expr =(expr_template**) ggc_realloc(local_expr,sizeof(expr_template*)*e_old_size_local);
        }
                        
        if(iter_local_expr == local_expr_count)
        {
                local_expr[local_expr_count] = NULL;
                local_expr[local_expr_count] =(expr_template*)ggc_alloc_cleared(sizeof(expr_template));
                local_expr[local_expr_count]->expr = expr;
                                                        
                switch(TREE_CODE(op0))
                {
                        case VAR_DECL:
                                local_expr[local_expr_count]->op0_index = op0_index = ENTITY_INDEX(op0->decl_minimal);        
                                break;

                        case INTEGER_CST:
                                op0_index = -1;
                                /* store the value of int constant at op1_index field */        
                                local_expr[local_expr_count]->op0_index = TREE_INT_CST_LOW(op0);
                                break;

                        default:
                                break;
                }

                switch(TREE_CODE(op1))
                {
                        case VAR_DECL:
                                local_expr[local_expr_count]->op1_index = op1_index = ENTITY_INDEX(op1->decl_minimal);
                                break;
                        case INTEGER_CST:
                                op1_index = -1;
                                /* store the value of int constant at op1_index field */        
                                local_expr[local_expr_count]->op1_index = TREE_INT_CST_LOW(op1);
                                break;

                        default:
                                break;

                }
                int expr_index=-1;

                expr_index = ENTITY_INDEX(*expr) = local_expr_count++;                        

                /*add expr to expr_index_list of the variables used in expr*/
                if(op0_index!= -1)        
                 create_add_node_expr_index_list(expr_index,exprs_of_vars,op0_index);
                if(op1_index!= -1)
                 create_add_node_expr_index_list(expr_index,exprs_of_vars,op1_index);
        }
                                                
}

static void 
assign_indices_to_exprs(void)
{
        basic_block bb;
        block_stmt_iterator bsi;
        int type,iter,tempo;

        tree stmt;
        tree expr,op0,op1;
                

        /*data structure to store all expression templates */
        local_expr = (expr_template**)ggc_alloc_cleared(sizeof(expr_template*)*e_old_size_local);


        /*data structure to accumulate expressions of a given variable */

        exprs_of_vars  =(expr_index_list**)ggc_alloc_cleared(sizeof(*exprs_of_vars)*local_var_count);
     
        
        FOR_EACH_BB_FWD(ENTRY_BLOCK_PTR)
        {
                int logical_stmt_no =0;
                FOR_EACH_STMT_FWD
                {
                        stmt = bsi_stmt(bsi);
                        logical_stmt_no++;

                        expr = extract_expr(stmt);
                        if(expr)
                        {
                                if(is_local_expr(expr))
                                        assign_indices_to_local_expr(expr);
                                else         /* assign index as -1 */
                                        ENTITY_INDEX(*expr) = -1;
                        }
                }/* for loop of BSI(stmt)*/
        }/*For loop for bb*/
        
}

/*@Non-separable : START*/
static void 
assign_indices_to_stmt(void)
{
        basic_block bb;
        int index;
        block_stmt_iterator bsi;
        tree stmt = NULL, lval = NULL;

        /*data structure to store all statements TO DO : free local_stmt_list*/ 
        local_stmt_list = (tree *)ggc_alloc_cleared(sizeof(tree )*s_old_size_local);

        FOR_EACH_BB_FWD(ENTRY_BLOCK_PTR) 
        {
                FOR_EACH_STMT_FWD 
                {
                        stmt = bsi_stmt(bsi);
                        if(is_valid_stmt(stmt)) 
                        {
                                if(local_stmt_count == s_old_size_local) {
                                        s_old_size_local += s_new_size_local;
                                        local_stmt_list =(tree *) ggc_realloc(local_stmt_list,sizeof(tree )*s_old_size_local);
                                }

                                lval = extract_operand(stmt,0);
                                index = ENTITY_INDEX(*lval);

                                if(-1 != index) 
                                {
                                        ENTITY_INDEX(*stmt) = local_stmt_count;
                                        local_stmt_list[local_stmt_count] = stmt;
                                        local_stmt_count++;
                                } else {
                                        ENTITY_INDEX(*stmt) = -1;
                                }
                        }
                }
        }

        /*if(local_defn_count != 0 && NULL != local_defn_list ) 
        {
                local_stmt_list = (tree *)ggc_alloc_cleared(sizeof(tree )*(local_defn_count+local_stmt_count));
                for(index = 0; index < local_defn_count; index++) 
                {
                        local_stmt_list[index] = local_defn_list[index]; 
                }
                for(index = 0; index < local_stmt_count; index++) 
                {
                        local_stmt_list[local_defn_count + index] = temp_local_stmt_list[index]; 
                }
                local_stmt_count += local_defn_count;
        } else {
                local_stmt_list = temp_local_stmt_list;
        }*/
}
/*@Non-separable : END*/

static void 
assign_indices_to_defn(void)
{
	basic_block bb;
        block_stmt_iterator bsi;
	int index;
        defn_scope type;
        tree stmt=NULL,lval=NULL;

	/*data structure to store all definitions*/ 
        local_defn_list = (tree *)ggc_alloc_cleared(sizeof(tree )*d_old_size_local);

        /*data structure to accumulate definitions of a given variable */
        defns_of_vars  =(defn_index_list**)ggc_alloc_cleared(sizeof(*defns_of_vars)*local_var_count);
 
        FOR_EACH_BB_FWD(ENTRY_BLOCK_PTR)
        {
                int logical_stmt_no =0;
                FOR_EACH_STMT_FWD
                {
                        stmt = bsi_stmt(bsi);
                        logical_stmt_no++;

                        if(is_valid_defn(stmt))
			{	
				if(local_defn_count == d_old_size_local)
		                {
                	   		d_old_size_local += d_new_size_local;
                        		local_defn_list =(tree *) ggc_realloc(local_defn_list,sizeof(tree )*d_old_size_local);
                		}

				type = type_of_defn(stmt);
				switch (type)
				{
					case locally_scoped_defn:
						ENTITY_INDEX(*stmt) = local_defn_count;
						local_defn_list[local_defn_count] = stmt;
						lval = extract_operand(stmt,0);
						index = ENTITY_INDEX(*lval);
						create_add_node_defn_index_list(local_defn_count,defns_of_vars,index);
						local_defn_count++;

						break;

					case globally_scoped_defn:
			                case artificial_defn:
                      			case temporary_defn:
					case uninteresting_defn_type:
						ENTITY_INDEX(*stmt) = -1;
						break;

					default:
				                report_dfa_spec_error ("Which type of definition is this? (Function assign_indices_to_defn)");
						break;
				}	
			}
			
		}/* for loop of BSI(stmt)*/
        }/*For loop for bb*/
    
}

static void
validate_expr_index_list(void)
{
        expr_index_list * temp = NULL;
        int iter;
        for(iter=0;iter<local_var_count;iter++)
        {
                for(temp = exprs_of_vars[iter] ;temp;temp=temp->next)
                {
                        printf(" %d ",temp->expr_no);
                }
                
        }
}

/**  End of functions to assign indices to local expressions, variables, and definitions **/

/**  Functions to perform depth first numbering of gimple cfg  **/

/* Traverse CFG in dfs manner and assign DFS numbers 
 * and store in an array dfs_num a block pointers
 * so dfs_num[i] = node says that dfs number of node is i;
 *
 * for a forward traversal 
 * for (i=0;i<maxdfs;i++)
 *  node = dfs_num[i]
 *
 *  and backward
 *
 * for (i=maxdfs-1 ;i>= 0;i--)
 *  node = dfs_num[i]
 * we will store basic_blocks in dfs_numbered manner in varray dfs_ordered_basic_blocks
*/

static int dfs_num=0;
static int *visit_dfs=NULL;

static varray_type 
add_to_varray_bb(varray_type to_varray, basic_block bb, int index)
{
        if(to_varray->elements_used == to_varray->num_elements)
                        {
                               int new_size = to_varray->num_elements + GROW_STEP;
                                VARRAY_GROW(to_varray,new_size);
                                to_varray->num_elements=new_size;
                        }

                        VARRAY_BB(to_varray,index) = bb;
                        to_varray->elements_used++;
                        return to_varray;
}


static void
dfs_numbering_of_bb(void)
{
                VARRAY_BB_INIT (dfs_ordered_basic_blocks, number_of_nodes, "dfs_ordered_bb");
                visit_dfs = (int*)ggc_alloc_cleared(sizeof(int)*number_of_nodes);
                init_stack(number_of_nodes);
                dfs_num = number_of_nodes;
                basic_block bb = ENTRY_BLOCK_PTR;

#if NON_RECURSIVE_DFS
                push_bb(bb); visit_dfs[bb->index]=1; 
#endif
                dfs_numbering_of_bb_inner(bb);
                visit_dfs = NULL;
                stack_bb = NULL;
}

#if NON_RECURSIVE_DFS
static void 
dfs_numbering_of_bb_inner(basic_block bb) /*non recursive one*/
{

        edge_iterator ei ;
        edge e ;
        basic_block succ_bb=NULL;
        int bb_index; 
        
        push_bb(bb);
        bb_index = find_index_bb(bb);
        visit_dfs[bb_index]=1; 

        do{
                ei = ei_start(bb->succs);
                for(;e=ei_safe_edge(ei);ei_next(&ei))
                {
                        succ_bb = e->dest;
                        bb_index = find_index_bb(succ_bb);
                        if(!visit_dfs[bb_index] && !(succ_bb->dfs_number >= 0 && succ_bb->dfs_number<number_of_nodes) )
                        {
                                push_bb(succ_bb);
                                visit_dfs[bb_index]=1;
                                bb = succ_bb;
                                ei = ei_start(bb->succs);
                        }
                }
        
                bb = pop_bb();
                VARRAY_BB(dfs_ordered_basic_blocks,--dfs_num) = bb;
                bb->dfs_number = dfs_num;
                bb_index = find_index_bb(bb);
                visit_dfs[bb_index] = 0;
                
        }while(!is_empty_stack_bb());
}

#else 

static void 
dfs_numbering_of_bb_inner(basic_block bb) /* Recursive version */
{

        edge_iterator ei ;
        edge e ;
        basic_block succ_bb=NULL;

        visit_dfs[bb->index] = 1;


        FOR_EACH_EDGE(e,ei,bb->succs)
        {
                succ_bb = e->dest;
                if(!visit_dfs[succ_bb->index])
                dfs_numbering_of_bb_inner(succ_bb);
        }

        VARRAY_BB(dfs_ordered_basic_blocks,--dfs_num) = bb;
        bb->dfs_number = dfs_num;

}
#endif

static signed int bb_stack_top=-1;

static void
init_stack(int number_of_elements)
{
        stack_bb = NULL;
        stack_bb = (basic_block*)ggc_alloc_cleared(sizeof(basic_block)*number_of_elements);
        bb_stack_top = -1;
}
static bool
is_empty_stack_bb(void)
{
        if(bb_stack_top == -1)
        return true;
        else
        return false;
}
static bool
is_full_stack_bb(void)
{
        if(bb_stack_top == number_of_nodes-1)
        return true;
        else
        return false;
}
static void
push_bb(basic_block bb)
{
        if(!is_full_stack_bb())
        {
                stack_bb[++bb_stack_top]=bb;
        }
}

static basic_block
pop_bb(void)
{
        basic_block bb;
        if(!is_empty_stack_bb())
        {
                bb = stack_bb[bb_stack_top];
                bb_stack_top--;        
        }
        else 
                bb = NULL;

        return bb;
}

int
find_index_bb(basic_block bb)
{
                int nid = -1;
                if (bb->index >=0 && bb->index < n_basic_blocks)
                         nid = bb->index;
                else 
	                report_dfa_spec_error ("Wrong index of basic block (Function find_index_bb)");
                return nid;
}


/**  End of functions to perform depth first numbering of gimple cfg  **/

/**  Accessor functions for entities in gimple IR **/

static var_scope
type_of_var(tree var)
{
      if(TREE_CODE(var) == VAR_DECL) 
      {	
        if(var->decl_minimal.name == NULL)
                return temporary_var;
        else if (LOCALISED_VERSION(var)) 
        {
		/*variable is local copy of some global variable
		or local copy of local variable. This includes
		versions such as c.0 for local var c also.
		*/
		return artificial_var;
	}        
	else if (is_global_var_grc(var))  
		return globally_scoped_var;
	else 	/* if not local version of global var and not temporary var then pure local variable*/
		return locally_scoped_var;
      }
      else
	return uninteresting_var_type;

}

/* This function has given "grc" suffix, because is_global_var function is already provided by
 * GCC, but it return true on static_flag, which could be true for static local variables. */
 
static bool
is_global_var_grc (tree var)
{
	bool return_value = false;
	return_value = ( TREE_STATIC(var)==1 && 
			 TREE_PUBLIC(var)==1 && 
			 DECL_SEEN_IN_BIND_EXPR_P(var)==0);
			/* Even check on context field which is NULL for global 
			variable can be added */
	return return_value;
}

static bool
is_local_expr(tree expr)
{
        tree op0,op1;
        op0 = extract_operand(expr,0);
        op1 = extract_operand(expr,1);


        if(TREE_CODE(op0) == INTEGER_CST)
        {
        
                if (ENTITY_INDEX(*op1) != -1 )
                        return true;
                else 
                        return false;
        }

        if(TREE_CODE(op1) == INTEGER_CST)
        {
                if (ENTITY_INDEX(*op0) != -1 )
                        return true;
                else 
                        return false;
        }
        
        if(TREE_CODE(op0) == VAR_DECL && TREE_CODE(op1) == VAR_DECL)
        {
                if ((ENTITY_INDEX(*op0) != -1 ) && (ENTITY_INDEX(*op1) != -1 ))
                        return true;
                else 
                        return false;
        }
        else
               return false;
}


int
find_index_of_local_var(tree var)
{        
        int index=-1;        

        if (var)
        {        
                if((TREE_CODE(var) == VAR_DECL) && (ENTITY_INDEX(*var) != -1 ))
                        index = ENTITY_INDEX(*var);
        }
        return index;
}

int
find_index_of_local_expr(tree expr)
{       
        int index=-1;        

        if (expr)
        {        
                if (is_local_expr(expr))
                        index = ENTITY_INDEX(*expr);
        }
        return index;
}

int 
find_index_of_local_defn(tree defn)
{
	int index=-1;
	if(defn && is_valid_defn(defn))
	{
		if(type_of_defn(defn) == locally_scoped_defn)
		index = ENTITY_INDEX(*defn);
	}
	
	return index;
}

/*@Non-separable : START*/
int
find_index_of_local_stmt(tree stmt)
{
        int index=-1;
        if(stmt && is_valid_stmt(stmt))
        {
                index = ENTITY_INDEX(*stmt);
        }

        return index;
}
/*@Non-separable : END*/

static bool
is_valid_expr(tree expr)
{
        bool valid;

        tree op0,op1;


        switch(TREE_CODE(expr))
        { 
                case MULT_EXPR:
                case PLUS_EXPR:
                case MINUS_EXPR:
                case LT_EXPR:
                case LE_EXPR:
                case GT_EXPR:
                case GE_EXPR:
                case NE_EXPR:
                case EQ_EXPR:
                        op0 = extract_operand(expr,0);
                        op1 = extract_operand(expr,1);
                        if((TREE_CODE(op0) == VAR_DECL || TREE_CODE(op0) == INTEGER_CST) && (TREE_CODE(op1) == VAR_DECL || TREE_CODE(op1) == INTEGER_CST))
                                valid = true;
                        else 
                                valid = false;
                        break;
                default:
                        valid = false;
                        break;
                                
        }
        return valid;
}

/**Arguments :variable pointer
Checks to see that the var passed is not a temp var then,
  Returns : the string ie. name of the variable **/
static char *
extract_string(tree var)
{
        char *var_name=NULL;
        if(var->decl_minimal.name)
        {
                var_name = var->decl_minimal.name->identifier.id.str;
        }
        else
                var_name = NULL;
        return var_name;
}

static bool
is_valid_defn(tree stmt)
{
	switch(TREE_CODE(stmt))
	{	
		case MODIFY_EXPR:
		case GIMPLE_MODIFY_STMT:
			return true;
			break;
		default:
			return false;
			break;
	}

}

static bool
is_valid_stmt(tree stmt)
{
    switch(TREE_CODE(stmt)) {
        case COND_EXPR:
        case GIMPLE_MODIFY_STMT:
        case MODIFY_EXPR:
            return true;
        default:
            return false;
    }
}


/* 
There are four kinds of possible definitions
[locally_scoped_defn]	al   = .... ; where "al" is some local variable
[globally_scoped_defn]  ag   = .... ; where "ag" is some global variable
[artificial_defn]   	ag.0 = .... ; where ag.0 is local copy of some global variable
[temorary_defn] 	t123 = .... ; where t123 is a temprary variable created for intermediate computations is complex exression (Gimple IR specific)i
*/
 
static defn_scope
type_of_defn(tree defn)
{
	tree lval=NULL;
	var_scope type;
	/* Fix me: Can be used extract_lval, but currently it's returning only for locals*/
	lval = extract_operand(defn,0);
	if ( lval && TREE_CODE(lval) == VAR_DECL)
	{
		type = type_of_var(lval);
		switch(type)
		{
			case locally_scoped_var:
				return locally_scoped_defn;
				break;

			case globally_scoped_var:
				return globally_scoped_defn;
				break;
			case artificial_var:
				return artificial_defn;
				break;
			case temporary_var: 
				return temporary_defn;
				break;
                        default:
				return uninteresting_defn_type;
				break;
		}
	}

	return uninteresting_defn_type;
}

/**  End of accessor functions for entities in gimple IR **/

/** End of local functions **/


tree 
extract_expr(tree stmt)
{
        tree expr=NULL, lval=NULL;
        switch(TREE_CODE(stmt))
        {
                case COND_EXPR:
                                expr = extract_operand(stmt,0);
                                break;
                case MODIFY_EXPR:
                case GIMPLE_MODIFY_STMT:
                                lval = extract_operand(stmt,0);
                                expr = extract_operand(stmt,1);
                                
                                /*Skip modify expressions of type a.0 = a*/
                                if( TREE_CODE(expr) == VAR_DECL && ENTITY_INDEX(*lval) == ENTITY_INDEX(*expr) 
                                   )
                                        expr = NULL;
                                break;
                default:
                                expr = NULL;
        }
        if(expr && is_valid_expr(expr))
                return expr;
        else 
                return NULL;
        
}

tree 
extract_lval(tree stmt)
{
        tree expr=NULL, lval=NULL;
        switch(TREE_CODE(stmt))
        {
                case MODIFY_EXPR:
                case GIMPLE_MODIFY_STMT:
                        lval =  extract_operand (stmt, 0);
                        if(ENTITY_INDEX(*lval) == -1) 
                                lval = NULL;
                        break;
                default:
                        lval = NULL;
                        break;
        }
        return lval;
}

tree 
extract_operand(tree expr,int op_num)
{        
        tree opd = NULL;
        
        assert ((op_num == 0) || (op_num ==1));

        if (TREE_CODE(expr) == GIMPLE_MODIFY_STMT)
                opd =  GIMPLE_STMT_OPERAND (expr, op_num);
        else
                opd = TREE_OPERAND(expr,op_num);


        return opd;
}



void
report_dfa_spec_error(const char * mesg)
{
        fprintf(stderr,"DFA initialization error: %s\n",mesg);
        exit(1);
}

/****************** dfvalue interface  ********************/
/* defined in terms of bitmap support available in gcc */
/* please see the sbitmap.h and sbitmap.c files        */

bool
is_dfvalue_equal(dfvalue value1, dfvalue value2)
{
	return (sbitmap_equal(value1,value2));
}

void
free_dfvalue_space(dfvalue value)
{
    sbitmap_free(value);
}

dfvalue 
intersect_dfvalues (dfvalue value1, dfvalue value2)
{
	sbitmap temp;

        temp  = make_uninitialised_dfvalue();

        sbitmap_a_and_b(temp, value1, value2);
	
	return temp;
}

dfvalue 
a_plus_b_minus_c(dfvalue v_a, dfvalue v_b, dfvalue v_c)
{
	sbitmap temp;

        temp  = make_uninitialised_dfvalue();

        sbitmap_union_of_diff(temp, v_a, v_b, v_c);
	
	return temp;
}

dfvalue 
union_dfvalues (dfvalue value1, dfvalue value2)
{
	sbitmap temp;

        temp  = make_uninitialised_dfvalue();

        sbitmap_a_or_b(temp, value1, value2);
	
	return temp;
}

dfvalue
make_initialised_dfvalue(initial_value value)
{
        sbitmap temp;

        temp = make_uninitialised_dfvalue();

        switch (value)
        {        
                case ONES:
                        sbitmap_ones(temp);
                        break;
                case ZEROS:
                        sbitmap_zero(temp);
                        break;
                default:
                        report_dfa_spec_error ("Wrong initial value (Function make_initialised_dfvalue)");
                        break;
        }
        return temp;
}

extern int relevant_pfbv_entity_count;

dfvalue
make_uninitialised_dfvalue(void)
{        
        sbitmap temp;
        
        temp = sbitmap_alloc(relevant_pfbv_entity_count);

        return temp;
}

void
dump_dfvalue (FILE * file, dfvalue value)
{
	dump_sbitmap(file, value);
}

