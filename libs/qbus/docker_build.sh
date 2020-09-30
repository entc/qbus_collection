#!/bin/bash

if ! [[ -v QORE_CONFIG_PATH ]]; then

  read -p "Enter your Build Environment : " path
  
  export QORE_CONFIG_PATH=$path

fi

echo "Using Config Path: " $2/etc/$path

if ! [[ -d $2/etc/$path ]]; then

  echo "Config Path doesn't exists"
  exit 1

fi


docker build -t qore/qbus:buster -f $1/docker/Dockerfile_buster $2/etc/${QORE_CONFIG_PATH}/ --build-arg CACHEBUST=0
