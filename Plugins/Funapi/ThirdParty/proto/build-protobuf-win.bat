:: Copyright (C) 2019 iFunFactory Inc. All Rights Reserved.
::
:: This work is confidential and proprietary to iFunFactory Inc. and
:: must not be used, disclosed, copied, or distributed without the prior
:: consent of iFunFactory Inc.

@ECHO OFF
ECHO "Start build .proto files"

:: Setting user project name
Set PROJECT_NAME=funapi_plugin_ue4
Set PROJECT_SOURCE_DIR=..\..\..\..\Source

:::::::::::::::::::::::::::::::::::::::::::::
:: Build plugin's proto file. DO NOT EDIT!
:::::::::::::::::::::::::::::::::::::::::::::

protoc.exe --cpp_out=dllexport_decl=FUNAPI_API:..\..\Source\Funapi\Public ^
    funapi\management\maintenance_message.proto ^
    funapi\network\fun_message.proto ^
    funapi\network\ping_message.proto ^
    funapi\service\multicast_message.proto ^
    funapi\service\redirect_message.proto ^
    funapi\distribution\fun_dedicated_server_rpc_message.proto

move /y ..\..\Source\Funapi\Public\funapi\management\maintenance_message.pb.cc ^
    ..\..\Source\Funapi\Private\funapi\management

move /y ..\..\Source\Funapi\Public\funapi\network\fun_message.pb.cc ^
    ..\..\Source\Funapi\Private\funapi\network

move /y ..\..\Source\Funapi\Public\funapi\network\ping_message.pb.cc ^
    ..\..\Source\Funapi\Private\funapi\network

move /y ..\..\Source\Funapi\Public\funapi\service\multicast_message.pb.cc ^
    ..\..\Source\Funapi\Private\funapi\service

move /y ..\..\Source\Funapi\Public\funapi\service\redirect_message.pb.cc ^
    ..\..\Source\Funapi\Private\funapi\service

move /y ..\..\Source\Funapi\Public\funapi\distribution\fun_dedicated_server_rpc_message.pb.cc  ^
    ..\..\Source\Funapi\Private\funapi\distribution

:::::::::::::::::::::::::::::::::::::::::::::
:: Build test proto files.
:::::::::::::::::::::::::::::::::::::::::::::

:: Test echo message file build
protoc.exe --cpp_out=%PROJECT_SOURCE_DIR%\%PROJECT_NAME% test_messages.proto

:: Test UE4 dedicated rpc file build
protoc --cpp_out=%PROJECT_SOURCE_DIR%\%PROJECT_NAME% test_dedicated_server_rpc_messages.proto

:::::::::::::::::::::::::::::::::::::::::::::
:: Build user's proto files.
:::::::::::::::::::::::::::::::::::::::::::::

:: NOTE(sungjin)
:: Build .proto files in {Project/Source} directory.

SET ROOT_PATH=%~dp0
:: Build your custom .proto file, Please Edit Path
Set USER_PROTO_FILE_INPUT_PATH=%ROOT_PATH%%PROJECT_SOURCE_DIR%\%PROJECT_NAME%
Set USER_PROTO_FILE_OUT_PATH=%ROOT_PATH%%PROJECT_SOURCE_DIR%\%PROJECT_NAME%

setLocal enableDelayedExpansion
set n=0

:: Copy proto files to funapi plugin directory.
FOR %%I IN (%USER_PROTO_FILE_INPUT_PATH%\*.proto) DO (
    :: save copyed files name
    set /a n+=1
    set FILE_LIST!n!=%%I
    :: copy file
    copy %%I .
)

:: Copy proto files to funapi plugin directory.
FOR %%I IN (*.proto) DO (
    protoc --cpp_out=%PROJECT_SOURCE_DIR%\%PROJECT_NAME% %%I
)

:::: Remove copyed proto files
FOR /L %%i IN (1,1,!n!) Do (
    FOR /F %%a in ("!FILE_LIST%%i!") DO (
        DEL /F %%~na%%~xa
    )
)

EXIT /B 0