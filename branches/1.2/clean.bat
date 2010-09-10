REM Delete all tempoary files 

erase /A H *.suo
erase *.ncb
erase *.opt
erase *.ilk
erase *.sdf

erase bin\*.pdb
erase bin\*.ilk
erase bin\*d.exe
erase bin\*d.dll
erase bin\*LIB.exe

erase docs\Warnings.txt

erase lib\*d.lib
erase lib\*.exp
erase lib\*LIB.lib

rmdir /S /Q thirdparty\lib

rmdir /S /Q reporting\crashcon\Debug
rmdir /S /Q reporting\crashcon\Release
rmdir /S /Q "reporting\crashcon\Release LIB"
rmdir /S /Q reporting\crashcon\x64
erase /A H reporting\crashcon\*.user
erase reporting\crashcon\*.aps

rmdir /S /Q reporting\crashrpt\Debug
rmdir /S /Q reporting\crashrpt\Release
rmdir /S /Q reporting\crashrpt\x64
erase /A H reporting\crashrpt\*.user
erase reporting\crashrpt\*.aps

rmdir /S /Q reporting\CrashRptTest\Debug
rmdir /S /Q reporting\CrashRptTest\Release
rmdir /S /Q "reporting\CrashRptTest\Release LIB"
rmdir /S /Q reporting\CrashRptTest\x64
erase /A H reporting\CrashRptTest\*.user
erase reporting\CrashRptTest\*.aps

rmdir /S /Q reporting\CrashSender\Debug
rmdir /S /Q reporting\CrashSender\Release
rmdir /S /Q "reporting\CrashRptTest\Release LIB"
rmdir /S /Q reporting\CrashSender\x64
erase /A H reporting\CrashSender\*.user
erase reporting\CrashSender\*.aps
erase reporting\CrashSender\*.aps

rmdir /S /Q thirdparty\zlib\Debug
rmdir /S /Q thirdparty\zlib\Release
rmdir /S /Q "thirdparty\zlib\Release LIB"
rmdir /S /Q thirdparty\zlib\x64
rmdir /S /Q thirdparty\zlib\lib
erase /A H thirdparty\zlib\*.user
erase thirdparty\zlib\*.aps

rmdir /S /Q libpng\src\Debug
rmdir /S /Q libpng\src\Release
rmdir /S /Q "libpng\src\Release LIB"
rmdir /S /Q libpng\src\x64
rmdir /S /Q libpng\lib
erase /A H libpng\src\*.user
erase libpng\src\*.aps

rmdir /S /Q minizip\src\Debug
rmdir /S /Q minizip\src\Release
rmdir /S /Q "minizip\src\Release LIB"
rmdir /S /Q minizip\src\x64
rmdir /S /Q minizip\lib
erase /A H minizip\src\*.user
erase minizip\src\*.aps

rmdir /S /Q CrashRptProbe\src\Debug
rmdir /S /Q CrashRptProbe\src\Release
rmdir /S /Q "CrashRptProbe\src\Release LIB"
rmdir /S /Q CrashRptProbe\src\x64
erase /A H CrashRptProbe\src\*.user
erase CrashRptProbe\src\*.aps

rmdir /S /Q crprober\src\Debug
rmdir /S /Q crprober\src\Release
rmdir /S /Q "crprober\src\Release LIB"
rmdir /S /Q crprober\src\x64
erase /A H crprober\src\*.user
erase crprober\src\*.aps

rmdir /S /Q tests\Debug
rmdir /S /Q tests\Release
rmdir /S /Q "tests\Release LIB"
rmdir /S /Q tests\x64
erase /A H tests\*.user
erase tests\*.aps

