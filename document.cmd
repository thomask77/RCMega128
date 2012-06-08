@echo off
regsvr32 /s ..\tools\htmlhelp\itcc.dll
..\tools\doxygen\doxygen
start docs\index.chm
