#!/bin/bash
#
# Copyright (c) 2009, Whispersoft s.r.l.
# All rights reserved.


$2/osd_associator_test  --destroy_osds_on_change=false > /tmp/osd_associator_test.1
$2/osd_associator_test  --destroy_osds_on_change=true > /tmp/osd_associator_test.2

echo "Diffing osd_associator_test.1"
diff /tmp/osd_associator_test.1 $1/test/osd_associator_test.1 || exit 1
echo "Diffing osd_associator_test.2"
diff /tmp/osd_associator_test.2 $1/test/osd_associator_test.2 || exit 1
