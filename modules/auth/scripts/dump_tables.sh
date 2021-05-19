#!/bin/bash

MYSQLDUMP="/opt/local/lib/mariadb/bin/mysqldump"

$MYSQLDUMP --set-gtid-purged=off -u $1 -p $2 \
auth_sessions \
auth_sessions_wp_view \
auth_users_secret_view \
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
> ../../../../var/auth.sql
