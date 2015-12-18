/*
     gdfa 1.0
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

/***    Instantiations of Generic Bit Vector Data Flow Analyzer for 
        Gimple IR for
        - available expressions analysis
        - partially available expressions analysis
        - anticipable expressions analysis
        - live variables analysis
        - partial redundancy elimination.
        - faint variable analysis
        - possibly uninitialised variable analysis


        For more details, please refer to the documentation in
        gimple-pfbvdfa-driver.c.

***/


/************* Specification of available expressions analysis *****************/

pfbv_dfi ** AV_pfbv_dfi = NULL;

static unsigned int gimple_pfbv_ave_dfa(void);

struct gimple_pfbv_dfa_spec gdfa_ave = 
{
        entity_expr,                   /* entity;                 */
        ONES,                          /* top_value;              */
        ZEROS,                         /* entry_info;             */
        ONES,                          /* exit_info;              */
        FORWARD,                       /* traversal_order;        */
        INTERSECTION,                  /* confluence;             */
        entity_use,                    /* gen_effect;             */
        down_exp,                      /* gen_exposition;         */
        entity_mod,                    /* kill_effect;            */
        any_where,                     /* kill_exposition;        */
        global_only,                   /* preserved_dfi;          */
        identity_forward_edge_flow,    /* forward_edge_flow       */
        stop_flow_along_edge,          /* backward_edge_flow      */
        forward_gen_kill_node_flow,    /* forward_node_flow       */
        stop_flow_along_node,          /* backward_node_flow      */
        /*@Non-separable : START*/
        IGNORE_STATEMENT_TYPE,         /* constgen_statement_type */
        IGNORE_PRECONDITION,           /* constgen_precondition   */
        IGNORE_STATEMENT_TYPE,         /* constkill_statement_type*/
        IGNORE_PRECONDITION,           /* constkill_precondition  */
        IGNORE_ENTITY_DEPENDENCE,      /* dependent_gen           */
        IGNORE_ENTITY_DEPENDENCE       /* dependent_kill          */
        /*@Non-separable : END*/
};

static unsigned int
gimple_pfbv_ave_dfa(void)
{

        AV_pfbv_dfi = gdfa_driver(gdfa_ave);

        return 0;
}


/* This is the specification of ave pass in GCC */

struct tree_opt_pass pass_gimple_pfbv_ave_dfa =
{
  "gdfa_ave",                         /* name */
  NULL,                               /* gate */
  gimple_pfbv_ave_dfa,                /* execute */
  NULL,                               /* sub */
  NULL,                               /* next */
  0,                                  /* static_pass_number */
  0,                                  /* tv_id */
  0,                                  /* properties_required */
  0,                                  /* properties_provided */
  0,                                  /* properties_destroyed */
  0,                                  /* todo_flags_start */
  0,                                  /* todo_flags_finish */
  0                                   /* letter */
};

/********** Specification of partially available expressions analysis **************/

pfbv_dfi ** PAV_pfbv_dfi = NULL;

static unsigned int gimple_pfbv_pav_dfa(void);

struct gimple_pfbv_dfa_spec gdfa_pav = 
{
        entity_expr,                   /* entity;                 */
        ZEROS,                         /* top_value;              */
        ZEROS,                         /* entry_info;             */
        ZEROS,                         /* exit_info;              */
        FORWARD,                       /* traversal_order;        */
        UNION,                         /* confluence;             */
        entity_use,                    /* gen_effect;             */
        down_exp,                      /* gen_exposition;         */
        entity_mod,                    /* kill_effect;            */
        any_where,                     /* kill_exposition;        */
        global_only,                   /* preserved_dfi;          */
        identity_forward_edge_flow,    /* forward_edge_flow       */
        stop_flow_along_edge,          /* backward_edge_flow      */
        forward_gen_kill_node_flow,    /* forward_node_flow       */
        stop_flow_along_node,          /* backward_node_flow      */
        /*@Non-separable : START*/
        IGNORE_STATEMENT_TYPE,         /* constgen_statement_type */
        IGNORE_PRECONDITION,           /* constgen_precondition   */
        IGNORE_STATEMENT_TYPE,         /* constkill_statement_type*/
        IGNORE_PRECONDITION,           /* constkill_precondition  */
        IGNORE_ENTITY_DEPENDENCE,      /* dependent_gen           */
        IGNORE_ENTITY_DEPENDENCE       /* dependent_kill          */
        /*@Non-separable : END*/
};

