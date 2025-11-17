#!/bin/bash

MYSQLDUMP="mysqldump"
#MYSQLDUMP="/opt/local/lib/mariadb/bin/mysqldump"

#$MYSQLDUMP --set-gtid-purged=off -u $1 -p $2 \
$MYSQLDUMP --single-transaction -h 127.0.0.1 -P 13306 -u $1 -p $2 \
auth_sessions \
auth_sessions_wp_view \
auth_users_secret_view \
auth_roles_gpid_view \
auth_tokens_jobs \
auth_perm \
auth_logins \
glob_emails \
glob_persons \
q5_users \
rbac_clts \
rbac_roles \
rbac_roles_view \
rbac_users \
rbac_users_roles \
rbac_users_view \
rbac_workspaces \
rbac_workspaces_roles \
rbac_workspaces_roles_view \
auth_logins_last_view \
auth_logins_cnt_view \
rbac_users_logins_view \
| sed -e 's/DEFINER[ ]*=[ ]*[^*]*\*/\*/' > ../../../../var/auth.sql
