rem LFNFOR ON
del D:\DATA\Delphi9DCU\Delphi9DCU\libIL\Debug\int\* /Q
for %%A in ("D:\DATA\Delphi9DCU\Delphi9DCU\libIL\Debug\*.dcuil") do dcu32int.exe "-UD:\DATA\Delphi9DCU\Delphi9DCU\libIL\Debug" "%%A" D:\DATA\Delphi9DCU\Delphi9DCU\libIL\Debug\int\* >>D:\DATA\Delphi9DCU\Delphi9DCU\libIL\Debug\int\res
