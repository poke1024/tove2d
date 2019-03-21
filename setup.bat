@echo off

REM configures demos and compiles tove2d on Windows

REM for compiling, you need:
REM Microsoft Visual Studio, Community Edition (https://visualstudio.microsoft.com/de/downloads/)
REM Python2 or Python3 (https://www.python.org/downloads/)
REM scons (https://scons.org/pages/download.html); you need to run "python scons-folder/setup.py""

if [%1]==[/C] (
	REM compile if given /C parameter
	cd /D "%~dp0"
	scons.py
)

set demos=alpha blob clippath editor fillrule hearts renderers tesselation zoom

cd /D "%~dp0"
for %%d in (%demos%) do  (
	if not exist ".\demos\%%d\tove\" (
		echo "creating link for .\demos\%%d\tove"
		del /Q .\demos\%%d\tove
		mklink /J .\demos\%%d\tove .\tove
	)
	if not exist ".\demos\%%d\assets\" (
		del /Q .\demos\%%d\assets
		mklink /J .\demos\%%d\assets .\demos\assets
	)
)