static unsigned int
gimple_pfbv_pav_dfa(void)
{

        PAV_pfbv_dfi = gdfa_driver(gdfa_pav);

        return 0;
}

struct tree_opt_pass pass_gimple_pfbv_pav_dfa =
{
  "gdfa_pav",                         /* name */
  NULL,                               /* gate */
  gimple_pfbv_pav_dfa,                /* execute */
  NULL,                               /* sub */
  NULL,                               /* next */
  0,                                  /* static_pass_number */
  0,                                  /* tv_id */
  0,                                  /* properties_required */
  0,                                  /* properties_provided */
  0,                                  /* properties_destroyed */
  0,                                  /* todo_flags_start */
  0,                                  /* todo_flags_finish */
  0                                   /* letter */
};

/************* Specification of anticipable expressions analysis *****************/

pfbv_dfi ** ANT_pfbv_dfi = NULL;

static unsigned int gimple_pfbv_ant_dfa(void);


struct gimple_pfbv_dfa_spec gdfa_ant = 
{
        entity_expr,                   /* entity;                 */
        ONES,                          /* top_value;              */
        ONES,                          /* entry_info;             */
        ZEROS,                         /* exit_info;              */
        BACKWARD,                      /* traversal_order;        */
        INTERSECTION,                  /* confluence;             */
        entity_use,                    /* gen_effect;             */
        up_exp,                        /* gen_exposition;         */
        entity_mod,                    /* kill_effect;            */
        any_where,                     /* kill_exposition;        */
        global_only,                   /* preserved_dfi;          */
        stop_flow_along_edge,          /* forward_edge_flow       */
        identity_backward_edge_flow,   /* backward_edge_flow      */
        stop_flow_along_node,          /* forward_node_flow       */
        backward_gen_kill_node_flow,   /* backward_node_flow      */
        /*@Non-separable : START*/
        IGNORE_STATEMENT_TYPE,         /* constgen_statement_type */
        IGNORE_PRECONDITION,           /* constgen_precondition   */
        IGNORE_STATEMENT_TYPE,         /* constkill_statement_type*/
        IGNORE_PRECONDITION,           /* constkill_precondition  */
        IGNORE_ENTITY_DEPENDENCE,      /* dependent_gen           */
        IGNORE_ENTITY_DEPENDENCE       /* dependent_kill          */
        /*@Non-separable : END*/
};


static unsigned int
gimple_pfbv_ant_dfa(void)
{
        ANT_pfbv_dfi = gdfa_driver(gdfa_ant);

        return 0;
}

struct tree_opt_pass pass_gimple_pfbv_ant_dfa =
{
  "gdfa_ant",                         /* name */
  NULL,                               /* gate */
  gimple_pfbv_ant_dfa,                /* execute */
  NULL,                               /* sub */
  NULL,                               /* next */
  0,                                  /* static_pass_number */
  0,                                  /* tv_id */
  0,                                  /* properties_required */
  0,                                  /* properties_provided */
  0,                                  /* properties_destroyed */
  0,                                  /* todo_flags_start */
  0,                                  /* todo_flags_finish */
  0                                   /* letter */
};

/************* Specification of live variables analysis *****************/

pfbv_dfi ** LV_pfbv_dfi = NULL;

static unsigned int gimple_pfbv_lv_dfa(void);


