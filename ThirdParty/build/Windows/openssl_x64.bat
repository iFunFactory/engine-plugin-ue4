%comspec% /k "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86_amd64

perl Configure VC-WIN64A no-idea no-md2 no-mdc2 no-rc5 no-rc4 no-asm --prefix=.\build
ms\do_win64a.bat
nmake -f ms\nt.mak
nmake -f ms\nt.mak install
