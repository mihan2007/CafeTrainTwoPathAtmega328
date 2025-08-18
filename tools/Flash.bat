@echo off
cd /d "%~dp0"

:: Указываем программатор и микроконтроллер
set PROGRAMMER=usbasp
set MCU=m328pb
set HEX_FILE=firmware\BlinkTest.hex

:: Проверяем, существует ли файл прошивки
if not exist "%HEX_FILE%" (
    echo ❌ Ошибка: HEX-файл "%HEX_FILE%" не найден!
    pause
    exit
)

:: Запуск прошивки
echo 🔍 Прошиваем %MCU% через %PROGRAMMER%...
avrdude -c %PROGRAMMER% -p %MCU% -B 10 -U flash:w:"%HEX_FILE%":i

echo ✅ Прошивка завершена!
pause