struct gimple_pfbv_dfa_spec gdfa_lv = 
{
        entity_var,                   /* entity;                 */
        ZEROS,                        /* top_value;              */
        ZEROS,                        /* entry_info;             */
        ZEROS,                        /* exit_info;              */
        BACKWARD,                     /* traversal_order;        */
        UNION,                        /* confluence;             */
        entity_use,                   /* gen_effect;             */
        up_exp,                       /* gen_exposition;         */
        entity_mod,                   /* kill_effect;            */
        any_where,                    /* kill_exposition;        */
        global_only,                  /* preserved_dfi;          */
        stop_flow_along_edge,         /* forward_edge_flow       */
        identity_backward_edge_flow,  /* backward_edge_flow      */
        stop_flow_along_node,         /* forward_node_flow       */
        backward_gen_kill_node_flow,  /* backward_node_flow      */
        /*@Non-separable : START*/
        IGNORE_STATEMENT_TYPE,         /* constgen_statement_type */
        IGNORE_PRECONDITION,           /* constgen_precondition   */
        IGNORE_STATEMENT_TYPE,         /* constkill_statement_type*/
        IGNORE_PRECONDITION,           /* constkill_precondition  */
        IGNORE_ENTITY_DEPENDENCE,      /* dependent_gen           */
        IGNORE_ENTITY_DEPENDENCE       /* dependent_kill          */
        /*@Non-separable : END*/
};


static unsigned int
gimple_pfbv_lv_dfa(void)
{
        LV_pfbv_dfi = gdfa_driver(gdfa_lv);

        return 0;
}


struct tree_opt_pass pass_gimple_pfbv_lv_dfa =
{
  "gdfa_lv",                          /* name */
  NULL,                               /* gate */
  gimple_pfbv_lv_dfa,                 /* execute */
  NULL,                               /* sub */
  NULL,                               /* next */
  0,                                  /* static_pass_number */
  0,                                  /* tv_id */
  0,                                  /* properties_required */
  0,                                  /* properties_provided */
  0,                                  /* properties_destroyed */
  0,                                  /* todo_flags_start */
  0,                                  /* todo_flags_finish */
  0                                   /* letter */
};

/************* Specification of partial redundancy elimination *****************/


/***  This is the classical bidirectional formulation of  PRE. 
      The data flow equations are:

        IN(bb) = INTERSECTION_over_preds (AVOUT(pred_bb) UNION OUT(pred_bb))
                 MEET
                 (PAVIN(bb) INTERSECTION 
                  ((OUT(bb) - KILL(bb)) + ANTGEN(b))

        OUT(bb) = INTERSECTION_over_succs (IN(succ_bb))

      Thus, 
            - forward_node_flow = stop_flow_along_node
            - backward_edge_flow = identity_backward_edge_flow

      and we need to define 

            - forward_edge_flow because it includes AVOUT also
            - backward_node_flow because it included PAVIN also

      It is important to ensure that this pass is executed after available
      expressions analysis and partially available expressions analysis.
***/

pfbv_dfi ** PRE_pfbv_dfi = NULL;

static unsigned int gimple_pfbv_pre_dfa(void);
dfvalue forward_edge_flow_pre(basic_block src, basic_block dest);
dfvalue backward_node_flow_pre(basic_block bb);


struct gimple_pfbv_dfa_spec gdfa_pre = 
{
        entity_expr,                   /* entity;                 */
        ONES,                          /* top_value;              */
        ZEROS,                         /* entry_info;             */
        ZEROS,                         /* exit_info;              */
        BACKWARD,                      /* traversal_order;        */
        INTERSECTION,                  /* confluence;             */
        entity_use,                    /* gen_effect;             */
        up_exp,                        /* gen_exposition;         */
        entity_mod,                    /* kill_effect;            */
        any_where,                     /* kill_exposition;        */
        global_only,                   /* preserved_dfi;          */
        forward_edge_flow_pre,         /* forward_edge_flow       */
        identity_backward_edge_flow,   /* backward_edge_flow      */
        stop_flow_along_node,          /* forward_node_flow       */
        backward_node_flow_pre,        /* backward_node_flow      */
        /*@Non-separable : START*/
        IGNORE_STATEMENT_TYPE,         /* constgen_statement_type */
        IGNORE_PRECONDITION,           /* constgen_precondition   */
        IGNORE_STATEMENT_TYPE,         /* constkill_statement_type*/
        IGNORE_PRECONDITION,           /* constkill_precondition  */
        IGNORE_ENTITY_DEPENDENCE,      /* dependent_gen           */
        IGNORE_ENTITY_DEPENDENCE       /* dependent_kill          */
        /*@Non-separable : END*/
};


