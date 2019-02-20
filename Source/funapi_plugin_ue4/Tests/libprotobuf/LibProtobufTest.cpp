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

#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/substitute.h>
#include <google/protobuf/stubs/stl_util.h>

#include <google/protobuf/stubs/common.h>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/dynamic_message.h>

#include <google/protobuf/stubs/stringprintf.h>

#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/wire_format_lite_inl.h>

#include "googletest.h"
#include "unittest.pb.h"
#include "unittest_custom_options.pb.h"
#include "unittest_import.pb.h"
#include "test_util.h"
#include "unittest_mset.pb.h"
// #include "test_util_lite.h"

#include <sstream>
#include <string>
#include <functional>
#include <algorithm>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFunapiLibProtobufEncodedDescriptorDatabaseExtraTest, "LibProtobuf.EncodedDescriptorDatabaseExtraTest", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFunapiLibProtobufEncodedDescriptorDatabaseExtraTest::RunTest(const FString& Parameters)
{
  class DescriptorDatabaseTestCase {
  public:
    virtual ~DescriptorDatabaseTestCase() {}

    virtual google::protobuf::DescriptorDatabase* GetDatabase() = 0;
    virtual bool AddToDatabase(const google::protobuf::FileDescriptorProto& file) = 0;
  };

  // Factory function type.
  typedef DescriptorDatabaseTestCase* DescriptorDatabaseTestCaseFactory();

  // Specialization for SimpleDescriptorDatabase.
  class SimpleDescriptorDatabaseTestCase : public DescriptorDatabaseTestCase {
  public:
    static DescriptorDatabaseTestCase* New() {
      return new SimpleDescriptorDatabaseTestCase;
    }

    virtual ~SimpleDescriptorDatabaseTestCase() {}

    virtual google::protobuf::DescriptorDatabase* GetDatabase() {
      return &database_;
    }
    virtual bool AddToDatabase(const google::protobuf::FileDescriptorProto& file) {
      return database_.Add(file);
    }

  private:
    google::protobuf::SimpleDescriptorDatabase database_;
  };

  // Specialization for EncodedDescriptorDatabase.
  class EncodedDescriptorDatabaseTestCase : public DescriptorDatabaseTestCase {
  public:
    static DescriptorDatabaseTestCase* New() {
      return new EncodedDescriptorDatabaseTestCase;
    }

    virtual ~EncodedDescriptorDatabaseTestCase() {}

    virtual google::protobuf::DescriptorDatabase* GetDatabase() {
      return &database_;
    }
    virtual bool AddToDatabase(const google::protobuf::FileDescriptorProto& file) {
      fun::string data;
      file.SerializeToString(&data);
      return database_.AddCopy(data.data(), data.size());
    }

  private:
    google::protobuf::EncodedDescriptorDatabase database_;
  };

  // Specialization for DescriptorPoolDatabase.
  class DescriptorPoolDatabaseTestCase : public DescriptorDatabaseTestCase {
  public:
    static DescriptorDatabaseTestCase* New() {
      return new EncodedDescriptorDatabaseTestCase;
    }

    DescriptorPoolDatabaseTestCase() : database_(pool_) {}
    virtual ~DescriptorPoolDatabaseTestCase() {}

    virtual google::protobuf::DescriptorDatabase* GetDatabase() {
      return &database_;
    }
    virtual bool AddToDatabase(const google::protobuf::FileDescriptorProto& file) {
      return pool_.BuildFile(file);
    }

  private:
    google::protobuf::DescriptorPool pool_;
    google::protobuf::DescriptorPoolDatabase database_;
  };

  auto ExpectContainsType = [](
    const google::protobuf::FileDescriptorProto& proto,
    const fun::string& type_name) ->bool
  {
    for (int i = 0; i < proto.message_type_size(); i++) {
      if (proto.message_type(i).name() == type_name) return true;
    }

    fun::stringstream ss;
    ss << "\"" << proto.name()
      << "\" did not contain expected type \""
      << type_name << "\".";

    UE_LOG(LogFunapiExample, Log, TEXT("%s"), *FString(ss.str().c_str()));
    return false;
  };

  google::protobuf::DescriptorDatabase* database_;
  for (int i = 0; i <= 2; ++i) {

    google::protobuf::internal::scoped_ptr<DescriptorDatabaseTestCase> test_case_;

    // SetUp
    auto SetUp = [&test_case_, &database_](int index) {
      switch (index) {
      case 0:
        test_case_.reset(SimpleDescriptorDatabaseTestCase::New());
        break;
      case 1:
        test_case_.reset(EncodedDescriptorDatabaseTestCase::New());
        break;
      case 2:
        test_case_.reset(DescriptorPoolDatabaseTestCase::New());
        break;
      default:
        verify(false);
        break;
      }
      database_ = test_case_->GetDatabase();
    };

    auto AddToDatabase = [&test_case_](const char* file_descriptor_text) {
      google::protobuf::FileDescriptorProto file_proto;
      verify(google::protobuf::TextFormat::ParseFromString(file_descriptor_text, &file_proto));
      verify(test_case_->AddToDatabase(file_proto));
    };

    auto AddToDatabaseWithError = [&test_case_](const char* file_descriptor_text) {
      google::protobuf::FileDescriptorProto file_proto;
      verify(google::protobuf::TextFormat::ParseFromString(file_descriptor_text, &file_proto));
      verify(false == test_case_->AddToDatabase(file_proto));
    };

    // FindFileByName
    SetUp(i);
    AddToDatabase(
      "name: \"foo.proto\" "
      "message_type { name:\"Foo\" }");
    AddToDatabase(
      "name: \"bar.proto\" "
      "message_type { name:\"Bar\" }");

    {
      google::protobuf::FileDescriptorProto file;
      verify(database_->FindFileByName("foo.proto", &file));
      verify("foo.proto" == file.name());
      verify(ExpectContainsType(file, "Foo"));
    }

    {
      google::protobuf::FileDescriptorProto file;
      verify(database_->FindFileByName("bar.proto", &file));
      verify("bar.proto" == file.name());
      verify(ExpectContainsType(file, "Bar"));
    }

    {
      // Fails to find undefined files.
      google::protobuf::FileDescriptorProto file;
      verify(false == database_->FindFileByName("baz.proto", &file));
    }
    // // // //

    // FindFileContainingSymbol
    SetUp(i);
    AddToDatabase(
      "name: \"foo.proto\" "
      "message_type { "
      "  name: \"Foo\" "
      "  field { name:\"qux\" }"
      "  nested_type { name: \"Grault\" } "
      "  enum_type { name: \"Garply\" } "
      "} "
      "enum_type { "
      "  name: \"Waldo\" "
      "  value { name:\"FRED\" } "
      "} "
      "extension { name: \"plugh\" } "
      "service { "
      "  name: \"Xyzzy\" "
      "  method { name: \"Thud\" } "
      "}"
    );
    AddToDatabase(
      "name: \"bar.proto\" "
      "package: \"corge\" "
      "message_type { name: \"Bar\" }");

    {
      google::protobuf::FileDescriptorProto file;
      verify(database_->FindFileContainingSymbol("Foo", &file));
      verify("foo.proto" == file.name());
    }

    {
      // Can find fields.
      google::protobuf::FileDescriptorProto file;
      verify(database_->FindFileContainingSymbol("Foo.qux", &file));
      verify("foo.proto" == file.name());
    }

    {
      // Can find nested types.
      google::protobuf::FileDescriptorProto file;
      verify(database_->FindFileContainingSymbol("Foo.Grault", &file));
      verify("foo.proto" == file.name());
    }

    {
      // Can find nested enums.
      google::protobuf::FileDescriptorProto file;
      verify(database_->FindFileContainingSymbol("Foo.Garply", &file));
      verify("foo.proto" == file.name());
    }

    {
      // Can find enum types.
      google::protobuf::FileDescriptorProto file;
      verify(database_->FindFileContainingSymbol("Waldo", &file));
      verify("foo.proto" == file.name());
    }

    {
      // Can find enum values.
      google::protobuf::FileDescriptorProto file;
      verify(database_->FindFileContainingSymbol("Waldo.FRED", &file));
      verify("foo.proto" == file.name());
    }

    {
      // Can find extensions.
      google::protobuf::FileDescriptorProto file;
      verify(database_->FindFileContainingSymbol("plugh", &file));
      verify("foo.proto" == file.name());
    }

    {
      // Can find services.
      google::protobuf::FileDescriptorProto file;
      verify(database_->FindFileContainingSymbol("Xyzzy", &file));
      verify("foo.proto" == file.name());
    }

    {
      // Can find methods.
      google::protobuf::FileDescriptorProto file;
      verify(database_->FindFileContainingSymbol("Xyzzy.Thud", &file));
      verify("foo.proto" == file.name());
    }

    {
      // Can find things in packages.
      google::protobuf::FileDescriptorProto file;
      verify(database_->FindFileContainingSymbol("corge.Bar", &file));
      verify("bar.proto" == file.name());
    }

    {
      // Fails to find undefined symbols.
      google::protobuf::FileDescriptorProto file;
      verify(false == database_->FindFileContainingSymbol("Baz", &file));
    }

    {
      // Names must be fully-qualified.
      google::protobuf::FileDescriptorProto file;
      verify(false == database_->FindFileContainingSymbol("Bar", &file));
    }
    // // // //

    // FindFileContainingExtension
    SetUp(i);
    AddToDatabase(
      "name: \"foo.proto\" "
      "message_type { "
      "  name: \"Foo\" "
      "  extension_range { start: 1 end: 1000 } "
      "  extension { name:\"qux\" label:LABEL_OPTIONAL type:TYPE_INT32 number:5 "
      "              extendee: \".Foo\" }"
      "}");
    AddToDatabase(
      "name: \"bar.proto\" "
      "package: \"corge\" "
      "dependency: \"foo.proto\" "
      "message_type { "
      "  name: \"Bar\" "
      "  extension_range { start: 1 end: 1000 } "
      "} "
      "extension { name:\"grault\" extendee: \".Foo\"       number:32 } "
      "extension { name:\"garply\" extendee: \".corge.Bar\" number:70 } "
      "extension { name:\"waldo\"  extendee: \"Bar\"        number:56 } ");

    {
      google::protobuf::FileDescriptorProto file;
      verify(database_->FindFileContainingExtension("Foo", 5, &file));
      verify("foo.proto" == file.name());
    }

    {
      google::protobuf::FileDescriptorProto file;
      verify(database_->FindFileContainingExtension("Foo", 32, &file));
      verify("bar.proto" == file.name());
    }

    {
      // Can find extensions for qualified type names.
      google::protobuf::FileDescriptorProto file;
      verify(database_->FindFileContainingExtension("corge.Bar", 70, &file));
      verify("bar.proto" == file.name());
    }

    {
      // Can't find extensions whose extendee was not fully-qualified in the
      // FileDescriptorProto.
      google::protobuf::FileDescriptorProto file;
      verify(false == database_->FindFileContainingExtension("Bar", 56, &file));
      verify(
        false == database_->FindFileContainingExtension("corge.Bar", 56, &file));
    }

    {
      // Can't find non-existent extension numbers.
      google::protobuf::FileDescriptorProto file;
      verify(false == database_->FindFileContainingExtension("Foo", 12, &file));
    }

    {
      // Can't find extensions for non-existent types.
      google::protobuf::FileDescriptorProto file;
      verify(
        false == database_->FindFileContainingExtension("NoSuchType", 5, &file));
    }

    {
      // Can't find extensions for unqualified type names.
      google::protobuf::FileDescriptorProto file;
      verify(false == database_->FindFileContainingExtension("Bar", 70, &file));
    }
    // // // //

    // FindAllExtensionNumbers
    SetUp(i);
    AddToDatabase(
      "name: \"foo.proto\" "
      "message_type { "
      "  name: \"Foo\" "
      "  extension_range { start: 1 end: 1000 } "
      "  extension { name:\"qux\" label:LABEL_OPTIONAL type:TYPE_INT32 number:5 "
      "              extendee: \".Foo\" }"
      "}");
    AddToDatabase(
      "name: \"bar.proto\" "
      "package: \"corge\" "
      "dependency: \"foo.proto\" "
      "message_type { "
      "  name: \"Bar\" "
      "  extension_range { start: 1 end: 1000 } "
      "} "
      "extension { name:\"grault\" extendee: \".Foo\"       number:32 } "
      "extension { name:\"garply\" extendee: \".corge.Bar\" number:70 } "
      "extension { name:\"waldo\"  extendee: \"Bar\"        number:56 } ");

    {
      fun::vector<int> numbers;
      verify(database_->FindAllExtensionNumbers("Foo", &numbers));
      verify(2 == numbers.size());
      sort(numbers.begin(), numbers.end());
      verify(5 == numbers[0]);
      verify(32 == numbers[1]);
    }

    {
      fun::vector<int> numbers;
      verify(database_->FindAllExtensionNumbers("corge.Bar", &numbers));
      // Note: won't find extension 56 due to the name not being fully qualified.
      verify(1 == numbers.size());
      verify(70 == numbers[0]);
    }

    {
      // Can't find extensions for non-existent types.
      fun::vector<int> numbers;
      verify(false == database_->FindAllExtensionNumbers("NoSuchType", &numbers));
    }

    {
      // Can't find extensions for unqualified types.
      fun::vector<int> numbers;
      verify(false == database_->FindAllExtensionNumbers("Bar", &numbers));
    }
    // // // //

    // ConflictingFileError
    SetUp(i);
    AddToDatabase(
      "name: \"foo.proto\" "
      "message_type { "
      "  name: \"Foo\" "
      "}");
    AddToDatabaseWithError(
      "name: \"foo.proto\" "
      "message_type { "
      "  name: \"Bar\" "
      "}");

    // ConflictingTypeError
    SetUp(i);
    AddToDatabase(
      "name: \"foo.proto\" "
      "message_type { "
      "  name: \"Foo\" "
      "}");
    AddToDatabaseWithError(
      "name: \"bar.proto\" "
      "message_type { "
      "  name: \"Foo\" "
      "}");

    // ConflictingExtensionError
    SetUp(i);
    AddToDatabase(
      "name: \"foo.proto\" "
      "extension { name:\"foo\" label:LABEL_OPTIONAL type:TYPE_INT32 number:5 "
      "            extendee: \".Foo\" }");
    AddToDatabaseWithError(
      "name: \"bar.proto\" "
      "extension { name:\"bar\" label:LABEL_OPTIONAL type:TYPE_INT32 number:5 "
      "            extendee: \".Foo\" }");
  }
  // // // //

  // Create two files, one of which is in two parts.
  google::protobuf::FileDescriptorProto file1, file2a, file2b;
  file1.set_name("foo.proto");
  file1.set_package("foo");
  file1.add_message_type()->set_name("Foo");
  file2a.set_name("bar.proto");
  file2b.set_package("bar");
  file2b.add_message_type()->set_name("Bar");

  // Normal serialization allows our optimization to kick in.
  fun::string data1 = file1.SerializeAsString();

  // Force out-of-order serialization to test slow path.
  fun::string data2 = file2b.SerializeAsString() + file2a.SerializeAsString();

  // Create EncodedDescriptorDatabase containing both files.
  google::protobuf::EncodedDescriptorDatabase db;
  db.Add(data1.data(), data1.size());
  db.Add(data2.data(), data2.size());

  // Test!
  fun::string filename;
  verify(db.FindNameOfFileContainingSymbol("foo.Foo", &filename));
  verify("foo.proto" == filename);
  verify(db.FindNameOfFileContainingSymbol("foo.Foo.Blah", &filename));
  verify("foo.proto" == filename);
  verify(db.FindNameOfFileContainingSymbol("bar.Bar", &filename));
  verify("bar.proto" == filename);
  verify(db.FindNameOfFileContainingSymbol("foo", &filename) == false);
  verify(db.FindNameOfFileContainingSymbol("bar", &filename) == false);
  verify(db.FindNameOfFileContainingSymbol("baz.Baz", &filename) == false);

  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFunapiLibProtobufReflectionOpsTest, "LibProtobuf.ReflectionOpsTest", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFunapiLibProtobufReflectionOpsTest::RunTest(const FString& Parameters)
{
  // TEST(ReflectionOpsTest, SanityCheck)
  {
    google::protobuf::unittest::TestAllTypes message;

    google::protobuf::TestUtil::SetAllFields(&message);
    google::protobuf::TestUtil::ExpectAllFieldsSet(message);
  }

  // TEST(ReflectionOpsTest, Copy)
  {
    google::protobuf::unittest::TestAllTypes message, message2;

    google::protobuf::TestUtil::SetAllFields(&message);

    google::protobuf::internal::ReflectionOps::Copy(message, &message2);

    google::protobuf::TestUtil::ExpectAllFieldsSet(message2);

    // Copying from self should be a no-op.
    google::protobuf::internal::ReflectionOps::Copy(message2, &message2);
    google::protobuf::TestUtil::ExpectAllFieldsSet(message2);
  }

  // TEST(ReflectionOpsTest, CopyExtensions)
  {
    google::protobuf::unittest::TestAllExtensions message, message2;

    google::protobuf::TestUtil::SetAllExtensions(&message);

    google::protobuf::internal::ReflectionOps::Copy(message, &message2);

    google::protobuf::TestUtil::ExpectAllExtensionsSet(message2);
  }

  // TEST(ReflectionOpsTest, CopyOneof)
  {
    google::protobuf::unittest::TestOneof2 message, message2;
    google::protobuf::TestUtil::SetOneof1(&message);
    google::protobuf::internal::ReflectionOps::Copy(message, &message2);
    google::protobuf::TestUtil::ExpectOneofSet1(message2);

    google::protobuf::TestUtil::SetOneof2(&message);
    google::protobuf::TestUtil::ExpectOneofSet2(message);
    google::protobuf::internal::ReflectionOps::Copy(message, &message2);
    google::protobuf::TestUtil::ExpectOneofSet2(message2);
  }

  // TEST(ReflectionOpsTest, Merge)
  {
    // Note:  Copy is implemented in terms of Merge() so technically the Copy
    //   test already tested most of this.

    google::protobuf::unittest::TestAllTypes message, message2;

    google::protobuf::TestUtil::SetAllFields(&message);

    // This field will test merging into an empty spot.
    message2.set_optional_int32(message.optional_int32());
    message.clear_optional_int32();

    // This tests overwriting.
    message2.set_optional_string(message.optional_string());
    message.set_optional_string("something else");

    // This tests concatenating.
    message2.add_repeated_int32(message.repeated_int32(1));
    int32 i = message.repeated_int32(0);
    message.clear_repeated_int32();
    message.add_repeated_int32(i);

    google::protobuf::internal::ReflectionOps::Merge(message2, &message);

    google::protobuf::TestUtil::ExpectAllFieldsSet(message);
  }

  // TEST(ReflectionOpsTest, MergeExtensions)
  {
    // Note:  Copy is implemented in terms of Merge() so technically the Copy
    //   test already tested most of this.

    google::protobuf::unittest::TestAllExtensions message, message2;

    google::protobuf::TestUtil::SetAllExtensions(&message);

    // This field will test merging into an empty spot.
    message2.SetExtension(google::protobuf::unittest::optional_int32_extension,
      message.GetExtension(google::protobuf::unittest::optional_int32_extension));
    message.ClearExtension(google::protobuf::unittest::optional_int32_extension);

    // This tests overwriting.
    message2.SetExtension(google::protobuf::unittest::optional_string_extension,
      message.GetExtension(google::protobuf::unittest::optional_string_extension));
    message.SetExtension(google::protobuf::unittest::optional_string_extension, "something else");

    // This tests concatenating.
    message2.AddExtension(google::protobuf::unittest::repeated_int32_extension,
      message.GetExtension(google::protobuf::unittest::repeated_int32_extension, 1));
    int32 i = message.GetExtension(google::protobuf::unittest::repeated_int32_extension, 0);
    message.ClearExtension(google::protobuf::unittest::repeated_int32_extension);
    message.AddExtension(google::protobuf::unittest::repeated_int32_extension, i);

    google::protobuf::internal::ReflectionOps::Merge(message2, &message);

    google::protobuf::TestUtil::ExpectAllExtensionsSet(message);
  }

  // TEST(ReflectionOpsTest, MergeUnknown)
  {
    // Test that the messages' UnknownFieldSets are correctly merged.
    google::protobuf::unittest::TestEmptyMessage message1, message2;
    message1.mutable_unknown_fields()->AddVarint(1234, 1);
    message2.mutable_unknown_fields()->AddVarint(1234, 2);

    google::protobuf::internal::ReflectionOps::Merge(message2, &message1);

    verify(2 == message1.unknown_fields().field_count());
    verify(google::protobuf::UnknownField::TYPE_VARINT ==
      message1.unknown_fields().field(0).type());
    verify(1 == message1.unknown_fields().field(0).varint());
    verify(google::protobuf::UnknownField::TYPE_VARINT ==
      message1.unknown_fields().field(1).type());
    verify(2 == message1.unknown_fields().field(1).varint());
  }

  // TEST(ReflectionOpsTest, MergeOneof)
  {
    google::protobuf::unittest::TestOneof2 message1, message2;
    google::protobuf::TestUtil::SetOneof1(&message1);

    // Merge to empty message
    google::protobuf::internal::ReflectionOps::Merge(message1, &message2);
    google::protobuf::TestUtil::ExpectOneofSet1(message2);

    // Merge with the same oneof fields
    google::protobuf::internal::ReflectionOps::Merge(message1, &message2);
    google::protobuf::TestUtil::ExpectOneofSet1(message2);

    // Merge with different oneof fields
    google::protobuf::TestUtil::SetOneof2(&message1);
    google::protobuf::internal::ReflectionOps::Merge(message1, &message2);
    google::protobuf::TestUtil::ExpectOneofSet2(message2);
  }

  // TEST(ReflectionOpsTest, Clear)
  {
    google::protobuf::unittest::TestAllTypes message;

    google::protobuf::TestUtil::SetAllFields(&message);

    google::protobuf::internal::ReflectionOps::Clear(&message);

    google::protobuf::TestUtil::ExpectClear(message);

    // Check that getting embedded messages returns the objects created during
    // SetAllFields() rather than default instances.
    verify(&google::protobuf::unittest::TestAllTypes::OptionalGroup::default_instance() !=
      &message.optionalgroup());
    verify(&google::protobuf::unittest::TestAllTypes::NestedMessage::default_instance() !=
      &message.optional_nested_message());
    verify(&google::protobuf::unittest::ForeignMessage::default_instance() !=
      &message.optional_foreign_message());
    verify(&google::protobuf::unittest_import::ImportMessage::default_instance() !=
      &message.optional_import_message());
  }

  // TEST(ReflectionOpsTest, ClearExtensions)
  {
    google::protobuf::unittest::TestAllExtensions message;

    google::protobuf::TestUtil::SetAllExtensions(&message);

    google::protobuf::internal::ReflectionOps::Clear(&message);

    google::protobuf::TestUtil::ExpectExtensionsClear(message);

    // Check that getting embedded messages returns the objects created during
    // SetAllExtensions() rather than default instances.
    verify(&google::protobuf::unittest::OptionalGroup_extension::default_instance() !=
      &message.GetExtension(google::protobuf::unittest::optionalgroup_extension));
    verify(&google::protobuf::unittest::TestAllTypes::NestedMessage::default_instance() !=
      &message.GetExtension(google::protobuf::unittest::optional_nested_message_extension));
    verify(&google::protobuf::unittest::ForeignMessage::default_instance() !=
      &message.GetExtension(
        google::protobuf::unittest::optional_foreign_message_extension));
    verify(&google::protobuf::unittest_import::ImportMessage::default_instance() !=
      &message.GetExtension(google::protobuf::unittest::optional_import_message_extension));
  }

  // TEST(ReflectionOpsTest, ClearUnknown)
  {
    // Test that the message's UnknownFieldSet is correctly cleared.
    google::protobuf::unittest::TestEmptyMessage message;
    message.mutable_unknown_fields()->AddVarint(1234, 1);

    google::protobuf::internal::ReflectionOps::Clear(&message);

    verify(0 == message.unknown_fields().field_count());
  }

  // TEST(ReflectionOpsTest, ClearOneof)
  {
    google::protobuf::unittest::TestOneof2 message;

    google::protobuf::TestUtil::ExpectOneofClear(message);
    google::protobuf::TestUtil::SetOneof1(&message);
    google::protobuf::TestUtil::ExpectOneofSet1(message);
    google::protobuf::internal::ReflectionOps::Clear(&message);
    google::protobuf::TestUtil::ExpectOneofClear(message);

    google::protobuf::TestUtil::SetOneof1(&message);
    google::protobuf::TestUtil::ExpectOneofSet1(message);
    google::protobuf::TestUtil::SetOneof2(&message);
    google::protobuf::TestUtil::ExpectOneofSet2(message);
    google::protobuf::internal::ReflectionOps::Clear(&message);
    google::protobuf::TestUtil::ExpectOneofClear(message);
  }

  // TEST(ReflectionOpsTest, DiscardUnknownFields)
  {
    google::protobuf::unittest::TestAllTypes message;
    google::protobuf::TestUtil::SetAllFields(&message);

    // Set some unknown fields in message.
    message.mutable_unknown_fields()
      ->AddVarint(123456, 654321);
    message.mutable_optional_nested_message()
      ->mutable_unknown_fields()
      ->AddVarint(123456, 654321);
    message.mutable_repeated_nested_message(0)
      ->mutable_unknown_fields()
      ->AddVarint(123456, 654321);

    verify(1 == message.unknown_fields().field_count());
    verify(1 == message.optional_nested_message()
      .unknown_fields().field_count());
    verify(1 == message.repeated_nested_message(0)
      .unknown_fields().field_count());

    // Discard them.
    google::protobuf::internal::ReflectionOps::DiscardUnknownFields(&message);
    google::protobuf::TestUtil::ExpectAllFieldsSet(message);

    verify(0 == message.unknown_fields().field_count());
    verify(0 == message.optional_nested_message()
      .unknown_fields().field_count());
    verify(0 == message.repeated_nested_message(0)
      .unknown_fields().field_count());
  }

  // TEST(ReflectionOpsTest, DiscardUnknownExtensions)
  {
    google::protobuf::unittest::TestAllExtensions message;
    google::protobuf::TestUtil::SetAllExtensions(&message);

    // Set some unknown fields.
    message.mutable_unknown_fields()
      ->AddVarint(123456, 654321);
    message.MutableExtension(google::protobuf::unittest::optional_nested_message_extension)
      ->mutable_unknown_fields()
      ->AddVarint(123456, 654321);
    message.MutableExtension(google::protobuf::unittest::repeated_nested_message_extension, 0)
      ->mutable_unknown_fields()
      ->AddVarint(123456, 654321);

    verify(1 == message.unknown_fields().field_count());
    verify(1 ==
      message.GetExtension(google::protobuf::unittest::optional_nested_message_extension)
      .unknown_fields().field_count());
    verify(1 ==
      message.GetExtension(google::protobuf::unittest::repeated_nested_message_extension, 0)
      .unknown_fields().field_count());

    // Discard them.
    google::protobuf::internal::ReflectionOps::DiscardUnknownFields(&message);
    google::protobuf::TestUtil::ExpectAllExtensionsSet(message);

    verify(0 == message.unknown_fields().field_count());
    verify(0 ==
      message.GetExtension(google::protobuf::unittest::optional_nested_message_extension)
      .unknown_fields().field_count());
    verify(0 ==
      message.GetExtension(google::protobuf::unittest::repeated_nested_message_extension, 0)
      .unknown_fields().field_count());
  }

  // TEST(ReflectionOpsTest, IsInitialized)
  {
    google::protobuf::unittest::TestRequired message;

    verify(false == google::protobuf::internal::ReflectionOps::IsInitialized(message));
    message.set_a(1);
    verify(false == google::protobuf::internal::ReflectionOps::IsInitialized(message));
    message.set_b(2);
    verify(false == google::protobuf::internal::ReflectionOps::IsInitialized(message));
    message.set_c(3);
    verify(google::protobuf::internal::ReflectionOps::IsInitialized(message));
  }

  // TEST(ReflectionOpsTest, ForeignIsInitialized)
  {
    google::protobuf::unittest::TestRequiredForeign message;

    // Starts out initialized because the foreign message is itself an optional
    // field.
    verify(google::protobuf::internal::ReflectionOps::IsInitialized(message));

    // Once we create that field, the message is no longer initialized.
    message.mutable_optional_message();
    verify(false == google::protobuf::internal::ReflectionOps::IsInitialized(message));

    // Initialize it.  Now we're initialized.
    message.mutable_optional_message()->set_a(1);
    message.mutable_optional_message()->set_b(2);
    message.mutable_optional_message()->set_c(3);
    verify(google::protobuf::internal::ReflectionOps::IsInitialized(message));

    // Add a repeated version of the message.  No longer initialized.
    google::protobuf::unittest::TestRequired* sub_message = message.add_repeated_message();
    verify(false == google::protobuf::internal::ReflectionOps::IsInitialized(message));

    // Initialize that repeated version.
    sub_message->set_a(1);
    sub_message->set_b(2);
    sub_message->set_c(3);
    verify(google::protobuf::internal::ReflectionOps::IsInitialized(message));
  }

  // TEST(ReflectionOpsTest, ExtensionIsInitialized)
  {
    google::protobuf::unittest::TestAllExtensions message;

    // Starts out initialized because the foreign message is itself an optional
    // field.
    verify(google::protobuf::internal::ReflectionOps::IsInitialized(message));

    // Once we create that field, the message is no longer initialized.
    message.MutableExtension(google::protobuf::unittest::TestRequired::single);
    verify(false == google::protobuf::internal::ReflectionOps::IsInitialized(message));

    // Initialize it.  Now we're initialized.
    message.MutableExtension(google::protobuf::unittest::TestRequired::single)->set_a(1);
    message.MutableExtension(google::protobuf::unittest::TestRequired::single)->set_b(2);
    message.MutableExtension(google::protobuf::unittest::TestRequired::single)->set_c(3);
    verify(google::protobuf::internal::ReflectionOps::IsInitialized(message));

    // Add a repeated version of the message.  No longer initialized.
    message.AddExtension(google::protobuf::unittest::TestRequired::multi);
    verify(false == google::protobuf::internal::ReflectionOps::IsInitialized(message));

    // Initialize that repeated version.
    message.MutableExtension(google::protobuf::unittest::TestRequired::multi, 0)->set_a(1);
    message.MutableExtension(google::protobuf::unittest::TestRequired::multi, 0)->set_b(2);
    message.MutableExtension(google::protobuf::unittest::TestRequired::multi, 0)->set_c(3);
    verify(google::protobuf::internal::ReflectionOps::IsInitialized(message));
  }

  // TEST(ReflectionOpsTest, OneofIsInitialized)
  {
    google::protobuf::unittest::TestRequiredOneof message;
    verify(google::protobuf::internal::ReflectionOps::IsInitialized(message));

    message.mutable_foo_message();
    verify(false == google::protobuf::internal::ReflectionOps::IsInitialized(message));

    message.set_foo_int(1);
    verify(google::protobuf::internal::ReflectionOps::IsInitialized(message));

    message.mutable_foo_message();
    verify(false == google::protobuf::internal::ReflectionOps::IsInitialized(message));
    message.mutable_foo_message()->set_required_double(0.1);
    verify(google::protobuf::internal::ReflectionOps::IsInitialized(message));
  }

  auto FindInitializationErrors = [](const google::protobuf::Message& message) -> fun::string {
    fun::vector<fun::string> errors;
    google::protobuf::internal::ReflectionOps::FindInitializationErrors(message, "", &errors);
    return google::protobuf::Join(errors, ",");
  };

  // TEST(ReflectionOpsTest, FindInitializationErrors)
  {
    google::protobuf::unittest::TestRequired message;
    verify("a,b,c" == FindInitializationErrors(message));
  }

  // TEST(ReflectionOpsTest, FindForeignInitializationErrors)
  {
    google::protobuf::unittest::TestRequiredForeign message;
    message.mutable_optional_message();
    message.add_repeated_message();
    message.add_repeated_message();
    verify("optional_message.a,"
      "optional_message.b,"
      "optional_message.c,"
      "repeated_message[0].a,"
      "repeated_message[0].b,"
      "repeated_message[0].c,"
      "repeated_message[1].a,"
      "repeated_message[1].b,"
      "repeated_message[1].c" ==
      FindInitializationErrors(message));
  }

  // TEST(ReflectionOpsTest, FindExtensionInitializationErrors)
  {
    google::protobuf::unittest::TestAllExtensions message;
    message.MutableExtension(google::protobuf::unittest::TestRequired::single);
    message.AddExtension(google::protobuf::unittest::TestRequired::multi);
    message.AddExtension(google::protobuf::unittest::TestRequired::multi);
    verify("(protobuf_unittest.TestRequired.single).a,"
      "(protobuf_unittest.TestRequired.single).b,"
      "(protobuf_unittest.TestRequired.single).c,"
      "(protobuf_unittest.TestRequired.multi)[0].a,"
      "(protobuf_unittest.TestRequired.multi)[0].b,"
      "(protobuf_unittest.TestRequired.multi)[0].c,"
      "(protobuf_unittest.TestRequired.multi)[1].a,"
      "(protobuf_unittest.TestRequired.multi)[1].b,"
      "(protobuf_unittest.TestRequired.multi)[1].c" ==
      FindInitializationErrors(message));
  }

  // TEST(ReflectionOpsTest, FindOneofInitializationErrors)
  {
    google::protobuf::unittest::TestRequiredOneof message;
    message.mutable_foo_message();
    verify("foo_message.required_double" ==
      FindInitializationErrors(message));
  }

  return true;
}

/*
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFunapiLibProtobufRepeatedFieldReflectionTest, "LibProtobuf.RepeatedFieldReflectionTest", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFunapiLibProtobufRepeatedFieldReflectionTest::RunTest(const FString& Parameters)
{
  // using namespace std;
  // using namespace google::protobuf;

  using google::protobuf::unittest::ForeignMessage;
  using google::protobuf::unittest::TestAllTypes;
  using google::protobuf::unittest::TestAllExtensions;

  auto Func = [](int i, int j) -> int {
    return i * j;
  };

  auto StrFunc = [&Func](int i, int j) -> fun::string {
    fun::string str;
    google::protobuf::SStringPrintf(&str, "%d", Func(i, 4));
    return str;
  };

  // TEST(RepeatedFieldReflectionTest, RegularFields)
  {
    TestAllTypes message;
    const google::protobuf::Reflection* refl = message.GetReflection();
    const google::protobuf::Descriptor* desc = message.GetDescriptor();

    for (int i = 0; i < 10; ++i) {
      message.add_repeated_int32(Func(i, 1));
      message.add_repeated_double(Func(i, 2));
      message.add_repeated_string(StrFunc(i, 5));
      message.add_repeated_foreign_message()->set_c(Func(i, 6));
    }

    // Get FieldDescriptors for all the fields of interest.
    const google::protobuf::FieldDescriptor* fd_repeated_int32 =
      desc->FindFieldByName("repeated_int32");
    const google::protobuf::FieldDescriptor* fd_repeated_double =
      desc->FindFieldByName("repeated_double");
    const google::protobuf::FieldDescriptor* fd_repeated_string =
      desc->FindFieldByName("repeated_string");
    const google::protobuf::FieldDescriptor* fd_repeated_foreign_message =
      desc->FindFieldByName("repeated_foreign_message");

    // Get RepeatedField objects for all fields of interest.
    const google::protobuf::RepeatedField<google::protobuf::int32>& rf_int32 =
      refl->GetRepeatedField<google::protobuf::int32>(message, fd_repeated_int32);
    const google::protobuf::RepeatedField<double>& rf_double =
      refl->GetRepeatedField<double>(message, fd_repeated_double);

    // Get mutable RepeatedField objects for all fields of interest.
    google::protobuf::RepeatedField<int32>* mrf_int32 =
      refl->MutableRepeatedField<int32>(&message, fd_repeated_int32);
    google::protobuf::RepeatedField<double>* mrf_double =
      refl->MutableRepeatedField<double>(&message, fd_repeated_double);

    // Get RepeatedPtrField objects for all fields of interest.
    const google::protobuf::RepeatedPtrField<fun::string>& rpf_string =
      refl->GetRepeatedPtrField<fun::string>(message, fd_repeated_string);
    const google::protobuf::RepeatedPtrField<ForeignMessage>& rpf_foreign_message =
      refl->GetRepeatedPtrField<ForeignMessage>(
        message, fd_repeated_foreign_message);
    const google::protobuf::RepeatedPtrField<google::protobuf::Message>& rpf_message =
      refl->GetRepeatedPtrField<google::protobuf::Message>(
        message, fd_repeated_foreign_message);

    // Get mutable RepeatedPtrField objects for all fields of interest.
    google::protobuf::RepeatedPtrField<fun::string>* mrpf_string =
      refl->MutableRepeatedPtrField<fun::string>(&message, fd_repeated_string);
    google::protobuf::RepeatedPtrField<ForeignMessage>* mrpf_foreign_message =
      refl->MutableRepeatedPtrField<ForeignMessage>(
        &message, fd_repeated_foreign_message);
    google::protobuf::RepeatedPtrField<google::protobuf::Message>* mrpf_message =
      refl->MutableRepeatedPtrField<google::protobuf::Message>(
        &message, fd_repeated_foreign_message);

    // Make sure we can do get and sets through the Repeated[Ptr]Field objects.
    for (int i = 0; i < 10; ++i) {
      // Check gets through const objects.
      verify(rf_int32.Get(i) == Func(i, 1));
      verify(rf_double.Get(i) == Func(i, 2));
      verify(rpf_string.Get(i) == StrFunc(i, 5));
      verify(rpf_foreign_message.Get(i).c() == Func(i, 6));
      verify(google::protobuf::down_cast<const ForeignMessage*>(&rpf_message.Get(i))->c() ==
        Func(i, 6));

      // Check gets through mutable objects.
      verify(mrf_int32->Get(i) == Func(i, 1));
      verify(mrf_double->Get(i) == Func(i, 2));
      verify(mrpf_string->Get(i) == StrFunc(i, 5));
      verify(mrpf_foreign_message->Get(i).c() == Func(i, 6));
      verify(google::protobuf::down_cast<const ForeignMessage*>(&mrpf_message->Get(i))->c() ==
        Func(i, 6));

      // Check sets through mutable objects.
      mrf_int32->Set(i, Func(i, -1));
      mrf_double->Set(i, Func(i, -2));
      mrpf_string->Mutable(i)->assign(StrFunc(i, -5));
      mrpf_foreign_message->Mutable(i)->set_c(Func(i, -6));
      verify(message.repeated_int32(i) == Func(i, -1));
      verify(message.repeated_double(i) == Func(i, -2));
      verify(message.repeated_string(i) == StrFunc(i, -5));
      verify(message.repeated_foreign_message(i).c() == Func(i, -6));
      google::protobuf::down_cast<ForeignMessage*>(mrpf_message->Mutable(i))->set_c(Func(i, 7));
      verify(message.repeated_foreign_message(i).c() == Func(i, 7));
    }
  }

  // TEST(RepeatedFieldReflectionTest, ExtensionFields)
  {
    TestAllExtensions extended_message;
    const google::protobuf::Reflection* refl = extended_message.GetReflection();
    const google::protobuf::Descriptor* desc = extended_message.GetDescriptor();

    for (int i = 0; i < 10; ++i) {
      extended_message.AddExtension(
        google::protobuf::unittest::repeated_int64_extension, Func(i, 1));
    }

    const google::protobuf::FieldDescriptor* fd_repeated_int64_extension =
      desc->file()->FindExtensionByName("repeated_int64_extension");
    GOOGLE_CHECK(fd_repeated_int64_extension != NULL);

    const google::protobuf::RepeatedField<google::protobuf::int64>& rf_int64_extension =
      refl->GetRepeatedField<google::protobuf::int64>(extended_message,
        fd_repeated_int64_extension);

    google::protobuf::RepeatedField<google::protobuf::int64>* mrf_int64_extension =
      refl->MutableRepeatedField<google::protobuf::int64>(&extended_message,
        fd_repeated_int64_extension);

    for (int i = 0; i < 10; ++i) {
      verify(Func(i, 1) == rf_int64_extension.Get(i));
      mrf_int64_extension->Set(i, Func(i, -1));
      verify(Func(i, -1) ==
        extended_message.GetExtension(google::protobuf::unittest::repeated_int64_extension, i));
    }
  }

  return true;
}
*/

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFunapiLibProtobufUnknownFieldSetTest, "LibProtobuf.UnknownFieldSetTest", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFunapiLibProtobufUnknownFieldSetTest::RunTest(const FString& Parameters)
{
  using google::protobuf::internal::WireFormat;

  const google::protobuf::Descriptor* descriptor_;
  google::protobuf::unittest::TestAllTypes all_fields_;
  fun::string all_fields_data_;

  // An empty message that has been parsed from all_fields_data_.  So, it has
  // unknown fields of every type.
  google::protobuf::unittest::TestEmptyMessage empty_message_;
  google::protobuf::UnknownFieldSet* unknown_fields_;

  auto SetUp = [&descriptor_, &all_fields_, &unknown_fields_, &empty_message_, &all_fields_data_]()
  {
    all_fields_data_ = "";

    descriptor_ = google::protobuf::unittest::TestAllTypes::descriptor();
    google::protobuf::TestUtil::SetAllFields(&all_fields_);
    all_fields_.SerializeToString(&all_fields_data_);
    verify(empty_message_.ParseFromString(all_fields_data_));
    unknown_fields_ = empty_message_.mutable_unknown_fields();
  };

  SetUp();

  auto GetField = [&descriptor_, &unknown_fields_](const fun::string& name) -> const google::protobuf::UnknownField* {
    const google::protobuf::FieldDescriptor* field = descriptor_->FindFieldByName(name);
    if (field == NULL) return NULL;
    for (int i = 0; i < unknown_fields_->field_count(); i++) {
      if (unknown_fields_->field(i).number() == field->number()) {
        return &unknown_fields_->field(i);
      }
    }
    return NULL;
  };

  // Constructs a protocol buffer which contains fields with all the same
  // numbers as all_fields_data_ except that each field is some other wire
  // type.
  auto GetBizarroData = [&unknown_fields_]() -> fun::string
  {
    google::protobuf::unittest::TestEmptyMessage bizarro_message;
    google::protobuf::UnknownFieldSet* bizarro_unknown_fields =
      bizarro_message.mutable_unknown_fields();
    for (int i = 0; i < unknown_fields_->field_count(); i++) {
      const google::protobuf::UnknownField& unknown_field = unknown_fields_->field(i);
      if (unknown_field.type() == google::protobuf::UnknownField::TYPE_VARINT) {
        bizarro_unknown_fields->AddFixed32(unknown_field.number(), 1);
      }
      else {
        bizarro_unknown_fields->AddVarint(unknown_field.number(), 1);
      }
    }

    fun::string data;
    verify(bizarro_message.SerializeToString(&data));
    return data;
  };

  // TEST_F(UnknownFieldSetTest, AllFieldsPresent)
  {
    // All fields of TestAllTypes should be present, in numeric order (because
    // that's the order we parsed them in).  Fields that are not valid field
    // numbers of TestAllTypes should NOT be present.

    int pos = 0;

    for (int i = 0; i < 1000; i++) {
      const google::protobuf::FieldDescriptor* field = descriptor_->FindFieldByNumber(i);
      if (field != NULL) {
        verify(pos < unknown_fields_->field_count());
        // Do not check oneof field if it is not set
        if (field->containing_oneof() == NULL) {
          verify(i == unknown_fields_->field(pos++).number());
        }
        else if (i == unknown_fields_->field(pos).number()) {
          pos++;
        }
        if (field->is_repeated()) {
          // Should have a second instance.
          verify(pos < unknown_fields_->field_count());
          verify(i == unknown_fields_->field(pos++).number());
        }
      }
    }
    verify(unknown_fields_->field_count() == pos);
  }

  // TEST_F(UnknownFieldSetTest, Varint)
  {
    const google::protobuf::UnknownField* field = GetField("optional_int32");
    verify(field != NULL);

    verify(google::protobuf::UnknownField::TYPE_VARINT == field->type());
    verify(all_fields_.optional_int32() == field->varint());
  }

  // TEST_F(UnknownFieldSetTest, Fixed32)
  {
    const google::protobuf::UnknownField* field = GetField("optional_fixed32");
    verify(field != NULL);

    verify(google::protobuf::UnknownField::TYPE_FIXED32 == field->type());
    verify(all_fields_.optional_fixed32() == field->fixed32());
  }

  // TEST_F(UnknownFieldSetTest, Fixed64)
  {
    const google::protobuf::UnknownField* field = GetField("optional_fixed64");
    verify(field != NULL);

    verify(google::protobuf::UnknownField::TYPE_FIXED64 == field->type());
    verify(all_fields_.optional_fixed64() == field->fixed64());
  }

  // TEST_F(UnknownFieldSetTest, LengthDelimited)
  {
    const google::protobuf::UnknownField* field = GetField("optional_string");
    verify(field != NULL);

    verify(google::protobuf::UnknownField::TYPE_LENGTH_DELIMITED == field->type());
    verify(all_fields_.optional_string() == field->length_delimited());
  }

  // TEST_F(UnknownFieldSetTest, Group)
  {
    const google::protobuf::UnknownField* field = GetField("optionalgroup");
    verify(field != NULL);

    verify(google::protobuf::UnknownField::TYPE_GROUP == field->type());
    verify(1 == field->group().field_count());

    const google::protobuf::UnknownField& nested_field = field->group().field(0);
    const google::protobuf::FieldDescriptor* nested_field_descriptor =
      google::protobuf::unittest::TestAllTypes::OptionalGroup::descriptor()->FindFieldByName("a");
    verify(nested_field_descriptor != NULL);

    verify(nested_field_descriptor->number() == nested_field.number());
    verify(google::protobuf::UnknownField::TYPE_VARINT == nested_field.type());
    verify(all_fields_.optionalgroup().a() == nested_field.varint());
  }

  // TEST_F(UnknownFieldSetTest, SerializeFastAndSlowAreEquivalent)
  {
    int size = WireFormat::ComputeUnknownFieldsSize(
      empty_message_.unknown_fields());
    fun::string slow_buffer;
    fun::string fast_buffer;
    slow_buffer.resize(size);
    fast_buffer.resize(size);

    uint8* target = reinterpret_cast<uint8*>(google::protobuf::string_as_array(&fast_buffer));
    uint8* result = WireFormat::SerializeUnknownFieldsToArray(
      empty_message_.unknown_fields(), target);
    verify(size == result - target);

    {
      google::protobuf::io::ArrayOutputStream raw_stream(google::protobuf::string_as_array(&slow_buffer), size, 1);
      google::protobuf::io::CodedOutputStream output_stream(&raw_stream);
      WireFormat::SerializeUnknownFields(empty_message_.unknown_fields(),
        &output_stream);
      verify(false == output_stream.HadError());
    }
    verify(fast_buffer == slow_buffer);
  }

  // TEST_F(UnknownFieldSetTest, Serialize)
  {
    // Check that serializing the UnknownFieldSet produces the original data
    // again.

    fun::string data;
    empty_message_.SerializeToString(&data);

    // Don't use EXPECT_EQ because we don't want to dump raw binary data to
    // stdout.
    verify(data == all_fields_data_);
  }

  // TEST_F(UnknownFieldSetTest, ParseViaReflection)
  {
    // Make sure fields are properly parsed to the UnknownFieldSet when parsing
    // via reflection.

    google::protobuf::unittest::TestEmptyMessage message;
    google::protobuf::io::ArrayInputStream raw_input(all_fields_data_.data(),
      all_fields_data_.size());
    google::protobuf::io::CodedInputStream input(&raw_input);
    verify(WireFormat::ParseAndMergePartial(&input, &message));

    verify(message.DebugString() == empty_message_.DebugString());
  }

  // TEST_F(UnknownFieldSetTest, SerializeViaReflection)
  {
    // Make sure fields are properly written from the UnknownFieldSet when
    // serializing via reflection.

    fun::string data;

    {
      google::protobuf::io::StringOutputStream raw_output(&data);
      google::protobuf::io::CodedOutputStream output(&raw_output);
      int size = WireFormat::ByteSize(empty_message_);
      WireFormat::SerializeWithCachedSizes(empty_message_, size, &output);
      verify(false == output.HadError());
    }

    // Don't use EXPECT_EQ because we don't want to dump raw binary data to
    // stdout.
    verify(data == all_fields_data_);
  }

  // TEST_F(UnknownFieldSetTest, CopyFrom)
  {
    google::protobuf::unittest::TestEmptyMessage message;

    message.CopyFrom(empty_message_);

    verify(empty_message_.DebugString() == message.DebugString());
  }

  // TEST_F(UnknownFieldSetTest, Swap)
  {
    google::protobuf::unittest::TestEmptyMessage other_message;
    verify(other_message.ParseFromString(GetBizarroData()));

    verify(empty_message_.unknown_fields().field_count() > 0);
    verify(other_message.unknown_fields().field_count() > 0);
    const fun::string debug_string = empty_message_.DebugString();
    const fun::string other_debug_string = other_message.DebugString();
    verify(debug_string != other_debug_string);

    empty_message_.Swap(&other_message);
    verify(debug_string == other_message.DebugString());
    verify(other_debug_string == empty_message_.DebugString());
  }

  // TEST_F(UnknownFieldSetTest, SwapWithSelf)
  {
    const fun::string debug_string = empty_message_.DebugString();
    verify(empty_message_.unknown_fields().field_count() > 0);

    empty_message_.Swap(&empty_message_);
    verify(empty_message_.unknown_fields().field_count() > 0);
    verify(debug_string == empty_message_.DebugString());
  }

  // TEST_F(UnknownFieldSetTest, MergeFrom)
  {
    google::protobuf::unittest::TestEmptyMessage source, destination;

    destination.mutable_unknown_fields()->AddVarint(1, 1);
    destination.mutable_unknown_fields()->AddVarint(3, 2);
    source.mutable_unknown_fields()->AddVarint(2, 3);
    source.mutable_unknown_fields()->AddVarint(3, 4);

    destination.MergeFrom(source);

    verify(
      // Note:  The ordering of fields here depends on the ordering of adds
      //   and merging, above.
      "1: 1\n"
      "3: 2\n"
      "2: 3\n"
      "3: 4\n" ==
      destination.DebugString());
  }


  // TEST_F(UnknownFieldSetTest, Clear)
  SetUp();
  {
    // Clear the set
    empty_message_.Clear();
    verify(0 == unknown_fields_->field_count());
  }

  // TEST_F(UnknownFieldSetTest, ClearAndFreeMemory)
  SetUp();
  {
    verify(unknown_fields_->field_count() > 0);
    unknown_fields_->ClearAndFreeMemory();
    verify(0 == unknown_fields_->field_count());
    unknown_fields_->AddVarint(123456, 654321);
    verify(1 == unknown_fields_->field_count());
  }

  // TEST_F(UnknownFieldSetTest, ParseKnownAndUnknown)
  SetUp();
  {
    // Test mixing known and unknown fields when parsing.

    google::protobuf::unittest::TestEmptyMessage source;
    source.mutable_unknown_fields()->AddVarint(123456, 654321);
    fun::string data;
    verify(source.SerializeToString(&data));

    google::protobuf::unittest::TestAllTypes destination;
    verify(destination.ParseFromString(all_fields_data_ + data));

    // google::protobuf::TestUtil::ExpectAllFieldsSet(destination);
    verify(1 == destination.unknown_fields().field_count());
    verify(google::protobuf::UnknownField::TYPE_VARINT ==
      destination.unknown_fields().field(0).type());
    verify(654321 == destination.unknown_fields().field(0).varint());
  }

  // TEST_F(UnknownFieldSetTest, WrongTypeTreatedAsUnknown)
  SetUp();
  {
    // Test that fields of the wrong wire type are treated like unknown fields
    // when parsing.

    google::protobuf::unittest::TestAllTypes all_types_message;
    google::protobuf::unittest::TestEmptyMessage empty_message;
    fun::string bizarro_data = GetBizarroData();
    verify(all_types_message.ParseFromString(bizarro_data));
    verify(empty_message.ParseFromString(bizarro_data));

    // All fields should have been interpreted as unknown, so the debug strings
    // should be the same.
    verify(empty_message.DebugString() == all_types_message.DebugString());
  }

  // TEST_F(UnknownFieldSetTest, WrongTypeTreatedAsUnknownViaReflection)
  SetUp();
  {
    // Same as WrongTypeTreatedAsUnknown but via the reflection interface.

    google::protobuf::unittest::TestAllTypes all_types_message;
    google::protobuf::unittest::TestEmptyMessage empty_message;
    fun::string bizarro_data = GetBizarroData();
    google::protobuf::io::ArrayInputStream raw_input(bizarro_data.data(), bizarro_data.size());
    google::protobuf::io::CodedInputStream input(&raw_input);
    verify(WireFormat::ParseAndMergePartial(&input, &all_types_message));
    verify(empty_message.ParseFromString(bizarro_data));

    verify(empty_message.DebugString() == all_types_message.DebugString());
  }

  // TEST_F(UnknownFieldSetTest, UnknownExtensions)
  SetUp();
  {
    // Make sure fields are properly parsed to the UnknownFieldSet even when
    // they are declared as extension numbers.

    google::protobuf::unittest::TestEmptyMessageWithExtensions message;
    verify(message.ParseFromString(all_fields_data_));

    verify(message.DebugString() == empty_message_.DebugString());
  }

  // TEST_F(UnknownFieldSetTest, UnknownExtensionsReflection)
  SetUp();
  {
    // Same as UnknownExtensions except parsing via reflection.

    google::protobuf::unittest::TestEmptyMessageWithExtensions message;
    google::protobuf::io::ArrayInputStream raw_input(all_fields_data_.data(),
      all_fields_data_.size());
    google::protobuf::io::CodedInputStream input(&raw_input);
    verify(WireFormat::ParseAndMergePartial(&input, &message));

    verify(message.DebugString() == empty_message_.DebugString());
  }

  // TEST_F(UnknownFieldSetTest, WrongExtensionTypeTreatedAsUnknown)
  SetUp();
  {
    // Test that fields of the wrong wire type are treated like unknown fields
    // when parsing extensions.

    google::protobuf::unittest::TestAllExtensions all_extensions_message;
    google::protobuf::unittest::TestEmptyMessage empty_message;
    fun::string bizarro_data = GetBizarroData();
    verify(all_extensions_message.ParseFromString(bizarro_data));
    verify(empty_message.ParseFromString(bizarro_data));

    // All fields should have been interpreted as unknown, so the debug strings
    // should be the same.
    verify(empty_message.DebugString() == all_extensions_message.DebugString());
  }

  // TEST_F(UnknownFieldSetTest, UnknownEnumValue)
  SetUp();
  {
    using google::protobuf::unittest::TestAllTypes;
    using google::protobuf::unittest::TestAllExtensions;
    using google::protobuf::unittest::TestEmptyMessage;

    const google::protobuf::FieldDescriptor* singular_field =
      TestAllTypes::descriptor()->FindFieldByName("optional_nested_enum");
    const google::protobuf::FieldDescriptor* repeated_field =
      TestAllTypes::descriptor()->FindFieldByName("repeated_nested_enum");
    verify(singular_field != NULL);
    verify(repeated_field != NULL);

    fun::string data;

    {
      TestEmptyMessage empty_message;
      google::protobuf::UnknownFieldSet* unknown_fields = empty_message.mutable_unknown_fields();
      unknown_fields->AddVarint(singular_field->number(), TestAllTypes::BAR);
      unknown_fields->AddVarint(singular_field->number(), 5);  // not valid
      unknown_fields->AddVarint(repeated_field->number(), TestAllTypes::FOO);
      unknown_fields->AddVarint(repeated_field->number(), 4);  // not valid
      unknown_fields->AddVarint(repeated_field->number(), TestAllTypes::BAZ);
      unknown_fields->AddVarint(repeated_field->number(), 6);  // not valid
      empty_message.SerializeToString(&data);
    }

    {
      TestAllTypes message;
      verify(message.ParseFromString(data));
      verify(TestAllTypes::BAR == message.optional_nested_enum());
      verify(2 == message.repeated_nested_enum_size());
      verify(TestAllTypes::FOO == message.repeated_nested_enum(0));
      verify(TestAllTypes::BAZ == message.repeated_nested_enum(1));

      const google::protobuf::UnknownFieldSet& unknown_fields = message.unknown_fields();
      verify(3 == unknown_fields.field_count());

      verify(singular_field->number() == unknown_fields.field(0).number());
      verify(google::protobuf::UnknownField::TYPE_VARINT == unknown_fields.field(0).type());
      verify(5 == unknown_fields.field(0).varint());

      verify(repeated_field->number() == unknown_fields.field(1).number());
      verify(google::protobuf::UnknownField::TYPE_VARINT == unknown_fields.field(1).type());
      verify(4 == unknown_fields.field(1).varint());

      verify(repeated_field->number() == unknown_fields.field(2).number());
      verify(google::protobuf::UnknownField::TYPE_VARINT == unknown_fields.field(2).type());
      verify(6 == unknown_fields.field(2).varint());
    }

    {
      using google::protobuf::unittest::optional_nested_enum_extension;
      using google::protobuf::unittest::repeated_nested_enum_extension;

      TestAllExtensions message;
      verify(message.ParseFromString(data));
      verify(TestAllTypes::BAR ==
        message.GetExtension(optional_nested_enum_extension));
      verify(2 == message.ExtensionSize(repeated_nested_enum_extension));
      verify(TestAllTypes::FOO ==
        message.GetExtension(repeated_nested_enum_extension, 0));
      verify(TestAllTypes::BAZ ==
        message.GetExtension(repeated_nested_enum_extension, 1));

      const google::protobuf::UnknownFieldSet& unknown_fields = message.unknown_fields();
      verify(3 == unknown_fields.field_count());

      verify(singular_field->number() == unknown_fields.field(0).number());
      verify(google::protobuf::UnknownField::TYPE_VARINT == unknown_fields.field(0).type());
      verify(5 == unknown_fields.field(0).varint());

      verify(repeated_field->number() == unknown_fields.field(1).number());
      verify(google::protobuf::UnknownField::TYPE_VARINT == unknown_fields.field(1).type());
      verify(4 == unknown_fields.field(1).varint());

      verify(repeated_field->number() == unknown_fields.field(2).number());
      verify(google::protobuf::UnknownField::TYPE_VARINT == unknown_fields.field(2).type());
      verify(6 == unknown_fields.field(2).varint());
    }
  }

  // TEST_F(UnknownFieldSetTest, SpaceUsed)
  SetUp();
  {
    google::protobuf::unittest::TestEmptyMessage empty_message;

    // Make sure an unknown field fun::set has zero space used until a field is
    // actually added.
    int base_size = empty_message.SpaceUsed();
    google::protobuf::UnknownFieldSet* unknown_fields = empty_message.mutable_unknown_fields();
    verify(base_size == empty_message.SpaceUsed());

    // Make sure each thing we add to the fun::set increases the SpaceUsed().
    unknown_fields->AddVarint(1, 0);
    verify(base_size < empty_message.SpaceUsed());
    base_size = empty_message.SpaceUsed();

    fun::string* str = unknown_fields->AddLengthDelimited(1);
    verify(base_size < empty_message.SpaceUsed());
    base_size = empty_message.SpaceUsed();

    str->assign(sizeof(fun::string) + 1, 'x');
    verify(base_size < empty_message.SpaceUsed());
    base_size = empty_message.SpaceUsed();

    google::protobuf::UnknownFieldSet* group = unknown_fields->AddGroup(1);
    verify(base_size < empty_message.SpaceUsed());
    base_size = empty_message.SpaceUsed();

    group->AddVarint(1, 0);
    verify(base_size < empty_message.SpaceUsed());
  }


  // TEST_F(UnknownFieldSetTest, Empty)
  SetUp();
  {
    google::protobuf::UnknownFieldSet unknown_fields;
    verify(unknown_fields.empty());
    unknown_fields.AddVarint(6, 123);
    verify(false == unknown_fields.empty());
    unknown_fields.Clear();
    verify(unknown_fields.empty());
  }

  // TEST_F(UnknownFieldSetTest, DeleteSubrange)
  SetUp();
  {
    // Exhaustively test the deletion of every possible subrange in arrays of all
    // sizes from 0 through 9.
    for (int size = 0; size < 10; ++size) {
      for (int num = 0; num <= size; ++num) {
        for (int start = 0; start < size - num; ++start) {
          // Create a fun::set with "size" fields.
          google::protobuf::UnknownFieldSet unknown;
          for (int i = 0; i < size; ++i) {
            unknown.AddFixed32(i, i);
          }
          // Delete the specified subrange.
          unknown.DeleteSubrange(start, num);
          // Make sure the resulting field values are still correct.
          verify(size - num == unknown.field_count());
          for (int i = 0; i < unknown.field_count(); ++i) {
            if (i < start) {
              verify(i == unknown.field(i).fixed32());
            }
            else {
              verify(i + num == unknown.field(i).fixed32());
            }
          }
        }
      }
    }
  }

  auto CheckDeleteByNumber = [](const fun::vector<int>& field_numbers, int deleted_number,
    const fun::vector<int>& expected_field_nubmers) {
    google::protobuf::UnknownFieldSet unknown_fields;
    for (int i = 0; i < (int)field_numbers.size(); ++i) {
      unknown_fields.AddFixed32(field_numbers[i], i);
    }
    unknown_fields.DeleteByNumber(deleted_number);
    verify(expected_field_nubmers.size() == unknown_fields.field_count());
    for (int i = 0; i < (int)expected_field_nubmers.size(); ++i) {
      verify(expected_field_nubmers[i] ==
        unknown_fields.field(i).number());
    }
  };

#define MAKE_VECTOR(x) fun::vector<int>(x, x + GOOGLE_ARRAYSIZE(x))
  // TEST_F(UnknownFieldSetTest, DeleteByNumber)
  SetUp();
  {
    CheckDeleteByNumber(fun::vector<int>(), 1, fun::vector<int>());
    static const int kTestFieldNumbers1[] = { 1, 2, 3 };
    static const int kFieldNumberToDelete1 = 1;
    static const int kExpectedFieldNumbers1[] = { 2, 3 };
    CheckDeleteByNumber(MAKE_VECTOR(kTestFieldNumbers1), kFieldNumberToDelete1,
      MAKE_VECTOR(kExpectedFieldNumbers1));
    static const int kTestFieldNumbers2[] = { 1, 2, 3 };
    static const int kFieldNumberToDelete2 = 2;
    static const int kExpectedFieldNumbers2[] = { 1, 3 };
    CheckDeleteByNumber(MAKE_VECTOR(kTestFieldNumbers2), kFieldNumberToDelete2,
      MAKE_VECTOR(kExpectedFieldNumbers2));
    static const int kTestFieldNumbers3[] = { 1, 2, 3 };
    static const int kFieldNumberToDelete3 = 3;
    static const int kExpectedFieldNumbers3[] = { 1, 2 };
    CheckDeleteByNumber(MAKE_VECTOR(kTestFieldNumbers3), kFieldNumberToDelete3,
      MAKE_VECTOR(kExpectedFieldNumbers3));
    static const int kTestFieldNumbers4[] = { 1, 2, 1, 4, 1 };
    static const int kFieldNumberToDelete4 = 1;
    static const int kExpectedFieldNumbers4[] = { 2, 4 };
    CheckDeleteByNumber(MAKE_VECTOR(kTestFieldNumbers4), kFieldNumberToDelete4,
      MAKE_VECTOR(kExpectedFieldNumbers4));
    static const int kTestFieldNumbers5[] = { 1, 2, 3, 4, 5 };
    static const int kFieldNumberToDelete5 = 6;
    static const int kExpectedFieldNumbers5[] = { 1, 2, 3, 4, 5 };
    CheckDeleteByNumber(MAKE_VECTOR(kTestFieldNumbers5), kFieldNumberToDelete5,
      MAKE_VECTOR(kExpectedFieldNumbers5));
  }
#undef MAKE_VECTOR
  // // // //

  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFunapiLibProtobufWireFormatUnitTest, "LibProtobuf.WireFormatUnitTest", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

// Test differences between fun::string and bytes.
// Value of a fun::string type must be valid UTF-8 fun::string.  When UTF-8
// validation is enabled (GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED):
// WriteInvalidUTF8String:  see error message.
// ReadInvalidUTF8String:  see error message.
// WriteValidUTF8String: fine.
// ReadValidUTF8String:  fine.
// WriteAnyBytes: fine.
// ReadAnyBytes: fine.
const char * kInvalidUTF8String = "Invalid UTF-8: \xA0\xB0\xC0\xD0";
// This used to be "Valid UTF-8: \x01\x02\u8C37\u6B4C", but MSVC seems to
// interpret \u differently from GCC.
const char * kValidUTF8String = "Valid UTF-8: \x01\x02\350\260\267\346\255\214";

template<typename T>
bool WriteMessage(const char *value, T *message, fun::string *wire_buffer) {
  message->set_data(value);
  wire_buffer->clear();
  message->AppendToString(wire_buffer);
  return (wire_buffer->size() > 0);
}

template<typename T>
bool ReadMessage(const fun::string &wire_buffer, T *message) {
  return message->ParseFromArray(wire_buffer.data(), wire_buffer.size());
}

bool FFunapiLibProtobufWireFormatUnitTest::RunTest(const FString& Parameters)
{
  using google::protobuf::internal::WireFormat;

  // TEST(WireFormatTest, EnumsInSync)
  {
    // Verify that WireFormatLite::FieldType and WireFormatLite::CppType match
    // FieldDescriptor::Type and FieldDescriptor::CppType.

    verify(google::protobuf::internal::implicit_cast<int>(google::protobuf::FieldDescriptor::MAX_TYPE) ==
      google::protobuf::internal::implicit_cast<int>(google::protobuf::internal::WireFormatLite::MAX_FIELD_TYPE));
    verify(google::protobuf::internal::implicit_cast<int>(google::protobuf::FieldDescriptor::MAX_CPPTYPE) ==
      google::protobuf::internal::implicit_cast<int>(google::protobuf::internal::WireFormatLite::MAX_CPPTYPE));

    for (int i = 1; i <= google::protobuf::internal::WireFormatLite::MAX_FIELD_TYPE; i++) {
      verify(
        google::protobuf::internal::implicit_cast<int>(google::protobuf::FieldDescriptor::TypeToCppType(
          static_cast<google::protobuf::FieldDescriptor::Type>(i))) ==
        google::protobuf::internal::implicit_cast<int>(google::protobuf::internal::WireFormatLite::FieldTypeToCppType(
          static_cast<google::protobuf::internal::WireFormatLite::FieldType>(i))));
    }
  }

  // TEST(WireFormatTest, MaxFieldNumber)
  {
    // Make sure the max field number constant is accurate.
    verify((1 << (32 - google::protobuf::internal::WireFormatLite::kTagTypeBits)) - 1 ==
      google::protobuf::FieldDescriptor::kMaxNumber);
  }

  // TEST(WireFormatTest, Parse)
  {
    google::protobuf::unittest::TestAllTypes source, dest;
    fun::string data;

    // Serialize using the generated code.
    google::protobuf::TestUtil::SetAllFields(&source);
    source.SerializeToString(&data);

    // Parse using WireFormat.
    google::protobuf::io::ArrayInputStream raw_input(data.data(), data.size());
    google::protobuf::io::CodedInputStream input(&raw_input);
    google::protobuf::internal::WireFormat::ParseAndMergePartial(&input, &dest);

    // Check.
    google::protobuf::TestUtil::ExpectAllFieldsSet(dest);
  }

  // TEST(WireFormatTest, ParseExtensions)
  {
    google::protobuf::unittest::TestAllExtensions source, dest;
    fun::string data;

    // Serialize using the generated code.
    google::protobuf::TestUtil::SetAllExtensions(&source);
    source.SerializeToString(&data);

    // Parse using WireFormat.
    google::protobuf::io::ArrayInputStream raw_input(data.data(), data.size());
    google::protobuf::io::CodedInputStream input(&raw_input);
    google::protobuf::internal::WireFormat::ParseAndMergePartial(&input, &dest);

    // Check.
    google::protobuf::TestUtil::ExpectAllExtensionsSet(dest);
  }

  // TEST(WireFormatTest, ParsePacked)
  {
    google::protobuf::unittest::TestPackedTypes source, dest;
    fun::string data;

    // Serialize using the generated code.
    google::protobuf::TestUtil::SetPackedFields(&source);
    source.SerializeToString(&data);

    // Parse using WireFormat.
    google::protobuf::io::ArrayInputStream raw_input(data.data(), data.size());
    google::protobuf::io::CodedInputStream input(&raw_input);
    google::protobuf::internal::WireFormat::ParseAndMergePartial(&input, &dest);

    // Check.
    google::protobuf::TestUtil::ExpectPackedFieldsSet(dest);
  }

  // TEST(WireFormatTest, ParsePackedFromUnpacked)
  {
    // Serialize using the generated code.
    google::protobuf::unittest::TestUnpackedTypes source;
    google::protobuf::TestUtil::SetUnpackedFields(&source);
    fun::string data = source.SerializeAsString();

    // Parse using WireFormat.
    google::protobuf::unittest::TestPackedTypes dest;
    google::protobuf::io::ArrayInputStream raw_input(data.data(), data.size());
    google::protobuf::io::CodedInputStream input(&raw_input);
    google::protobuf::internal::WireFormat::ParseAndMergePartial(&input, &dest);

    // Check.
    google::protobuf::TestUtil::ExpectPackedFieldsSet(dest);
  }

  // TEST(WireFormatTest, ParseUnpackedFromPacked)
  {
    // Serialize using the generated code.
    google::protobuf::unittest::TestPackedTypes source;
    google::protobuf::TestUtil::SetPackedFields(&source);
    fun::string data = source.SerializeAsString();

    // Parse using WireFormat.
    google::protobuf::unittest::TestUnpackedTypes dest;
    google::protobuf::io::ArrayInputStream raw_input(data.data(), data.size());
    google::protobuf::io::CodedInputStream input(&raw_input);
    google::protobuf::internal::WireFormat::ParseAndMergePartial(&input, &dest);

    // Check.
    google::protobuf::TestUtil::ExpectUnpackedFieldsSet(dest);
  }

  // TEST(WireFormatTest, ParsePackedExtensions)
  {
    google::protobuf::unittest::TestPackedExtensions source, dest;
    fun::string data;

    // Serialize using the generated code.
    google::protobuf::TestUtil::SetPackedExtensions(&source);
    source.SerializeToString(&data);

    // Parse using WireFormat.
    google::protobuf::io::ArrayInputStream raw_input(data.data(), data.size());
    google::protobuf::io::CodedInputStream input(&raw_input);
    google::protobuf::internal::WireFormat::ParseAndMergePartial(&input, &dest);

    // Check.
    google::protobuf::TestUtil::ExpectPackedExtensionsSet(dest);
  }

  // TEST(WireFormatTest, ParseOneof)
  {
    google::protobuf::unittest::TestOneof2 source, dest;
    fun::string data;

    // Serialize using the generated code.
    google::protobuf::TestUtil::SetOneof1(&source);
    source.SerializeToString(&data);

    // Parse using WireFormat.
    google::protobuf::io::ArrayInputStream raw_input(data.data(), data.size());
    google::protobuf::io::CodedInputStream input(&raw_input);
    google::protobuf::internal::WireFormat::ParseAndMergePartial(&input, &dest);

    // Check.
    google::protobuf::TestUtil::ExpectOneofSet1(dest);
  }

  // TEST(WireFormatTest, OneofOnlySetLast)
  {
    google::protobuf::unittest::TestOneofBackwardsCompatible source;
    google::protobuf::unittest::TestOneof oneof_dest;
    fun::string data;

    // Set two fields
    source.set_foo_int(100);
    source.set_foo_string("101");

    // Serialize and parse to oneof message.
    source.SerializeToString(&data);
    google::protobuf::io::ArrayInputStream raw_input(data.data(), data.size());
    google::protobuf::io::CodedInputStream input(&raw_input);
    google::protobuf::internal::WireFormat::ParseAndMergePartial(&input, &oneof_dest);

    // Only the last field is set
    verify(false == oneof_dest.has_foo_int());
    verify(oneof_dest.has_foo_string());
  }

  // TEST(WireFormatTest, ByteSize)
  {
    google::protobuf::unittest::TestAllTypes message;
    google::protobuf::TestUtil::SetAllFields(&message);

    verify(message.ByteSize() == google::protobuf::internal::WireFormat::ByteSize(message));
    message.Clear();
    verify(0 == message.ByteSize());
    verify(0 == google::protobuf::internal::WireFormat::ByteSize(message));
  }

  // TEST(WireFormatTest, ByteSizeExtensions)
  {
    google::protobuf::unittest::TestAllExtensions message;
    google::protobuf::TestUtil::SetAllExtensions(&message);

    verify(message.ByteSize() ==
      google::protobuf::internal::WireFormat::ByteSize(message));
    message.Clear();
    verify(0 == message.ByteSize());
    verify(0 == google::protobuf::internal::WireFormat::ByteSize(message));
  }

  // TEST(WireFormatTest, ByteSizePacked)
  {
    google::protobuf::unittest::TestPackedTypes message;
    google::protobuf::TestUtil::SetPackedFields(&message);

    verify(message.ByteSize() == google::protobuf::internal::WireFormat::ByteSize(message));
    message.Clear();
    verify(0 == message.ByteSize());
    verify(0 == google::protobuf::internal::WireFormat::ByteSize(message));
  }

  // TEST(WireFormatTest, ByteSizePackedExtensions)
  {
    google::protobuf::unittest::TestPackedExtensions message;
    google::protobuf::TestUtil::SetPackedExtensions(&message);

    verify(message.ByteSize() ==
      google::protobuf::internal::WireFormat::ByteSize(message));
    message.Clear();
    verify(0 == message.ByteSize());
    verify(0 == google::protobuf::internal::WireFormat::ByteSize(message));
  }

  // TEST(WireFormatTest, ByteSizeOneof)
  {
    google::protobuf::unittest::TestOneof2 message;
    google::protobuf::TestUtil::SetOneof1(&message);

    verify(message.ByteSize() ==
      google::protobuf::internal::WireFormat::ByteSize(message));
    message.Clear();

    verify(0 == message.ByteSize());
    verify(0 == google::protobuf::internal::WireFormat::ByteSize(message));
  }

  // TEST(WireFormatTest, Serialize)
  {
    google::protobuf::unittest::TestAllTypes message;
    fun::string generated_data;
    fun::string dynamic_data;

    google::protobuf::TestUtil::SetAllFields(&message);
    int size = message.ByteSize();

    // Serialize using the generated code.
    {
      google::protobuf::io::StringOutputStream raw_output(&generated_data);
      google::protobuf::io::CodedOutputStream output(&raw_output);
      message.SerializeWithCachedSizes(&output);
      verify(false == output.HadError());
    }

    // Serialize using WireFormat.
    {
      google::protobuf::io::StringOutputStream raw_output(&dynamic_data);
      google::protobuf::io::CodedOutputStream output(&raw_output);
      google::protobuf::internal::WireFormat::SerializeWithCachedSizes(message, size, &output);
      verify(false == output.HadError());
    }

    // Should be the same.
    // Don't use EXPECT_EQ here because we're comparing raw binary data and
    // we really don't want it dumped to stdout on failure.
    verify(dynamic_data == generated_data);
  }

  // TEST(WireFormatTest, SerializeExtensions)
  {
    google::protobuf::unittest::TestAllExtensions message;
    fun::string generated_data;
    fun::string dynamic_data;

    google::protobuf::TestUtil::SetAllExtensions(&message);
    int size = message.ByteSize();

    // Serialize using the generated code.
    {
      google::protobuf::io::StringOutputStream raw_output(&generated_data);
      google::protobuf::io::CodedOutputStream output(&raw_output);
      message.SerializeWithCachedSizes(&output);
      verify(false == output.HadError());
    }

    // Serialize using WireFormat.
    {
      google::protobuf::io::StringOutputStream raw_output(&dynamic_data);
      google::protobuf::io::CodedOutputStream output(&raw_output);
      WireFormat::SerializeWithCachedSizes(message, size, &output);
      verify(false == output.HadError());
    }

    // Should be the same.
    // Don't use EXPECT_EQ here because we're comparing raw binary data and
    // we really don't want it dumped to stdout on failure.
    verify(dynamic_data == generated_data);
  }

  // TEST(WireFormatTest, SerializeFieldsAndExtensions)
  {
    google::protobuf::unittest::TestFieldOrderings message;
    fun::string generated_data;
    fun::string dynamic_data;

    google::protobuf::TestUtil::SetAllFieldsAndExtensions(&message);
    int size = message.ByteSize();

    // Serialize using the generated code.
    {
      google::protobuf::io::StringOutputStream raw_output(&generated_data);
      google::protobuf::io::CodedOutputStream output(&raw_output);
      message.SerializeWithCachedSizes(&output);
      verify(false == output.HadError());
    }

    // Serialize using WireFormat.
    {
      google::protobuf::io::StringOutputStream raw_output(&dynamic_data);
      google::protobuf::io::CodedOutputStream output(&raw_output);
      WireFormat::SerializeWithCachedSizes(message, size, &output);
      verify(false == output.HadError());
    }

    // Should be the same.
    // Don't use EXPECT_EQ here because we're comparing raw binary data and
    // we really don't want it dumped to stdout on failure.
    verify(dynamic_data == generated_data);

    // Should output in canonical order.
    google::protobuf::TestUtil::ExpectAllFieldsAndExtensionsInOrder(dynamic_data);
    google::protobuf::TestUtil::ExpectAllFieldsAndExtensionsInOrder(generated_data);
  }

  // TEST(WireFormatTest, SerializeOneof)
  {
    google::protobuf::unittest::TestOneof2 message;
    fun::string generated_data;
    fun::string dynamic_data;

    google::protobuf::TestUtil::SetOneof1(&message);
    int size = message.ByteSize();

    // Serialize using the generated code.
    {
      google::protobuf::io::StringOutputStream raw_output(&generated_data);
      google::protobuf::io::CodedOutputStream output(&raw_output);
      message.SerializeWithCachedSizes(&output);
      verify(false == output.HadError());
    }

    // Serialize using WireFormat.
    {
      google::protobuf::io::StringOutputStream raw_output(&dynamic_data);
      google::protobuf::io::CodedOutputStream output(&raw_output);
      WireFormat::SerializeWithCachedSizes(message, size, &output);
      verify(false == output.HadError());
    }

    // Should be the same.
    // Don't use EXPECT_EQ here because we're comparing raw binary data and
    // we really don't want it dumped to stdout on failure.
    verify(dynamic_data == generated_data);
  }

  // TEST(WireFormatTest, ParseMultipleExtensionRanges)
  {
    // Make sure we can parse a message that contains multiple extensions ranges.
    google::protobuf::unittest::TestFieldOrderings source;
    fun::string data;

    google::protobuf::TestUtil::SetAllFieldsAndExtensions(&source);
    source.SerializeToString(&data);

    {
      google::protobuf::unittest::TestFieldOrderings dest;
      verify(dest.ParseFromString(data));
      verify(source.DebugString() == dest.DebugString());
    }

    // Also test using reflection-based parsing.
    {
      google::protobuf::unittest::TestFieldOrderings dest;
      google::protobuf::io::ArrayInputStream raw_input(data.data(), data.size());
      google::protobuf::io::CodedInputStream coded_input(&raw_input);
      verify(WireFormat::ParseAndMergePartial(&coded_input, &dest));
      verify(source.DebugString() == dest.DebugString());
    }
  }

  const int kUnknownTypeId = 1550055;

  // TEST(WireFormatTest, SerializeMessageSet)
  {
    // Set up a TestMessageSet with two known messages and an unknown one.
    google::protobuf::unittest::TestMessageSet message_set;
    message_set.MutableExtension(
      google::protobuf::unittest::TestMessageSetExtension1::message_set_extension)->set_i(123);
    message_set.MutableExtension(
      google::protobuf::unittest::TestMessageSetExtension2::message_set_extension)->set_str("foo");
    message_set.mutable_unknown_fields()->AddLengthDelimited(
      kUnknownTypeId, "bar");

    fun::string data;
    verify(message_set.SerializeToString(&data));

    // Parse back using RawMessageSet and check the contents.
    google::protobuf::unittest::RawMessageSet raw;
    verify(raw.ParseFromString(data));

    verify(0 == raw.unknown_fields().field_count());

    verify(3 == raw.item_size());
    verify(
      google::protobuf::unittest::TestMessageSetExtension1::descriptor()->extension(0)->number() ==
      raw.item(0).type_id());
    verify(
      google::protobuf::unittest::TestMessageSetExtension2::descriptor()->extension(0)->number() ==
      raw.item(1).type_id());
    verify(kUnknownTypeId == raw.item(2).type_id());

    google::protobuf::unittest::TestMessageSetExtension1 message1;
    verify(message1.ParseFromString(raw.item(0).message()));
    verify(123 == message1.i());

    google::protobuf::unittest::TestMessageSetExtension2 message2;
    verify(message2.ParseFromString(raw.item(1).message()));
    verify("foo" == message2.str());

    verify("bar" == raw.item(2).message());
  }

  // TEST(WireFormatTest, SerializeMessageSetVariousWaysAreEqual)
  {
    // Serialize a MessageSet to a stream and to a flat array using generated
    // code, and also using WireFormat, and check that the results are equal.
    // Set up a TestMessageSet with two known messages and an unknown one, as
    // above.

    google::protobuf::unittest::TestMessageSet message_set;
    message_set.MutableExtension(
      google::protobuf::unittest::TestMessageSetExtension1::message_set_extension)->set_i(123);
    message_set.MutableExtension(
      google::protobuf::unittest::TestMessageSetExtension2::message_set_extension)->set_str("foo");
    message_set.mutable_unknown_fields()->AddLengthDelimited(
      kUnknownTypeId, "bar");

    int size = message_set.ByteSize();
    verify(size == message_set.GetCachedSize());
    verify(size == WireFormat::ByteSize(message_set));

    fun::string flat_data;
    fun::string stream_data;
    fun::string dynamic_data;
    flat_data.resize(size);
    stream_data.resize(size);

    // Serialize to flat array
    {
      uint8* target = reinterpret_cast<uint8*>(google::protobuf::string_as_array(&flat_data));
      uint8* end = message_set.SerializeWithCachedSizesToArray(target);
      verify(size == end - target);
    }

    // Serialize to buffer
    {
      google::protobuf::io::ArrayOutputStream array_stream(google::protobuf::string_as_array(&stream_data), size, 1);
      google::protobuf::io::CodedOutputStream output_stream(&array_stream);
      message_set.SerializeWithCachedSizes(&output_stream);
      verify(false == output_stream.HadError());
    }

    // Serialize to buffer with WireFormat.
    {
      google::protobuf::io::StringOutputStream string_stream(&dynamic_data);
      google::protobuf::io::CodedOutputStream output_stream(&string_stream);
      WireFormat::SerializeWithCachedSizes(message_set, size, &output_stream);
      verify(false == output_stream.HadError());
    }

    verify(flat_data == stream_data);
    verify(flat_data == dynamic_data);
  }

  // TEST(WireFormatTest, ParseMessageSet)
  {
    // Set up a RawMessageSet with two known messages and an unknown one.
    google::protobuf::unittest::RawMessageSet raw;

    {
      google::protobuf::unittest::RawMessageSet::Item* item = raw.add_item();
      item->set_type_id(
        google::protobuf::unittest::TestMessageSetExtension1::descriptor()->extension(0)->number());
      google::protobuf::unittest::TestMessageSetExtension1 message;
      message.set_i(123);
      message.SerializeToString(item->mutable_message());
    }

    {
      google::protobuf::unittest::RawMessageSet::Item* item = raw.add_item();
      item->set_type_id(
        google::protobuf::unittest::TestMessageSetExtension2::descriptor()->extension(0)->number());
      google::protobuf::unittest::TestMessageSetExtension2 message;
      message.set_str("foo");
      message.SerializeToString(item->mutable_message());
    }

    {
      google::protobuf::unittest::RawMessageSet::Item* item = raw.add_item();
      item->set_type_id(kUnknownTypeId);
      item->set_message("bar");
    }

    fun::string data;
    verify(raw.SerializeToString(&data));

    // Parse as a TestMessageSet and check the contents.
    google::protobuf::unittest::TestMessageSet message_set;
    verify(message_set.ParseFromString(data));

    verify(123 == message_set.GetExtension(
      google::protobuf::unittest::TestMessageSetExtension1::message_set_extension).i());
    verify("foo" == message_set.GetExtension(
      google::protobuf::unittest::TestMessageSetExtension2::message_set_extension).str());

    verify(1 == message_set.unknown_fields().field_count());
    verify(google::protobuf::UnknownField::TYPE_LENGTH_DELIMITED ==
      message_set.unknown_fields().field(0).type());
    verify("bar" == message_set.unknown_fields().field(0).length_delimited());

    // Also parse using WireFormat.
    google::protobuf::unittest::TestMessageSet dynamic_message_set;
    google::protobuf::io::CodedInputStream input(reinterpret_cast<const uint8*>(data.data()),
      data.size());
    verify(WireFormat::ParseAndMergePartial(&input, &dynamic_message_set));
    verify(message_set.DebugString() == dynamic_message_set.DebugString());
  }

  // TEST(WireFormatTest, ParseMessageSetWithReverseTagOrder)
  {
    fun::string data;
    {
      google::protobuf::unittest::TestMessageSetExtension1 message;
      message.set_i(123);
      // Build a MessageSet manually with its message content put before its
      // type_id.
      google::protobuf::io::StringOutputStream output_stream(&data);
      google::protobuf::io::CodedOutputStream coded_output(&output_stream);
      coded_output.WriteTag(google::protobuf::internal::WireFormatLite::kMessageSetItemStartTag);
      // Write the message content first.
      google::protobuf::internal::WireFormatLite::WriteTag(google::protobuf::internal::WireFormatLite::kMessageSetMessageNumber,
        google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED,
        &coded_output);
      coded_output.WriteVarint32(message.ByteSize());
      message.SerializeWithCachedSizes(&coded_output);
      // Write the type id.
      uint32 type_id = message.GetDescriptor()->extension(0)->number();
      google::protobuf::internal::WireFormatLite::WriteUInt32(google::protobuf::internal::WireFormatLite::kMessageSetTypeIdNumber,
        type_id, &coded_output);
      coded_output.WriteTag(google::protobuf::internal::WireFormatLite::kMessageSetItemEndTag);
    }
    {
      google::protobuf::unittest::TestMessageSet message_set;
      verify(message_set.ParseFromString(data));

      verify(123 == message_set.GetExtension(
        google::protobuf::unittest::TestMessageSetExtension1::message_set_extension).i());
    }
    {
      // Test parse the message via Reflection.
      google::protobuf::unittest::TestMessageSet message_set;
      google::protobuf::io::CodedInputStream input(
        reinterpret_cast<const uint8*>(data.data()), data.size());
      verify(WireFormat::ParseAndMergePartial(&input, &message_set));
      verify(input.ConsumedEntireMessage());

      verify(123 == message_set.GetExtension(
        google::protobuf::unittest::TestMessageSetExtension1::message_set_extension).i());
    }
  }

  // TEST(WireFormatTest, ParseBrokenMessageSet)
  {
    google::protobuf::unittest::TestMessageSet message_set;
    fun::string input("goodbye");  // Invalid wire format data.
    verify(false == message_set.ParseFromString(input));
  }

  // TEST(WireFormatTest, RecursionLimit)
  {
    google::protobuf::unittest::TestRecursiveMessage message;
    message.mutable_a()->mutable_a()->mutable_a()->mutable_a()->set_i(1);
    fun::string data;
    message.SerializeToString(&data);

    {
      google::protobuf::io::ArrayInputStream raw_input(data.data(), data.size());
      google::protobuf::io::CodedInputStream input(&raw_input);
      input.SetRecursionLimit(4);
      google::protobuf::unittest::TestRecursiveMessage message2;
      verify(message2.ParseFromCodedStream(&input));
    }

    {
      google::protobuf::io::ArrayInputStream raw_input(data.data(), data.size());
      google::protobuf::io::CodedInputStream input(&raw_input);
      input.SetRecursionLimit(3);
      google::protobuf::unittest::TestRecursiveMessage message2;
      verify(false == message2.ParseFromCodedStream(&input));
    }
  }

  // TEST(WireFormatTest, UnknownFieldRecursionLimit)
  {
    google::protobuf::unittest::TestEmptyMessage message;
    message.mutable_unknown_fields()
      ->AddGroup(1234)
      ->AddGroup(1234)
      ->AddGroup(1234)
      ->AddGroup(1234)
      ->AddVarint(1234, 123);
    fun::string data;
    message.SerializeToString(&data);

    {
      google::protobuf::io::ArrayInputStream raw_input(data.data(), data.size());
      google::protobuf::io::CodedInputStream input(&raw_input);
      input.SetRecursionLimit(4);
      google::protobuf::unittest::TestEmptyMessage message2;
      verify(message2.ParseFromCodedStream(&input));
    }

    {
      google::protobuf::io::ArrayInputStream raw_input(data.data(), data.size());
      google::protobuf::io::CodedInputStream input(&raw_input);
      input.SetRecursionLimit(3);
      google::protobuf::unittest::TestEmptyMessage message2;
      verify(false == message2.ParseFromCodedStream(&input));
    }
  }

  // TEST(WireFormatTest, ZigZag)
  {
    // avoid line-wrapping
#define LL(x) GOOGLE_LONGLONG(x)
#define ULL(x) GOOGLE_ULONGLONG(x)
#define ZigZagEncode32(x) google::protobuf::internal::WireFormatLite::ZigZagEncode32(x)
#define ZigZagDecode32(x) google::protobuf::internal::WireFormatLite::ZigZagDecode32(x)
#define ZigZagEncode64(x) google::protobuf::internal::WireFormatLite::ZigZagEncode64(x)
#define ZigZagDecode64(x) google::protobuf::internal::WireFormatLite::ZigZagDecode64(x)

    verify(0u == ZigZagEncode32(0));
    verify(1u == ZigZagEncode32(-1));
    verify(2u == ZigZagEncode32(1));
    verify(3u == ZigZagEncode32(-2));
    verify(0x7FFFFFFEu == ZigZagEncode32(0x3FFFFFFF));
    verify(0x7FFFFFFFu == ZigZagEncode32(0xC0000000));
    verify(0xFFFFFFFEu == ZigZagEncode32(0x7FFFFFFF));
    verify(0xFFFFFFFFu == ZigZagEncode32(0x80000000));

    verify(0 == ZigZagDecode32(0u));
    verify(-1 == ZigZagDecode32(1u));
    verify(1 == ZigZagDecode32(2u));
    verify(-2 == ZigZagDecode32(3u));
    verify(0x3FFFFFFF == ZigZagDecode32(0x7FFFFFFEu));
    verify(0xC0000000 == ZigZagDecode32(0x7FFFFFFFu));
    verify(0x7FFFFFFF == ZigZagDecode32(0xFFFFFFFEu));
    verify(0x80000000 == ZigZagDecode32(0xFFFFFFFFu));

    verify(0u == ZigZagEncode64(0));
    verify(1u == ZigZagEncode64(-1));
    verify(2u == ZigZagEncode64(1));
    verify(3u == ZigZagEncode64(-2));
    verify(ULL(0x000000007FFFFFFE) == ZigZagEncode64(LL(0x000000003FFFFFFF)));
    verify(ULL(0x000000007FFFFFFF) == ZigZagEncode64(LL(0xFFFFFFFFC0000000)));
    verify(ULL(0x00000000FFFFFFFE) == ZigZagEncode64(LL(0x000000007FFFFFFF)));
    verify(ULL(0x00000000FFFFFFFF) == ZigZagEncode64(LL(0xFFFFFFFF80000000)));
    verify(ULL(0xFFFFFFFFFFFFFFFE) == ZigZagEncode64(LL(0x7FFFFFFFFFFFFFFF)));
    verify(ULL(0xFFFFFFFFFFFFFFFF) == ZigZagEncode64(LL(0x8000000000000000)));

    verify(0 == ZigZagDecode64(0u));
    verify(-1 == ZigZagDecode64(1u));
    verify(1 == ZigZagDecode64(2u));
    verify(-2 == ZigZagDecode64(3u));
    verify(LL(0x000000003FFFFFFF) == ZigZagDecode64(ULL(0x000000007FFFFFFE)));
    verify(LL(0xFFFFFFFFC0000000) == ZigZagDecode64(ULL(0x000000007FFFFFFF)));
    verify(LL(0x000000007FFFFFFF) == ZigZagDecode64(ULL(0x00000000FFFFFFFE)));
    verify(LL(0xFFFFFFFF80000000) == ZigZagDecode64(ULL(0x00000000FFFFFFFF)));
    verify(LL(0x7FFFFFFFFFFFFFFF) == ZigZagDecode64(ULL(0xFFFFFFFFFFFFFFFE)));
    verify(LL(0x8000000000000000) == ZigZagDecode64(ULL(0xFFFFFFFFFFFFFFFF)));

    // Some easier-to-verify round-trip tests.  The inputs (other than 0, 1, -1)
    // were chosen semi-randomly via keyboard bashing.
    verify(0 == ZigZagDecode32(ZigZagEncode32(0)));
    verify(1 == ZigZagDecode32(ZigZagEncode32(1)));
    verify(-1 == ZigZagDecode32(ZigZagEncode32(-1)));
    verify(14927 == ZigZagDecode32(ZigZagEncode32(14927)));
    verify(-3612 == ZigZagDecode32(ZigZagEncode32(-3612)));

    verify(0 == ZigZagDecode64(ZigZagEncode64(0)));
    verify(1 == ZigZagDecode64(ZigZagEncode64(1)));
    verify(-1 == ZigZagDecode64(ZigZagEncode64(-1)));
    verify(14927 == ZigZagDecode64(ZigZagEncode64(14927)));
    verify(-3612 == ZigZagDecode64(ZigZagEncode64(-3612)));

    verify(LL(856912304801416) == ZigZagDecode64(ZigZagEncode64(
      LL(856912304801416))));
    verify(LL(-75123905439571256) == ZigZagDecode64(ZigZagEncode64(
      LL(-75123905439571256))));
  }

  // TEST(WireFormatTest, RepeatedScalarsDifferentTagSizes)
  {
    // At one point checks would trigger when parsing repeated fixed scalar
    // fields.
    protobuf_unittest::TestRepeatedScalarDifferentTagSizes msg1, msg2;
    for (int i = 0; i < 100; ++i) {
      msg1.add_repeated_fixed32(i);
      msg1.add_repeated_int32(i);
      msg1.add_repeated_fixed64(i);
      msg1.add_repeated_int64(i);
      msg1.add_repeated_float(i);
      msg1.add_repeated_uint64(i);
    }

    // Make sure that we have a variety of tag sizes.
    const google::protobuf::Descriptor* desc = msg1.GetDescriptor();
    const google::protobuf::FieldDescriptor* field;
    field = desc->FindFieldByName("repeated_fixed32");
    verify(field != NULL);
    verify(1 == WireFormat::TagSize(field->number(), field->type()));
    field = desc->FindFieldByName("repeated_int32");
    verify(field != NULL);
    verify(1 == WireFormat::TagSize(field->number(), field->type()));
    field = desc->FindFieldByName("repeated_fixed64");
    verify(field != NULL);
    verify(2 == WireFormat::TagSize(field->number(), field->type()));
    field = desc->FindFieldByName("repeated_int64");
    verify(field != NULL);
    verify(2 == WireFormat::TagSize(field->number(), field->type()));
    field = desc->FindFieldByName("repeated_float");
    verify(field != NULL);
    verify(3 == WireFormat::TagSize(field->number(), field->type()));
    field = desc->FindFieldByName("repeated_uint64");
    verify(field != NULL);
    verify(3 == WireFormat::TagSize(field->number(), field->type()));

    verify(msg2.ParseFromString(msg1.SerializeAsString()));
    verify(msg1.DebugString() == msg2.DebugString());
  }

  // TEST(WireFormatTest, CompatibleTypes)
  {
    const int64 data = 0x100000000;
    google::protobuf::unittest::Int64Message msg1;
    msg1.set_data(data);
    fun::string serialized;
    msg1.SerializeToString(&serialized);

    // Test int64 is compatible with bool
    google::protobuf::unittest::BoolMessage msg2;
    verify(msg2.ParseFromString(serialized));
    verify(static_cast<bool>(data) == msg2.data());

    // Test int64 is compatible with uint64
    google::protobuf::unittest::Uint64Message msg3;
    verify(msg3.ParseFromString(serialized));
    verify(static_cast<uint64>(data) == msg3.data());

    // Test int64 is compatible with int32
    google::protobuf::unittest::Int32Message msg4;
    verify(msg4.ParseFromString(serialized));
    // verify(static_cast<int32>(data) == msg4.data());

    // Test int64 is compatible with uint32
    google::protobuf::unittest::Uint32Message msg5;
    verify(msg5.ParseFromString(serialized));
    // verify(static_cast<uint32>(data) == msg5.data());
  }
  // // // //

  // // // //
  // WireFormatInvalidInputTest

  // Make a serialized TestAllTypes in which the field optional_nested_message
  // contains exactly the given bytes, which may be invalid.
  auto MakeInvalidEmbeddedMessage = [](const char* bytes, int size) -> fun::string {
    const google::protobuf::FieldDescriptor* field =
      google::protobuf::unittest::TestAllTypes::descriptor()->FindFieldByName(
        "optional_nested_message");
    GOOGLE_CHECK(field != NULL);

    fun::string result;

    {
      google::protobuf::io::StringOutputStream raw_output(&result);
      google::protobuf::io::CodedOutputStream output(&raw_output);

      google::protobuf::internal::WireFormatLite::WriteBytes(field->number(), fun::string(bytes, size), &output);
    }

    return result;
  };

  // Make a serialized TestAllTypes in which the field optionalgroup
  // contains exactly the given bytes -- which may be invalid -- and
  // possibly no end tag.
  auto MakeInvalidGroup = [](const char* bytes, int size, bool include_end_tag) -> fun::string {
    const google::protobuf::FieldDescriptor* field =
      google::protobuf::unittest::TestAllTypes::descriptor()->FindFieldByName(
        "optionalgroup");
    GOOGLE_CHECK(field != NULL);

    fun::string result;

    {
      google::protobuf::io::StringOutputStream raw_output(&result);
      google::protobuf::io::CodedOutputStream output(&raw_output);

      output.WriteVarint32(WireFormat::MakeTag(field));
      output.WriteString(fun::string(bytes, size));
      if (include_end_tag) {
        output.WriteVarint32(google::protobuf::internal::WireFormatLite::MakeTag(
          field->number(), google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP));
      }
    }

    return result;
  };

  // TEST_F(WireFormatInvalidInputTest, InvalidSubMessage)
  {
    google::protobuf::unittest::TestAllTypes message;

    // Control case.
    verify(message.ParseFromString(MakeInvalidEmbeddedMessage("", 0)));

    // The byte is a valid varint, but not a valid tag (zero).
    verify(false == message.ParseFromString(MakeInvalidEmbeddedMessage("\0", 1)));

    // The byte is a malformed varint.
    verify(false == message.ParseFromString(MakeInvalidEmbeddedMessage("\200", 1)));

    // The byte is an endgroup tag, but we aren't parsing a group.
    verify(false == message.ParseFromString(MakeInvalidEmbeddedMessage("\014", 1)));

    // The byte is a valid varint but not a valid tag (bad wire type).
    verify(false == message.ParseFromString(MakeInvalidEmbeddedMessage("\017", 1)));
  }

  // TEST_F(WireFormatInvalidInputTest, InvalidGroup)
  {
    google::protobuf::unittest::TestAllTypes message;

    // Control case.
    verify(message.ParseFromString(MakeInvalidGroup("", 0, true)));

    // Missing end tag.  Groups cannot end at EOF.
    verify(false == message.ParseFromString(MakeInvalidGroup("", 0, false)));

    // The byte is a valid varint, but not a valid tag (zero).
    verify(false == message.ParseFromString(MakeInvalidGroup("\0", 1, false)));

    // The byte is a malformed varint.
    verify(false == message.ParseFromString(MakeInvalidGroup("\200", 1, false)));

    // The byte is an endgroup tag, but not the right one for this group.
    verify(false == message.ParseFromString(MakeInvalidGroup("\014", 1, false)));

    // The byte is a valid varint but not a valid tag (bad wire type).
    verify(false == message.ParseFromString(MakeInvalidGroup("\017", 1, true)));
  }

  // TEST_F(WireFormatInvalidInputTest, InvalidUnknownGroup)
  {
    // Use TestEmptyMessage so that the group made by MakeInvalidGroup will not
    // be a known tag number.
    google::protobuf::unittest::TestEmptyMessage message;

    // Control case.
    verify(message.ParseFromString(MakeInvalidGroup("", 0, true)));

    // Missing end tag.  Groups cannot end at EOF.
    verify(false == message.ParseFromString(MakeInvalidGroup("", 0, false)));

    // The byte is a valid varint, but not a valid tag (zero).
    verify(false == message.ParseFromString(MakeInvalidGroup("\0", 1, false)));

    // The byte is a malformed varint.
    verify(false == message.ParseFromString(MakeInvalidGroup("\200", 1, false)));

    // The byte is an endgroup tag, but not the right one for this group.
    verify(false == message.ParseFromString(MakeInvalidGroup("\014", 1, false)));

    // The byte is a valid varint but not a valid tag (bad wire type).
    verify(false == message.ParseFromString(MakeInvalidGroup("\017", 1, true)));
  }

  // TEST_F(WireFormatInvalidInputTest, InvalidStringInUnknownGroup)
  {
    // Test a bug fix:  SkipMessage should fail if the message contains a fun::string
    // whose length would extend beyond the message end.

    google::protobuf::unittest::TestAllTypes message;
    message.set_optional_string("foo foo foo foo");
    fun::string data;
    message.SerializeToString(&data);

    // Chop some bytes off the end.
    data.resize(data.size() - 4);

    // Try to skip it.  Note that the bug was only present when parsing to an
    // UnknownFieldSet.
    google::protobuf::io::ArrayInputStream raw_input(data.data(), data.size());
    google::protobuf::io::CodedInputStream coded_input(&raw_input);
    google::protobuf::UnknownFieldSet unknown_fields;
    verify(false == WireFormat::SkipMessage(&coded_input, &unknown_fields));
  }
  // // // //

  // // // //
  // Utf8ValidationTest

#ifndef _MSC_VER
#define GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
#endif

  auto StartsWith = [](const fun::string& s, const fun::string& prefix) -> bool {
    return s.substr(0, prefix.length()) == prefix;
  };

  // TEST_F(Utf8ValidationTest, WriteInvalidUTF8String)
  {
    fun::string wire_buffer;
    protobuf_unittest::OneString input;
    fun::vector<fun::string> errors;
    {
      google::protobuf::ScopedMemoryLog log;
      WriteMessage(kInvalidUTF8String, &input, &wire_buffer);
      errors = log.GetMessages(google::protobuf::ERROR);
    }
#ifdef GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
    verify(1 == errors.size());
    verify(StartsWith(errors[0],
      "String field 'data' contains invalid UTF-8 data when "
      "serializing a protocol buffer. Use the "
      "'bytes' type if you intend to send raw bytes."));
#else
    verify(0 == errors.size());
#endif  // GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
  }


  // TEST_F(Utf8ValidationTest, ReadInvalidUTF8String)
  {
    fun::string wire_buffer;
    protobuf_unittest::OneString input;
    WriteMessage(kInvalidUTF8String, &input, &wire_buffer);
    protobuf_unittest::OneString output;
    fun::vector<fun::string> errors;
    {
      google::protobuf::ScopedMemoryLog log;
      ReadMessage(wire_buffer, &output);
      errors = log.GetMessages(google::protobuf::ERROR);
    }
#ifdef GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
    verify(1 == errors.size());
    verify(StartsWith(errors[0],
      "String field 'data' contains invalid UTF-8 data when "
      "parsing a protocol buffer. Use the "
      "'bytes' type if you intend to send raw bytes."));

#else
    verify(0 == errors.size());
#endif  // GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
  }


  // TEST_F(Utf8ValidationTest, WriteValidUTF8String)
  {
    fun::string wire_buffer;
    protobuf_unittest::OneString input;
    fun::vector<fun::string> errors;
    {
      google::protobuf::ScopedMemoryLog log;
      WriteMessage(kValidUTF8String, &input, &wire_buffer);
      errors = log.GetMessages(google::protobuf::ERROR);
    }
    verify(0 == errors.size());
  }

  // TEST_F(Utf8ValidationTest, ReadValidUTF8String)
  {
    fun::string wire_buffer;
    protobuf_unittest::OneString input;
    WriteMessage(kValidUTF8String, &input, &wire_buffer);
    protobuf_unittest::OneString output;
    fun::vector<fun::string> errors;
    {
      google::protobuf::ScopedMemoryLog log;
      ReadMessage(wire_buffer, &output);
      errors = log.GetMessages(google::protobuf::ERROR);
    }
    verify(0 == errors.size());
    verify(input.data() == output.data());
  }

  // Bytes: anything can pass as bytes, use invalid UTF-8 fun::string to test
  // TEST_F(Utf8ValidationTest, WriteArbitraryBytes)
  {
    fun::string wire_buffer;
    protobuf_unittest::OneBytes input;
    fun::vector<fun::string> errors;
    {
      google::protobuf::ScopedMemoryLog log;
      WriteMessage(kInvalidUTF8String, &input, &wire_buffer);
      errors = log.GetMessages(google::protobuf::ERROR);
    }
    verify(0 == errors.size());
  }

  // TEST_F(Utf8ValidationTest, ReadArbitraryBytes)
  {
    fun::string wire_buffer;
    protobuf_unittest::OneBytes input;
    WriteMessage(kInvalidUTF8String, &input, &wire_buffer);
    protobuf_unittest::OneBytes output;
    fun::vector<fun::string> errors;
    {
      google::protobuf::ScopedMemoryLog log;
      ReadMessage(wire_buffer, &output);
      errors = log.GetMessages(google::protobuf::ERROR);
    }
    verify(0 == errors.size());
    verify(input.data() == output.data());
  }

  // TEST_F(Utf8ValidationTest, ParseRepeatedString)
  {
    protobuf_unittest::MoreBytes input;
    input.add_data(kValidUTF8String);
    input.add_data(kInvalidUTF8String);
    input.add_data(kInvalidUTF8String);
    fun::string wire_buffer = input.SerializeAsString();

    protobuf_unittest::MoreString output;
    fun::vector<fun::string> errors;
    {
      google::protobuf::ScopedMemoryLog log;
      ReadMessage(wire_buffer, &output);
      errors = log.GetMessages(google::protobuf::ERROR);
    }
#ifdef GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
    verify(2 == errors.size());
#else
    verify(0 == errors.size());
#endif  // GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
    verify(wire_buffer == output.SerializeAsString());
  }

  /*
  // Test the old VerifyUTF8String() function, which may still be called by old
  // generated code.
  // TEST_F(Utf8ValidationTest, OldVerifyUTF8String)
  {
    fun::string data(kInvalidUTF8String);

    fun::vector<fun::string> errors;
    {
      google::protobuf::ScopedMemoryLog log;
      WireFormat::VerifyUTF8String(data.data(), data.size(),
        WireFormat::SERIALIZE);
      errors = log.GetMessages(google::protobuf::ERROR);
    }
#ifdef GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
    verify(1 == errors.size());
    verify(StartsWith(errors[0],
      "String field contains invalid UTF-8 data when "
      "serializing a protocol buffer. Use the "
      "'bytes' type if you intend to send raw bytes."));
#else
    verify(0 == errors.size());
#endif
  }
  */

  return true;
}

#endif