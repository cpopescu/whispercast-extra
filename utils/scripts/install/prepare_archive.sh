#!/bin/bash

P4CLIENT=export
P4HOME=/tmp/export
mkdir -p $P4HOME

if [ $# -eq 0 ]; then
  echo "Usage: $0 branchname archname label"
  echo "Where:"
  echo "  branchname: e.g. 'whispersoft' or 'whispersoft-evolva'"
  echo "  archname: must be one of: "
  echo "    - 'full': complete whispersoft"
  echo "    - 'dist': complete whispersoft no example flvs"
  echo "    - 'normal': complete whispersoft, without private"
  echo "    - 'small': complete whispersoft,without private, without whispersoft/www/media"
  echo "    - 'minimal': complete whispersoft,without private, without whispersoft/www/media, without whisperer, without whispermedialib"
  echo "  label: p4 label name, without the front '@'"
  exit
fi


BRANCHNAME=whispersoft
if [ $# -gt 0 ]; then
  BRANCHNAME=$1
  shift
fi

ARCHNAME=full
if [ $# -gt 0 ]; then
  ARCHNAME=$1
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

echo "== > Using branch: [$BRANCHNAME], archname: [$ARCHNAME] , label: [$LABEL]"
if [[ "$ARCHNAME" != "full" && "$ARCHNAME" != "dist" && "$ARCHNAME" != "small" && "$ARCHNAME" != whisperlib && "$ARCHNAME" != "normal" && "$ARCHNAME" != "minimal" ]] ; then
  echo "Unknown archive name. Valid names are: full, normal, small."
  exit
fi

echo "== > Collecting file names ..."
p4 sync -f //depot/$BRANCHNAME/...$LABEL || exit 1
mkdir -p $BRANCHNAME/private
touch $BRANCHNAME/private/.dummy
touch $BRANCHNAME/whispermedialib/.dummy
touch $BRANCHNAME/whisperer/.dummy
touch $BRANCHNAME/whisperstreamlib/.dummy
touch $BRANCHNAME/whispercast/.dummy

echo "== > P4 Sync done"

if [ "$ARCHNAME" = "full" ] ; then
  p4 sync -f -n  //depot/$BRANCHNAME/...$LABEL | awk '{print substr($0, 9)}' | cut -d"#" -f1 | sed "s/%40/@/g" > file_list
elif [ "$ARCHNAME" = "dist" ] ; then
  p4 sync -f -n  //depot/$BRANCHNAME/...$LABEL | awk '{print substr($0, 9)}' | cut -d"#" -f1  | grep -v "\.flv$" | grep -v "\.f4v$" | sed "s/%40/@/g" > file_list
elif [ "$ARCHNAME" = "small" ] ; then
  p4 sync -f -n  //depot/$BRANCHNAME/...$LABEL | awk '{print substr($0, 9)}' | cut -d"#" -f1  | grep -v "^$BRANCHNAME\/private\/" | grep -v "\.flv$" | grep -v "\.f4v$" | sed "s/%40/@/g" > file_list
  echo $BRANCHNAME/private/.dummy >> file_list
elif [ "$ARCHNAME" = "normal" ] ; then
  p4 sync -f -n  //depot/$BRANCHNAME/...$LABEL | awk '{print substr($0, 9)}' | cut -d"#" -f1  | grep -v "^$BRANCHNAME\/private\/" | grep -v "\.flv$" | grep -v "\.f4v$" | sed "s/%40/@/g" > file_list
  p4 sync -f -n  //depot/$BRANCHNAME/whispercast/www/media/...$LABEL | awk '{print substr($0, 9)}' | cut -d"#" -f1  | grep -v "^$BRANCHNAME\/private\/" | sed "s/%40/@/g" >> file_list
  echo $BRANCHNAME/private/.dummy >> file_list
elif [ "$ARCHNAME" = "whisperlib" ] ; then
  p4 sync -f -n  //depot/$BRANCHNAME/...$LABEL | awk '{print substr($0, 9)}' | cut -d"#" -f1  | grep "^$BRANCHNAME\/whisperlib\/" | sed "s/%40/@/g" > file_list
  echo $BRANCHNAME/private/.dummy >> file_list
  echo $BRANCHNAME/whispercast/.dummy >> file_list
  echo $BRANCHNAME/whisperstreamlib/.dummy >> file_list
  echo $BRANCHNAME/whispermedialib/.dummy >> file_list
  echo $BRANCHNAME/whisperer/.dummy >> file_list
elif [ "$ARCHNAME" = "minimal" ] ; then
  p4 sync -f -n  //depot/$BRANCHNAME/...$LABEL | awk '{print substr($0, 9)}' | cut -d"#" -f1  | grep -v "^$BRANCHNAME\/private\/" | grep -v "\.flv$" | grep -v "\.f4v$" | grep -v "^$BRANCHNAME\/whisperer\/" | grep -v "^$BRANCHNAME\/whispermedialib\/" | sed "s/%40/@/g" > file_list
  echo $BRANCHNAME/private/.dummy >> file_list
  echo $BRANCHNAME/whispermedialib/.dummy >> file_list
  echo $BRANCHNAME/whisperer/.dummy >> file_list
else
  echo "Unknown archive name. Valid names are: full, normal, small."
  exit
fi

touch $BRANCHNAME/.dummy
echo $BRANCHNAME/.dummy >> file_list

echo "== > Found: " $(wc -l file_list) " files"
echo "== > Making a tarball..."
tar -c -T file_list -f whispersoft.tar
echo "== > Compressing ..."
gzip -f whispersoft.tar
rm -f whispersoft.tar
rm -f file_list
FILE="$BRANCHNAME"-"$ARCHNAME"-"$VERSION".tar.gz
mv whispersoft.tar.gz $FILE
echo "==> Your archive is: " $P4HOME/$FILE
ls -l $P4HOME/$FILE
