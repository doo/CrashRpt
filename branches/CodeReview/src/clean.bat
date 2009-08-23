REM Delete all tempoary files 

erase /A H *.suo
erase *.ncb
erase *.opt
erase *.ilk

erase bin\*.pdb
erase bin\*.ilk
erase bin\*d.exe
erase bin\*d.dll
erase bin\*LIB.exe

erase lib\*d.lib
erase lib\*LIB.lib
erase lib\*.exp

rmdir /S /Q crashcon\src\Debug
rmdir /S /Q crashcon\src\Release
rmdir /S /Q "crashcon\src\Release LIB"
erase /A H crashcon\src\*.user
erase crashcon\src\*.aps

rmdir /S /Q crashrpt\src\Debug
rmdir /S /Q crashrpt\src\Release
rmdir /S /Q "crashrpt\src\Release LIB"
erase /A H crashrpt\src\*.user
erase crashrpt\src\*.aps

rmdir /S /Q CrashRptTest\src\Debug
rmdir /S /Q CrashRptTest\src\Release
rmdir /S /Q "CrashRptTest\src\Release LIB"
erase /A H CrashRptTest\src\*.user
erase CrashRptTest\src\*.aps

rmdir /S /Q CrashSender\src\Debug
rmdir /S /Q CrashSender\src\Release
erase /A H CrashSender\src\*.user
erase CrashSender\src\*.aps

rmdir /S /Q zlib\src\Debug
rmdir /S /Q zlib\src\Release
rmdir /S /Q zlib\lib
erase /A H zlib\src\*.user
erase zlib\src\*.aps
