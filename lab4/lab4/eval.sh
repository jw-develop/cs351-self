#!/bin/bash
mkdir -p eval
rm tsh
make
for i in {01..16}
do
echo "test $i"
make test$i > eval/$i
make rtest$i > eval/r$i
diff eval/$i eval/r$i
done
