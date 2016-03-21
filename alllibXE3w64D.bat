set LibDir=I:\DATA\XE3\lib\win64\debug
del /Q C:\DATA\DCUChkRes\*
for %%A in ("%LibDir%\*.dcu") do dcu32int.exe -AC "-U%LibDir%" "%%A" C:\DATA\DCUChkRes\* >>C:\DATA\DCUChkRes\res
goto :EOF
