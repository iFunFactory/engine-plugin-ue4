:: Copyright (C) 2019 iFunFactory Inc. All Rights Reserved.
::
:: This work is confidential and proprietary to iFunFactory Inc. and
:: must not be used, disclosed, copied, or distributed without the prior
:: consent of iFunFactory Inc.


@ECHO OFF
ECHO It is a file to respond to the 4.21.2 CachedCPPEnvironments issue.
ECHO Start changing file extension ".cc" to ".cpp"

SET root_path=%~dp0

ECHO Checking exist funapi plugin folder
IF not exist %root_path%\Plugins\Funapi (
  ECHO Not exist funapi plugin folder
  cmd /k
)


CD %root_path%Plugins\Funapi

FOR /R %%I IN (*.cc) DO (
 ECHO REN "%%I" "%%~nI.cpp"
 REN "%%I" "%%~nI.cpp"
)

CD %root_path%Source

ECHO Check yours .cc files that in Source/ directory
ECHO That will .cc file change to .cpp
ECHO Continue press any key

pause

FOR /R %%I IN (*.cc) DO (
 ECHO REN "%%I" "%%~nI.cpp"
 REN "%%I" "%%~nI.cpp"
)
