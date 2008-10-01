@echo off

set DIR=%1
set JAVASCRIPTCORE_DIR="..\..\..\third_party\WebKit\JavaScriptCore"
setlocal

mkDIR 2>NUL %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\API\APICast.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\API\JSBase.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\API\JSValueRef.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\API\JSObjectRef.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\API\JSRetainPtr.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\API\JSContextRef.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\API\JSStringRef.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\API\JSStringRefCF.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\API\JSStringRefBSTR.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\API\JavaScriptCore.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\bindings\npruntime.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\bindings\runtime.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\bindings\np_jsobject.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\bindings\npruntime_internal.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\bindings\npruntime_impl.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\bindings\runtime_object.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\bindings\runtime_root.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\kjs\JSLock.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\kjs\collector.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\kjs\interpreter.h" %DIR%
xcopy /y /d "%JAVASCRIPTCORE_DIR%\wtf\HashCountedSet.h" %DIR%

endlocal
