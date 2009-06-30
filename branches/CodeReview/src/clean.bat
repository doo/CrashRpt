REM Delete all tempoary files 

erase /A H *.suo
erase *.ncb
erase *.opt
erase *.ilk

rmdir /S /Q crashcon\src\Debug
rmdir /S /Q crashcon\src\Release
erase /A H crashcon\src\*.user

rmdir /S /Q crashrpt\src\Debug
rmdir /S /Q crashrpt\src\Release
erase /A H crashrpt\src\*.user

rmdir /S /Q CrashRptTest\src\Debug
rmdir /S /Q CrashRptTest\src\Release
erase /A H CrashRptTest\src\*.user

rmdir /S /Q CrashSender\src\Debug
rmdir /S /Q CrashSender\src\Release
erase /A H CrashSender\src\*.user
