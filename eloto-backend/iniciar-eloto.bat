@echo off
title E-LOTO Backend
cd /d "%~dp0"

where node >nul 2>&1
if errorlevel 1 (
  echo Node.js nao encontrado. Instale em https://nodejs.org e tente de novo.
  echo.
  pause
  exit /b 1
)

if not exist node_modules (
  echo Instalando dependencias pela primeira vez...
  call npm install
  if errorlevel 1 (
    echo.
    echo ERRO ao instalar as dependencias.
    pause
    exit /b 1
  )
)

set ELOTO_KEY=TCC

echo.
echo  E-LOTO ligando...
echo  Site: http://localhost:3000
echo.
echo  Deixe esta janela aberta. Feche-a para desligar o servidor.
echo.

start "" cmd /c "timeout /t 3 /nobreak >nul & start http://localhost:3000"

node server.js

echo.
echo O servidor parou.
pause
