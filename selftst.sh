#!/bin/sh
echo "Started">/home/alex/DCU32INT/resSlf/res
for F in /home/alex/DCU32INT/*.dcu; do
  /home/alex/DCU32INT/DCU32INT -U/home/alex/kylix/lib $F "/home/alex/DCU32INT/resSlf/*" |tee -a /home/alex/DCU32INT/resSlf/res
done
echo "Ended" >>/home/alex/DCU32INT/resSlf/res

