@echo off
cd /d "%~dp0"
avrdude -c usbasp -p m328p
pause