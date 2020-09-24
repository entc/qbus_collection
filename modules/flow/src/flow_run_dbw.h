#ifndef __FLOW__RUN_DBW__H
#define __FLOW__RUN_DBW__H 1

// CAPE includes
#include <sys/cape_export.h>
#include <sys/cape_types.h>
#include <sys/cape_queue.h>

// QBUS includes
#include <qbus.h>

// ADBL includes
#include <adbl.h>

//-----------------------------------------------------------------------------

#define FLOW_ACTION__PRIM             0    // first time this task was called
#define FLOW_ACTION__REDO             1    // redo
#define FLOW_ACTION__SET              2    // set
#define FLOW_ACTION__OVERRIDE         3    // override
#define FLOW_ACTION__ABORT            4    // abort -> deactivate the task
#define FLOW_ACTION__NONE             7    // ignore

//-----------------------------------------------------------------------------

#define FLOW_SEQUENCE__DEFAULT        1    // default workflow sequence
#define FLOW_SEQUENCE__ABORT          2    // abort sequence
#define FLOW_SEQUENCE__FAILOVER       3    // in case of failure

//-----------------------------------------------------------------------------

struct FlowRunDbw_s; typedef struct FlowRunDbw_s* FlowRunDbw;

//-----------------------------------------------------------------------------
// constructor / destructor

__CAPE_LOCAL   FlowRunDbw   flow_run_dbw_new               (QBus, AdblSession, CapeQueue, number_t wpid, number_t psid, const CapeString remote, CapeUdc rinfo, number_t refid);

__CAPE_LOCAL   void         flow_run_dbw_del               (FlowRunDbw*);

__CAPE_LOCAL   FlowRunDbw   flow_run_dbw_clone             (FlowRunDbw, number_t psid, number_t sqid);

//-----------------------------------------------------------------------------
// QBUS methods

                            /* starts a flow in init mode */
__CAPE_LOCAL   int          flow_run_dbw_start             (FlowRunDbw*, number_t action, CapeUdc* p_params, CapeErr err);

                            /* checks if the flow can be continued */
__CAPE_LOCAL   int          flow_run_dbw_set               (FlowRunDbw*, number_t action, CapeUdc* p_params, CapeErr err);

                            /* continue with the next process step */
__CAPE_LOCAL   int          flow_run_dbw_continue          (FlowRunDbw*, number_t action, CapeUdc* p_params, CapeErr err);

                            /* continue with the next process step */
__CAPE_LOCAL   int          flow_run_dbw_inherit           (FlowRunDbw, number_t wfid, number_t syncid, CapeUdc* p_params, CapeErr err);

                            /* change the sequence id of a process */
__CAPE_LOCAL   int          flow_run_dbw_sqt               (FlowRunDbw*, number_t sequence_id, number_t from_child, number_t from_parent, CapeErr err);

                            /* reruns the step, but don't continues */
__CAPE_LOCAL   int          flow_run_dbw_once              (FlowRunDbw*, CapeErr err);

//-----------------------------------------------------------------------------
// getter

__CAPE_LOCAL   number_t     flow_run_dbw_state_get         (FlowRunDbw);

__CAPE_LOCAL   number_t     flow_run_dbw_fctid_get         (FlowRunDbw);

__CAPE_LOCAL   number_t     flow_run_dbw_synct_get         (FlowRunDbw);

__CAPE_LOCAL   CapeUdc      flow_run_dbw_rinfo_get         (FlowRunDbw);

//-----------------------------------------------------------------------------
// process manipulators

                            /* create entries in the database for the new process */
__CAPE_LOCAL   number_t     flow_run_dbw_init              (FlowRunDbw, AdblTrx, number_t wfid, number_t syncid, int add_psid, CapeErr err);

                            /* set a new state and ends the current processing step */
__CAPE_LOCAL   void         flow_run_dbw_state_set         (FlowRunDbw, number_t state, CapeErr result_err);

//-----------------------------------------------------------------------------
// data manipulators

                            /* retrieve QBUS info for continue */
__CAPE_LOCAL   int          flow_run_dbw_pdata__qbus         (FlowRunDbw, CapeString* module, CapeString* method, CapeUdc* params, CapeErr err);

                            /* retrieve a list with all splits */
__CAPE_LOCAL   int          flow_run_dbw_xdata__split        (FlowRunDbw, CapeUdc* list, number_t* wfid, CapeErr err);

                            /* retrieve all data for a switch */
__CAPE_LOCAL   int          flow_run_dbw_xdata__switch       (FlowRunDbw, CapeString* p_value, CapeUdc* p_switch_node, CapeErr err);

                            /* retrieve all data from an if */
__CAPE_LOCAL   int          flow_run_dbw_xdata__if           (FlowRunDbw, CapeUdc* p_value_node, number_t* p_wfid, CapeErr err);

                            /* check pdata if the logs shall be merged into params */
__CAPE_LOCAL   int          flow_run_dbw_pdata__logs_merge   (FlowRunDbw, CapeUdc params, CapeErr err);

                            /* merge content of TDATA to params */
__CAPE_LOCAL   void         flow_run_dbw_tdata__merge_in     (FlowRunDbw, CapeUdc params);

                            /* merge content of params to TDATA */
__CAPE_LOCAL   void         flow_run_dbw_tdata__merge_to     (FlowRunDbw, CapeUdc* params);

                            /* copy variable in TDATA */
__CAPE_LOCAL   int          flow_run_dbw_xdata__var_copy     (FlowRunDbw, CapeErr err);

                            /* copy variable in TDATA */
__CAPE_LOCAL   int          flow_run_dbw_xdata__var_move     (FlowRunDbw, CapeErr err);

                            /* craete a new node in TDATA */
__CAPE_LOCAL   int          flow_run_dbw_xdata__create_node  (FlowRunDbw, CapeErr err);

                            /* add an vdata event */
__CAPE_LOCAL   void         flow_run_dbw__event_add          (FlowRunDbw, number_t err_code, const CapeString err_text, number_t vaid, number_t stype);

                            /* create all wait items */
__CAPE_LOCAL   int          flow_run_dbw_wait__init          (FlowRunDbw, CapeErr err);

                            /* check for a wait item */
__CAPE_LOCAL   int          flow_run_dbw_wait__check_item    (FlowRunDbw, const CapeString uuid, const CapeString code, CapeErr err);

//-----------------------------------------------------------------------------
// sync tools

__CAPE_LOCAL   number_t     flow_run_dbw_sync__add         (FlowRunDbw, number_t cnt, CapeErr);

//-----------------------------------------------------------------------------
// data tools

__CAPE_LOCAL   CapeUdc      flow_data_get                  (AdblSession adbl_session, number_t dataid, CapeErr err);

__CAPE_LOCAL   number_t     flow_data_add                  (AdblTrx trx, CapeUdc content, CapeErr err);

__CAPE_LOCAL   int          flow_data_rm                   (AdblTrx trx, number_t dataid, CapeErr err);

__CAPE_LOCAL   int          flow_data_set                  (AdblTrx trx, number_t dataid, CapeUdc content, CapeErr err);

//-----------------------------------------------------------------------------
// log tools

__CAPE_LOCAL   int          flow_log_add_trx               (AdblTrx trx, number_t psid, number_t wsid, number_t log_type, number_t log_state, CapeUdc data, const CapeString vsec, CapeErr err);

__CAPE_LOCAL   int          flow_log_add                   (AdblSession adbl_session, number_t psid, number_t wsid, number_t log_type, number_t log_state, CapeUdc data, const CapeString vsec, CapeErr err);

//-----------------------------------------------------------------------------

#endif
