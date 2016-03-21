rem LFNFOR ON
del D:\DATA\DCU8\int\* /Q
for %%A in ("D:\DATA\DCU8\*.dcuil") do dcu32int.exe "-UD:\DATA\DCU8" "%%A" D:\DATA\DCU8\int\* >>D:\DATA\DCU8\int\res
