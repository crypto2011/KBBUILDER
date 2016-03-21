LFNFOR ON
del i:\tmp\resK\res
del i:\tmp\resK\err
for %%A in (I:\TMP\DCUTST\vk\lib\*.dcu) do h:\prg\dcu32pas\dcu32int.exe "-UI:\TMP\DCUTST\vk\lib" %%A i:\tmp\resK\* >>i:\tmp\resK\res
