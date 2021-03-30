#/bin/bash
mv $1 old.$1
cat ../tests/license_add.txt > $1
cat old.$1 >> $1
