rem LFNFOR ON
del D:\DATA\DCU8\debug\int\* /Q
for %%A in ("D:\DATA\DCU8\debug\*.dcuil") do dcu32int.exe "-UD:\DATA\DCU8\debug" "%%A" D:\DATA\DCU8\debug\int\* >>D:\DATA\DCU8\debug\int\res
