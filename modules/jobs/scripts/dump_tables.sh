#!/bin/bash

MYSQLDUMP="mysqldump"
#MYSQLDUMP="/opt/local/lib/mariadb/bin/mysqldump"

#$MYSQLDUMP --set-gtid-purged=off -u $1 -p $2 \
$MYSQLDUMP --single-transaction -u $1 -p $2 \
jobs_list \
| sed -e 's/DEFINER[ ]*=[ ]*[^*]*\*/\*/' > ../../../../var/jobs.sql
