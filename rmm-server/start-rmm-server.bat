@echo off
rem Vyrja RMM server (PocketBase) - listens on the whole LAN.
rem Put pocketbase.exe in this folder, then double-click this file.
pocketbase.exe serve --http=0.0.0.0:8090
pause