static unsigned int
gimple_pfbv_pre_dfa(void)
{
        PRE_pfbv_dfi = gdfa_driver(gdfa_pre);

        return 0;
}

struct tree_opt_pass pass_gimple_pfbv_pre_dfa =
{
  "gdfa_pre",                         /* name */
  NULL,                               /* gate */
  gimple_pfbv_pre_dfa,                /* execute */
  NULL,                               /* sub */
  NULL,                               /* next */
  0,                                  /* static_pass_number */
  0,                                  /* tv_id */
  0,                                  /* properties_required */
  0,                                  /* properties_provided */
  0,                                  /* properties_destroyed */
  0,                                  /* todo_flags_start */
  0,                                  /* todo_flags_finish */
  0                                   /* letter */
};

dfvalue 
forward_edge_flow_pre(basic_block src, basic_block dest)
{        
        dfvalue temp;

        temp = union_dfvalues (OUT(AV_pfbv_dfi,src), CURRENT_OUT(src));

        return temp; 
}

dfvalue 
backward_node_flow_pre(basic_block bb)
{        
        dfvalue temp1, temp2;

        temp1 = backward_gen_kill_node_flow(bb);

        temp2 = intersect_dfvalues (IN(PAV_pfbv_dfi,bb), temp1);

        if (temp1)
                free_dfvalue_space(temp1);

        return temp2; 
}

/************* Specification of Reaching defination analysis *****************/

pfbv_dfi ** RD_pfbv_dfi = NULL;

static unsigned int gimple_pfbv_rd_dfa(void);

struct gimple_pfbv_dfa_spec gdfa_rd = 
{
        entity_defn,                   /* entity;                 */
        ZEROS,                         /* top_value;              */
        ZEROS, 			       /* entry_info;             */
        ZEROS,                         /* exit_info;              */
        FORWARD,                       /* traversal_order;        */
        UNION,                         /* confluence;             */
        entity_use,                    /* gen_effect;             */
        down_exp,                      /* gen_exposition;         */
        entity_mod,                    /* kill_effect;            */
        any_where,                     /* kill_exposition;        */
        global_only,                   /* preserved_dfi;          */
        identity_forward_edge_flow,    /* forward_edge_flow       */
        stop_flow_along_edge,          /* backward_edge_flow      */
        forward_gen_kill_node_flow,    /* forward_node_flow       */
        stop_flow_along_node,          /* backward_node_flow      */
        /*@Non-separable : START*/
        IGNORE_STATEMENT_TYPE,         /* constgen_statement_type */
        IGNORE_PRECONDITION,           /* constgen_precondition   */
        IGNORE_STATEMENT_TYPE,         /* constkill_statement_type*/
        IGNORE_PRECONDITION,           /* constkill_precondition  */
        IGNORE_ENTITY_DEPENDENCE,      /* dependent_gen           */
        IGNORE_ENTITY_DEPENDENCE       /* dependent_kill          */
        /*@Non-separable : END*/
};

static unsigned int
gimple_pfbv_rd_dfa(void)
{

        RD_pfbv_dfi = gdfa_driver(gdfa_rd);

        return 0;
}


/* This is the specification of rd (Reaching defination) pass in GCC */

struct tree_opt_pass pass_gimple_pfbv_rd_dfa =
{
  "gdfa_rd",                          /* name */
  NULL,                               /* gate */
  gimple_pfbv_rd_dfa,                 /* execute */
  NULL,                               /* sub */
  NULL,                               /* next */
  0,                                  /* static_pass_number */
  0,                                  /* tv_id */
  0,                                  /* properties_required */
  0,                                  /* properties_provided */
  0,                                  /* properties_destroyed */
  0,                                  /* todo_flags_start */
  0,                                  /* todo_flags_finish */
  0                                   /* letter */
};

/*@Non-separable : START*/
/************* Specification of faint variables  *****************/

pfbv_dfi ** FV_pfbv_dfi = NULL;

static unsigned int gimple_pfbv_fv_dfa(void);


