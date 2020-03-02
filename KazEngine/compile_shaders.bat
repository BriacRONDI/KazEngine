@ECHO off

SETLOCAL
CALL :COMPILE dynamic_model.vert
CALL :COMPILE basic_model.vert
CALL :COMPILE basic_model_geom.vert
REM CALL :COMPILE colored_model.frag
CALL :COMPILE textured_model.frag
CALL :COMPILE normal_debug.geom
CALL :COMPILE base.frag
CALL :COMPILE material_basic_model.vert
CALL :COMPILE material_basic_model.frag
CALL :COMPILE texture_basic_model.vert
CALL :COMPILE material_skeleton_model.vert
CALL :COMPILE global_bone_model.vert
CALL :COMPILE textured_map.vert
CALL :COMPILE textured_map.frag
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
