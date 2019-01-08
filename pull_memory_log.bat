@echo off
title pull

set log_name=log.txt
set dir=%~dp0
set package=com.example.ffmpeg.c
set activity=com.ffmpeg.MainActivity
set abi=armeabi-v7a
set modulefile=libnative-lib

set so_path=%dir%app\build\intermediates\cmake\debug\obj\%abi%\%modulefile%.so
set log_path=/sdcard/%log_name%
set out_path=G:\vm_share\LeakTracer\helpers

echo %so_path%
echo %dir%
echo %log_path%
adb pull %log_path% %dir%

copy %dir%\%log_name% %out_path%
copy %so_path% %out_path%
rem pause
