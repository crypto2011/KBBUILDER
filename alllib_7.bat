:LFNFOR ON
set LIB_PATH=%1
del d:\data\int\* /Q
for %%A in ("%LIB_PATH%*.dcu";"%LIB_PATH%*.dcuil") do C:\PRG\DCU32PAS\dcu32int.exe "-U%LIB_PATH%;D:\Program Files\Borland\Delphi7\Lib" "%%A" d:\data\int\* -AC -RWinTypes=Windows;WinProcs=Windows;DbiTypes=BDE;DbiProcs=BDE;DbiErrs=BDE; %2 %3 >>d:\data\int\res