#!/bin/sh

USER=$1
PASS=$2

# construct the IV
iv=`printf %016d $(date +%s)`

# construct the ID
id=`printf $USER | openssl dgst -sha256`

# construct the DA
h1=`printf %s:%s $USER $PASS | openssl dgst -sha256`
h2=`printf %s:%s $iv $h1 | openssl dgst -sha256`

echo "send request"

# request
result=$(curl -v -i -s \
	-H "Authorization: Crypt4 {\"ha\":\"$iv\",\"id\":\"$id\",\"da\":\"$h2\"}" \
	-H "Content-Type: application/json" \
	$3/json/auth/vault_set\
	--data "{\"secret\":\"$PASS\"}")
