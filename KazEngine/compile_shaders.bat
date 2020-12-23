@ECHO off

SETLOCAL
CALL :COMPILE map.vert
CALL :COMPILE map.frag
CALL :COMPILE textured_model.vert
CALL :COMPILE textured_model.frag
CALL :COMPILE dynamic_model.vert
CALL :COMPILE interface.vert
CALL :COMPILE interface.frag
CALL :COMPILE cross.vert
CALL :COMPILE cross.frag
CALL :COMPILE cull_lod.comp
CALL :COMPILE cull_lod_anim.comp
CALL :COMPILE move_collision.comp
CALL :COMPILE move_groups.comp
CALL :COMPILE init_group.comp
pause

:COMPILE
DEL %CD%\Shaders\%~1.spv 2>NUL
DEL %CD%\x64\Debug\Shaders\%~1.spv 2>NUL
DEL %CD%\x64\Release\Shaders\%~1.spv 2>NUL

%VK_SDK_PATH%\bin\glslangvalidator -V %CD%\Shaders\%~1 -o %CD%\Shaders\%~1.spv 1>NUL
IF %ERRORLEVEL% NEQ 0 (
	ECHO %~1 : FAILED
	%VK_SDK_PATH%\bin\glslangvalidator -V %CD%\Shaders\%~1 -o %CD%\Shaders\%~1.spv
) else (
	ECHO %~1 : OK
	xcopy %CD%\Shaders\%~1.spv %CD%\..\x64\Debug\Shaders\ /Y /Q 1>NUL
	xcopy %CD%\Shaders\%~1.spv %CD%\..\x64\Release\Shaders\ /Y /Q 1>NUL
)
EXIT /B 0
