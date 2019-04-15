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

#ifndef FUNAPI_UE4_PLATFORM_PS4

#include "funapi_plugin_ue4.h"
#include "AutomationTest.h"

#include <google/protobuf/message.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif
#include <sstream>
#include <fstream>
#include <algorithm>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include "unittest.pb.h"
#include "test_util.h"

#include <google/protobuf/stubs/common.h>
#include "googletest.h"
// #include <gtest/gtest.h>

#ifndef O_BINARY
#ifdef _O_BINARY
#define O_BINARY _O_BINARY
#else
#define O_BINARY 0     // If this isn't defined, the platform doesn't need it.
#endif
#endif

void ExpectMessageMerged(const google::protobuf::unittest::TestAllTypes& message) {
  verify(3 == message.optional_int32());
  verify(2 == message.optional_int64());
  verify("hello" == message.optional_string());
}

void AssignParsingMergeMessages(
  google::protobuf::unittest::TestAllTypes* msg1,
  google::protobuf::unittest::TestAllTypes* msg2,
  google::protobuf::unittest::TestAllTypes* msg3) {
  msg1->set_optional_int32(1);
  msg2->set_optional_int64(2);
  msg3->set_optional_int32(3);
  msg3->set_optional_string("hello");
}

static std::string TestSourceDir() {
  return std::string(TCHAR_TO_UTF8(*(FPaths::ProjectSavedDir()))) + "../ThirdParty";
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFunapiLibProtobufMessageTest, "LibProtobuf.MessageTest", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFunapiLibProtobufMessageTest::RunTest(const FString& Parameters)
{
  // TEST(MessageTest, SerializeHelpers)
  {
    // TODO(kenton):  Test more helpers?  They're all two-liners so it seems
    //   like a waste of time.

    protobuf_unittest::TestAllTypes message;
    google::protobuf::TestUtil::SetAllFields(&message);
    std::stringstream stream;

    std::string str1("foo");
    std::string str2("bar");

    verify(message.SerializeToString(&str1));
    verify(message.AppendToString(&str2));
    verify(message.SerializeToOstream(&stream));

    verify(str1.size() + 3 == str2.size());
    verify("bar" == str2.substr(0, 3));
    // Don't use EXPECT_EQ because we don't want to dump raw binary data to
    // stdout.
    verify(str2.substr(3) == str1);

    // GCC gives some sort of error if we try to just do stream.str() == str1.
    std::string temp = stream.str();
    verify(temp == str1);

    verify(message.SerializeAsString() == str1);

  }

  // TEST(MessageTest, SerializeToBrokenOstream)
  {
    std::ofstream out;
    protobuf_unittest::TestAllTypes message;
    message.set_optional_int32(123);

    verify(false == message.SerializeToOstream(&out));
  }

  // TEST(MessageTest, ParseFromFileDescriptor)
  {
    std::string filename = TestSourceDir() +
      "/google/protobuf/testdata/golden_message";
    int file = open(filename.c_str(), O_RDONLY | O_BINARY);

    google::protobuf::unittest::TestAllTypes message;
    verify(message.ParseFromFileDescriptor(file));
    google::protobuf::TestUtil::ExpectAllFieldsSet(message);

    verify(close(file) >= 0);
  }

  // TEST(MessageTest, ParsePackedFromFileDescriptor)
  {
    std::string filename =
      TestSourceDir() +
      "/google/protobuf/testdata/golden_packed_fields_message";

    int file = open(filename.c_str(), O_RDONLY | O_BINARY);

    google::protobuf::unittest::TestPackedTypes message;
    verify(message.ParseFromFileDescriptor(file));
    google::protobuf::TestUtil::ExpectPackedFieldsSet(message);

    verify(close(file) >= 0);
  }

  // TEST(MessageTest, ParseHelpers)
  {
    // TODO(kenton):  Test more helpers?  They're all two-liners so it seems
    //   like a waste of time.
    std::string data;

    {
      // Set up.
      protobuf_unittest::TestAllTypes message;
      google::protobuf::TestUtil::SetAllFields(&message);
      message.SerializeToString(&data);
    }

    {
      // Test ParseFromString.
      protobuf_unittest::TestAllTypes message;
      verify(message.ParseFromString(data));
      google::protobuf::TestUtil::ExpectAllFieldsSet(message);
    }

    {
      // Test ParseFromIstream.
      protobuf_unittest::TestAllTypes message;
      std::stringstream stream(data);
      verify(message.ParseFromIstream(&stream));
      verify(stream.eof());
      google::protobuf::TestUtil::ExpectAllFieldsSet(message);
    }

    {
      // Test ParseFromBoundedZeroCopyStream.
      std::string data_with_junk(data);
      data_with_junk.append("some junk on the end");
      google::protobuf::io::ArrayInputStream stream(data_with_junk.data(), data_with_junk.size());
      protobuf_unittest::TestAllTypes message;
      verify(message.ParseFromBoundedZeroCopyStream(&stream, data.size()));
      google::protobuf::TestUtil::ExpectAllFieldsSet(message);
    }

    {
      // Test that ParseFromBoundedZeroCopyStream fails (but doesn't crash) if
      // EOF is reached before the expected number of bytes.
      google::protobuf::io::ArrayInputStream stream(data.data(), data.size());
      protobuf_unittest::TestAllTypes message;
      verify(false ==
        message.ParseFromBoundedZeroCopyStream(&stream, data.size() + 1));
    }
  }

  // TEST(MessageTest, ParseFailsIfNotInitialized)
  {
    google::protobuf::unittest::TestRequired message;
    std::vector<std::string> errors;

    {
      google::protobuf::ScopedMemoryLog log;
      verify(false == message.ParseFromString(""));
      errors = log.GetMessages(google::protobuf::ERROR);
    }

    verify(1 == errors.size());
    verify("Can't parse message of type \"protobuf_unittest.TestRequired\" "
      "because it is missing required fields: a, b, c" ==
      errors[0]);
  }

  // TEST(MessageTest, BypassInitializationCheckOnParse)
  {
    google::protobuf::unittest::TestRequired message;
    google::protobuf::io::ArrayInputStream raw_input(NULL, 0);
    google::protobuf::io::CodedInputStream input(&raw_input);
    verify(message.MergePartialFromCodedStream(&input));
  }

  // TEST(MessageTest, InitializationErrorString)
  {
    google::protobuf::unittest::TestRequired message;
    verify("a, b, c" == message.InitializationErrorString());
  }

  // TEST(MessageTest, BypassInitializationCheckOnSerialize)
  {
    google::protobuf::unittest::TestRequired message;
    google::protobuf::io::ArrayOutputStream raw_output(NULL, 0);
    google::protobuf::io::CodedOutputStream output(&raw_output);
    verify(message.SerializePartialToCodedStream(&output));
  }

  // TEST(MessageTest, FindInitializationErrors)
  {
    google::protobuf::unittest::TestRequired message;
    std::vector<std::string> errors;
    message.FindInitializationErrors(&errors);
    verify(3 == errors.size());
    verify("a" == errors[0]);
    verify("b" == errors[1]);
    verify("c" == errors[2]);
  }

  // TEST(MessageTest, ParseFailsOnInvalidMessageEnd)
  {
    google::protobuf::unittest::TestAllTypes message;

    // Control case.
    verify(message.ParseFromArray("", 0));

    // The byte is a valid varint, but not a valid tag (zero).
    verify(false == message.ParseFromArray("\0", 1));

    // The byte is a malformed varint.
    verify(false == message.ParseFromArray("\200", 1));

    // The byte is an endgroup tag, but we aren't parsing a group.
    verify(false == message.ParseFromArray("\014", 1));
  }

  // Test that if an optional or required message/group field appears multiple
  // times in the input, they need to be merged.
  // TEST(MessageTest, ParsingMerge)
  {
    google::protobuf::unittest::TestParsingMerge::RepeatedFieldsGenerator generator;
    google::protobuf::unittest::TestAllTypes* msg1;
    google::protobuf::unittest::TestAllTypes* msg2;
    google::protobuf::unittest::TestAllTypes* msg3;

#define ASSIGN_REPEATED_FIELD(FIELD)                \
  msg1 = generator.add_##FIELD();                   \
  msg2 = generator.add_##FIELD();                   \
  msg3 = generator.add_##FIELD();                   \
  AssignParsingMergeMessages(msg1, msg2, msg3)

    ASSIGN_REPEATED_FIELD(field1);
    ASSIGN_REPEATED_FIELD(field2);
    ASSIGN_REPEATED_FIELD(field3);
    ASSIGN_REPEATED_FIELD(ext1);
    ASSIGN_REPEATED_FIELD(ext2);

#undef ASSIGN_REPEATED_FIELD
#define ASSIGN_REPEATED_GROUP(FIELD)                \
  msg1 = generator.add_##FIELD()->mutable_field1(); \
  msg2 = generator.add_##FIELD()->mutable_field1(); \
  msg3 = generator.add_##FIELD()->mutable_field1(); \
  AssignParsingMergeMessages(msg1, msg2, msg3)

    ASSIGN_REPEATED_GROUP(group1);
    ASSIGN_REPEATED_GROUP(group2);

#undef ASSIGN_REPEATED_GROUP

    std::string buffer;
    generator.SerializeToString(&buffer);
    google::protobuf::unittest::TestParsingMerge parsing_merge;
    parsing_merge.ParseFromString(buffer);

    // Required and optional fields should be merged.
    ExpectMessageMerged(parsing_merge.required_all_types());
    ExpectMessageMerged(parsing_merge.optional_all_types());
    ExpectMessageMerged(
      parsing_merge.optionalgroup().optional_group_all_types());
    ExpectMessageMerged(
      parsing_merge.GetExtension(google::protobuf::unittest::TestParsingMerge::optional_ext));

    // Repeated fields should not be merged.
    verify(3 == parsing_merge.repeated_all_types_size());
    verify(3 == parsing_merge.repeatedgroup_size());
    verify(3 == parsing_merge.ExtensionSize(
      google::protobuf::unittest::TestParsingMerge::repeated_ext));
  }

  // TEST(MessageTest, MergeFrom)
  {
    google::protobuf::unittest::TestAllTypes source;
    google::protobuf::unittest::TestAllTypes dest;

    // Optional fields
    source.set_optional_int32(1);  // only source
    source.set_optional_int64(2);  // both source and dest
    dest.set_optional_int64(3);
    dest.set_optional_uint32(4);   // only dest

                                   // Optional fields with defaults
    source.set_default_int32(13);  // only source
    source.set_default_int64(14);  // both source and dest
    dest.set_default_int64(15);
    dest.set_default_uint32(16);   // only dest

                                   // Repeated fields
    source.add_repeated_int32(5);  // only source
    source.add_repeated_int32(6);
    source.add_repeated_int64(7);  // both source and dest
    source.add_repeated_int64(8);
    dest.add_repeated_int64(9);
    dest.add_repeated_int64(10);
    dest.add_repeated_uint32(11);  // only dest
    dest.add_repeated_uint32(12);

    dest.MergeFrom(source);

    // Optional fields: source overwrites dest if source is specified
    verify(1 == dest.optional_int32());  // only source: use source
    verify(2 == dest.optional_int64());  // source and dest: use source
    verify(4 == dest.optional_uint32());  // only dest: use dest
    verify(0 == dest.optional_uint64());  // neither: use default

                                           // Optional fields with defaults
    verify(13 == dest.default_int32());  // only source: use source
    verify(14 == dest.default_int64());  // source and dest: use source
    verify(16 == dest.default_uint32());  // only dest: use dest
    verify(44 == dest.default_uint64());  // neither: use default

                                           // Repeated fields: concatenate source onto the end of dest
    verify(2 == dest.repeated_int32_size());
    verify(5 == dest.repeated_int32(0));
    verify(6 == dest.repeated_int32(1));
    verify(4 == dest.repeated_int64_size());
    verify(9 == dest.repeated_int64(0));
    verify(10 == dest.repeated_int64(1));
    verify(7 == dest.repeated_int64(2));
    verify(8 == dest.repeated_int64(3));
    verify(2 == dest.repeated_uint32_size());
    verify(11 == dest.repeated_uint32(0));
    verify(12 == dest.repeated_uint32(1));
    verify(0 == dest.repeated_uint64_size());
  }

  // TEST(MessageFactoryTest, GeneratedFactoryLookup)
  {
    verify(
      google::protobuf::MessageFactory::generated_factory()->GetPrototype(
        protobuf_unittest::TestAllTypes::descriptor()) ==
      &protobuf_unittest::TestAllTypes::default_instance());
  }

  // TEST(MessageFactoryTest, GeneratedFactoryUnknownType)
  {
    // Construct a new descriptor.
    google::protobuf::DescriptorPool pool;
    google::protobuf::FileDescriptorProto file;
    file.set_name("foo.proto");
    file.add_message_type()->set_name("Foo");
    const google::protobuf::Descriptor* descriptor = pool.BuildFile(file)->message_type(0);

    // Trying to construct it should return NULL.
    verify(
      google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor) == NULL);
  }

  return true;
}

#endif
