@echo off

set XPJ="xpj4.exe"

@rem %XPJ% -v 1 -t VC14 -p WIN32 -x ConvexDecomposition.xpj
%XPJ% -v 1 -t VC14 -p WIN64 -x ConvexDecomposition.xpj

cd ..
cd vc14win64

goto cleanExit

:pauseExit
pause

:cleanExit

