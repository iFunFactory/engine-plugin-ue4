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
find ${PROJECT_SOURCE_DIR}/${PROJECT_NAME} -maxdepth 1 -name "test_messages.pb.*"

# Test UE4 dedicated rpc file build
./protoc --cpp_out=${PROJECT_SOURCE_DIR}/${PROJECT_NAME} test_dedicated_server_rpc_messages.proto
find ${PROJECT_SOURCE_DIR}/${PROJECT_NAME} -maxdepth 1 -name "test_dedicated_server_rpc_messages.pb.*"


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