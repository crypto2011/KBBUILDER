rem LFNFOR ON
del D:\DATA\Delphi9DCU\Delphi9DCU\libIL\DCUIL\int\* /Q
for %%A in ("D:\DATA\Delphi9DCU\Delphi9DCU\libIL\DCUIL\*.dcuil") do dcu32int.exe "-UD:\DATA\Delphi9DCU\Delphi9DCU\libIL\DCUIL" "%%A" D:\DATA\Delphi9DCU\Delphi9DCU\libIL\DCUIL\int\* >>D:\DATA\Delphi9DCU\Delphi9DCU\libIL\DCUIL\int\res
