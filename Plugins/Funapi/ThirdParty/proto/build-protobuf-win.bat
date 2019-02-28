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

mv -f ..\..\Source\Funapi\Public\funapi\management\maintenance_message.pb.cc ^
    ..\..\Source\Funapi\Private\funapi\management

mv -f ..\..\Source\Funapi\Public\funapi\network\fun_message.pb.cc ^
    ..\..\Source\Funapi\Private\funapi\network

mv -f ..\..\Source\Funapi\Public\funapi\network\ping_message.pb.cc ^
    ..\..\Source\Funapi\Private\funapi\network

mv -f ..\..\Source\Funapi\Public\funapi\service\multicast_message.pb.cc ^
    ..\..\Source\Funapi\Private\funapi\service

mv -f ..\..\Source\Funapi\Public\funapi\service\redirect_message.pb.cc ^
    ..\..\Source\Funapi\Private\funapi\service

mv -f ..\..\Source\Funapi\Public\funapi\distribution\fun_dedicated_server_rpc_message.pb.cc  ^
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


:::::::::::::::::::::::::::::::::::::::::::::
:: Replace std container
:::::::::::::::::::::::::::::::::::::::::::::
FOR /R %ROOT_PATH%..\..\Source\Funapi\Private\funapi %%i IN (*.pb.*) DO (
    CALL :ReplaceStdToFun %%i
)

FOR /R %ROOT_PATH%..\..\Source\Funapi\Public\funapi %%i IN (*.pb.*) DO (
    CALL :ReplaceStdToFun %%i
)

CD %USER_PROTO_FILE_OUT_PATH%
FOR %%i IN (*.pb.*) DO (
    CALL :ReplaceStdToFun %cd%\%%i
)

EXIT /B 0

:::::::::::::::::::::::::::::::::::::::::::::
:: Replace funtion
:::::::::::::::::::::::::::::::::::::::::::::

:ReplaceStdToFun
    %~dp0replace.exe "\bostringstream" "FUN_OSTRINGSTREAM" %~1
    :: remove xx_ word
    %~dp0replace.exe "\bFUN_OSTRINGSTREAM_" "ostringstream_" %~1

    %~dp0replace.exe "\bstd::istringstream" "FUN_ISTRINGSTREAM" %~1
    %~dp0replace.exe "\bistringstream" "FUN_ISTRINGSTREAM" %~1
    :: remove xx_ word
    %~dp0replace.exe "\bFUN_ISTRINGSTREAM_" "istringstream_" %~1


    %~dp0replace.exe "\bstd::stringstream" "FUN_STRINGSTREAM" %~1
    %~dp0replace.exe "\bstringstream" "FUN_STRINGSTREAM" %~1
    :: remove xx_ word
    %~dp0replace.exe "\bFUN_STRINGSTREAM_" "stringstream_" %~1


    %~dp0replace.exe "\bstd::string" "FUN_STRING" %~1
    %~dp0replace.exe "\bstring" "FUN_STRING" %~1
    :: remove xx_ word
    %~dp0replace.exe "\bFUN_STRING_" "string_" %~1


    %~dp0replace.exe "\bstd::vector" "FUN_VECTOR" %~1
    %~dp0replace.exe "\bvector" "FUN_VECTOR" %~1
    :: remove xx_ word
    %~dp0replace.exe "\bFUN_VECTOR_" "vector_" %~1


    %~dp0replace.exe "\bstd::deque" "FUN_DEQUE" %~1
    %~dp0replace.exe "\bdeque" "FUN_DEQUE" %~1
    :: remove xx_ word
    %~dp0replace.exe "\bFUN_DEQUE_" "deque_" %~1


    %~dp0replace.exe "\bstd::queue" "FUN_QUEUE" %~1
    %~dp0replace.exe "\bqueue" "FUN_QUEUE" %~1
    :: remove xx_ word
    %~dp0replace.exe "\bFUN_QUEUE_" "queue_" %~1


    %~dp0replace.exe "\bstd::set" "FUN_SET" %~1
    %~dp0replace.exe "\bset" "FUN_SET" %~1
    :: remove xx_ word
    %~dp0replace.exe "\bFUN_SET_" "set_" %~1

    %~dp0replace.exe "\bstd::unordered_set" "FUN_UNORDERED_SET" %~1
    %~dp0replace.exe "\bunordered_set" "FUN_UNORDERED_SET" %~1
    :: remove xx_ word
    %~dp0replace.exe "\bFUN_UNORDERED_SET_" "unordered_set_" %~1


    %~dp0replace.exe "\bstd::map" "FUN_MAP" %~1
    %~dp0replace.exe "\bmap" "FUN_MAP" %~1
    :: remove xx_ word
    %~dp0replace.exe "\bFUN_MAP_" "map_" %~1


    %~dp0replace.exe "\bstd::unordered_map" "FUN_UNORDERED_MAP" %~1
    %~dp0replace.exe "\bunordered_map" "FUN_UNORDERED_MAP" %~1
    :: remove xx_ word
    %~dp0replace.exe "\bFUN_UNORDERED_MAP_" "unordered_map_" %~1


    %~dp0replace.exe "\bFUN_OSTRINGSTREAM" "fun::ostringstream" %~1
    %~dp0replace.exe "\bFUN_ISTRINGSTREAM" "fun::istringstream" %~1
    %~dp0replace.exe "\bFUN_STRINGSTREAM" "fun::stringstream" %~1
    %~dp0replace.exe "\bFUN_STRING" "fun::string" %~1


    %~dp0replace.exe "\bFUN_VECTOR" "fun::vector" %~1
    %~dp0replace.exe "\bFUN_DEQUE" "fun::deque" %~1
    %~dp0replace.exe "\bFUN_QUEUE" "fun::queue" %~1
    %~dp0replace.exe "\bFUN_SET" "fun::set" %~1
    %~dp0replace.exe "\FUN_UNORDERED_SET" "fun::unordered_set" %~1
    %~dp0replace.exe "\bFUN_MAP" "fun::map" %~1
    %~dp0replace.exe "\bFUN_UNORDERED_MAP" "fun::unordered_map" %~1


    %~dp0replace.exe "#include <fun::ostringstream>" "#include <ostringstream>" %~1
    %~dp0replace.exe "#include <fun::istringstream>" "#include <istringstream>" %~1
    %~dp0replace.exe "#include <fun::stringstream>" "#include <stringstream>" %~1
    %~dp0replace.exe "#include <fun::string>" "#include <string>" %~1
    %~dp0replace.exe "#include <fun::vector>" "#include <vector>" %~1
    %~dp0replace.exe "#include <fun::deque>" "#include <deque>" %~1
    %~dp0replace.exe "#include <fun::queue>" "#include <queue>" %~1
    %~dp0replace.exe "#include <fun::set>" "#include <set>" %~1
    %~dp0replace.exe "#include <fun::map>" "#include <map>" %~1
    %~dp0replace.exe "#include <fun::unordered_map>" "#include <unordered_map>" %~1
    ECHO "Replaced std container to fun container. FileName: %~1"
EXIT /B 0