"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin\vcvars32.bat"

cd winbuild
nmake -f Makefile.vc mode=static VC=12 ENABLE_SSPI=no ENABLE_IPV6=no ENABLE_IDN=no MACHINE=x86
