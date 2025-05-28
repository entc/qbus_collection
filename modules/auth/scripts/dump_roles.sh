#!/bin/bash

MYSQLDUMP="mysqldump"
#MYSQLDUMP="/opt/local/lib/mariadb/bin/mysqldump"

#$MYSQLDUMP --set-gtid-purged=off -u $1 -p $2 \
$MYSQLDUMP --single-transaction -u $1 -p $2 \
rbac_roles \
| sed -e 's/DEFINER[ ]*=[ ]*[^*]*\*/\*/' > ../../../../var/auth_roles.sql
