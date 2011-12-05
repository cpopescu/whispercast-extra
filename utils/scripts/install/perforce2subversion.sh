#!/bin/bash

P4CLIENT=export
P4HOME=/tmp/export

mkdir $P4HOME

# sync to P4 home dir: /tmp/export
p4 sync -f //depot/whispersoft/...@whispersoft_0_2_0 || exit 1
echo "====> P4 Sync OK"

echo "====> Pushing to SVN"
echo "      You will be prompted for password of whispercastorg@whispercast.org and a commit description"

svn import $P4HOME/whispersoft/ svn+ssh://whispercastorg@whispercast.org/var/www/vhosts/whispercast.org/private/svn/whispercast/trunk/
