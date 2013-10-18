#!/bin/bash

if [ $# -ne "2" ]
then
        echo "Usage: ./apply_repo_updates.sh [testing|stable] user@host"
        exit 1
fi

SUBREPO=$1
SSH_STRING=$2

echo "Do: ssh ${SSH_STRING} sudo /opt/smart-m3-repo/import_packages.sh ${SUBREPO}"

ssh ${SSH_STRING} sudo /opt/smart-m3-repo/import_packages.sh ${SUBREPO}
