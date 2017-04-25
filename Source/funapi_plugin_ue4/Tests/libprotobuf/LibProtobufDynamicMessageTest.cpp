// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "funapi_plugin_ue4.h"
#include "AutomationTest.h"

#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/substitute.h>
#include <google/protobuf/stubs/stl_util.h>

#include <google/protobuf/stubs/common.h>

#include <google/protobuf/dynamic_message.h>

#include <google/protobuf/wire_format.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "googletest.h"
#include "unittest.pb.h"
#include "unittest_custom_options.pb.h"
#include "unittest_import.pb.h"
#include "test_util.h"
#include "unittest_mset.pb.h"

#include <sstream>
#include <string>
#include <functional>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFunapiLibProtobufDynamicMessageTest, "LibProtobuf.DynamicMessageTest", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFunapiLibProtobufDynamicMessageTest::RunTest(const FString& Parameters)
{
  google::protobuf::DescriptorPool pool_;
  google::protobuf::DynamicMessageFactory factory_(&pool_);
  const google::protobuf::Descriptor* descriptor_;
  const google::protobuf::Message* prototype_;
  const google::protobuf::Descriptor* extensions_descriptor_;
  const google::protobuf::Message* extensions_prototype_;
  const google::protobuf::Descriptor* packed_descriptor_;
  const google::protobuf::Message* packed_prototype_;
  const google::protobuf::Descriptor* oneof_descriptor_;
  const google::protobuf::Message* oneof_prototype_;

  // SetUp()
  {
    // We want to make sure that DynamicMessage works (particularly with
    // extensions) even if we use descriptors that are *not* from compiled-in
    // types, so we make copies of the descriptors for unittest.proto and
    // unittest_import.proto.
    google::protobuf::FileDescriptorProto unittest_file;
    google::protobuf::FileDescriptorProto unittest_import_file;
    google::protobuf::FileDescriptorProto unittest_import_public_file;

    google::protobuf::unittest::TestAllTypes::descriptor()->file()->CopyTo(&unittest_file);
    google::protobuf::unittest_import::ImportMessage::descriptor()->file()->CopyTo(
      &unittest_import_file);
    google::protobuf::unittest_import::PublicImportMessage::descriptor()->file()->CopyTo(
      &unittest_import_public_file);

    verify(pool_.BuildFile(unittest_import_public_file) != NULL);
    verify(pool_.BuildFile(unittest_import_file) != NULL);
    verify(pool_.BuildFile(unittest_file) != NULL);

    descriptor_ = pool_.FindMessageTypeByName("protobuf_unittest.TestAllTypes");
    verify(descriptor_ != NULL);
    prototype_ = factory_.GetPrototype(descriptor_);

    extensions_descriptor_ =
      pool_.FindMessageTypeByName("protobuf_unittest.TestAllExtensions");
    verify(extensions_descriptor_ != NULL);
    extensions_prototype_ = factory_.GetPrototype(extensions_descriptor_);

    packed_descriptor_ =
      pool_.FindMessageTypeByName("protobuf_unittest.TestPackedTypes");
    verify(packed_descriptor_ != NULL);
    packed_prototype_ = factory_.GetPrototype(packed_descriptor_);

    oneof_descriptor_ =
      pool_.FindMessageTypeByName("protobuf_unittest.TestOneof2");
    verify(oneof_descriptor_ != NULL);
    oneof_prototype_ = factory_.GetPrototype(oneof_descriptor_);
  }

  // DynamicMessageTest, Descriptor
  {
    // Check that the descriptor on the DynamicMessage matches the descriptor
    // passed to GetPrototype().
    verify(prototype_->GetDescriptor() == descriptor_);
  }

  // DynamicMessageTest, OnePrototype
  {
    // Check that requesting the same prototype twice produces the same object.
    verify(prototype_ == factory_.GetPrototype(descriptor_));
  }

  // DynamicMessageTest, Defaults
  {
    // Check that all default values are set correctly in the initial message.
    google::protobuf::TestUtil::ReflectionTester reflection_tester(descriptor_);
    reflection_tester.ExpectClearViaReflection(*prototype_);
  }

  // DynamicMessageTest, IndependentOffsets
  {
    // Check that all fields have independent offsets by setting each
    // one to a unique value then checking that they all still have those
    // unique values (i.e. they don't stomp each other).
    google::protobuf::scoped_ptr<google::protobuf::Message> message(prototype_->New());
    google::protobuf::TestUtil::ReflectionTester reflection_tester(descriptor_);

    reflection_tester.SetAllFieldsViaReflection(message.get());
    reflection_tester.ExpectAllFieldsSetViaReflection(*message);
  }

  // DynamicMessageTest, Extensions
  {
    // Check that extensions work.
    google::protobuf::scoped_ptr<google::protobuf::Message> message(extensions_prototype_->New());
    google::protobuf::TestUtil::ReflectionTester reflection_tester(extensions_descriptor_);

    reflection_tester.SetAllFieldsViaReflection(message.get());
    reflection_tester.ExpectAllFieldsSetViaReflection(*message);
  }

  // DynamicMessageTest, PackedFields
  {
    // Check that packed fields work properly.
    google::protobuf::scoped_ptr<google::protobuf::Message> message(packed_prototype_->New());
    google::protobuf::TestUtil::ReflectionTester reflection_tester(packed_descriptor_);

    reflection_tester.SetPackedFieldsViaReflection(message.get());
    reflection_tester.ExpectPackedFieldsSetViaReflection(*message);
  }

  // DynamicMessageTest, Oneof
  {
    // Check that oneof fields work properly.
    google::protobuf::scoped_ptr<google::protobuf::Message> message(oneof_prototype_->New());

    // Check default values.
    const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
    const google::protobuf::Reflection* reflection = message->GetReflection();
    verify(0 == reflection->GetInt32(
      *message, descriptor->FindFieldByName("foo_int")));
    verify("" == reflection->GetString(
      *message, descriptor->FindFieldByName("foo_string")));
    verify("" == reflection->GetString(
      *message, descriptor->FindFieldByName("foo_cord")));
    verify("" == reflection->GetString(
      *message, descriptor->FindFieldByName("foo_string_piece")));
    verify("" == reflection->GetString(
      *message, descriptor->FindFieldByName("foo_bytes")));
    verify(google::protobuf::unittest::TestOneof2::FOO == reflection->GetEnum(
      *message, descriptor->FindFieldByName("foo_enum"))->number());
    const google::protobuf::Descriptor* nested_descriptor;
    const google::protobuf::Message* nested_prototype;
    nested_descriptor =
      pool_.FindMessageTypeByName("protobuf_unittest.TestOneof2.NestedMessage");
    nested_prototype = factory_.GetPrototype(nested_descriptor);
    verify(nested_prototype ==
      &reflection->GetMessage(
        *message, descriptor->FindFieldByName("foo_message")));
    const google::protobuf::Descriptor* foogroup_descriptor;
    const google::protobuf::Message* foogroup_prototype;
    foogroup_descriptor =
      pool_.FindMessageTypeByName("protobuf_unittest.TestOneof2.FooGroup");
    foogroup_prototype = factory_.GetPrototype(foogroup_descriptor);
    verify(foogroup_prototype ==
      &reflection->GetMessage(
        *message, descriptor->FindFieldByName("foogroup")));
    verify(foogroup_prototype !=
      &reflection->GetMessage(
        *message, descriptor->FindFieldByName("foo_lazy_message")));
    verify(5 == reflection->GetInt32(
      *message, descriptor->FindFieldByName("bar_int")));
    verify("STRING" == reflection->GetString(
      *message, descriptor->FindFieldByName("bar_string")));
    verify("CORD" == reflection->GetString(
      *message, descriptor->FindFieldByName("bar_cord")));
    verify("SPIECE" == reflection->GetString(
      *message, descriptor->FindFieldByName("bar_string_piece")));
    verify("BYTES" == reflection->GetString(
      *message, descriptor->FindFieldByName("bar_bytes")));
    verify(google::protobuf::unittest::TestOneof2::BAR == reflection->GetEnum(
      *message, descriptor->FindFieldByName("bar_enum"))->number());

    // Check set functions.
    google::protobuf::TestUtil::ReflectionTester reflection_tester(oneof_descriptor_);
    reflection_tester.SetOneofViaReflection(message.get());
    reflection_tester.ExpectOneofSetViaReflection(*message);
  }

  // DynamicMessageTest, SpaceUsed
  {
    // Test that SpaceUsed() works properly

    // Since we share the implementation with generated messages, we don't need
    // to test very much here.  Just make sure it appears to be working.

    google::protobuf::scoped_ptr<google::protobuf::Message> message(prototype_->New());
    google::protobuf::TestUtil::ReflectionTester reflection_tester(descriptor_);

    int initial_space_used = message->SpaceUsed();

    reflection_tester.SetAllFieldsViaReflection(message.get());
    verify(initial_space_used < message->SpaceUsed());
  }

  return true;
}
