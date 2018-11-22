@echo off
title install so

call gradlew.bat compileDebugSources

set dir=%~dp0
set package=com.example.ffmpeg.c
set activity=com.ffmpeg.MainActivity
set abi=armeabi-v7a
set modulefile=libnative-lib

set so_path=%dir%app\build\intermediates\cmake\debug\obj\%abi%\%modulefile%.so
set tem_path=/sdcard/%modulefile%.so

adb install -r %DIRNAME%

echo %so_path%
adb push %so_path% %tem_path%

adb shell "su -c ' cp %tem_path% /data/data/%package%/lib'"

adb shell am force-stop %package%
adb shell am start -n %package%/%activity%

