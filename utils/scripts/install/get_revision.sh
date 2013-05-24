#!/bin/bash

P4CLIENT=export
P4HOME=/tmp/export
mkdir -p $P4HOME

if [ $# -eq 0 ]; then
  echo "Usage: $0 branchname label"
  echo "Where:"
  echo "  branchname: e.g. 'whispersoft' or 'whispersoft-evolva'"
  echo "  label: if missing then the head is retrieved"
  echo "         else: p4 label name, without the front '@'"
  exit
fi


BRANCHNAME=whispersoft
if [ $# -gt 0 ]; then
  BRANCHNAME=$1
  shift
fi

LABEL=
VERSION=`date +%Y_%m_%d`
#VERSION=unversioned
if [ $# -gt 0 ]; then
  LABEL=@$1
  VERSION=$1
  shift
fi

cd $P4HOME

echo "== > Using branch: [$BRANCHNAME], label: [$LABEL]"

echo "== > Getting revision...$LABEL"
p4 sync //depot/$BRANCHNAME/...$LABEL || exit 1

echo "== > P4 Sync done"
echo "Your files are here: $P4HOME/$BRANCHNAME"
