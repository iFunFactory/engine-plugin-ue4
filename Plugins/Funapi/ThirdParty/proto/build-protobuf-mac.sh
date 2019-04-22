#!/bin/bash -e
# Copyright (C) 2019 iFunFactory Inc. All Rights Reserved.
#
# This work is confidential and proprietary to iFunFactory Inc. and
# must not be used, disclosed, copied, or distributed without the prior
# consent of iFunFactory Inc.
set -e


echo "Start build .proto files"
# Setting user project name
PROJECT_NAME=funapi_plugin_ue4
PROJECT_SOURCE_DIR=../../../../Source

# NOTE: Avoid IOS Heap curroption
# pb.cc, pb.h 파일의 std::{container} 를 fun::{container} 로 변경할 수 있습니다.
# 사용시 주의 점은 아래 링크를 참고해 주세요
# https://www.ifunfactory.com/engine/documents/reference/ko/client-plugin.html#iphone-xs
#############################################
# Replace funtion
#############################################

ReplaceStdToFun()
{
  set -e

  RelaceFile=$1
  mono replace.exe "\bstd::ostringstream\b" "FUN_OSTRINGSTREAM" $RelaceFile
  mono replace.exe "\bostringstream\b" "FUN_OSTRINGSTREAM" $RelaceFile
  # remove xx_ word
  mono replace.exe "\bFUN_OSTRINGSTREAM_" "ostringstream_" $RelaceFile

  mono replace.exe "\bstd::istringstream\b" "FUN_ISTRINGSTREAM" $RelaceFile
  mono replace.exe "\bistringstream\b" "FUN_ISTRINGSTREAM" $RelaceFile
  # remove xx_ word
  mono replace.exe "\bFUN_ISTRINGSTREAM_" "istringstream_" $RelaceFile

  mono replace.exe "\bstd::stringstream\b" "FUN_STRINGSTREAM" $RelaceFile
  mono replace.exe "\bstringstream\b" "FUN_STRINGSTREAM" $RelaceFile
  # remove xx_ word
  mono replace.exe "\bFUN_STRINGSTREAM_" "stringstream_" $RelaceFile

  mono replace.exe "\bstd::string\b" "FUN_STRING" $RelaceFile
  mono replace.exe "\bstring\b" "FUN_STRING" $RelaceFile
  # remove xx_ word
  mono replace.exe "\bFUN_STRING_" "string_" $RelaceFile

  mono replace.exe "\bstd::vector\b" "FUN_VECTOR" $RelaceFile
  mono replace.exe "\bvector\b" "FUN_VECTOR" $RelaceFile
  # remove xx_ word
  mono replace.exe "\bFUN_VECTOR_" "vector_" $RelaceFile

  mono replace.exe "\bstd::deque\b" "FUN_DEQUE" $RelaceFile
  mono replace.exe "\bdeque\b" "FUN_DEQUE" $RelaceFile
  # remove xx_ word
  mono replace.exe "\bFUN_DEQUE_" "deque_" $RelaceFile

  mono replace.exe "\bstd::queue\b" "FUN_QUEUE" $RelaceFile
  mono replace.exe "\bqueue\b" "FUN_QUEUE" $RelaceFile
  # remove xx_ word
  mono replace.exe "\bFUN_QUEUE_" "queue_" $RelaceFile

  mono replace.exe "\bstd::set\b" "FUN_SET" $RelaceFile
  mono replace.exe "\bset\b" "FUN_SET" $RelaceFile
  mono replace.exe "\bFUN_SET_" "set_" $RelaceFile

  mono replace.exe "\bstd::unordered_set\b" "FUN_UNORDERED_SET" $RelaceFile
  mono replace.exe "\bunordered_set\b" "FUN_UNORDERED_SET" $RelaceFile
  mono replace.exe "\bFUN_UNORDERED_SET_" "unordered_set_" $RelaceFile

  mono replace.exe "\bstd::map\b" "FUN_MAP" $RelaceFile
  mono replace.exe "\bmap\b" "FUN_MAP" $RelaceFile
  # remove xx_ word
  mono replace.exe "\bFUN_MAP_" "map_" $RelaceFile

  mono replace.exe "\bstd::unordered_map\b" "FUN_UNORDERED_MAP" $RelaceFile
  mono replace.exe "\bunordered_map\b" "FUN_UNORDERED_MAP" $RelaceFile
  # remove xx_ word
  mono replace.exe "\bFUN_UNORDERED_MAP_" "unordered_map_" $RelaceFile

  mono replace.exe "\bFUN_OSTRINGSTREAM" "fun::ostringstream" $RelaceFile
  mono replace.exe "\bFUN_ISTRINGSTREAM" "fun::istringstream" $RelaceFile
  mono replace.exe "\bFUN_STRINGSTREAM" "fun::stringstream" $RelaceFile
  mono replace.exe "\bFUN_STRING" "fun::string" $RelaceFile

  mono replace.exe "\bFUN_VECTOR" "fun::vector" $RelaceFile
  mono replace.exe "\bFUN_DEQUE" "fun::deque" $RelaceFile
  mono replace.exe "\bFUN_QUEUE" "fun::queue" $RelaceFile
  mono replace.exe "\bFUN_SET" "fun::set" $RelaceFile
  mono replace.exe "\bFUN_UNORDERED_SET" "fun::unordered_set" $RelaceFile
  mono replace.exe "\bFUN_MAP" "fun::map" $RelaceFile
  mono replace.exe "\bFUN_UNORDERED_MAP" "fun::unordered_map" $RelaceFile

  mono replace.exe "#include <fun::ostringstream>" "#include <ostringstream>" $RelaceFile
  mono replace.exe "#include <fun::istringstream>" "#include <istringstream>" $RelaceFile
  mono replace.exe "#include <fun::stringstream>" "#include <stringstream>" $RelaceFile
  mono replace.exe "#include <fun::string>" "#include <string>" $RelaceFile
  mono replace.exe "#include <fun::vector>" "#include <vector>" $RelaceFile
  mono replace.exe "#include <fun::deque>" "#include <deque>" $RelaceFile
  mono replace.exe "#include <fun::queue>" "#include <queue>" $RelaceFile
  mono replace.exe "#include <fun::set>" "#include <set>" $RelaceFile
  mono replace.exe "#include <fun::map>" "#include <map>" $RelaceFile
  mono replace.exe "#include <fun::unordered_map>" "#include <unordered_map>" $RelaceFile

  echo "Replaced std container to fun container. FileName: $RelaceFile"
}
export -f ReplaceStdToFun

