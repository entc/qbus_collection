#!/bin/bash

MYSQLDUMP="mysqldump"
#MYSQLDUMP="/opt/local/lib/mariadb/bin/mysqldump"

#$MYSQLDUMP --set-gtid-purged=off -u $1 -p $2 \
$MYSQLDUMP --single-transaction -u $1 -p $2 \
proc_workflows \
proc_worksteps \
proc_tasks \
proc_data \
proc_task_sync \
flow_instance \
flow_log \
proc_current_workstep_view \
proc_next_workstep_view \
proc_task_logs_view \
proc_task_view \
flow_chain_view \
flow_last_prev_view \
flow_proc_worksteps_view \
flow_process_details_view \
flow_process_get_view \
flow_process_prev_view \
flow_process_sync_view \
flow_workflows_view \
flow_worksteps_prev_succ_view \
| sed -e 's/DEFINER[ ]*=[ ]*[^*]*\*/\*/' > ../../../../var/flow.sql
