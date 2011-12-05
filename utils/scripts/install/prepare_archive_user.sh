#!/bin/bash

# Similar to prepare_archive.sh but this version uses the local p4 user.
# So you need a valid P4 client shell setup.

if [ $# -eq 0 ]; then
  echo "Usage: $0 branchname label"
  echo "Where:"
  echo "  branchname: e.g. 'whispersoft' or 'whispersoft-evolva'"
  echo "  label: if missing then the head is retrieved"
  echo "         else: p4 label name, without the front '@'"
  exit
fi

ARCHNAME=dist

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

if [ "$P4HOME" = "" ]; then
  echo "Missing P4HOME environment variable"
  exit
fi
cd $P4HOME

echo "== > Using branch: [$BRANCHNAME], label: [$LABEL]"

echo "== > Getting revision...$LABEL"
p4 sync -f -n  //depot/$BRANCHNAME/...$LABEL | awk '{print substr($0, 9)}' | cut -d"#" -f1  | grep -v "\.flv$" | grep -v "\.f4v$" | sed "s/%40/@/g" > file_list
p4 opened | grep //depot/$BRANCHNAME | awk '{print substr($0, 9)}' | cut -d"#" -f1  | grep -v "\.flv$" | grep -v "\.f4v$" | sed "s/%40/@/g" >> file_list
echo "== > P4 Sync done"

echo "== > Found: " $(wc -l file_list | cut -d ' ' -f 1) " files"
echo "== > Making a tarball..."
tar -c -T file_list -f whispersoft.tar
echo "== > Compressing ..."
gzip -f whispersoft.tar
rm -f whispersoft.tar
rm -f file_list
BNAME=$BRANCHNAME
if [ $BRANCHNAME == "whispersoft" ]; then
  BNAME="whispersoft-main"
fi
FILE="$BNAME"-"$ARCHNAME"-"$VERSION".tar.gz
mv whispersoft.tar.gz $FILE
echo "==> Your archive is: " $P4HOME/$FILE
ls -l $P4HOME/$FILE

