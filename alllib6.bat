LFNFOR ON
del i:\tmp\res6\res
del i:\tmp\res6\err
:for %%A in (I:\TMP\DCUTST\v6\*.dcu) do command /C h:\prg\dcu32pas\do1.bat "-UI:\TMP\DCUTST\v6" %%A i:\tmp\res6
for %%A in (I:\TMP\DCUTST\v6\*.dcu) do h:\prg\dcu32pas\dcu32int.exe "-UI:\TMP\DCUTST\v6" %%A i:\tmp\res6\* >>i:\tmp\res6\res
