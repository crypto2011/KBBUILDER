#!/bin/sh
echo "Started">/home/alex/DCU32INT/res/res
for F in /home/alex/kylix/lib/*.dcu; do
  /home/alex/DCU32INT/DCU32INT -U/home/alex/kylix/lib $F "/home/alex/DCU32INT/res/*" |tee -a /home/alex/DCU32INT/res/res
done
echo "Ended" >>/home/alex/DCU32INT/res/res