#############################################
# Build plugin's proto file. DO NOT EDIT!
#############################################

./protoc --cpp_out=dllexport_decl=FUNAPI_API:../../Source/Funapi/Public \
  funapi/management/maintenance_message.proto \
  funapi/network/fun_message.proto \
  funapi/network/ping_message.proto \
  funapi/service/multicast_message.proto \
  funapi/service/redirect_message.proto \
  funapi/distribution/fun_dedicated_server_rpc_message.proto

mv -f ../../Source/Funapi/Public/funapi/management/maintenance_message.pb.cc \
  ../../Source/Funapi/Private/funapi/management

mv -f ../../Source/Funapi/Public/funapi/network/fun_message.pb.cc \
  ../../Source/Funapi/Private/funapi/network

mv -f ../../Source/Funapi/Public/funapi/network/ping_message.pb.cc \
  ../../Source/Funapi/Private/funapi/network

mv -f ../../Source/Funapi/Public/funapi/service/multicast_message.pb.cc \
  ../../Source/Funapi/Private/funapi/service

mv -f ../../Source/Funapi/Public/funapi/service/redirect_message.pb.cc \
  ../../Source/Funapi/Private/funapi/service

mv -f ../../Source/Funapi/Public/funapi/distribution/fun_dedicated_server_rpc_message.pb.cc  \
  ../../Source/Funapi/Private/funapi/distribution/

#############################################
# Build test proto files.
#############################################

# Test echo message file build
./protoc --cpp_out=${PROJECT_SOURCE_DIR}/${PROJECT_NAME} test_messages.proto

# NOTE: Avoid IOS Heap curroption
# find ${PROJECT_SOURCE_DIR}/${PROJECT_NAME} -maxdepth 1 -name "test_messages.pb.*" \
#   -exec bash -c 'ReplaceStdToFun "$0"' {} \;

# Test UE4 dedicated rpc file build
./protoc --cpp_out=${PROJECT_SOURCE_DIR}/${PROJECT_NAME} test_dedicated_server_rpc_messages.proto

# NOTE: Avoid IOS Heap curroption
# find ${PROJECT_SOURCE_DIR}/${PROJECT_NAME} -maxdepth 1 -name "test_dedicated_server_rpc_messages.pb.*" \
#   -exec bash -c 'ReplaceStdToFun "$0"' {} \;


#############################################
# Build user's proto files.
#############################################

# NOTE(sungjin)
# Build .proto files in {Project/Source} directory.

# Build your custom .proto file, Please Edit Path
USER_PROTO_FILE_INPUT_PATH=${PROJECT_SOURCE_DIR}/${PROJECT_NAME}
USER_PROTO_FILE_OUT_PATH=${PROJECT_SOURCE_DIR}/${PROJECT_NAME}

FILE_LIST=(`find ${USER_PROTO_FILE_INPUT_PATH} -maxdepth 1 -name "*.proto"`)

# Copy proto files to funapi plugin directory.
find ${USER_PROTO_FILE_INPUT_PATH} -maxdepth 1 -name "*.proto" -exec cp {} . \;

# Build proto files to user directory
find ./ -maxdepth 1 -name "*.proto" -exec ./protoc --cpp_out=${USER_PROTO_FILE_OUT_PATH} {} \;

# Remove proto files
for value in "${FILE_LIST[@]}"; do
    name=$(basename "$value" ".proto")
    rm -f $name.proto
done


# NOTE: Avoid IOS Heap curroption
#############################################
# Replace std container
#############################################

# find ${USER_PROTO_FILE_OUT_PATH} -maxdepth 1 -name "*.pb.*" \
#   -exec bash -c 'ReplaceStdToFun "$0"' {} \;


# NOTE(sungjin): replace std to fun FunapiPlugin Protobuf
find ${PWD}/../../Source/Funapi/Public/funapi -name "*.pb.*" \
  -exec bash -c 'ReplaceStdToFun "$0"' {} \;

find ${PWD}/../../Source/Funapi/Private/funapi -name "*.pb.*" \
  -exec bash -c 'ReplaceStdToFun "$0"' {} \;