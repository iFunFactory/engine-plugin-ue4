"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin\vcvars32.bat"

perl Configure VC-WIN32 no-idea no-md2 no-mdc2 no-rc5 no-rc4 no-asm --prefix=.\build
ms\do_ms.bat
nmake -f ms\nt.mak
nmake -f ms\nt.mak install