struct gimple_pfbv_dfa_spec gdfa_fv =
{
        entity_var,                          /* entity;                 */
        ONES,                                /* top_value;              */
        ONES,                                /* entry_info;             */
        ONES,                                /* exit_info;              */
        BACKWARD,                            /* traversal_order;        */
        INTERSECTION,                        /* confluence;             */
        entity_mod,                          /* gen_effect;             */
        up_exp,                              /* gen_exposition;         */
        entity_use,                          /* kill_effect;            */
        any_where,                           /* kill_exposition;        */
        global_only,                         /* preserved_dfi;          */
        stop_flow_along_edge,                /* forward_edge_flow       */
        identity_backward_edge_flow,         /* backward_edge_flow      */
        stop_flow_along_node,                /* forward_node_flow       */
        backward_gen_kill_node_flow,         /* backward_node_flow      */
        READ_X,                              /* constgen_statement_type */
        X_NOT_IN_OPERAND,                    /* constgen_precondition   */
        USE_X,                               /* constkill_statement_type*/
        IGNORE_PRECONDITION,                 /* constkill_precondition  */
        IGNORE_ENTITY_DEPENDENCE,            /* dependent_gen           */
        X_NOT_IN_GLOBAL_DATA_FLOW_VALUE      /* dependent_kill          */
};


static unsigned int
gimple_pfbv_fv_dfa(void)
{
        FV_pfbv_dfi = gdfa_driver(gdfa_fv);

        return 0;
}


struct tree_opt_pass pass_gimple_pfbv_fv_dfa =
{
  "gdfa_fv",                          /* name */
  NULL,                               /* gate */
  gimple_pfbv_fv_dfa,                 /* execute */
  NULL,                               /* sub */
  NULL,                               /* next */
  0,                                  /* static_pass_number */
  0,                                  /* tv_id */
  0,                                  /* properties_required */
  0,                                  /* properties_provided */
  0,                                  /* properties_destroyed */
  0,                                  /* todo_flags_start */
  0,                                  /* todo_flags_finish */
  0                                   /* letter */
};

/************* Specification of possibly uninitialised variable analysis  *****************/

pfbv_dfi ** PUV_pfbv_dfi = NULL;

static unsigned int gimple_pfbv_puv_dfa(void);


struct gimple_pfbv_dfa_spec gdfa_puv =
{
        entity_var,                                /* entity;                 */
        ZEROS,                                     /* top_value;              */
        ONES,                                      /* entry_info;             */
        ZEROS,                                     /* exit_info;              */
        FORWARD,                                   /* traversal_order;        */
        UNION,                                     /* confluence;             */
        entity_mod,                                /* gen_effect;             */
        up_exp,                                    /* gen_exposition;         */
        entity_mod,                                /* kill_effect;            */
        any_where,                                 /* kill_exposition;        */
        global_only,                               /* preserved_dfi;          */
        identity_forward_edge_flow,                /* forward_edge_flow       */
        stop_flow_along_edge,                      /* backward_edge_flow      */
        forward_gen_kill_node_flow,                /* forward_node_flow       */
        stop_flow_along_node,                      /* backward_node_flow      */
        IGNORE_STATEMENT_TYPE,                     /* constgen_statement_type */
        IGNORE_PRECONDITION,                       /* constgen_precondition   */
        READ_X,                                    /* constkill_statement_type*/
        OPERAND_IS_CONST,                          /* constkill_precondition  */
        OPER_IN_GLOBAL_DATA_FLOW_VALUE,            /* dependent_gen           */
        OPER_NOT_IN_GLOBAL_DATA_FLOW_VALUE         /* dependent_kill          */
};


static unsigned int
gimple_pfbv_puv_dfa(void)
{
        PUV_pfbv_dfi = gdfa_driver(gdfa_puv);

        return 0;
}


struct tree_opt_pass pass_gimple_pfbv_puv_dfa =
{
  "gdfa_puv",                         /* name */
  NULL,                               /* gate */
  gimple_pfbv_puv_dfa,                /* execute */
  NULL,                               /* sub */
  NULL,                               /* next */
  0,                                  /* static_pass_number */
  0,                                  /* tv_id */
  0,                                  /* properties_required */
  0,                                  /* properties_provided */
  0,                                  /* properties_destroyed */
  0,                                  /* todo_flags_start */
  0,                                  /* todo_flags_finish */
  0                                   /* letter */
};

/*@Non-separable : END*/
