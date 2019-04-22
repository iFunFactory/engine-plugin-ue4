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
#include <algorithm>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFunapiLibProtobufMergedDescriptorDatabaseTest, "LibProtobuf.MergedDescriptorDatabaseTest", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFunapiLibProtobufMergedDescriptorDatabaseTest::RunTest(const FString& Parameters)
{
  auto AddToDatabase = [](google::protobuf::SimpleDescriptorDatabase* database,
                          const char* file_text)
  {
    google::protobuf::FileDescriptorProto file_proto;
    // EXPECT_TRUE(TextFormat::ParseFromString(file_text, &file_proto));
    verify(google::protobuf::TextFormat::ParseFromString(file_text, &file_proto));
    database->Add(file_proto);
  };

  auto ExpectContainsType = [](const google::protobuf::FileDescriptorProto& proto,
                               const fun::string& type_name) ->bool {
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

  google::protobuf::SimpleDescriptorDatabase database1_;
  google::protobuf::SimpleDescriptorDatabase database2_;

  google::protobuf::MergedDescriptorDatabase forward_merged_(&database1_, &database2_);
  google::protobuf::MergedDescriptorDatabase reverse_merged_(&database2_, &database1_);

  // SetUp
  AddToDatabase(&database1_,
    "name: \"foo.proto\" "
    "message_type { name:\"Foo\" extension_range { start: 1 end: 100 } } "
    "extension { name:\"foo_ext\" extendee: \".Foo\" number:3 "
    "            label:LABEL_OPTIONAL type:TYPE_INT32 } ");
  AddToDatabase(&database2_,
    "name: \"bar.proto\" "
    "message_type { name:\"Bar\" extension_range { start: 1 end: 100 } } "
    "extension { name:\"bar_ext\" extendee: \".Bar\" number:5 "
    "            label:LABEL_OPTIONAL type:TYPE_INT32 } ");

  // baz.proto exists in both pools, with different definitions.
  AddToDatabase(&database1_,
    "name: \"baz.proto\" "
    "message_type { name:\"Baz\" extension_range { start: 1 end: 100 } } "
    "message_type { name:\"FromPool1\" } "
    "extension { name:\"baz_ext\" extendee: \".Baz\" number:12 "
    "            label:LABEL_OPTIONAL type:TYPE_INT32 } "
    "extension { name:\"database1_only_ext\" extendee: \".Baz\" number:13 "
    "            label:LABEL_OPTIONAL type:TYPE_INT32 } ");
  AddToDatabase(&database2_,
    "name: \"baz.proto\" "
    "message_type { name:\"Baz\" extension_range { start: 1 end: 100 } } "
    "message_type { name:\"FromPool2\" } "
    "extension { name:\"baz_ext\" extendee: \".Baz\" number:12 "
    "            label:LABEL_OPTIONAL type:TYPE_INT32 } ");

  // FindFileByName
  {
    // Can find file that is only in database1_.
    google::protobuf::FileDescriptorProto file;
    verify(forward_merged_.FindFileByName("foo.proto", &file));
    verify("foo.proto" == file.name());
    verify(ExpectContainsType(file, "Foo"));
  }

  {
    // Can find file that is only in database2_.
    google::protobuf::FileDescriptorProto file;
    verify(forward_merged_.FindFileByName("bar.proto", &file));
    verify("bar.proto" == file.name());
    verify(ExpectContainsType(file, "Bar"));
  }

  {
    // In forward_merged_, database1_'s baz.proto takes precedence.
    google::protobuf::FileDescriptorProto file;
    verify(forward_merged_.FindFileByName("baz.proto", &file));
    verify("baz.proto" == file.name());
    verify(ExpectContainsType(file, "FromPool1"));
  }

  {
    // In reverse_merged_, database2_'s baz.proto takes precedence.
    google::protobuf::FileDescriptorProto file;
    verify(reverse_merged_.FindFileByName("baz.proto", &file));
    verify("baz.proto" == file.name());
    verify(ExpectContainsType(file, "FromPool2"));
  }

  {
    // Can't find non-existent file.
    google::protobuf::FileDescriptorProto file;
    verify(false == forward_merged_.FindFileByName("no_such.proto", &file));
  }
  // // // //

  // FindFileContainingSymbol
  {
    // Can find file that is only in database1_.
    google::protobuf::FileDescriptorProto file;
    verify(forward_merged_.FindFileContainingSymbol("Foo", &file));
    verify("foo.proto" == file.name());
    verify(ExpectContainsType(file, "Foo"));
  }

  {
    // Can find file that is only in database2_.
    google::protobuf::FileDescriptorProto file;
    verify(forward_merged_.FindFileContainingSymbol("Bar", &file));
    verify("bar.proto" == file.name());
    verify(ExpectContainsType(file, "Bar"));
  }

  {
    // In forward_merged_, database1_'s baz.proto takes precedence.
    google::protobuf::FileDescriptorProto file;
    verify(forward_merged_.FindFileContainingSymbol("Baz", &file));
    verify("baz.proto" == file.name());
    verify(ExpectContainsType(file, "FromPool1"));
  }

  {
    // In reverse_merged_, database2_'s baz.proto takes precedence.
    google::protobuf::FileDescriptorProto file;
    verify(reverse_merged_.FindFileContainingSymbol("Baz", &file));
    verify("baz.proto" == file.name());
    verify(ExpectContainsType(file, "FromPool2"));
  }

  {
    // FromPool1 only shows up in forward_merged_ because it is masked by
    // database2_'s baz.proto in reverse_merged_.
    google::protobuf::FileDescriptorProto file;
    verify(forward_merged_.FindFileContainingSymbol("FromPool1", &file));
    verify(false == reverse_merged_.FindFileContainingSymbol("FromPool1", &file));
  }

  {
    // Can't find non-existent symbol.
    google::protobuf::FileDescriptorProto file;
    verify(
      false == forward_merged_.FindFileContainingSymbol("NoSuchType", &file));
  }
  // // // //

  // FindFileContainingExtension
  {
    // Can find file that is only in database1_.
    google::protobuf::FileDescriptorProto file;
    verify(
      forward_merged_.FindFileContainingExtension("Foo", 3, &file));
    verify("foo.proto" == file.name());
    verify(ExpectContainsType(file, "Foo"));
  }

  {
    // Can find file that is only in database2_.
    google::protobuf::FileDescriptorProto file;
    verify(
      forward_merged_.FindFileContainingExtension("Bar", 5, &file));
    verify("bar.proto" == file.name());
    verify(ExpectContainsType(file, "Bar"));
  }

  {
    // In forward_merged_, database1_'s baz.proto takes precedence.
    google::protobuf::FileDescriptorProto file;
    verify(
      forward_merged_.FindFileContainingExtension("Baz", 12, &file));
    verify("baz.proto" == file.name());
    verify(ExpectContainsType(file, "FromPool1"));
  }

  {
    // In reverse_merged_, database2_'s baz.proto takes precedence.
    google::protobuf::FileDescriptorProto file;
    verify(
      reverse_merged_.FindFileContainingExtension("Baz", 12, &file));
    verify("baz.proto" == file.name());
    verify(ExpectContainsType(file, "FromPool2"));
  }

  {
    // Baz's extension 13 only shows up in forward_merged_ because it is
    // masked by database2_'s baz.proto in reverse_merged_.
    google::protobuf::FileDescriptorProto file;
    verify(forward_merged_.FindFileContainingExtension("Baz", 13, &file));
    verify(false == reverse_merged_.FindFileContainingExtension("Baz", 13, &file));
  }

  {
    // Can't find non-existent extension.
    google::protobuf::FileDescriptorProto file;
    verify(
      false == forward_merged_.FindFileContainingExtension("Foo", 6, &file));
  }
  // // // //

  // FindAllExtensionNumbers
  {
    // Message only has extension in database1_
    fun::vector<int> numbers;
    verify(forward_merged_.FindAllExtensionNumbers("Foo", &numbers));
    verify(1 == numbers.size());
    verify(3 == numbers[0]);
  }

  {
    // Message only has extension in database2_
    fun::vector<int> numbers;
    verify(forward_merged_.FindAllExtensionNumbers("Bar", &numbers));
    verify(1 == numbers.size());
    verify(5 == numbers[0]);
  }

  {
    // Merge results from the two databases.
    fun::vector<int> numbers;
    verify(forward_merged_.FindAllExtensionNumbers("Baz", &numbers));
    verify(2 == numbers.size());
    sort(numbers.begin(), numbers.end());
    verify(12 == numbers[0]);
    verify(13 == numbers[1]);
  }

  {
    fun::vector<int> numbers;
    verify(reverse_merged_.FindAllExtensionNumbers("Baz", &numbers));
    verify(2 == numbers.size());
    sort(numbers.begin(), numbers.end());
    verify(12 == numbers[0]);
    verify(13 == numbers[1]);
  }

  {
    // Can't find extensions for a non-existent message.
    fun::vector<int> numbers;
    verify(false == reverse_merged_.FindAllExtensionNumbers("Blah", &numbers));
  }
  // // // //

  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFunapiLibProtobufDescriptorUnittest, "LibProtobuf.DescriptorUnittest", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFunapiLibProtobufDescriptorUnittest::RunTest(const FString& Parameters)
{
  // Some helpers to make assembling descriptors faster.
  auto AddMessage = [](google::protobuf::FileDescriptorProto* file, const fun::string& name) -> google::protobuf::DescriptorProto* {
    google::protobuf::DescriptorProto* result = file->add_message_type();
    result->set_name(name);
    return result;
  };

  auto AddNestedMessage = [](google::protobuf::DescriptorProto* parent, const fun::string& name) -> google::protobuf::DescriptorProto* {
    google::protobuf::DescriptorProto* result = parent->add_nested_type();
    result->set_name(name);
    return result;
  };

  auto AddEnum = [](google::protobuf::FileDescriptorProto* file, const fun::string& name) -> google::protobuf::EnumDescriptorProto* {
    google::protobuf::EnumDescriptorProto* result = file->add_enum_type();
    result->set_name(name);
    return result;
  };

  auto AddNestedEnum = [](google::protobuf::DescriptorProto* parent, const fun::string& name) -> google::protobuf::EnumDescriptorProto* {
    google::protobuf::EnumDescriptorProto* result = parent->add_enum_type();
    result->set_name(name);
    return result;
  };

  auto AddService = [](google::protobuf::FileDescriptorProto* file,
    const fun::string& name) -> google::protobuf::ServiceDescriptorProto* {
    google::protobuf::ServiceDescriptorProto* result = file->add_service();
    result->set_name(name);
    return result;
  };

  auto AddField = [](google::protobuf::DescriptorProto* parent,
    const fun::string& name, int number,
    google::protobuf::FieldDescriptorProto::Label label,
    google::protobuf::FieldDescriptorProto::Type type) -> google::protobuf::FieldDescriptorProto* {
    google::protobuf::FieldDescriptorProto* result = parent->add_field();
    result->set_name(name);
    result->set_number(number);
    result->set_label(label);
    result->set_type(type);
    return result;
  };

  auto AddExtension = [](google::protobuf::FileDescriptorProto* file,
    const fun::string& extendee,
    const fun::string& name, int number,
    google::protobuf::FieldDescriptorProto::Label label,
    google::protobuf::FieldDescriptorProto::Type type) -> google::protobuf::FieldDescriptorProto* {
    google::protobuf::FieldDescriptorProto* result = file->add_extension();
    result->set_name(name);
    result->set_number(number);
    result->set_label(label);
    result->set_type(type);
    result->set_extendee(extendee);
    return result;
  };

  auto AddNestedExtension = [](google::protobuf::DescriptorProto* parent,
    const fun::string& extendee,
    const fun::string& name, int number,
    google::protobuf::FieldDescriptorProto::Label label,
    google::protobuf::FieldDescriptorProto::Type type) -> google::protobuf::FieldDescriptorProto* {
    google::protobuf::FieldDescriptorProto* result = parent->add_extension();
    result->set_name(name);
    result->set_number(number);
    result->set_label(label);
    result->set_type(type);
    result->set_extendee(extendee);
    return result;
  };

  auto AddExtensionRange = [](google::protobuf::DescriptorProto* parent,
    int start, int end) -> google::protobuf::DescriptorProto::ExtensionRange* {
    google::protobuf::DescriptorProto::ExtensionRange* result = parent->add_extension_range();
    result->set_start(start);
    result->set_end(end);
    return result;
  };

  auto AddEnumValue = [](google::protobuf::EnumDescriptorProto* enum_proto,
    const fun::string& name, int number) -> google::protobuf::EnumValueDescriptorProto* {
    google::protobuf::EnumValueDescriptorProto* result = enum_proto->add_value();
    result->set_name(name);
    result->set_number(number);
    return result;
  };

  auto AddMethod = [](google::protobuf::ServiceDescriptorProto* service,
    const fun::string& name,
    const fun::string& input_type,
    const fun::string& output_type) -> google::protobuf::MethodDescriptorProto* {
    google::protobuf::MethodDescriptorProto* result = service->add_method();
    result->set_name(name);
    result->set_input_type(input_type);
    result->set_output_type(output_type);
    return result;
  };

  // Empty enums technically aren't allowed.  We need to insert a dummy value
  // into them.
  auto AddEmptyEnum = [&AddEnumValue, &AddEnum](google::protobuf::FileDescriptorProto* file, const fun::string& name) {
    AddEnumValue(AddEnum(file, name), name + "_DUMMY", 1);
  };

  // // // //
  // FileDescriptorTest
  {
    google::protobuf::DescriptorPool pool_;

    const google::protobuf::FileDescriptor* foo_file_;
    const google::protobuf::FileDescriptor* bar_file_;
    const google::protobuf::FileDescriptor* baz_file_;

    const google::protobuf::Descriptor*          foo_message_;
    const google::protobuf::EnumDescriptor*      foo_enum_;
    const google::protobuf::EnumValueDescriptor* foo_enum_value_;
    const google::protobuf::ServiceDescriptor*   foo_service_;
    const google::protobuf::FieldDescriptor*     foo_extension_;

    const google::protobuf::Descriptor*          bar_message_;
    const google::protobuf::EnumDescriptor*      bar_enum_;
    const google::protobuf::EnumValueDescriptor* bar_enum_value_;
    const google::protobuf::ServiceDescriptor*   bar_service_;
    const google::protobuf::FieldDescriptor*     bar_extension_;

    auto SetUp = [&AddExtensionRange, &AddEnumValue, &AddService, &AddExtension, &AddMessage, &AddEnum,
      &foo_file_, &bar_file_, &baz_file_, &pool_,
      &foo_message_, &foo_enum_, &foo_enum_value_, &foo_service_, &foo_extension_,
      &bar_message_, &bar_enum_, &bar_enum_value_, &bar_service_, &bar_extension_]() {
      // Build descriptors for the following definitions:
      //
      //   // in "foo.proto"
      //   message FooMessage { extensions 1; }
      //   enum FooEnum {FOO_ENUM_VALUE = 1;}
      //   service FooService {}
      //   extend FooMessage { optional int32 foo_extension = 1; }
      //
      //   // in "bar.proto"
      //   package bar_package;
      //   message BarMessage { extensions 1; }
      //   enum BarEnum {BAR_ENUM_VALUE = 1;}
      //   service BarService {}
      //   extend BarMessage { optional int32 bar_extension = 1; }
      //
      // Also, we have an empty file "baz.proto".  This file's purpose is to
      // make sure that even though it has the same package as foo.proto,
      // searching it for members of foo.proto won't work.

      google::protobuf::FileDescriptorProto foo_file;
      foo_file.set_name("foo.proto");
      AddExtensionRange(AddMessage(&foo_file, "FooMessage"), 1, 2);
      AddEnumValue(AddEnum(&foo_file, "FooEnum"), "FOO_ENUM_VALUE", 1);
      AddService(&foo_file, "FooService");
      AddExtension(&foo_file, "FooMessage", "foo_extension", 1,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);

      google::protobuf::FileDescriptorProto bar_file;
      bar_file.set_name("bar.proto");
      bar_file.set_package("bar_package");
      bar_file.add_dependency("foo.proto");
      AddExtensionRange(AddMessage(&bar_file, "BarMessage"), 1, 2);
      AddEnumValue(AddEnum(&bar_file, "BarEnum"), "BAR_ENUM_VALUE", 1);
      AddService(&bar_file, "BarService");
      AddExtension(&bar_file, "bar_package.BarMessage", "bar_extension", 1,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);

      google::protobuf::FileDescriptorProto baz_file;
      baz_file.set_name("baz.proto");

      // Build the descriptors and get the pointers.
      foo_file_ = pool_.BuildFile(foo_file);
      verify(foo_file_ != NULL);

      bar_file_ = pool_.BuildFile(bar_file);
      verify(bar_file_ != NULL);

      baz_file_ = pool_.BuildFile(baz_file);
      verify(baz_file_ != NULL);

      verify(1 == foo_file_->message_type_count());
      foo_message_ = foo_file_->message_type(0);
      verify(1 == foo_file_->enum_type_count());
      foo_enum_ = foo_file_->enum_type(0);
      verify(1 == foo_enum_->value_count());
      foo_enum_value_ = foo_enum_->value(0);
      verify(1 == foo_file_->service_count());
      foo_service_ = foo_file_->service(0);
      verify(1 == foo_file_->extension_count());
      foo_extension_ = foo_file_->extension(0);

      verify(1 == bar_file_->message_type_count());
      bar_message_ = bar_file_->message_type(0);
      verify(1 == bar_file_->enum_type_count());
      bar_enum_ = bar_file_->enum_type(0);
      verify(1 == bar_enum_->value_count());
      bar_enum_value_ = bar_enum_->value(0);
      verify(1 == bar_file_->service_count());
      bar_service_ = bar_file_->service(0);
      verify(1 == bar_file_->extension_count());
      bar_extension_ = bar_file_->extension(0);
    };

    SetUp();

    // FileDescriptorTest, Name
    verify("foo.proto" == foo_file_->name());
    verify("bar.proto" == bar_file_->name());
    verify("baz.proto" == baz_file_->name());

    // FileDescriptorTest, Package
    verify("" == foo_file_->package());
    verify("bar_package" == bar_file_->package());

    // FileDescriptorTest, Dependencies
    verify(0 == foo_file_->dependency_count());
    verify(1 == bar_file_->dependency_count());
    verify(foo_file_ == bar_file_->dependency(0));

    // FileDescriptorTest, FindMessageTypeByName
    verify(foo_message_ == foo_file_->FindMessageTypeByName("FooMessage"));
    verify(bar_message_ == bar_file_->FindMessageTypeByName("BarMessage"));

    verify(foo_file_->FindMessageTypeByName("BarMessage") == NULL);
    verify(bar_file_->FindMessageTypeByName("FooMessage") == NULL);
    verify(baz_file_->FindMessageTypeByName("FooMessage") == NULL);

    verify(foo_file_->FindMessageTypeByName("NoSuchMessage") == NULL);
    verify(foo_file_->FindMessageTypeByName("FooEnum") == NULL);

    // FileDescriptorTest, FindEnumTypeByName
    verify(foo_enum_ == foo_file_->FindEnumTypeByName("FooEnum"));
    verify(bar_enum_ == bar_file_->FindEnumTypeByName("BarEnum"));

    verify(foo_file_->FindEnumTypeByName("BarEnum") == NULL);
    verify(bar_file_->FindEnumTypeByName("FooEnum") == NULL);
    verify(baz_file_->FindEnumTypeByName("FooEnum") == NULL);

    verify(foo_file_->FindEnumTypeByName("NoSuchEnum") == NULL);
    verify(foo_file_->FindEnumTypeByName("FooMessage") == NULL);

    // FileDescriptorTest, FindEnumValueByName
    verify(foo_enum_value_ == foo_file_->FindEnumValueByName("FOO_ENUM_VALUE"));
    verify(bar_enum_value_ == bar_file_->FindEnumValueByName("BAR_ENUM_VALUE"));

    verify(foo_file_->FindEnumValueByName("BAR_ENUM_VALUE") == NULL);
    verify(bar_file_->FindEnumValueByName("FOO_ENUM_VALUE") == NULL);
    verify(baz_file_->FindEnumValueByName("FOO_ENUM_VALUE") == NULL);

    verify(foo_file_->FindEnumValueByName("NO_SUCH_VALUE") == NULL);
    verify(foo_file_->FindEnumValueByName("FooMessage") == NULL);

    // FileDescriptorTest, FindServiceByName
    verify(foo_service_ == foo_file_->FindServiceByName("FooService"));
    verify(bar_service_ == bar_file_->FindServiceByName("BarService"));

    verify(foo_file_->FindServiceByName("BarService") == NULL);
    verify(bar_file_->FindServiceByName("FooService") == NULL);
    verify(baz_file_->FindServiceByName("FooService") == NULL);

    verify(foo_file_->FindServiceByName("NoSuchService") == NULL);
    verify(foo_file_->FindServiceByName("FooMessage") == NULL);

    // FileDescriptorTest, FindExtensionByName
    verify(foo_extension_ == foo_file_->FindExtensionByName("foo_extension"));
    verify(bar_extension_ == bar_file_->FindExtensionByName("bar_extension"));

    verify(foo_file_->FindExtensionByName("bar_extension") == NULL);
    verify(bar_file_->FindExtensionByName("foo_extension") == NULL);
    verify(baz_file_->FindExtensionByName("foo_extension") == NULL);

    verify(foo_file_->FindExtensionByName("no_such_extension") == NULL);
    verify(foo_file_->FindExtensionByName("FooMessage") == NULL);

    // FileDescriptorTest, FindExtensionByNumber
    verify(foo_extension_ == pool_.FindExtensionByNumber(foo_message_, 1));
    verify(bar_extension_ == pool_.FindExtensionByNumber(bar_message_, 1));

    verify(pool_.FindExtensionByNumber(foo_message_, 2) == NULL);

    // FileDescriptorTest, BuildAgain
    // Test that if te call BuildFile again on the same input we get the same
    // FileDescriptor back.
    google::protobuf::FileDescriptorProto file;
    foo_file_->CopyTo(&file);
    verify(foo_file_ == pool_.BuildFile(file));

    // But if we change the file then it won't work.
    file.set_package("some.other.package");
    verify(pool_.BuildFile(file) == NULL);
  }
  // // // //

  // // // //
  // DescriptorTest
  {
    google::protobuf::DescriptorPool pool_;

    const google::protobuf::FileDescriptor* foo_file_;
    const google::protobuf::FileDescriptor* bar_file_;

    const google::protobuf::Descriptor* message_;
    const google::protobuf::Descriptor* message2_;
    const google::protobuf::Descriptor* foreign_;
    const google::protobuf::EnumDescriptor* enum_;

    const google::protobuf::FieldDescriptor* foo_;
    const google::protobuf::FieldDescriptor* bar_;
    const google::protobuf::FieldDescriptor* baz_;
    const google::protobuf::FieldDescriptor* qux_;

    const google::protobuf::FieldDescriptor* foo2_;
    const google::protobuf::FieldDescriptor* bar2_;
    const google::protobuf::FieldDescriptor* quux2_;

    auto SetUp = [&AddMessage, &AddNestedMessage, &AddEnum, &AddNestedEnum, &AddService, &AddField,
      &AddExtension, &AddNestedExtension, &AddExtensionRange, &AddEnumValue, &AddMethod, &AddEmptyEnum,
      &pool_, &foo_file_, &bar_file_,
      &message_, &message2_, &foreign_, &enum_,
      &foo_, &bar_, &baz_, &qux_,
      &foo2_, &bar2_, &quux2_]()
    {
      // Build descriptors for the following definitions:
      //
      //   // in "foo.proto"
      //   message TestForeign {}
      //   enum TestEnum {}
      //
      //   message TestMessage {
      //     required fun::string      foo = 1;
      //     optional TestEnum    bar = 6;
      //     repeated TestForeign baz = 500000000;
      //     optional group       qux = 15 {}
      //   }
      //
      //   // in "bar.proto"
      //   package corge.grault;
      //   message TestMessage2 {
      //     required fun::string foo = 1;
      //     required fun::string bar = 2;
      //     required fun::string quux = 6;
      //   }
      //
      // We cheat and use TestForeign as the type for qux rather than create
      // an actual nested type.
      //
      // Since all primitive types (including string) use the same building
      // code, there's no need to test each one individually.
      //
      // TestMessage2 is primarily here to test FindFieldByName and friends.
      // All messages created from the same DescriptorPool share the same lookup
      // table, so we need to insure that they don't interfere.

      google::protobuf::FileDescriptorProto foo_file;
      foo_file.set_name("foo.proto");
      AddMessage(&foo_file, "TestForeign");
      AddEmptyEnum(&foo_file, "TestEnum");

      google::protobuf::DescriptorProto* message = AddMessage(&foo_file, "TestMessage");
      AddField(message, "foo", 1,
        google::protobuf::FieldDescriptorProto::LABEL_REQUIRED,
        google::protobuf::FieldDescriptorProto::TYPE_STRING);
      AddField(message, "bar", 6,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_ENUM)
        ->set_type_name("TestEnum");
      AddField(message, "baz", 500000000,
        google::protobuf::FieldDescriptorProto::LABEL_REPEATED,
        google::protobuf::FieldDescriptorProto::TYPE_MESSAGE)
        ->set_type_name("TestForeign");
      AddField(message, "qux", 15,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_GROUP)
        ->set_type_name("TestForeign");

      google::protobuf::FileDescriptorProto bar_file;
      bar_file.set_name("bar.proto");
      bar_file.set_package("corge.grault");

      google::protobuf::DescriptorProto* message2 = AddMessage(&bar_file, "TestMessage2");
      AddField(message2, "foo", 1,
        google::protobuf::FieldDescriptorProto::LABEL_REQUIRED,
        google::protobuf::FieldDescriptorProto::TYPE_STRING);
      AddField(message2, "bar", 2,
        google::protobuf::FieldDescriptorProto::LABEL_REQUIRED,
        google::protobuf::FieldDescriptorProto::TYPE_STRING);
      AddField(message2, "quux", 6,
        google::protobuf::FieldDescriptorProto::LABEL_REQUIRED,
        google::protobuf::FieldDescriptorProto::TYPE_STRING);

      // Build the descriptors and get the pointers.
      foo_file_ = pool_.BuildFile(foo_file);
      verify(foo_file_ != NULL);

      bar_file_ = pool_.BuildFile(bar_file);
      verify(bar_file_ != NULL);

      verify(1 == foo_file_->enum_type_count());
      enum_ = foo_file_->enum_type(0);

      verify(2 == foo_file_->message_type_count());
      foreign_ = foo_file_->message_type(0);
      message_ = foo_file_->message_type(1);

      verify(4 == message_->field_count());
      foo_ = message_->field(0);
      bar_ = message_->field(1);
      baz_ = message_->field(2);
      qux_ = message_->field(3);

      verify(1 == bar_file_->message_type_count());
      message2_ = bar_file_->message_type(0);

      verify(3 == message2_->field_count());
      foo2_ = message2_->field(0);
      bar2_ = message2_->field(1);
      quux2_ = message2_->field(2);
    };

    SetUp();

    // DescriptorTest, Name
    verify("TestMessage" == message_->name());
    verify("TestMessage" == message_->full_name());
    verify(foo_file_ == message_->file());

    verify("TestMessage2" == message2_->name());
    verify("corge.grault.TestMessage2" == message2_->full_name());
    verify(bar_file_ == message2_->file());

    // DescriptorTest, ContainingType
    verify(message_->containing_type() == NULL);
    verify(message2_->containing_type() == NULL);

    // DescriptorTest, FieldsByIndex
    verify(4 == message_->field_count());
    verify(foo_ == message_->field(0));
    verify(bar_ == message_->field(1));
    verify(baz_ == message_->field(2));
    verify(qux_ == message_->field(3));

    // DescriptorTest, FindFieldByName
    // All messages in the same DescriptorPool share a single lookup table for
    // fields.  So, in addition to testing that FindFieldByName finds the fields
    // of the message, we need to test that it does *not* find the fields of
    // *other* messages.

    verify(foo_ == message_->FindFieldByName("foo"));
    verify(bar_ == message_->FindFieldByName("bar"));
    verify(baz_ == message_->FindFieldByName("baz"));
    verify(qux_ == message_->FindFieldByName("qux"));
    verify(message_->FindFieldByName("no_such_field") == NULL);
    verify(message_->FindFieldByName("quux") == NULL);

    verify(foo2_ == message2_->FindFieldByName("foo"));
    verify(bar2_ == message2_->FindFieldByName("bar"));
    verify(quux2_ == message2_->FindFieldByName("quux"));
    verify(message2_->FindFieldByName("baz") == NULL);
    verify(message2_->FindFieldByName("qux") == NULL);

    // DescriptorTest, FindFieldByNumber
    verify(foo_ == message_->FindFieldByNumber(1));
    verify(bar_ == message_->FindFieldByNumber(6));
    verify(baz_ == message_->FindFieldByNumber(500000000));
    verify(qux_ == message_->FindFieldByNumber(15));
    verify(message_->FindFieldByNumber(837592) == NULL);
    verify(message_->FindFieldByNumber(2) == NULL);

    verify(foo2_ == message2_->FindFieldByNumber(1));
    verify(bar2_ == message2_->FindFieldByNumber(2));
    verify(quux2_ == message2_->FindFieldByNumber(6));
    verify(message2_->FindFieldByNumber(15) == NULL);
    verify(message2_->FindFieldByNumber(500000000) == NULL);

    // DescriptorTest, FieldName
    verify("foo" == foo_->name());
    verify("bar" == bar_->name());
    verify("baz" == baz_->name());
    verify("qux" == qux_->name());

    // DescriptorTest, FieldFullName
    verify("TestMessage.foo" == foo_->full_name());
    verify("TestMessage.bar" == bar_->full_name());
    verify("TestMessage.baz" == baz_->full_name());
    verify("TestMessage.qux" == qux_->full_name());

    verify("corge.grault.TestMessage2.foo" == foo2_->full_name());
    verify("corge.grault.TestMessage2.bar" == bar2_->full_name());
    verify("corge.grault.TestMessage2.quux" == quux2_->full_name());

    // DescriptorTest, FieldFile
    verify(foo_file_ == foo_->file());
    verify(foo_file_ == bar_->file());
    verify(foo_file_ == baz_->file());
    verify(foo_file_ == qux_->file());

    verify(bar_file_ == foo2_->file());
    verify(bar_file_ == bar2_->file());
    verify(bar_file_ == quux2_->file());

    // DescriptorTest, FieldIndex
    verify(0 == foo_->index());
    verify(1 == bar_->index());
    verify(2 == baz_->index());
    verify(3 == qux_->index());

    // DescriptorTest, FieldNumber
    verify(1 == foo_->number());
    verify(6 == bar_->number());
    verify(500000000 == baz_->number());
    verify(15 == qux_->number());

    // DescriptorTest, FieldType
    verify(google::protobuf::FieldDescriptor::TYPE_STRING == foo_->type());
    verify(google::protobuf::FieldDescriptor::TYPE_ENUM == bar_->type());
    verify(google::protobuf::FieldDescriptor::TYPE_MESSAGE == baz_->type());
    verify(google::protobuf::FieldDescriptor::TYPE_GROUP == qux_->type());

    // DescriptorTest, FieldLabel
    verify(google::protobuf::FieldDescriptor::LABEL_REQUIRED == foo_->label());
    verify(google::protobuf::FieldDescriptor::LABEL_OPTIONAL == bar_->label());
    verify(google::protobuf::FieldDescriptor::LABEL_REPEATED == baz_->label());
    verify(google::protobuf::FieldDescriptor::LABEL_OPTIONAL == qux_->label());

    verify(foo_->is_required());
    verify(false == foo_->is_optional());
    verify(false == foo_->is_repeated());

    verify(false == bar_->is_required());
    verify(bar_->is_optional());
    verify(false == bar_->is_repeated());

    verify(false == baz_->is_required());
    verify(false == baz_->is_optional());
    verify(baz_->is_repeated());

    // DescriptorTest, FieldHasDefault
    verify(false == foo_->has_default_value());
    verify(false == bar_->has_default_value());
    verify(false == baz_->has_default_value());
    verify(false == qux_->has_default_value());

    // DescriptorTest, FieldContainingType
    verify(message_ == foo_->containing_type());
    verify(message_ == bar_->containing_type());
    verify(message_ == baz_->containing_type());
    verify(message_ == qux_->containing_type());

    verify(message2_ == foo2_->containing_type());
    verify(message2_ == bar2_->containing_type());
    verify(message2_ == quux2_->containing_type());

    // DescriptorTest, FieldMessageType
    verify(foo_->message_type() == NULL);
    verify(bar_->message_type() == NULL);

    verify(foreign_ == baz_->message_type());
    verify(foreign_ == qux_->message_type());

    // DescriptorTest, FieldEnumType
    verify(foo_->enum_type() == NULL);
    verify(baz_->enum_type() == NULL);
    verify(qux_->enum_type() == NULL);

    verify(enum_ == bar_->enum_type());
  }
  // // // //

  // // // //
  // OneofDescriptorTest
  {
    google::protobuf::DescriptorPool pool_;

    const google::protobuf::FileDescriptor* baz_file_;

    const google::protobuf::Descriptor* oneof_message_;

    const google::protobuf::OneofDescriptor* oneof_;
    const google::protobuf::OneofDescriptor* oneof2_;
    const google::protobuf::FieldDescriptor* a_;
    const google::protobuf::FieldDescriptor* b_;
    const google::protobuf::FieldDescriptor* c_;
    const google::protobuf::FieldDescriptor* d_;
    const google::protobuf::FieldDescriptor* e_;
    const google::protobuf::FieldDescriptor* f_;

    auto SetUp = [&AddMessage, &AddNestedMessage, &AddEnum, &AddNestedEnum, &AddService, &AddField,
      &AddExtension, &AddNestedExtension, &AddExtensionRange, &AddEnumValue, &AddMethod, &AddEmptyEnum,
      &pool_, &baz_file_, &oneof_message_,
      &oneof_, &oneof2_, &a_, &b_, &c_, &d_, &e_, &f_] ()
    {
      // Build descriptors for the following definitions:
      //
      //   package garply;
      //   message TestOneof {
      //     optional int32 a = 1;
      //     oneof foo {
      //       fun::string b = 2;
      //       TestOneof c = 3;
      //     }
      //     oneof bar {
      //       float d = 4;
      //     }
      //   }

      google::protobuf::FileDescriptorProto baz_file;
      baz_file.set_name("baz.proto");
      baz_file.set_package("garply");

      google::protobuf::DescriptorProto* oneof_message = AddMessage(&baz_file, "TestOneof");
      oneof_message->add_oneof_decl()->set_name("foo");
      oneof_message->add_oneof_decl()->set_name("bar");

      AddField(oneof_message, "a", 1,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);
      AddField(oneof_message, "b", 2,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_STRING);
      oneof_message->mutable_field(1)->set_oneof_index(0);
      AddField(oneof_message, "c", 3,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_MESSAGE);
      oneof_message->mutable_field(2)->set_oneof_index(0);
      oneof_message->mutable_field(2)->set_type_name("TestOneof");

      AddField(oneof_message, "d", 4,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_FLOAT);
      oneof_message->mutable_field(3)->set_oneof_index(1);

      // Build the descriptors and get the pointers.
      baz_file_ = pool_.BuildFile(baz_file);
      verify(baz_file_ != NULL);

      verify(1 == baz_file_->message_type_count());
      oneof_message_ = baz_file_->message_type(0);

      verify(2 == oneof_message_->oneof_decl_count());
      oneof_ = oneof_message_->oneof_decl(0);
      oneof2_ = oneof_message_->oneof_decl(1);

      verify(4 == oneof_message_->field_count());
      a_ = oneof_message_->field(0);
      b_ = oneof_message_->field(1);
      c_ = oneof_message_->field(2);
      d_ = oneof_message_->field(3);
    };

    SetUp();

    // OneofDescriptorTest, Normal
    {
      verify("foo" == oneof_->name());
      verify("garply.TestOneof.foo" == oneof_->full_name());
      verify(0 == oneof_->index());
      verify(2 == oneof_->field_count());
      verify(b_ == oneof_->field(0));
      verify(c_ == oneof_->field(1));
      verify(a_->containing_oneof() == NULL);
      verify(oneof_ == b_->containing_oneof());
      verify(oneof_ == c_->containing_oneof());
    }

    // OneofDescriptorTest, FindByName
    {
      verify(oneof_ == oneof_message_->FindOneofByName("foo"));
      verify(oneof2_ == oneof_message_->FindOneofByName("bar"));
      verify(oneof_message_->FindOneofByName("no_such_oneof") == NULL);
    }
  }
  // // // //

  // // // //
  // StylizedFieldNamesTest
  {
    google::protobuf::DescriptorPool pool_;
    const google::protobuf::FileDescriptor* file_;
    const google::protobuf::Descriptor* message_;

    auto SetUp = [&AddMessage, &AddNestedMessage, &AddEnum, &AddNestedEnum, &AddService, &AddField,
      &AddExtension, &AddNestedExtension, &AddExtensionRange, &AddEnumValue, &AddMethod, &AddEmptyEnum,
      &pool_, &file_, &message_] ()
    {
      google::protobuf::FileDescriptorProto file;
      file.set_name("foo.proto");

      AddExtensionRange(AddMessage(&file, "ExtendableMessage"), 1, 1000);

      google::protobuf::DescriptorProto* message = AddMessage(&file, "TestMessage");
      AddField(message, "foo_foo", 1,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);
      AddField(message, "FooBar", 2,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);
      AddField(message, "fooBaz", 3,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);
      AddField(message, "fooFoo", 4,  // Camel-case conflict with foo_foo.
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);
      AddField(message, "foobar", 5,  // Lower-case conflict with FooBar.
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);

      AddNestedExtension(message, "ExtendableMessage", "bar_foo", 1,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);
      AddNestedExtension(message, "ExtendableMessage", "BarBar", 2,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);
      AddNestedExtension(message, "ExtendableMessage", "BarBaz", 3,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);
      AddNestedExtension(message, "ExtendableMessage", "barFoo", 4,  // Conflict
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);
      AddNestedExtension(message, "ExtendableMessage", "barbar", 5,  // Conflict
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);

      AddExtension(&file, "ExtendableMessage", "baz_foo", 11,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);
      AddExtension(&file, "ExtendableMessage", "BazBar", 12,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);
      AddExtension(&file, "ExtendableMessage", "BazBaz", 13,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);
      AddExtension(&file, "ExtendableMessage", "bazFoo", 14,  // Conflict
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);
      AddExtension(&file, "ExtendableMessage", "bazbar", 15,  // Conflict
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);

      file_ = pool_.BuildFile(file);
      verify(file_ != NULL);
      verify(2 == file_->message_type_count());
      message_ = file_->message_type(1);
      verify("TestMessage" == message_->name());
      verify(5 == message_->field_count());
      verify(5 == message_->extension_count());
      verify(5 == file_->extension_count());
    };

    SetUp();

    // StylizedFieldNamesTest, LowercaseName
    {
      verify("foo_foo" == message_->field(0)->lowercase_name());
      verify("foobar" == message_->field(1)->lowercase_name());
      verify("foobaz" == message_->field(2)->lowercase_name());
      verify("foofoo" == message_->field(3)->lowercase_name());
      verify("foobar" == message_->field(4)->lowercase_name());

      verify("bar_foo" == message_->extension(0)->lowercase_name());
      verify("barbar" == message_->extension(1)->lowercase_name());
      verify("barbaz" == message_->extension(2)->lowercase_name());
      verify("barfoo" == message_->extension(3)->lowercase_name());
      verify("barbar" == message_->extension(4)->lowercase_name());

      verify("baz_foo" == file_->extension(0)->lowercase_name());
      verify("bazbar" == file_->extension(1)->lowercase_name());
      verify("bazbaz" == file_->extension(2)->lowercase_name());
      verify("bazfoo" == file_->extension(3)->lowercase_name());
      verify("bazbar" == file_->extension(4)->lowercase_name());
    }

    // StylizedFieldNamesTest, CamelcaseName
    {
      verify("fooFoo" == message_->field(0)->camelcase_name());
      verify("fooBar" == message_->field(1)->camelcase_name());
      verify("fooBaz" == message_->field(2)->camelcase_name());
      verify("fooFoo" == message_->field(3)->camelcase_name());
      verify("foobar" == message_->field(4)->camelcase_name());

      verify("barFoo" == message_->extension(0)->camelcase_name());
      verify("barBar" == message_->extension(1)->camelcase_name());
      verify("barBaz" == message_->extension(2)->camelcase_name());
      verify("barFoo" == message_->extension(3)->camelcase_name());
      verify("barbar" == message_->extension(4)->camelcase_name());

      verify("bazFoo" == file_->extension(0)->camelcase_name());
      verify("bazBar" == file_->extension(1)->camelcase_name());
      verify("bazBaz" == file_->extension(2)->camelcase_name());
      verify("bazFoo" == file_->extension(3)->camelcase_name());
      verify("bazbar" == file_->extension(4)->camelcase_name());
    }

    // StylizedFieldNamesTest, FindByLowercaseName
    {
      verify(message_->field(0) ==
        message_->FindFieldByLowercaseName("foo_foo"));
      verify(message_->field(1) ==
        message_->FindFieldByLowercaseName("foobar"));
      verify(message_->field(2) ==
        message_->FindFieldByLowercaseName("foobaz"));
      verify(message_->FindFieldByLowercaseName("FooBar") == NULL);
      verify(message_->FindFieldByLowercaseName("fooBaz") == NULL);
      verify(message_->FindFieldByLowercaseName("bar_foo") == NULL);
      verify(message_->FindFieldByLowercaseName("nosuchfield") == NULL);

      verify(message_->extension(0) ==
        message_->FindExtensionByLowercaseName("bar_foo"));
      verify(message_->extension(1) ==
        message_->FindExtensionByLowercaseName("barbar"));
      verify(message_->extension(2) ==
        message_->FindExtensionByLowercaseName("barbaz"));
      verify(message_->FindExtensionByLowercaseName("BarBar") == NULL);
      verify(message_->FindExtensionByLowercaseName("barBaz") == NULL);
      verify(message_->FindExtensionByLowercaseName("foo_foo") == NULL);
      verify(message_->FindExtensionByLowercaseName("nosuchfield") == NULL);

      verify(file_->extension(0) ==
        file_->FindExtensionByLowercaseName("baz_foo"));
      verify(file_->extension(1) ==
        file_->FindExtensionByLowercaseName("bazbar"));
      verify(file_->extension(2) ==
        file_->FindExtensionByLowercaseName("bazbaz"));
      verify(file_->FindExtensionByLowercaseName("BazBar") == NULL);
      verify(file_->FindExtensionByLowercaseName("bazBaz") == NULL);
      verify(file_->FindExtensionByLowercaseName("nosuchfield") == NULL);
    }

    // StylizedFieldNamesTest, FindByCamelcaseName
    {
      verify(message_->field(0) ==
        message_->FindFieldByCamelcaseName("fooFoo"));
      verify(message_->field(1) ==
        message_->FindFieldByCamelcaseName("fooBar"));
      verify(message_->field(2) ==
        message_->FindFieldByCamelcaseName("fooBaz"));
      verify(message_->FindFieldByCamelcaseName("foo_foo") == NULL);
      verify(message_->FindFieldByCamelcaseName("FooBar") == NULL);
      verify(message_->FindFieldByCamelcaseName("barFoo") == NULL);
      verify(message_->FindFieldByCamelcaseName("nosuchfield") == NULL);

      verify(message_->extension(0) ==
        message_->FindExtensionByCamelcaseName("barFoo"));
      verify(message_->extension(1) ==
        message_->FindExtensionByCamelcaseName("barBar"));
      verify(message_->extension(2) ==
        message_->FindExtensionByCamelcaseName("barBaz"));
      verify(message_->FindExtensionByCamelcaseName("bar_foo") == NULL);
      verify(message_->FindExtensionByCamelcaseName("BarBar") == NULL);
      verify(message_->FindExtensionByCamelcaseName("fooFoo") == NULL);
      verify(message_->FindExtensionByCamelcaseName("nosuchfield") == NULL);

      verify(file_->extension(0) ==
        file_->FindExtensionByCamelcaseName("bazFoo"));
      verify(file_->extension(1) ==
        file_->FindExtensionByCamelcaseName("bazBar"));
      verify(file_->extension(2) ==
        file_->FindExtensionByCamelcaseName("bazBaz"));
      verify(file_->FindExtensionByCamelcaseName("baz_foo") == NULL);
      verify(file_->FindExtensionByCamelcaseName("BazBar") == NULL);
      verify(file_->FindExtensionByCamelcaseName("nosuchfield") == NULL);
    }
  }
  // // // //

  // // // //
  // EnumDescriptorTest
  {
    google::protobuf::DescriptorPool pool_;

    const google::protobuf::FileDescriptor* foo_file_;
    const google::protobuf::FileDescriptor* bar_file_;

    const google::protobuf::EnumDescriptor* enum_;
    const google::protobuf::EnumDescriptor* enum2_;

    const google::protobuf::EnumValueDescriptor* foo_;
    const google::protobuf::EnumValueDescriptor* bar_;

    const google::protobuf::EnumValueDescriptor* foo2_;
    const google::protobuf::EnumValueDescriptor* baz2_;

    // SetUp()
    {
      // Build descriptors for the following definitions:
      //
      //   // in "foo.proto"
      //   enum TestEnum {
      //     FOO = 1;
      //     BAR = 2;
      //   }
      //
      //   // in "bar.proto"
      //   package corge.grault;
      //   enum TestEnum2 {
      //     FOO = 1;
      //     BAZ = 3;
      //   }
      //
      // TestEnum2 is primarily here to test FindValueByName and friends.
      // All enums created from the same DescriptorPool share the same lookup
      // table, so we need to insure that they don't interfere.

      // TestEnum
      google::protobuf::FileDescriptorProto foo_file;
      foo_file.set_name("foo.proto");

      google::protobuf::EnumDescriptorProto* enum_proto = AddEnum(&foo_file, "TestEnum");
      AddEnumValue(enum_proto, "FOO", 1);
      AddEnumValue(enum_proto, "BAR", 2);

      // TestEnum2
      google::protobuf::FileDescriptorProto bar_file;
      bar_file.set_name("bar.proto");
      bar_file.set_package("corge.grault");

      google::protobuf::EnumDescriptorProto* enum2_proto = AddEnum(&bar_file, "TestEnum2");
      AddEnumValue(enum2_proto, "FOO", 1);
      AddEnumValue(enum2_proto, "BAZ", 3);

      // Build the descriptors and get the pointers.
      foo_file_ = pool_.BuildFile(foo_file);
      verify(foo_file_ != NULL);

      bar_file_ = pool_.BuildFile(bar_file);
      verify(bar_file_ != NULL);

      verify(1 == foo_file_->enum_type_count());
enum_ = foo_file_->enum_type(0);

verify(2 == enum_->value_count());
foo_ = enum_->value(0);
bar_ = enum_->value(1);

verify(1 == bar_file_->enum_type_count());
enum2_ = bar_file_->enum_type(0);

verify(2 == enum2_->value_count());
foo2_ = enum2_->value(0);
baz2_ = enum2_->value(1);
    }

    // EnumDescriptorTest, Name
    {
    verify("TestEnum" == enum_->name());
    verify("TestEnum" == enum_->full_name());
    verify(foo_file_ == enum_->file());

    verify("TestEnum2" == enum2_->name());
    verify("corge.grault.TestEnum2" == enum2_->full_name());
    verify(bar_file_ == enum2_->file());
    }

    // EnumDescriptorTest, ContainingType
    {
      verify(enum_->containing_type() == NULL);
      verify(enum2_->containing_type() == NULL);
    }

    // EnumDescriptorTest, ValuesByIndex
    {
      verify(2 == enum_->value_count());
      verify(foo_ == enum_->value(0));
      verify(bar_ == enum_->value(1));
    }

    // EnumDescriptorTest, FindValueByName
    {
      verify(foo_ == enum_->FindValueByName("FOO"));
      verify(bar_ == enum_->FindValueByName("BAR"));
      verify(foo2_ == enum2_->FindValueByName("FOO"));
      verify(baz2_ == enum2_->FindValueByName("BAZ"));

      verify(enum_->FindValueByName("NO_SUCH_VALUE") == NULL);
      verify(enum_->FindValueByName("BAZ") == NULL);
      verify(enum2_->FindValueByName("BAR") == NULL);
    }

    // EnumDescriptorTest, FindValueByNumber
    {
      verify(foo_ == enum_->FindValueByNumber(1));
      verify(bar_ == enum_->FindValueByNumber(2));
      verify(foo2_ == enum2_->FindValueByNumber(1));
      verify(baz2_ == enum2_->FindValueByNumber(3));

      verify(enum_->FindValueByNumber(416) == NULL);
      verify(enum_->FindValueByNumber(3) == NULL);
      verify(enum2_->FindValueByNumber(2) == NULL);
    }

    // EnumDescriptorTest, ValueName
    {
      verify("FOO" == foo_->name());
      verify("BAR" == bar_->name());
    }

    // EnumDescriptorTest, ValueFullName
    {
      verify("FOO" == foo_->full_name());
      verify("BAR" == bar_->full_name());
      verify("corge.grault.FOO" == foo2_->full_name());
      verify("corge.grault.BAZ" == baz2_->full_name());
    }

    // EnumDescriptorTest, ValueIndex
    {
      verify(0 == foo_->index());
      verify(1 == bar_->index());
    }

    // EnumDescriptorTest, ValueNumber
    {
      verify(1 == foo_->number());
      verify(2 == bar_->number());
    }

    // EnumDescriptorTest, ValueType
    {
      verify(enum_ == foo_->type());
      verify(enum_ == bar_->type());
      verify(enum2_ == foo2_->type());
      verify(enum2_ == baz2_->type());
    }
  }
  // // // //

  // // // //
  // ServiceDescriptorTest
  {
    google::protobuf::DescriptorPool pool_;

    const google::protobuf::FileDescriptor* foo_file_;
    const google::protobuf::FileDescriptor* bar_file_;

    const google::protobuf::Descriptor* foo_request_;
    const google::protobuf::Descriptor* foo_response_;
    const google::protobuf::Descriptor* bar_request_;
    const google::protobuf::Descriptor* bar_response_;
    const google::protobuf::Descriptor* baz_request_;
    const google::protobuf::Descriptor* baz_response_;

    const google::protobuf::ServiceDescriptor* service_;
    const google::protobuf::ServiceDescriptor* service2_;

    const google::protobuf::MethodDescriptor* foo_;
    const google::protobuf::MethodDescriptor* bar_;

    const google::protobuf::MethodDescriptor* foo2_;
    const google::protobuf::MethodDescriptor* baz2_;

    // SetUp()
    {
      // Build descriptors for the following messages and service:
      //    // in "foo.proto"
      //    message FooRequest  {}
      //    message FooResponse {}
      //    message BarRequest  {}
      //    message BarResponse {}
      //    message BazRequest  {}
      //    message BazResponse {}
      //
      //    service TestService {
      //      rpc Foo(FooRequest) returns (FooResponse);
      //      rpc Bar(BarRequest) returns (BarResponse);
      //    }
      //
      //    // in "bar.proto"
      //    package corge.grault
      //    service TestService2 {
      //      rpc Foo(FooRequest) returns (FooResponse);
      //      rpc Baz(BazRequest) returns (BazResponse);
      //    }

      google::protobuf::FileDescriptorProto foo_file;
      foo_file.set_name("foo.proto");

      AddMessage(&foo_file, "FooRequest");
      AddMessage(&foo_file, "FooResponse");
      AddMessage(&foo_file, "BarRequest");
      AddMessage(&foo_file, "BarResponse");
      AddMessage(&foo_file, "BazRequest");
      AddMessage(&foo_file, "BazResponse");

      google::protobuf::ServiceDescriptorProto* service = AddService(&foo_file, "TestService");
      AddMethod(service, "Foo", "FooRequest", "FooResponse");
      AddMethod(service, "Bar", "BarRequest", "BarResponse");

      google::protobuf::FileDescriptorProto bar_file;
      bar_file.set_name("bar.proto");
      bar_file.set_package("corge.grault");
      bar_file.add_dependency("foo.proto");

      google::protobuf::ServiceDescriptorProto* service2 = AddService(&bar_file, "TestService2");
      AddMethod(service2, "Foo", "FooRequest", "FooResponse");
      AddMethod(service2, "Baz", "BazRequest", "BazResponse");

      // Build the descriptors and get the pointers.
      foo_file_ = pool_.BuildFile(foo_file);
      verify(foo_file_ != NULL);

      bar_file_ = pool_.BuildFile(bar_file);
      verify(bar_file_ != NULL);

      verify(6 == foo_file_->message_type_count());
      foo_request_ = foo_file_->message_type(0);
      foo_response_ = foo_file_->message_type(1);
      bar_request_ = foo_file_->message_type(2);
      bar_response_ = foo_file_->message_type(3);
      baz_request_ = foo_file_->message_type(4);
      baz_response_ = foo_file_->message_type(5);

      verify(1 == foo_file_->service_count());
      service_ = foo_file_->service(0);

      verify(2 == service_->method_count());
      foo_ = service_->method(0);
      bar_ = service_->method(1);

      verify(1 == bar_file_->service_count());
      service2_ = bar_file_->service(0);

      verify(2 == service2_->method_count());
      foo2_ = service2_->method(0);
      baz2_ = service2_->method(1);
    }

    // ServiceDescriptorTest, Name
    {
      verify("TestService" == service_->name());
      verify("TestService" == service_->full_name());
      verify(foo_file_ == service_->file());

      verify("TestService2" == service2_->name());
      verify("corge.grault.TestService2" == service2_->full_name());
      verify(bar_file_ == service2_->file());
    }

    // ServiceDescriptorTest, MethodsByIndex
    {
      verify(2 == service_->method_count());
      verify(foo_ == service_->method(0));
      verify(bar_ == service_->method(1));
    }

    // ServiceDescriptorTest, FindMethodByName
    {
      verify(foo_ == service_->FindMethodByName("Foo"));
      verify(bar_ == service_->FindMethodByName("Bar"));
      verify(foo2_ == service2_->FindMethodByName("Foo"));
      verify(baz2_ == service2_->FindMethodByName("Baz"));

      verify(service_->FindMethodByName("NoSuchMethod") == NULL);
      verify(service_->FindMethodByName("Baz") == NULL);
      verify(service2_->FindMethodByName("Bar") == NULL);
    }

    // ServiceDescriptorTest, MethodName
    {
      verify("Foo" == foo_->name());
      verify("Bar" == bar_->name());
    }

    // ServiceDescriptorTest, MethodFullName
    {
      verify("TestService.Foo" == foo_->full_name());
      verify("TestService.Bar" == bar_->full_name());
      verify("corge.grault.TestService2.Foo" == foo2_->full_name());
      verify("corge.grault.TestService2.Baz" == baz2_->full_name());
    }

    // ServiceDescriptorTest, MethodIndex
    {
      verify(0 == foo_->index());
      verify(1 == bar_->index());
    }

    // ServiceDescriptorTest, MethodParent
    {
      verify(service_ == foo_->service());
      verify(service_ == bar_->service());
    }

    // ServiceDescriptorTest, MethodInputType
    {
      verify(foo_request_ == foo_->input_type());
      verify(bar_request_ == bar_->input_type());
    }

    // ServiceDescriptorTest, MethodOutputType
    {
      verify(foo_response_ == foo_->output_type());
      verify(bar_response_ == bar_->output_type());
    }
  }
  // // // //

  // // // //
  // NestedDescriptorTest
  {
    google::protobuf::DescriptorPool pool_;

    const google::protobuf::FileDescriptor* foo_file_;
    const google::protobuf::FileDescriptor* bar_file_;

    const google::protobuf::Descriptor* message_;
    const google::protobuf::Descriptor* message2_;

    const google::protobuf::Descriptor* foo_;
    const google::protobuf::Descriptor* bar_;
    const google::protobuf::EnumDescriptor* baz_;
    const google::protobuf::EnumDescriptor* qux_;
    const google::protobuf::EnumValueDescriptor* a_;
    const google::protobuf::EnumValueDescriptor* b_;

    const google::protobuf::Descriptor* foo2_;
    const google::protobuf::Descriptor* baz2_;
    const google::protobuf::EnumDescriptor* qux2_;
    const google::protobuf::EnumDescriptor* quux2_;
    const google::protobuf::EnumValueDescriptor* a2_;
    const google::protobuf::EnumValueDescriptor* c2_;

    // SetUp()
    {
      // Build descriptors for the following definitions:
      //
      //   // in "foo.proto"
      //   message TestMessage {
      //     message Foo {}
      //     message Bar {}
      //     enum Baz { A = 1; }
      //     enum Qux { B = 1; }
      //   }
      //
      //   // in "bar.proto"
      //   package corge.grault;
      //   message TestMessage2 {
      //     message Foo {}
      //     message Baz {}
      //     enum Qux  { A = 1; }
      //     enum Quux { C = 1; }
      //   }
      //
      // TestMessage2 is primarily here to test FindNestedTypeByName and friends.
      // All messages created from the same DescriptorPool share the same lookup
      // table, so we need to insure that they don't interfere.
      //
      // We add enum values to the enums in order to test searching for enum
      // values across a message's scope.

      google::protobuf::FileDescriptorProto foo_file;
      foo_file.set_name("foo.proto");

      google::protobuf::DescriptorProto* message = AddMessage(&foo_file, "TestMessage");
      AddNestedMessage(message, "Foo");
      AddNestedMessage(message, "Bar");
      google::protobuf::EnumDescriptorProto* baz = AddNestedEnum(message, "Baz");
      AddEnumValue(baz, "A", 1);
      google::protobuf::EnumDescriptorProto* qux = AddNestedEnum(message, "Qux");
      AddEnumValue(qux, "B", 1);

      google::protobuf::FileDescriptorProto bar_file;
      bar_file.set_name("bar.proto");
      bar_file.set_package("corge.grault");

      google::protobuf::DescriptorProto* message2 = AddMessage(&bar_file, "TestMessage2");
      AddNestedMessage(message2, "Foo");
      AddNestedMessage(message2, "Baz");
      google::protobuf::EnumDescriptorProto* qux2 = AddNestedEnum(message2, "Qux");
      AddEnumValue(qux2, "A", 1);
      google::protobuf::EnumDescriptorProto* quux2 = AddNestedEnum(message2, "Quux");
      AddEnumValue(quux2, "C", 1);

      // Build the descriptors and get the pointers.
      foo_file_ = pool_.BuildFile(foo_file);
      verify(foo_file_ != NULL);

      bar_file_ = pool_.BuildFile(bar_file);
      verify(bar_file_ != NULL);

      verify(1 == foo_file_->message_type_count());
      message_ = foo_file_->message_type(0);

      verify(2 == message_->nested_type_count());
      foo_ = message_->nested_type(0);
      bar_ = message_->nested_type(1);

      verify(2 == message_->enum_type_count());
      baz_ = message_->enum_type(0);
      qux_ = message_->enum_type(1);

      verify(1 == baz_->value_count());
      a_ = baz_->value(0);
      verify(1 == qux_->value_count());
      b_ = qux_->value(0);

      verify(1 == bar_file_->message_type_count());
      message2_ = bar_file_->message_type(0);

      verify(2 == message2_->nested_type_count());
      foo2_ = message2_->nested_type(0);
      baz2_ = message2_->nested_type(1);

      verify(2 == message2_->enum_type_count());
      qux2_ = message2_->enum_type(0);
      quux2_ = message2_->enum_type(1);

      verify(1 == qux2_->value_count());
      a2_ = qux2_->value(0);
      verify(1 == quux2_->value_count());
      c2_ = quux2_->value(0);
    }

    // NestedDescriptorTest, MessageName
    {
      verify("Foo" == foo_->name());
      verify("Bar" == bar_->name());
      verify("Foo" == foo2_->name());
      verify("Baz" == baz2_->name());

      verify("TestMessage.Foo" == foo_->full_name());
      verify("TestMessage.Bar" == bar_->full_name());
      verify("corge.grault.TestMessage2.Foo" == foo2_->full_name());
      verify("corge.grault.TestMessage2.Baz" == baz2_->full_name());
    }

    // NestedDescriptorTest, MessageContainingType
    {
      verify(message_ == foo_->containing_type());
      verify(message_ == bar_->containing_type());
      verify(message2_ == foo2_->containing_type());
      verify(message2_ == baz2_->containing_type());
    }

    // NestedDescriptorTest, NestedMessagesByIndex
    {
      verify(2 == message_->nested_type_count());
      verify(foo_ == message_->nested_type(0));
      verify(bar_ == message_->nested_type(1));
    }

    // NestedDescriptorTest, FindFieldByNameDoesntFindNestedTypes
    {
      verify(message_->FindFieldByName("Foo") == NULL);
      verify(message_->FindFieldByName("Qux") == NULL);
      verify(message_->FindExtensionByName("Foo") == NULL);
      verify(message_->FindExtensionByName("Qux") == NULL);
    }

    // NestedDescriptorTest, FindNestedTypeByName
    {
      verify(foo_ == message_->FindNestedTypeByName("Foo"));
      verify(bar_ == message_->FindNestedTypeByName("Bar"));
      verify(foo2_ == message2_->FindNestedTypeByName("Foo"));
      verify(baz2_ == message2_->FindNestedTypeByName("Baz"));

      verify(message_->FindNestedTypeByName("NoSuchType") == NULL);
      verify(message_->FindNestedTypeByName("Baz") == NULL);
      verify(message2_->FindNestedTypeByName("Bar") == NULL);

      verify(message_->FindNestedTypeByName("Qux") == NULL);
    }

    // NestedDescriptorTest, EnumName
    {
      verify("Baz" == baz_->name());
      verify("Qux" == qux_->name());
      verify("Qux" == qux2_->name());
      verify("Quux" == quux2_->name());

      verify("TestMessage.Baz" == baz_->full_name());
      verify("TestMessage.Qux" == qux_->full_name());
      verify("corge.grault.TestMessage2.Qux" == qux2_->full_name());
      verify("corge.grault.TestMessage2.Quux" == quux2_->full_name());
    }

    // NestedDescriptorTest, EnumContainingType
    {
      verify(message_ == baz_->containing_type());
      verify(message_ == qux_->containing_type());
      verify(message2_ == qux2_->containing_type());
      verify(message2_ == quux2_->containing_type());
    }

    // NestedDescriptorTest, NestedEnumsByIndex
    {
      verify(2 == message_->nested_type_count());
      verify(foo_ == message_->nested_type(0));
      verify(bar_ == message_->nested_type(1));
    }

    // NestedDescriptorTest, FindEnumTypeByName
    {
      verify(baz_ == message_->FindEnumTypeByName("Baz"));
      verify(qux_ == message_->FindEnumTypeByName("Qux"));
      verify(qux2_ == message2_->FindEnumTypeByName("Qux"));
      verify(quux2_ == message2_->FindEnumTypeByName("Quux"));

      verify(message_->FindEnumTypeByName("NoSuchType") == NULL);
      verify(message_->FindEnumTypeByName("Quux") == NULL);
      verify(message2_->FindEnumTypeByName("Baz") == NULL);

      verify(message_->FindEnumTypeByName("Foo") == NULL);
    }

    // NestedDescriptorTest, FindEnumValueByName
    {
      verify(a_ == message_->FindEnumValueByName("A"));
      verify(b_ == message_->FindEnumValueByName("B"));
      verify(a2_ == message2_->FindEnumValueByName("A"));
      verify(c2_ == message2_->FindEnumValueByName("C"));

      verify(message_->FindEnumValueByName("NO_SUCH_VALUE") == NULL);
      verify(message_->FindEnumValueByName("C") == NULL);
      verify(message2_->FindEnumValueByName("B") == NULL);

      verify(message_->FindEnumValueByName("Foo") == NULL);
    }
  }
  // // // //

  // // // //
  // ExtensionDescriptorTest
  {
    google::protobuf::DescriptorPool pool_;

    const google::protobuf::FileDescriptor* foo_file_;

    const google::protobuf::Descriptor* foo_;
    const google::protobuf::Descriptor* bar_;
    const google::protobuf::EnumDescriptor* baz_;
    const google::protobuf::Descriptor* qux_;

    // SetUp()
    {
      // Build descriptors for the following definitions:
      //
      //   enum Baz {}
      //   message Qux {}
      //
      //   message Foo {
      //     extensions 10 to 19;
      //     extensions 30 to 39;
      //   }
      //   extends Foo with optional int32 foo_int32 = 10;
      //   extends Foo with repeated TestEnum foo_enum = 19;
      //   message Bar {
      //     extends Foo with optional Qux foo_message = 30;
      //     // (using Qux as the group type)
      //     extends Foo with repeated group foo_group = 39;
      //   }

      google::protobuf::FileDescriptorProto foo_file;
      foo_file.set_name("foo.proto");

      AddEmptyEnum(&foo_file, "Baz");
      AddMessage(&foo_file, "Qux");

      google::protobuf::DescriptorProto* foo = AddMessage(&foo_file, "Foo");
      AddExtensionRange(foo, 10, 20);
      AddExtensionRange(foo, 30, 40);

      AddExtension(&foo_file, "Foo", "foo_int32", 10,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);
      AddExtension(&foo_file, "Foo", "foo_enum", 19,
        google::protobuf::FieldDescriptorProto::LABEL_REPEATED,
        google::protobuf::FieldDescriptorProto::TYPE_ENUM)
        ->set_type_name("Baz");

      google::protobuf::DescriptorProto* bar = AddMessage(&foo_file, "Bar");
      AddNestedExtension(bar, "Foo", "foo_message", 30,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_MESSAGE)
        ->set_type_name("Qux");
      AddNestedExtension(bar, "Foo", "foo_group", 39,
        google::protobuf::FieldDescriptorProto::LABEL_REPEATED,
        google::protobuf::FieldDescriptorProto::TYPE_GROUP)
        ->set_type_name("Qux");

      // Build the descriptors and get the pointers.
      foo_file_ = pool_.BuildFile(foo_file);
      verify(foo_file_ != NULL);

      verify(1 == foo_file_->enum_type_count());
      baz_ = foo_file_->enum_type(0);

      verify(3 == foo_file_->message_type_count());
      qux_ = foo_file_->message_type(0);
      foo_ = foo_file_->message_type(1);
      bar_ = foo_file_->message_type(2);
    }

    // ExtensionDescriptorTest, ExtensionRanges
    {
      verify(0 == bar_->extension_range_count());
      verify(2 == foo_->extension_range_count());

      verify(10 == foo_->extension_range(0)->start);
      verify(30 == foo_->extension_range(1)->start);

      verify(20 == foo_->extension_range(0)->end);
      verify(40 == foo_->extension_range(1)->end);
    }

    // ExtensionDescriptorTest, Extensions
    {
      verify(0 == foo_->extension_count());
      verify(2 == foo_file_->extension_count());
      verify(2 == bar_->extension_count());

      verify(foo_file_->extension(0)->is_extension());
      verify(foo_file_->extension(1)->is_extension());
      verify(bar_->extension(0)->is_extension());
      verify(bar_->extension(1)->is_extension());

      verify("foo_int32" == foo_file_->extension(0)->name());
      verify("foo_enum" == foo_file_->extension(1)->name());
      verify("foo_message" == bar_->extension(0)->name());
      verify("foo_group" == bar_->extension(1)->name());

      verify(10 == foo_file_->extension(0)->number());
      verify(19 == foo_file_->extension(1)->number());
      verify(30 == bar_->extension(0)->number());
      verify(39 == bar_->extension(1)->number());

      verify(google::protobuf::FieldDescriptor::TYPE_INT32 == foo_file_->extension(0)->type());
      verify(google::protobuf::FieldDescriptor::TYPE_ENUM == foo_file_->extension(1)->type());
      verify(google::protobuf::FieldDescriptor::TYPE_MESSAGE == bar_->extension(0)->type());
      verify(google::protobuf::FieldDescriptor::TYPE_GROUP == bar_->extension(1)->type());

      verify(baz_ == foo_file_->extension(1)->enum_type());
      verify(qux_ == bar_->extension(0)->message_type());
      verify(qux_ == bar_->extension(1)->message_type());

      verify(google::protobuf::FieldDescriptor::LABEL_OPTIONAL == foo_file_->extension(0)->label());
      verify(google::protobuf::FieldDescriptor::LABEL_REPEATED == foo_file_->extension(1)->label());
      verify(google::protobuf::FieldDescriptor::LABEL_OPTIONAL == bar_->extension(0)->label());
      verify(google::protobuf::FieldDescriptor::LABEL_REPEATED == bar_->extension(1)->label());

      verify(foo_ == foo_file_->extension(0)->containing_type());
      verify(foo_ == foo_file_->extension(1)->containing_type());
      verify(foo_ == bar_->extension(0)->containing_type());
      verify(foo_ == bar_->extension(1)->containing_type());

      verify(foo_file_->extension(0)->extension_scope() == NULL);
      verify(foo_file_->extension(1)->extension_scope() == NULL);
      verify(bar_ == bar_->extension(0)->extension_scope());
      verify(bar_ == bar_->extension(1)->extension_scope());
    };

    // ExtensionDescriptorTest, IsExtensionNumber
    {
      verify(false == foo_->IsExtensionNumber(9));
      verify(foo_->IsExtensionNumber(10));
      verify(foo_->IsExtensionNumber(19));
      verify(false == foo_->IsExtensionNumber(20));
      verify(false == foo_->IsExtensionNumber(29));
      verify(foo_->IsExtensionNumber(30));
      verify(foo_->IsExtensionNumber(39));
      verify(false == foo_->IsExtensionNumber(40));
    }

    // ExtensionDescriptorTest, FindExtensionByName
    {
      // Note that FileDescriptor::FindExtensionByName() is tested by
      // FileDescriptorTest.
      verify(2 == bar_->extension_count());

      verify(bar_->extension(0) == bar_->FindExtensionByName("foo_message"));
      verify(bar_->extension(1) == bar_->FindExtensionByName("foo_group"));

      verify(bar_->FindExtensionByName("no_such_extension") == NULL);
      verify(foo_->FindExtensionByName("foo_int32") == NULL);
      verify(foo_->FindExtensionByName("foo_message") == NULL);
    }

    // ExtensionDescriptorTest, FindAllExtensions
    {
      fun::vector<const google::protobuf::FieldDescriptor*> extensions;
      pool_.FindAllExtensions(foo_, &extensions);
      verify(4 == extensions.size());
      verify(10 == extensions[0]->number());
      verify(19 == extensions[1]->number());
      verify(30 == extensions[2]->number());
      verify(39 == extensions[3]->number());
    }

    // ExtensionDescriptorTest, DuplicateFieldNumber
    {
      google::protobuf::DescriptorPool pool;
      google::protobuf::FileDescriptorProto file_proto;
      // Add "google/protobuf/descriptor.proto".
      google::protobuf::FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
      verify(pool.BuildFile(file_proto) != NULL);
      // Add "foo.proto":
      //   import "google/protobuf/descriptor.proto";
      //   extend google.protobuf.FieldOptions {
      //     optional int32 option1 = 1000;
      //   }
      file_proto.Clear();
      file_proto.set_name("foo.proto");
      file_proto.add_dependency("google/protobuf/descriptor.proto");
      AddExtension(&file_proto, "google.protobuf.FieldOptions", "option1", 1000,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);
      verify(pool.BuildFile(file_proto) != NULL);
      // Add "bar.proto":
      //   import "google/protobuf/descriptor.proto";
      //   extend google.protobuf.FieldOptions {
      //     optional int32 option2 = 1000;
      //   }
      file_proto.Clear();
      file_proto.set_name("bar.proto");
      file_proto.add_dependency("google/protobuf/descriptor.proto");
      AddExtension(&file_proto, "google.protobuf.FieldOptions", "option2", 1000,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);
      // Currently we only generate a warning for conflicting extension numbers.
      // TODO(xiaofeng): Change it to an error.
      verify(pool.BuildFile(file_proto) != NULL);
    }
  }
  // // // //

  // // // //
  // MiscTest
  {
    google::protobuf::scoped_ptr<google::protobuf::DescriptorPool> pool_;

    // Function which makes a field descriptor of the given type.
    auto GetFieldDescriptorOfType = [&AddEmptyEnum, &AddField, &AddMessage, &pool_]
    (google::protobuf::FieldDescriptor::Type type) -> const google::protobuf::FieldDescriptor*
    {
      google::protobuf::FileDescriptorProto file_proto;
      file_proto.set_name("foo.proto");
      AddEmptyEnum(&file_proto, "DummyEnum");

      google::protobuf::DescriptorProto* message = AddMessage(&file_proto, "TestMessage");
      google::protobuf::FieldDescriptorProto* field =
        AddField(message, "foo", 1, google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
          static_cast<google::protobuf::FieldDescriptorProto::Type>(static_cast<int>(type)));

      if (type == google::protobuf::FieldDescriptor::TYPE_MESSAGE ||
        type == google::protobuf::FieldDescriptor::TYPE_GROUP) {
        field->set_type_name("TestMessage");
      }
      else if (type == google::protobuf::FieldDescriptor::TYPE_ENUM) {
        field->set_type_name("DummyEnum");
      }

      // Build the descriptors and get the pointers.
      pool_.reset(new google::protobuf::DescriptorPool());
      const google::protobuf::FileDescriptor* file = pool_->BuildFile(file_proto);

      if (file != NULL &&
        file->message_type_count() == 1 &&
        file->message_type(0)->field_count() == 1) {
        return file->message_type(0)->field(0);
      }
      else {
        return NULL;
      }
    };

    auto GetTypeNameForFieldType = [&GetFieldDescriptorOfType]
    (google::protobuf::FieldDescriptor::Type type) -> const char*
    {
      const google::protobuf::FieldDescriptor* field = GetFieldDescriptorOfType(type);
      return field != NULL ? field->type_name() : "";
    };

    auto GetCppTypeForFieldType = [&GetFieldDescriptorOfType]
    (google::protobuf::FieldDescriptor::Type type) -> google::protobuf::FieldDescriptor::CppType
    {
      const google::protobuf::FieldDescriptor* field = GetFieldDescriptorOfType(type);
      return field != NULL ? field->cpp_type() :
        static_cast<google::protobuf::FieldDescriptor::CppType>(0);
    };

    auto GetCppTypeNameForFieldType = [&GetFieldDescriptorOfType]
    (google::protobuf::FieldDescriptor::Type type) -> const char* {
      const google::protobuf::FieldDescriptor* field = GetFieldDescriptorOfType(type);
      return field != NULL ? field->cpp_type_name() : "";
    };

    auto GetMessageDescriptorForFieldType = [&GetFieldDescriptorOfType]
    (google::protobuf::FieldDescriptor::Type type) -> const google::protobuf::Descriptor* {
      const google::protobuf::FieldDescriptor* field = GetFieldDescriptorOfType(type);
      return field != NULL ? field->message_type() : NULL;
    };

    auto GetEnumDescriptorForFieldType = [&GetFieldDescriptorOfType]
    (google::protobuf::FieldDescriptor::Type type) -> const google::protobuf::EnumDescriptor* {
      const google::protobuf::FieldDescriptor* field = GetFieldDescriptorOfType(type);
      return field != NULL ? field->enum_type() : NULL;
    };

    // MiscTest, TypeNames
    {
      // Test that correct type names are returned.

      typedef google::protobuf::FieldDescriptor FD;  // avoid ugly line wrapping

      verify("double" == fun::string(GetTypeNameForFieldType(FD::TYPE_DOUBLE)));
      verify("float" == fun::string(GetTypeNameForFieldType(FD::TYPE_FLOAT)));
      verify("int64" == fun::string(GetTypeNameForFieldType(FD::TYPE_INT64)));
      verify("uint64" == fun::string(GetTypeNameForFieldType(FD::TYPE_UINT64)));
      verify("int32" == fun::string(GetTypeNameForFieldType(FD::TYPE_INT32)));
      verify("fixed64" == fun::string(GetTypeNameForFieldType(FD::TYPE_FIXED64)));
      verify("fixed32" == fun::string(GetTypeNameForFieldType(FD::TYPE_FIXED32)));
      verify("bool" == fun::string(GetTypeNameForFieldType(FD::TYPE_BOOL)));
      verify("fun::string" == fun::string(GetTypeNameForFieldType(FD::TYPE_STRING)));
      verify("group" == fun::string(GetTypeNameForFieldType(FD::TYPE_GROUP)));
      verify("message" == fun::string(GetTypeNameForFieldType(FD::TYPE_MESSAGE)));
      verify("bytes" == fun::string(GetTypeNameForFieldType(FD::TYPE_BYTES)));
      verify("uint32" == fun::string(GetTypeNameForFieldType(FD::TYPE_UINT32)));
      verify("enum" == fun::string(GetTypeNameForFieldType(FD::TYPE_ENUM)));
      verify("sfixed32" == fun::string(GetTypeNameForFieldType(FD::TYPE_SFIXED32)));
      verify("sfixed64" == fun::string(GetTypeNameForFieldType(FD::TYPE_SFIXED64)));
      verify("sint32" == fun::string(GetTypeNameForFieldType(FD::TYPE_SINT32)));
      verify("sint64" == fun::string(GetTypeNameForFieldType(FD::TYPE_SINT64)));
    }

    // MiscTest, StaticTypeNames
    {
      // Test that correct type names are returned.

      typedef google::protobuf::FieldDescriptor FD;  // avoid ugly line wrapping

      verify("double" == fun::string(FD::TypeName(FD::TYPE_DOUBLE)));
      verify("float" == fun::string(FD::TypeName(FD::TYPE_FLOAT)));
      verify("int64" == fun::string(FD::TypeName(FD::TYPE_INT64)));
      verify("uint64" == fun::string(FD::TypeName(FD::TYPE_UINT64)));
      verify("int32" == fun::string(FD::TypeName(FD::TYPE_INT32)));
      verify("fixed64" == fun::string(FD::TypeName(FD::TYPE_FIXED64)));
      verify("fixed32" == fun::string(FD::TypeName(FD::TYPE_FIXED32)));
      verify("bool" == fun::string(FD::TypeName(FD::TYPE_BOOL)));
      verify("fun::string" == fun::string(FD::TypeName(FD::TYPE_STRING)));
      verify("group" == fun::string(FD::TypeName(FD::TYPE_GROUP)));
      verify("message" == fun::string(FD::TypeName(FD::TYPE_MESSAGE)));
      verify("bytes" == fun::string(FD::TypeName(FD::TYPE_BYTES)));
      verify("uint32" == fun::string(FD::TypeName(FD::TYPE_UINT32)));
      verify("enum" == fun::string(FD::TypeName(FD::TYPE_ENUM)));
      verify("sfixed32" == fun::string(FD::TypeName(FD::TYPE_SFIXED32)));
      verify("sfixed64" == fun::string(FD::TypeName(FD::TYPE_SFIXED64)));
      verify("sint32" == fun::string(FD::TypeName(FD::TYPE_SINT32)));
      verify("sint64" == fun::string(FD::TypeName(FD::TYPE_SINT64)));
    }

    // MiscTest, CppTypes
    {
      // Test that CPP types are assigned correctly.

      typedef google::protobuf::FieldDescriptor FD;  // avoid ugly line wrapping

      verify(FD::CPPTYPE_DOUBLE == GetCppTypeForFieldType(FD::TYPE_DOUBLE));
      verify(FD::CPPTYPE_FLOAT == GetCppTypeForFieldType(FD::TYPE_FLOAT));
      verify(FD::CPPTYPE_INT64 == GetCppTypeForFieldType(FD::TYPE_INT64));
      verify(FD::CPPTYPE_UINT64 == GetCppTypeForFieldType(FD::TYPE_UINT64));
      verify(FD::CPPTYPE_INT32 == GetCppTypeForFieldType(FD::TYPE_INT32));
      verify(FD::CPPTYPE_UINT64 == GetCppTypeForFieldType(FD::TYPE_FIXED64));
      verify(FD::CPPTYPE_UINT32 == GetCppTypeForFieldType(FD::TYPE_FIXED32));
      verify(FD::CPPTYPE_BOOL == GetCppTypeForFieldType(FD::TYPE_BOOL));
      verify(FD::CPPTYPE_STRING == GetCppTypeForFieldType(FD::TYPE_STRING));
      verify(FD::CPPTYPE_MESSAGE == GetCppTypeForFieldType(FD::TYPE_GROUP));
      verify(FD::CPPTYPE_MESSAGE == GetCppTypeForFieldType(FD::TYPE_MESSAGE));
      verify(FD::CPPTYPE_STRING == GetCppTypeForFieldType(FD::TYPE_BYTES));
      verify(FD::CPPTYPE_UINT32 == GetCppTypeForFieldType(FD::TYPE_UINT32));
      verify(FD::CPPTYPE_ENUM == GetCppTypeForFieldType(FD::TYPE_ENUM));
      verify(FD::CPPTYPE_INT32 == GetCppTypeForFieldType(FD::TYPE_SFIXED32));
      verify(FD::CPPTYPE_INT64 == GetCppTypeForFieldType(FD::TYPE_SFIXED64));
      verify(FD::CPPTYPE_INT32 == GetCppTypeForFieldType(FD::TYPE_SINT32));
      verify(FD::CPPTYPE_INT64 == GetCppTypeForFieldType(FD::TYPE_SINT64));
    }

    // MiscTest, CppTypeNames
    {
      // Test that correct CPP type names are returned.

      typedef google::protobuf::FieldDescriptor FD;  // avoid ugly line wrapping

      verify("double" == fun::string(GetCppTypeNameForFieldType(FD::TYPE_DOUBLE)));
      verify("float" == fun::string(GetCppTypeNameForFieldType(FD::TYPE_FLOAT)));
      verify("int64" == fun::string(GetCppTypeNameForFieldType(FD::TYPE_INT64)));
      verify("uint64" == fun::string(GetCppTypeNameForFieldType(FD::TYPE_UINT64)));
      verify("int32" == fun::string(GetCppTypeNameForFieldType(FD::TYPE_INT32)));
      verify("uint64" == fun::string(GetCppTypeNameForFieldType(FD::TYPE_FIXED64)));
      verify("uint32" == fun::string(GetCppTypeNameForFieldType(FD::TYPE_FIXED32)));
      verify("bool" == fun::string(GetCppTypeNameForFieldType(FD::TYPE_BOOL)));
      verify("fun::string" == fun::string(GetCppTypeNameForFieldType(FD::TYPE_STRING)));
      verify("message" == fun::string(GetCppTypeNameForFieldType(FD::TYPE_GROUP)));
      verify("message" == fun::string(GetCppTypeNameForFieldType(FD::TYPE_MESSAGE)));
      verify("fun::string" == fun::string(GetCppTypeNameForFieldType(FD::TYPE_BYTES)));
      verify("uint32" == fun::string(GetCppTypeNameForFieldType(FD::TYPE_UINT32)));
      verify("enum" == fun::string(GetCppTypeNameForFieldType(FD::TYPE_ENUM)));
      verify("int32" == fun::string(GetCppTypeNameForFieldType(FD::TYPE_SFIXED32)));
      verify("int64" == fun::string(GetCppTypeNameForFieldType(FD::TYPE_SFIXED64)));
      verify("int32" == fun::string(GetCppTypeNameForFieldType(FD::TYPE_SINT32)));
      verify("int64" == fun::string(GetCppTypeNameForFieldType(FD::TYPE_SINT64)));
    }

    // MiscTest, StaticCppTypeNames
    {
      // Test that correct CPP type names are returned.

      typedef google::protobuf::FieldDescriptor FD;  // avoid ugly line wrapping

      verify("int32" == fun::string(FD::CppTypeName(FD::CPPTYPE_INT32)));
      verify("int64" == fun::string(FD::CppTypeName(FD::CPPTYPE_INT64)));
      verify("uint32" == fun::string(FD::CppTypeName(FD::CPPTYPE_UINT32)));
      verify("uint64" == fun::string(FD::CppTypeName(FD::CPPTYPE_UINT64)));
      verify("double" == fun::string(FD::CppTypeName(FD::CPPTYPE_DOUBLE)));
      verify("float" == fun::string(FD::CppTypeName(FD::CPPTYPE_FLOAT)));
      verify("bool" == fun::string(FD::CppTypeName(FD::CPPTYPE_BOOL)));
      verify("enum" == fun::string(FD::CppTypeName(FD::CPPTYPE_ENUM)));
      verify("fun::string" == fun::string(FD::CppTypeName(FD::CPPTYPE_STRING)));
      verify("message" == fun::string(FD::CppTypeName(FD::CPPTYPE_MESSAGE)));
    }

    // MiscTest, MessageType
    {
      // Test that message_type() is NULL for non-aggregate fields

      typedef google::protobuf::FieldDescriptor FD;  // avoid ugly line wrapping

      verify(NULL == GetMessageDescriptorForFieldType(FD::TYPE_DOUBLE));
      verify(NULL == GetMessageDescriptorForFieldType(FD::TYPE_FLOAT));
      verify(NULL == GetMessageDescriptorForFieldType(FD::TYPE_INT64));
      verify(NULL == GetMessageDescriptorForFieldType(FD::TYPE_UINT64));
      verify(NULL == GetMessageDescriptorForFieldType(FD::TYPE_INT32));
      verify(NULL == GetMessageDescriptorForFieldType(FD::TYPE_FIXED64));
      verify(NULL == GetMessageDescriptorForFieldType(FD::TYPE_FIXED32));
      verify(NULL == GetMessageDescriptorForFieldType(FD::TYPE_BOOL));
      verify(NULL == GetMessageDescriptorForFieldType(FD::TYPE_STRING));
      verify(NULL != GetMessageDescriptorForFieldType(FD::TYPE_GROUP));
      verify(NULL != GetMessageDescriptorForFieldType(FD::TYPE_MESSAGE));
      verify(NULL == GetMessageDescriptorForFieldType(FD::TYPE_BYTES));
      verify(NULL == GetMessageDescriptorForFieldType(FD::TYPE_UINT32));
      verify(NULL == GetMessageDescriptorForFieldType(FD::TYPE_ENUM));
      verify(NULL == GetMessageDescriptorForFieldType(FD::TYPE_SFIXED32));
      verify(NULL == GetMessageDescriptorForFieldType(FD::TYPE_SFIXED64));
      verify(NULL == GetMessageDescriptorForFieldType(FD::TYPE_SINT32));
      verify(NULL == GetMessageDescriptorForFieldType(FD::TYPE_SINT64));
    }

    // MiscTest, EnumType
    {
      // Test that enum_type() is NULL for non-enum fields

      typedef google::protobuf::FieldDescriptor FD;  // avoid ugly line wrapping

      verify(NULL == GetEnumDescriptorForFieldType(FD::TYPE_DOUBLE));
      verify(NULL == GetEnumDescriptorForFieldType(FD::TYPE_FLOAT));
      verify(NULL == GetEnumDescriptorForFieldType(FD::TYPE_INT64));
      verify(NULL == GetEnumDescriptorForFieldType(FD::TYPE_UINT64));
      verify(NULL == GetEnumDescriptorForFieldType(FD::TYPE_INT32));
      verify(NULL == GetEnumDescriptorForFieldType(FD::TYPE_FIXED64));
      verify(NULL == GetEnumDescriptorForFieldType(FD::TYPE_FIXED32));
      verify(NULL == GetEnumDescriptorForFieldType(FD::TYPE_BOOL));
      verify(NULL == GetEnumDescriptorForFieldType(FD::TYPE_STRING));
      verify(NULL == GetEnumDescriptorForFieldType(FD::TYPE_GROUP));
      verify(NULL == GetEnumDescriptorForFieldType(FD::TYPE_MESSAGE));
      verify(NULL == GetEnumDescriptorForFieldType(FD::TYPE_BYTES));
      verify(NULL == GetEnumDescriptorForFieldType(FD::TYPE_UINT32));
      verify(NULL != GetEnumDescriptorForFieldType(FD::TYPE_ENUM));
      verify(NULL == GetEnumDescriptorForFieldType(FD::TYPE_SFIXED32));
      verify(NULL == GetEnumDescriptorForFieldType(FD::TYPE_SFIXED64));
      verify(NULL == GetEnumDescriptorForFieldType(FD::TYPE_SINT32));
      verify(NULL == GetEnumDescriptorForFieldType(FD::TYPE_SINT64));
    }

    // MiscTest, DefaultValues
    {
      // Test that setting default values works.
      google::protobuf::FileDescriptorProto file_proto;
      file_proto.set_name("foo.proto");

      google::protobuf::EnumDescriptorProto* enum_type_proto = AddEnum(&file_proto, "DummyEnum");
      AddEnumValue(enum_type_proto, "A", 1);
      AddEnumValue(enum_type_proto, "B", 2);

      google::protobuf::DescriptorProto* message_proto = AddMessage(&file_proto, "TestMessage");

      typedef google::protobuf::FieldDescriptorProto FD;  // avoid ugly line wrapping
      const FD::Label label = FD::LABEL_OPTIONAL;

      // Create fields of every CPP type with default values.
      AddField(message_proto, "int32", 1, label, FD::TYPE_INT32)
        ->set_default_value("-1");
      AddField(message_proto, "int64", 2, label, FD::TYPE_INT64)
        ->set_default_value("-1000000000000");
      AddField(message_proto, "uint32", 3, label, FD::TYPE_UINT32)
        ->set_default_value("42");
      AddField(message_proto, "uint64", 4, label, FD::TYPE_UINT64)
        ->set_default_value("2000000000000");
      AddField(message_proto, "float", 5, label, FD::TYPE_FLOAT)
        ->set_default_value("4.5");
      AddField(message_proto, "double", 6, label, FD::TYPE_DOUBLE)
        ->set_default_value("10e100");
      AddField(message_proto, "bool", 7, label, FD::TYPE_BOOL)
        ->set_default_value("true");
      AddField(message_proto, "fun::string", 8, label, FD::TYPE_STRING)
        ->set_default_value("hello");
      AddField(message_proto, "data", 9, label, FD::TYPE_BYTES)
        ->set_default_value("\\001\\002\\003");

      google::protobuf::FieldDescriptorProto* enum_field =
        AddField(message_proto, "enum", 10, label, FD::TYPE_ENUM);
      enum_field->set_type_name("DummyEnum");
      enum_field->set_default_value("B");

      // Strings are allowed to have empty defaults.  (At one point, due to
      // a bug, empty defaults for strings were rejected.  Oops.)
      AddField(message_proto, "empty_string", 11, label, FD::TYPE_STRING)
        ->set_default_value("");

      // Add a second fun::set of fields with implicit defalut values.
      AddField(message_proto, "implicit_int32", 21, label, FD::TYPE_INT32);
      AddField(message_proto, "implicit_int64", 22, label, FD::TYPE_INT64);
      AddField(message_proto, "implicit_uint32", 23, label, FD::TYPE_UINT32);
      AddField(message_proto, "implicit_uint64", 24, label, FD::TYPE_UINT64);
      AddField(message_proto, "implicit_float", 25, label, FD::TYPE_FLOAT);
      AddField(message_proto, "implicit_double", 26, label, FD::TYPE_DOUBLE);
      AddField(message_proto, "implicit_bool", 27, label, FD::TYPE_BOOL);
      AddField(message_proto, "implicit_string", 28, label, FD::TYPE_STRING);
      AddField(message_proto, "implicit_data", 29, label, FD::TYPE_BYTES);
      AddField(message_proto, "implicit_enum", 30, label, FD::TYPE_ENUM)
        ->set_type_name("DummyEnum");

      // Build it.
      google::protobuf::DescriptorPool pool;
      const google::protobuf::FileDescriptor* file = pool.BuildFile(file_proto);
      verify(file != NULL);

      verify(1 == file->enum_type_count());
      const google::protobuf::EnumDescriptor* enum_type = file->enum_type(0);
      verify(2 == enum_type->value_count());
      const google::protobuf::EnumValueDescriptor* enum_value_a = enum_type->value(0);
      const google::protobuf::EnumValueDescriptor* enum_value_b = enum_type->value(1);

      verify(1 == file->message_type_count());
      const google::protobuf::Descriptor* message = file->message_type(0);

      verify(21 == message->field_count());

      // Check the default values.
      verify(message->field(0)->has_default_value());
      verify(message->field(1)->has_default_value());
      verify(message->field(2)->has_default_value());
      verify(message->field(3)->has_default_value());
      verify(message->field(4)->has_default_value());
      verify(message->field(5)->has_default_value());
      verify(message->field(6)->has_default_value());
      verify(message->field(7)->has_default_value());
      verify(message->field(8)->has_default_value());
      verify(message->field(9)->has_default_value());
      verify(message->field(10)->has_default_value());

      verify(-1 == message->field(0)->default_value_int32());
      verify(42 == message->field(2)->default_value_uint32());
      verify(GOOGLE_ULONGLONG(2000000000000) ==
        message->field(3)->default_value_uint64());
      verify(4.5 == message->field(4)->default_value_float());
      verify(10e100 == message->field(5)->default_value_double());
      verify(message->field(6)->default_value_bool());
      verify("hello" == message->field(7)->default_value_string());
      verify("\001\002\003" == message->field(8)->default_value_string());
      verify(enum_value_b == message->field(9)->default_value_enum());
      verify("" == message->field(10)->default_value_string());

      verify(false == message->field(11)->has_default_value());
      verify(false == message->field(12)->has_default_value());
      verify(false == message->field(13)->has_default_value());
      verify(false == message->field(14)->has_default_value());
      verify(false == message->field(15)->has_default_value());
      verify(false == message->field(16)->has_default_value());
      verify(false == message->field(17)->has_default_value());
      verify(false == message->field(18)->has_default_value());
      verify(false == message->field(19)->has_default_value());
      verify(false == message->field(20)->has_default_value());

      verify(0 == message->field(11)->default_value_int32());
      verify(0 == message->field(12)->default_value_int64());
      verify(0 == message->field(13)->default_value_uint32());
      verify(0 == message->field(14)->default_value_uint64());
      verify(0.0f == message->field(15)->default_value_float());
      verify(0.0 == message->field(16)->default_value_double());
      verify(false == message->field(17)->default_value_bool());
      verify("" == message->field(18)->default_value_string());
      verify("" == message->field(19)->default_value_string());
      verify(enum_value_a == message->field(20)->default_value_enum());
    }

    // MiscTest, FieldOptions
    {
      // Try setting field options.

      google::protobuf::FileDescriptorProto file_proto;
      file_proto.set_name("foo.proto");

      google::protobuf::DescriptorProto* message_proto = AddMessage(&file_proto, "TestMessage");
      AddField(message_proto, "foo", 1,
        google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
        google::protobuf::FieldDescriptorProto::TYPE_INT32);
      google::protobuf::FieldDescriptorProto* bar_proto =
        AddField(message_proto, "bar", 2,
          google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL,
          google::protobuf::FieldDescriptorProto::TYPE_INT32);

      google::protobuf::FieldOptions* options = bar_proto->mutable_options();
      options->set_ctype(google::protobuf::FieldOptions::CORD);

      // Build the descriptors and get the pointers.
      google::protobuf::DescriptorPool pool;
      const google::protobuf::FileDescriptor* file = pool.BuildFile(file_proto);
      verify(file != NULL);

      verify(1 == file->message_type_count());
      const google::protobuf::Descriptor* message = file->message_type(0);

      verify(2 == message->field_count());
      const google::protobuf::FieldDescriptor* foo = message->field(0);
      const google::protobuf::FieldDescriptor* bar = message->field(1);

      // "foo" had no options fun::set, so it should return the default options.
      verify(&google::protobuf::FieldOptions::default_instance() == &foo->options());

      // "bar" had options set
      // EXPECT_NE(&google::protobuf::FieldOptions::default_instance(), options);
      verify(&google::protobuf::FieldOptions::default_instance() != options);
      verify(bar->options().has_ctype());
      verify(google::protobuf::FieldOptions::CORD == bar->options().ctype());
    }
  }
  // // // //

  enum DescriptorPoolMode {
    NO_DATABASE,
    FALLBACK_DATABASE
  };

  // // // //
  // AllowUnknownDependenciesTest
  {
    const google::protobuf::FileDescriptor* bar_file_;
    const google::protobuf::Descriptor* bar_type_;
    const google::protobuf::FileDescriptor* foo_file_;
    const google::protobuf::Descriptor* foo_type_;
    const google::protobuf::FieldDescriptor* bar_field_;
    const google::protobuf::FieldDescriptor* baz_field_;
    const google::protobuf::FieldDescriptor* qux_field_;

    google::protobuf::SimpleDescriptorDatabase db_;        // used if in FALLBACK_DATABASE mode.
    google::protobuf::scoped_ptr<google::protobuf::DescriptorPool> pool_;

    DescriptorPoolMode mode_ = NO_DATABASE;

    auto mode = [&mode_]() -> DescriptorPoolMode {
      return mode_;
    };

    auto BuildFile = [&mode, &pool_, &db_]
    (const google::protobuf::FileDescriptorProto& proto) -> const google::protobuf::FileDescriptor*
    {
      switch (mode()) {
        case NO_DATABASE:
          return pool_->BuildFile(proto);
          break;
        case FALLBACK_DATABASE: {
          verify(db_.Add(proto));
          return pool_->FindFileByName(proto.name());
        }
      }
      GOOGLE_LOG(FATAL) << "Can't get here.";
      return NULL;
    };

    auto SetUp = [&mode, &BuildFile,
      &db_, &pool_, &bar_file_, &bar_type_, &foo_file_, &foo_type_, &bar_field_, &baz_field_, &qux_field_]()
    {
      google::protobuf::FileDescriptorProto foo_proto, bar_proto;

      switch (mode()) {
      case NO_DATABASE:
        pool_.reset(new google::protobuf::DescriptorPool);
        break;
      case FALLBACK_DATABASE:
        pool_.reset(new google::protobuf::DescriptorPool(&db_));
        break;
      }

      pool_->AllowUnknownDependencies();

      verify(google::protobuf::TextFormat::ParseFromString(
        "name: 'foo.proto'"
        "dependency: 'bar.proto'"
        "dependency: 'baz.proto'"
        "message_type {"
        "  name: 'Foo'"
        "  field { name:'bar' number:1 label:LABEL_OPTIONAL type_name:'Bar' }"
        "  field { name:'baz' number:2 label:LABEL_OPTIONAL type_name:'Baz' }"
        "  field { name:'qux' number:3 label:LABEL_OPTIONAL"
        "    type_name: '.corge.Qux'"
        "    type: TYPE_ENUM"
        "    options {"
        "      uninterpreted_option {"
        "        name {"
        "          name_part: 'grault'"
        "          is_extension: true"
        "        }"
        "        positive_int_value: 1234"
        "      }"
        "    }"
        "  }"
        "}",
        &foo_proto));
      verify(google::protobuf::TextFormat::ParseFromString(
        "name: 'bar.proto'"
        "message_type { name: 'Bar' }",
        &bar_proto));

      // Collect pointers to stuff.
      bar_file_ = BuildFile(bar_proto);
      verify(bar_file_ != NULL);

      verify(1 == bar_file_->message_type_count());
      bar_type_ = bar_file_->message_type(0);

      foo_file_ = BuildFile(foo_proto);
      verify(foo_file_ != NULL);

      verify(1 == foo_file_->message_type_count());
      foo_type_ = foo_file_->message_type(0);

      verify(3 == foo_type_->field_count());
      bar_field_ = foo_type_->field(0);
      baz_field_ = foo_type_->field(1);
      qux_field_ = foo_type_->field(2);
    };

    for (auto temp_mode : { NO_DATABASE , NO_DATABASE })
    {
      mode_ = temp_mode;

      SetUp();

      // AllowUnknownDependenciesTest, PlaceholderFile
      {
        verify(2 == foo_file_->dependency_count());
        verify(bar_file_ == foo_file_->dependency(0));
        verify(false == bar_file_->is_placeholder());

        const google::protobuf::FileDescriptor* baz_file = foo_file_->dependency(1);
        verify("baz.proto" == baz_file->name());
        verify(0 == baz_file->message_type_count());
        verify(baz_file->is_placeholder());

        // Placeholder files should not be findable.
        verify(bar_file_ == pool_->FindFileByName(bar_file_->name()));
        verify(pool_->FindFileByName(baz_file->name()) == NULL);
      }

      // AllowUnknownDependenciesTest, PlaceholderTypes
      {
        verify(google::protobuf::FieldDescriptor::TYPE_MESSAGE == bar_field_->type());
        verify(bar_type_ == bar_field_->message_type());
        verify(false == bar_type_->is_placeholder());

        verify(google::protobuf::FieldDescriptor::TYPE_MESSAGE == baz_field_->type());
        const google::protobuf::Descriptor* baz_type = baz_field_->message_type();
        verify("Baz" == baz_type->name());
        verify("Baz" == baz_type->full_name());
        verify(0 == baz_type->extension_range_count());
        verify(baz_type->is_placeholder());

        verify(google::protobuf::FieldDescriptor::TYPE_ENUM == qux_field_->type());
        const google::protobuf::EnumDescriptor* qux_type = qux_field_->enum_type();
        verify("Qux" == qux_type->name());
        verify("corge.Qux" == qux_type->full_name());
        verify(qux_type->is_placeholder());

        // Placeholder types should not be findable.
        verify(bar_type_ == pool_->FindMessageTypeByName(bar_type_->full_name()));
        verify(pool_->FindMessageTypeByName(baz_type->full_name()) == NULL);
        verify(pool_->FindEnumTypeByName(qux_type->full_name()) == NULL);
      }

      // AllowUnknownDependenciesTest, CopyTo
      {
        // FieldDescriptor::CopyTo() should write non-fully-qualified type names
        // for placeholder types which were not originally fully-qualified.
        google::protobuf::FieldDescriptorProto proto;

        // Bar is not a placeholder, so it is fully-qualified.
        bar_field_->CopyTo(&proto);
        verify(".Bar" == proto.type_name());
        verify(google::protobuf::FieldDescriptorProto::TYPE_MESSAGE == proto.type());

        // Baz is an unqualified placeholder.
        proto.Clear();
        baz_field_->CopyTo(&proto);
        verify("Baz" == proto.type_name());
        verify(false == proto.has_type());

        // Qux is a fully-qualified placeholder.
        proto.Clear();
        qux_field_->CopyTo(&proto);
        verify(".corge.Qux" == proto.type_name());
        verify(google::protobuf::FieldDescriptorProto::TYPE_ENUM == proto.type());
      }

      // AllowUnknownDependenciesTest, CustomOptions
      {
        // Qux should still have the uninterpreted option attached.
        verify(1 == qux_field_->options().uninterpreted_option_size());
        const google::protobuf::UninterpretedOption& option =
          qux_field_->options().uninterpreted_option(0);
        verify(1 == option.name_size());
        verify("grault" == option.name(0).name_part());
      }

      // AllowUnknownDependenciesTest, UnknownExtendee
      {
        // Test that we can extend an unknown type.  This is slightly tricky because
        // it means that the placeholder type must have an extension range.

        google::protobuf::FileDescriptorProto extension_proto;

        verify(google::protobuf::TextFormat::ParseFromString(
          "name: 'extension.proto'"
          "extension { extendee: 'UnknownType' name:'some_extension' number:123"
          "            label:LABEL_OPTIONAL type:TYPE_INT32 }",
          &extension_proto));
        const google::protobuf::FileDescriptor* file = BuildFile(extension_proto);

        verify(file != NULL);

        verify(1 == file->extension_count());
        const google::protobuf::Descriptor* extendee = file->extension(0)->containing_type();
        verify("UnknownType" == extendee->name());
        verify(extendee->is_placeholder());
        verify(1 == extendee->extension_range_count());
        verify(1 == extendee->extension_range(0)->start);
        verify(google::protobuf::FieldDescriptor::kMaxNumber + 1 == extendee->extension_range(0)->end);
      }

      // AllowUnknownDependenciesTest, CustomOption
      {
        // Test that we can use a custom option without having parsed
        // descriptor.proto.

        google::protobuf::FileDescriptorProto option_proto;

        verify(google::protobuf::TextFormat::ParseFromString(
          "name: \"unknown_custom_options.proto\" "
          "dependency: \"google/protobuf/descriptor.proto\" "
          "extension { "
          "  extendee: \"google.protobuf.FileOptions\" "
          "  name: \"some_option\" "
          "  number: 123456 "
          "  label: LABEL_OPTIONAL "
          "  type: TYPE_INT32 "
          "} "
          "options { "
          "  uninterpreted_option { "
          "    name { "
          "      name_part: \"some_option\" "
          "      is_extension: true "
          "    } "
          "    positive_int_value: 1234 "
          "  } "
          "  uninterpreted_option { "
          "    name { "
          "      name_part: \"unknown_option\" "
          "      is_extension: true "
          "    } "
          "    positive_int_value: 1234 "
          "  } "
          "  uninterpreted_option { "
          "    name { "
          "      name_part: \"optimize_for\" "
          "      is_extension: false "
          "    } "
          "    identifier_value: \"SPEED\" "
          "  } "
          "}",
          &option_proto));

        const google::protobuf::FileDescriptor* file = BuildFile(option_proto);
        verify(file != NULL);

        // Verify that no extension options were fun::set, but they were left as
        // uninterpreted_options.
        fun::vector<const google::protobuf::FieldDescriptor*> fields;
        file->options().GetReflection()->ListFields(file->options(), &fields);
        verify(2 == fields.size());
        verify(file->options().has_optimize_for());
        verify(2 == file->options().uninterpreted_option_size());
      }

      // AllowUnknownDependenciesTest, UndeclaredDependencyTriggersBuildOfDependency
      {
        // Crazy case: suppose foo.proto refers to a symbol without declaring the
        // dependency that finds it. In the event that the pool is backed by a
        // DescriptorDatabase, the pool will attempt to find the symbol in the
        // database. If successful, it will build the undeclared dependency to verify
        // that the file does indeed contain the symbol. If that file fails to build,
        // then its descriptors must be rolled back. However, we still want foo.proto
        // to build successfully, since we are allowing unknown dependencies.

        google::protobuf::FileDescriptorProto undeclared_dep_proto;
        // We make this file fail to build by giving it two fields with tag 1.
        verify(google::protobuf::TextFormat::ParseFromString(
          "name: \"invalid_file_as_undeclared_dep.proto\" "
          "package: \"undeclared\" "
          "message_type: {  "
          "  name: \"Quux\"  "
          "  field { "
          "    name:'qux' number:1 label:LABEL_OPTIONAL type: TYPE_INT32 "
          "  }"
          "  field { "
          "    name:'quux' number:1 label:LABEL_OPTIONAL type: TYPE_INT64 "
          "  }"
          "}",
          &undeclared_dep_proto));
        // We can't use the BuildFile() helper because we don't actually want to build
        // it into the descriptor pool in the fallback database case: it just needs to
        // be sitting in the database so that it gets built during the building of
        // test.proto below.
        switch (mode()) {
        case NO_DATABASE: {
          verify(pool_->BuildFile(undeclared_dep_proto) == NULL);
          break;
        }
        case FALLBACK_DATABASE: {
          verify(db_.Add(undeclared_dep_proto));
        }
        }

        google::protobuf::FileDescriptorProto test_proto;
        verify(google::protobuf::TextFormat::ParseFromString(
          "name: \"test.proto\" "
          "message_type: { "
          "  name: \"Corge\" "
          "  field { "
          "    name:'quux' number:1 label: LABEL_OPTIONAL "
          "    type_name:'undeclared.Quux' type: TYPE_MESSAGE "
          "  }"
          "}",
          &test_proto));

        const google::protobuf::FileDescriptor* file = BuildFile(test_proto);
        verify(file != NULL);
        GOOGLE_LOG(INFO) << file->DebugString();

        verify(0 == file->dependency_count());
        verify(1 == file->message_type_count());
        const google::protobuf::Descriptor* corge_desc = file->message_type(0);
        verify("Corge" == corge_desc->name());
        verify(1 == corge_desc->field_count());
        verify(false == corge_desc->is_placeholder());

        const google::protobuf::FieldDescriptor* quux_field = corge_desc->field(0);
        verify(google::protobuf::FieldDescriptor::TYPE_MESSAGE == quux_field->type());
        verify("Quux" == quux_field->message_type()->name());
        verify("undeclared.Quux" == quux_field->message_type()->full_name());
        verify(quux_field->message_type()->is_placeholder());
        // The place holder type should not be findable.
        verify(pool_->FindMessageTypeByName("undeclared.Quux") == NULL);
      }
    }
  }
  // // // //

  // // // //
  // CustomOptions

  // CustomOptions, OptionLocations
  {
    const google::protobuf::Descriptor* message =
      protobuf_unittest::TestMessageWithCustomOptions::descriptor();
    const google::protobuf::FileDescriptor* file = message->file();
    const google::protobuf::FieldDescriptor* field = message->FindFieldByName("field1");
    const google::protobuf::EnumDescriptor* enm = message->FindEnumTypeByName("AnEnum");
    // TODO(benjy): Support EnumValue options, once the compiler does.
    const google::protobuf::ServiceDescriptor* service =
      file->FindServiceByName("TestServiceWithCustomOptions");
    const google::protobuf::MethodDescriptor* method = service->FindMethodByName("Foo");

    verify(GOOGLE_LONGLONG(9876543210) ==
      file->options().GetExtension(protobuf_unittest::file_opt1));
    verify(-56 ==
      message->options().GetExtension(protobuf_unittest::message_opt1));
    verify(GOOGLE_LONGLONG(8765432109) ==
      field->options().GetExtension(protobuf_unittest::field_opt1));
    verify(42 ==  // Check that we get the default for an option we don't set
      field->options().GetExtension(protobuf_unittest::field_opt2));
    verify(-789 ==
      enm->options().GetExtension(protobuf_unittest::enum_opt1));
    verify(123 ==
      enm->value(1)->options().GetExtension(
        protobuf_unittest::enum_value_opt1));
    verify(GOOGLE_LONGLONG(-9876543210) ==
      service->options().GetExtension(protobuf_unittest::service_opt1));
    verify(protobuf_unittest::METHODOPT1_VAL2 ==
      method->options().GetExtension(protobuf_unittest::method_opt1));

    // See that the regular options went through unscathed.
    verify(message->options().has_message_set_wire_format());
    verify(google::protobuf::FieldOptions::CORD == field->options().ctype());
  }

  // CustomOptions, OptionTypes
  {
    const google::protobuf::MessageOptions* options = NULL;

    options =
      &protobuf_unittest::CustomOptionMinIntegerValues::descriptor()->options();
    verify(false == options->GetExtension(protobuf_unittest::bool_opt));
    verify(google::protobuf::kint32min == options->GetExtension(protobuf_unittest::int32_opt));
    verify(google::protobuf::kint64min == options->GetExtension(protobuf_unittest::int64_opt));
    verify(0 == options->GetExtension(protobuf_unittest::uint32_opt));
    verify(0 == options->GetExtension(protobuf_unittest::uint64_opt));
    verify(google::protobuf::kint32min == options->GetExtension(protobuf_unittest::sint32_opt));
    verify(google::protobuf::kint64min == options->GetExtension(protobuf_unittest::sint64_opt));
    verify(0 == options->GetExtension(protobuf_unittest::fixed32_opt));
    verify(0 == options->GetExtension(protobuf_unittest::fixed64_opt));
    verify(google::protobuf::kint32min == options->GetExtension(protobuf_unittest::sfixed32_opt));
    verify(google::protobuf::kint64min == options->GetExtension(protobuf_unittest::sfixed64_opt));

    options =
      &protobuf_unittest::CustomOptionMaxIntegerValues::descriptor()->options();
    verify(true == options->GetExtension(protobuf_unittest::bool_opt));
    verify(google::protobuf::kint32max == options->GetExtension(protobuf_unittest::int32_opt));
    verify(google::protobuf::kint64max == options->GetExtension(protobuf_unittest::int64_opt));
    verify(google::protobuf::kuint32max == options->GetExtension(protobuf_unittest::uint32_opt));
    verify(google::protobuf::kuint64max == options->GetExtension(protobuf_unittest::uint64_opt));
    verify(google::protobuf::kint32max == options->GetExtension(protobuf_unittest::sint32_opt));
    verify(google::protobuf::kint64max == options->GetExtension(protobuf_unittest::sint64_opt));
    verify(google::protobuf::kuint32max == options->GetExtension(protobuf_unittest::fixed32_opt));
    verify(google::protobuf::kuint64max == options->GetExtension(protobuf_unittest::fixed64_opt));
    verify(google::protobuf::kint32max == options->GetExtension(protobuf_unittest::sfixed32_opt));
    verify(google::protobuf::kint64max == options->GetExtension(protobuf_unittest::sfixed64_opt));

    options =
      &protobuf_unittest::CustomOptionOtherValues::descriptor()->options();
    verify(-100 == options->GetExtension(protobuf_unittest::int32_opt));
    verify(static_cast<float>(12.3456789) ==
      options->GetExtension(protobuf_unittest::float_opt));
    verify(static_cast<double>(1.234567890123456789) ==
      options->GetExtension(protobuf_unittest::double_opt));
    verify("Hello, \"World\"" ==
      options->GetExtension(protobuf_unittest::string_opt));

    verify(fun::string("Hello\0World", 11) ==
      options->GetExtension(protobuf_unittest::bytes_opt));

    verify(protobuf_unittest::DummyMessageContainingEnum::TEST_OPTION_ENUM_TYPE2 ==
      options->GetExtension(protobuf_unittest::enum_opt));

    options =
      &protobuf_unittest::SettingRealsFromPositiveInts::descriptor()->options();
    verify(12 == options->GetExtension(protobuf_unittest::float_opt));
    verify(154 == options->GetExtension(protobuf_unittest::double_opt));

    options =
      &protobuf_unittest::SettingRealsFromNegativeInts::descriptor()->options();
    verify(-12 == options->GetExtension(protobuf_unittest::float_opt));
    verify(-154 == options->GetExtension(protobuf_unittest::double_opt));
  }

  // CustomOptions, ComplexExtensionOptions
  {
    const google::protobuf::MessageOptions* options =
      &protobuf_unittest::VariousComplexOptions::descriptor()->options();
    verify(options->GetExtension(protobuf_unittest::complex_opt1).foo() == 42);
    verify(options->GetExtension(protobuf_unittest::complex_opt1).
      GetExtension(protobuf_unittest::quux) == 324);
    verify(options->GetExtension(protobuf_unittest::complex_opt1).
      GetExtension(protobuf_unittest::corge).qux() == 876);
    verify(options->GetExtension(protobuf_unittest::complex_opt2).baz() == 987);
    verify(options->GetExtension(protobuf_unittest::complex_opt2).
      GetExtension(protobuf_unittest::grault) == 654);
    verify(options->GetExtension(protobuf_unittest::complex_opt2).bar().foo() ==
      743);
    verify(options->GetExtension(protobuf_unittest::complex_opt2).bar().
      GetExtension(protobuf_unittest::quux) == 1999);
    verify(options->GetExtension(protobuf_unittest::complex_opt2).bar().
      GetExtension(protobuf_unittest::corge).qux() == 2008);
    verify(options->GetExtension(protobuf_unittest::complex_opt2).
      GetExtension(protobuf_unittest::garply).foo() == 741);
    verify(options->GetExtension(protobuf_unittest::complex_opt2).
      GetExtension(protobuf_unittest::garply).
      GetExtension(protobuf_unittest::quux) == 1998);
    verify(options->GetExtension(protobuf_unittest::complex_opt2).
      GetExtension(protobuf_unittest::garply).
      GetExtension(protobuf_unittest::corge).qux() == 2121);
    verify(options->GetExtension(
      protobuf_unittest::ComplexOptionType2::ComplexOptionType4::complex_opt4).
      waldo() == 1971);
    verify(options->GetExtension(protobuf_unittest::complex_opt2).
      fred().waldo() == 321);
    verify(9 == options->GetExtension(protobuf_unittest::complex_opt3).qux());
    verify(22 == options->GetExtension(protobuf_unittest::complex_opt3).
      complexoptiontype5().plugh());
    verify(24 == options->GetExtension(protobuf_unittest::complexopt6).xyzzy());
  }

  // CustomOptions, OptionsFromOtherFile
  {
    // Test that to use a custom option, we only need to import the file
    // defining the option; we do not also have to import descriptor.proto.
    google::protobuf::DescriptorPool pool;

    google::protobuf::FileDescriptorProto file_proto;
    google::protobuf::FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
    verify(pool.BuildFile(file_proto) != NULL);

    protobuf_unittest::TestMessageWithCustomOptions::descriptor()
      ->file()->CopyTo(&file_proto);
    verify(pool.BuildFile(file_proto) != NULL);

    verify(google::protobuf::TextFormat::ParseFromString(
      "name: \"custom_options_import.proto\" "
      "package: \"protobuf_unittest\" "
      "dependency: \"google/protobuf/unittest_custom_options.proto\" "
      "options { "
      "  uninterpreted_option { "
      "    name { "
      "      name_part: \"file_opt1\" "
      "      is_extension: true "
      "    } "
      "    positive_int_value: 1234 "
      "  } "
      // Test a non-extension option too.  (At one point this failed due to a
      // bug.)
      "  uninterpreted_option { "
      "    name { "
      "      name_part: \"java_package\" "
      "      is_extension: false "
      "    } "
      "    string_value: \"foo\" "
      "  } "
      // Test that enum-typed options still work too.  (At one point this also
      // failed due to a bug.)
      "  uninterpreted_option { "
      "    name { "
      "      name_part: \"optimize_for\" "
      "      is_extension: false "
      "    } "
      "    identifier_value: \"SPEED\" "
      "  } "
      "}"
      ,
      &file_proto));

    const google::protobuf::FileDescriptor* file = pool.BuildFile(file_proto);
    verify(file != NULL);
    verify(1234 == file->options().GetExtension(protobuf_unittest::file_opt1));
    verify(file->options().has_java_package());
    verify("foo" == file->options().java_package());
    verify(file->options().has_optimize_for());
    verify(google::protobuf::FileOptions::SPEED == file->options().optimize_for());
  }

  // CustomOptions, MessageOptionThreeFieldsSet
  {
    // This tests a bug which previously existed in custom options parsing.  The
    // bug occurred when you defined a custom option with message type and then
    // fun::set three fields of that option on a single definition (see the example
    // below).  The bug is a bit hard to explain, so check the change history if
    // you want to know more.
    google::protobuf::DescriptorPool pool;

    google::protobuf::FileDescriptorProto file_proto;
    google::protobuf::FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
    verify(pool.BuildFile(file_proto) != NULL);

    protobuf_unittest::TestMessageWithCustomOptions::descriptor()
      ->file()->CopyTo(&file_proto);
    verify(pool.BuildFile(file_proto) != NULL);

    // The following represents the definition:
    //
    //   import "google/protobuf/unittest_custom_options.proto"
    //   package protobuf_unittest;
    //   message Foo {
    //     option (complex_opt1).foo  = 1234;
    //     option (complex_opt1).foo2 = 1234;
    //     option (complex_opt1).foo3 = 1234;
    //   }
    verify(google::protobuf::TextFormat::ParseFromString(
      "name: \"custom_options_import.proto\" "
      "package: \"protobuf_unittest\" "
      "dependency: \"google/protobuf/unittest_custom_options.proto\" "
      "message_type { "
      "  name: \"Foo\" "
      "  options { "
      "    uninterpreted_option { "
      "      name { "
      "        name_part: \"complex_opt1\" "
      "        is_extension: true "
      "      } "
      "      name { "
      "        name_part: \"foo\" "
      "        is_extension: false "
      "      } "
      "      positive_int_value: 1234 "
      "    } "
      "    uninterpreted_option { "
      "      name { "
      "        name_part: \"complex_opt1\" "
      "        is_extension: true "
      "      } "
      "      name { "
      "        name_part: \"foo2\" "
      "        is_extension: false "
      "      } "
      "      positive_int_value: 1234 "
      "    } "
      "    uninterpreted_option { "
      "      name { "
      "        name_part: \"complex_opt1\" "
      "        is_extension: true "
      "      } "
      "      name { "
      "        name_part: \"foo3\" "
      "        is_extension: false "
      "      } "
      "      positive_int_value: 1234 "
      "    } "
      "  } "
      "}",
      &file_proto));

    const google::protobuf::FileDescriptor* file = pool.BuildFile(file_proto);
    verify(file != NULL);
    verify(1 == file->message_type_count());

    const google::protobuf::MessageOptions& options = file->message_type(0)->options();
    verify(1234 == options.GetExtension(protobuf_unittest::complex_opt1).foo());
  }

  // CustomOptions, MessageOptionRepeatedLeafFieldSet
  {
    // This test verifies that repeated fields in custom options can be
    // given multiple values by repeating the option with a different value.
    // This test checks repeated leaf values. Each repeated custom value
    // appears in a different uninterpreted_option, which will be concatenated
    // when they are merged into the final option value.
    google::protobuf::DescriptorPool pool;

    google::protobuf::FileDescriptorProto file_proto;
    google::protobuf::FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
    verify(pool.BuildFile(file_proto) != NULL);

    protobuf_unittest::TestMessageWithCustomOptions::descriptor()
      ->file()->CopyTo(&file_proto);
    verify(pool.BuildFile(file_proto) != NULL);

    // The following represents the definition:
    //
    //   import "google/protobuf/unittest_custom_options.proto"
    //   package protobuf_unittest;
    //   message Foo {
    //     option (complex_opt1).foo4 = 12;
    //     option (complex_opt1).foo4 = 34;
    //     option (complex_opt1).foo4 = 56;
    //   }
    verify(google::protobuf::TextFormat::ParseFromString(
      "name: \"custom_options_import.proto\" "
      "package: \"protobuf_unittest\" "
      "dependency: \"google/protobuf/unittest_custom_options.proto\" "
      "message_type { "
      "  name: \"Foo\" "
      "  options { "
      "    uninterpreted_option { "
      "      name { "
      "        name_part: \"complex_opt1\" "
      "        is_extension: true "
      "      } "
      "      name { "
      "        name_part: \"foo4\" "
      "        is_extension: false "
      "      } "
      "      positive_int_value: 12 "
      "    } "
      "    uninterpreted_option { "
      "      name { "
      "        name_part: \"complex_opt1\" "
      "        is_extension: true "
      "      } "
      "      name { "
      "        name_part: \"foo4\" "
      "        is_extension: false "
      "      } "
      "      positive_int_value: 34 "
      "    } "
      "    uninterpreted_option { "
      "      name { "
      "        name_part: \"complex_opt1\" "
      "        is_extension: true "
      "      } "
      "      name { "
      "        name_part: \"foo4\" "
      "        is_extension: false "
      "      } "
      "      positive_int_value: 56 "
      "    } "
      "  } "
      "}",
      &file_proto));

    const google::protobuf::FileDescriptor* file = pool.BuildFile(file_proto);
    verify(file != NULL);
    verify(1 == file->message_type_count());

    const google::protobuf::MessageOptions& options = file->message_type(0)->options();
    verify(3 == options.GetExtension(protobuf_unittest::complex_opt1).foo4_size());
    verify(12 == options.GetExtension(protobuf_unittest::complex_opt1).foo4(0));
    verify(34 == options.GetExtension(protobuf_unittest::complex_opt1).foo4(1));
    verify(56 == options.GetExtension(protobuf_unittest::complex_opt1).foo4(2));
  }

  // CustomOptions, MessageOptionRepeatedMsgFieldSet
  {
    // This test verifies that repeated fields in custom options can be
    // given multiple values by repeating the option with a different value.
    // This test checks repeated message values. Each repeated custom value
    // appears in a different uninterpreted_option, which will be concatenated
    // when they are merged into the final option value.
    google::protobuf::DescriptorPool pool;

    google::protobuf::FileDescriptorProto file_proto;
    google::protobuf::FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
    verify(pool.BuildFile(file_proto) != NULL);

    protobuf_unittest::TestMessageWithCustomOptions::descriptor()
      ->file()->CopyTo(&file_proto);
    verify(pool.BuildFile(file_proto) != NULL);

    // The following represents the definition:
    //
    //   import "google/protobuf/unittest_custom_options.proto"
    //   package protobuf_unittest;
    //   message Foo {
    //     option (complex_opt2).barney = {waldo: 1};
    //     option (complex_opt2).barney = {waldo: 10};
    //     option (complex_opt2).barney = {waldo: 100};
    //   }
    verify(google::protobuf::TextFormat::ParseFromString(
      "name: \"custom_options_import.proto\" "
      "package: \"protobuf_unittest\" "
      "dependency: \"google/protobuf/unittest_custom_options.proto\" "
      "message_type { "
      "  name: \"Foo\" "
      "  options { "
      "    uninterpreted_option { "
      "      name { "
      "        name_part: \"complex_opt2\" "
      "        is_extension: true "
      "      } "
      "      name { "
      "        name_part: \"barney\" "
      "        is_extension: false "
      "      } "
      "      aggregate_value: \"waldo: 1\" "
      "    } "
      "    uninterpreted_option { "
      "      name { "
      "        name_part: \"complex_opt2\" "
      "        is_extension: true "
      "      } "
      "      name { "
      "        name_part: \"barney\" "
      "        is_extension: false "
      "      } "
      "      aggregate_value: \"waldo: 10\" "
      "    } "
      "    uninterpreted_option { "
      "      name { "
      "        name_part: \"complex_opt2\" "
      "        is_extension: true "
      "      } "
      "      name { "
      "        name_part: \"barney\" "
      "        is_extension: false "
      "      } "
      "      aggregate_value: \"waldo: 100\" "
      "    } "
      "  } "
      "}",
      &file_proto));

    const google::protobuf::FileDescriptor* file = pool.BuildFile(file_proto);
    verify(file != NULL);
    verify(1 == file->message_type_count());

    const google::protobuf::MessageOptions& options = file->message_type(0)->options();
    verify(3 == options.GetExtension(
      protobuf_unittest::complex_opt2).barney_size());
    verify(1 == options.GetExtension(
      protobuf_unittest::complex_opt2).barney(0).waldo());
    verify(10 == options.GetExtension(
      protobuf_unittest::complex_opt2).barney(1).waldo());
    verify(100 == options.GetExtension(
      protobuf_unittest::complex_opt2).barney(2).waldo());
  }

  // Check that aggregate options were parsed and saved correctly in
  // the appropriate descriptors.
  // CustomOptions, AggregateOptions
  {
    const google::protobuf::Descriptor* msg = protobuf_unittest::AggregateMessage::descriptor();
    const google::protobuf::FileDescriptor* file = msg->file();
    const google::protobuf::FieldDescriptor* field = msg->FindFieldByName("fieldname");
    const google::protobuf::EnumDescriptor* enumd = file->FindEnumTypeByName("AggregateEnum");
    const google::protobuf::EnumValueDescriptor* enumv = enumd->FindValueByName("VALUE");
    const google::protobuf::ServiceDescriptor* service = file->FindServiceByName(
      "AggregateService");
    const google::protobuf::MethodDescriptor* method = service->FindMethodByName("Method");

    // Tests for the different types of data embedded in fileopt
    const protobuf_unittest::Aggregate& file_options =
      file->options().GetExtension(protobuf_unittest::fileopt);
    verify(100 == file_options.i());
    verify("FileAnnotation" == file_options.s());
    verify("NestedFileAnnotation" == file_options.sub().s());
    verify("FileExtensionAnnotation" ==
      file_options.file().GetExtension(protobuf_unittest::fileopt).s());
    verify("EmbeddedMessageSetElement" ==
      file_options.mset().GetExtension(
        protobuf_unittest::AggregateMessageSetElement
        ::message_set_extension).s());

    // Simple tests for all the other types of annotations
    verify("MessageAnnotation" ==
      msg->options().GetExtension(protobuf_unittest::msgopt).s());
    verify("FieldAnnotation" ==
      field->options().GetExtension(protobuf_unittest::fieldopt).s());
    verify("EnumAnnotation" ==
      enumd->options().GetExtension(protobuf_unittest::enumopt).s());
    verify("EnumValueAnnotation" ==
      enumv->options().GetExtension(protobuf_unittest::enumvalopt).s());
    verify("ServiceAnnotation" ==
      service->options().GetExtension(protobuf_unittest::serviceopt).s());
    verify("MethodAnnotation" ==
      method->options().GetExtension(protobuf_unittest::methodopt).s());
  }

  // CustomOptions, UnusedImportWarning
  {
    google::protobuf::DescriptorPool pool;

    google::protobuf::FileDescriptorProto file_proto;
    google::protobuf::FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
    verify(pool.BuildFile(file_proto) != NULL);

    protobuf_unittest::TestMessageWithCustomOptions::descriptor()
      ->file()->CopyTo(&file_proto);
    verify(pool.BuildFile(file_proto) != NULL);


    pool.AddUnusedImportTrackFile("custom_options_import.proto");
    verify(google::protobuf::TextFormat::ParseFromString(
      "name: \"custom_options_import.proto\" "
      "package: \"protobuf_unittest\" "
      "dependency: \"google/protobuf/unittest_custom_options.proto\" ",
      &file_proto));
    pool.BuildFile(file_proto);
  }
  // // // //

  // ===================================================================

  // The tests below trigger every unique call to AddError() in descriptor.cc,
  // in the order in which they appear in that file.  I'm using TextFormat here
  // to specify the input descriptors because building them using code would
  // be too bulky.

  class MockErrorCollector : public google::protobuf::DescriptorPool::ErrorCollector {
  public:
    MockErrorCollector() {}
    ~MockErrorCollector() {}

    fun::string text_;
    fun::string warning_text_;

    // implements ErrorCollector ---------------------------------------
    void AddError(const fun::string& filename,
      const fun::string& element_name, const google::protobuf::Message* descriptor,
      ErrorLocation location, const fun::string& message) {
      const char* location_name = NULL;
      switch (location) {
        case NAME: location_name = "NAME"; break;
        case NUMBER: location_name = "NUMBER"; break;
        case TYPE: location_name = "TYPE"; break;
        case EXTENDEE: location_name = "EXTENDEE"; break;
        case DEFAULT_VALUE: location_name = "DEFAULT_VALUE"; break;
        case OPTION_NAME: location_name = "OPTION_NAME"; break;
        case OPTION_VALUE: location_name = "OPTION_VALUE"; break;
        case INPUT_TYPE: location_name = "INPUT_TYPE"; break;
        case OUTPUT_TYPE: location_name = "OUTPUT_TYPE"; break;
        case OTHER: location_name = "OTHER"; break;
      }

      google::protobuf::strings::SubstituteAndAppend(
        &text_, "$0: $1: $2: $3\n",
        filename, element_name, location_name, message);
    }

    // implements ErrorCollector ---------------------------------------
    void AddWarning(const fun::string& filename, const fun::string& element_name,
      const google::protobuf::Message* descriptor, ErrorLocation location,
      const fun::string& message) {
      const char* location_name = NULL;
      switch (location) {
      case NAME: location_name = "NAME"; break;
      case NUMBER: location_name = "NUMBER"; break;
      case TYPE: location_name = "TYPE"; break;
      case EXTENDEE: location_name = "EXTENDEE"; break;
      case DEFAULT_VALUE: location_name = "DEFAULT_VALUE"; break;
      case OPTION_NAME: location_name = "OPTION_NAME"; break;
      case OPTION_VALUE: location_name = "OPTION_VALUE"; break;
      case INPUT_TYPE: location_name = "INPUT_TYPE"; break;
      case OUTPUT_TYPE: location_name = "OUTPUT_TYPE"; break;
      case OTHER: location_name = "OTHER"; break;
      }

      google::protobuf::strings::SubstituteAndAppend(
        &warning_text_, "$0: $1: $2: $3\n",
        filename, element_name, location_name, message);
    }
  };

  // // // //
  // ValidationErrorTest
  {
    google::protobuf::DescriptorPool *pool_;

    // Parse file_text as a FileDescriptorProto in text format and add it
    // to the DescriptorPool.  Expect no errors.
    auto BuildFile = [&pool_](const fun::string& file_text)
    {
      google::protobuf::FileDescriptorProto file_proto;
      verify(google::protobuf::TextFormat::ParseFromString(file_text, &file_proto));
      verify(pool_->BuildFile(file_proto) != NULL);
    };

    // Parse file_text as a FileDescriptorProto in text format and add it
    // to the DescriptorPool.  Expect errors to be produced which match the
    // given error text.
    auto BuildFileWithErrors = [&pool_](const fun::string& file_text,
      const fun::string& expected_errors) {
      google::protobuf::FileDescriptorProto file_proto;
      verify(google::protobuf::TextFormat::ParseFromString(file_text, &file_proto));

      MockErrorCollector error_collector;
      verify(pool_->BuildFileCollectingErrors(file_proto, &error_collector) == NULL);
      verify(expected_errors == error_collector.text_);
    };

    // Parse file_text as a FileDescriptorProto in text format and add it
    // to the DescriptorPool.  Expect errors to be produced which match the
    // given warning text.
    auto BuildFileWithWarnings = [&pool_](const fun::string& file_text,
      const fun::string& expected_warnings) {
      google::protobuf::FileDescriptorProto file_proto;
      verify(google::protobuf::TextFormat::ParseFromString(file_text, &file_proto));

      MockErrorCollector error_collector;
      verify(pool_->BuildFileCollectingErrors(file_proto, &error_collector));
      verify(expected_warnings == error_collector.warning_text_);
    };

    // Builds some already-parsed file in our test pool.
    auto BuildFileInTestPool = [&pool_](const google::protobuf::FileDescriptor* file)
    {
      google::protobuf::FileDescriptorProto file_proto;
      file->CopyTo(&file_proto);
      verify(pool_->BuildFile(file_proto) != NULL);
    };

    // Build descriptor.proto in our test pool. This allows us to extend it in
    // the test pool, so we can test custom options.
    auto BuildDescriptorMessagesInTestPool = [&BuildFileInTestPool]() {
      BuildFileInTestPool(google::protobuf::DescriptorProto::descriptor()->file());
    };

    // ValidationErrorTest, AlreadyDefined)
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type { name: \"Foo\" }"
        "message_type { name: \"Foo\" }",

        "foo.proto: Foo: NAME: \"Foo\" is already defined.\n");
    }

    // ValidationErrorTest, AlreadyDefinedInPackage
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "package: \"foo.bar\" "
        "message_type { name: \"Foo\" }"
        "message_type { name: \"Foo\" }",

        "foo.proto: foo.bar.Foo: NAME: \"Foo\" is already defined in "
        "\"foo.bar\".\n");
    }

    // ValidationErrorTest, AlreadyDefinedInOtherFile
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFile(
        "name: \"foo.proto\" "
        "message_type { name: \"Foo\" }");

      BuildFileWithErrors(
        "name: \"bar.proto\" "
        "message_type { name: \"Foo\" }",

        "bar.proto: Foo: NAME: \"Foo\" is already defined in file "
        "\"foo.proto\".\n");
    }

    // ValidationErrorTest, PackageAlreadyDefined
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      // pool_
      BuildFile(
        "name: \"foo.proto\" "
        "message_type { name: \"foo\" }");
      BuildFileWithErrors(
        "name: \"bar.proto\" "
        "package: \"foo.bar\"",

        "bar.proto: foo: NAME: \"foo\" is already defined (as something other "
        "than a package) in file \"foo.proto\".\n");
    }

    // ValidationErrorTest, EnumValueAlreadyDefinedInParent
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "enum_type { name: \"Foo\" value { name: \"FOO\" number: 1 } } "
        "enum_type { name: \"Bar\" value { name: \"FOO\" number: 1 } } ",

        "foo.proto: FOO: NAME: \"FOO\" is already defined.\n"
        "foo.proto: FOO: NAME: Note that enum values use C++ scoping rules, "
        "meaning that enum values are siblings of their type, not children of "
        "it.  Therefore, \"FOO\" must be unique within the global scope, not "
        "just within \"Bar\".\n");
    }

    // ValidationErrorTest, EnumValueAlreadyDefinedInParentNonGlobal
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "package: \"pkg\" "
        "enum_type { name: \"Foo\" value { name: \"FOO\" number: 1 } } "
        "enum_type { name: \"Bar\" value { name: \"FOO\" number: 1 } } ",

        "foo.proto: pkg.FOO: NAME: \"FOO\" is already defined in \"pkg\".\n"
        "foo.proto: pkg.FOO: NAME: Note that enum values use C++ scoping rules, "
        "meaning that enum values are siblings of their type, not children of "
        "it.  Therefore, \"FOO\" must be unique within \"pkg\", not just within "
        "\"Bar\".\n");
    }

    // ValidationErrorTest, MissingName
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type { }",

        "foo.proto: : NAME: Missing name.\n");
    }

    // ValidationErrorTest, InvalidName
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type { name: \"$\" }",

        "foo.proto: $: NAME: \"$\" is not a valid identifier.\n");
    }

    // ValidationErrorTest, InvalidPackageName
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "package: \"foo.$\"",

        "foo.proto: foo.$: NAME: \"$\" is not a valid identifier.\n");
    }

    // ValidationErrorTest, MissingFileName
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "",

        ": : OTHER: Missing field: FileDescriptorProto.name.\n");
    }

    // ValidationErrorTest, DupeDependency
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFile("name: \"foo.proto\"");
      BuildFileWithErrors(
        "name: \"bar.proto\" "
        "dependency: \"foo.proto\" "
        "dependency: \"foo.proto\" ",

        "bar.proto: bar.proto: OTHER: Import \"foo.proto\" was listed twice.\n");
    }

    // ValidationErrorTest, UnknownDependency
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"bar.proto\" "
        "dependency: \"foo.proto\" ",

        "bar.proto: bar.proto: OTHER: Import \"foo.proto\" has not been loaded.\n");
    }

    // ValidationErrorTest, InvalidPublicDependencyIndex
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFile("name: \"foo.proto\"");
      BuildFileWithErrors(
        "name: \"bar.proto\" "
        "dependency: \"foo.proto\" "
        "public_dependency: 1",
        "bar.proto: bar.proto: OTHER: Invalid public dependency index.\n");
    }

    // ValidationErrorTest, ForeignUnimportedPackageNoCrash
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      // Used to crash:  If we depend on a non-existent file and then refer to a
      // package defined in a file that we didn't import, and that package is
      // nested within a parent package which this file is also in, and we don't
      // include that parent package in the name (i.e. we do a relative lookup)...
      // Yes, really.
      BuildFile(
        "name: 'foo.proto' "
        "package: 'outer.foo' ");
      BuildFileWithErrors(
        "name: 'bar.proto' "
        "dependency: 'baz.proto' "
        "package: 'outer.bar' "
        "message_type { "
        "  name: 'Bar' "
        "  field { name:'bar' number:1 label:LABEL_OPTIONAL type_name:'foo.Foo' }"
        "}",

        "bar.proto: bar.proto: OTHER: Import \"baz.proto\" has not been loaded.\n"
        "bar.proto: outer.bar.Bar.bar: TYPE: \"outer.foo\" seems to be defined in "
        "\"foo.proto\", which is not imported by \"bar.proto\".  To use it here, "
        "please add the necessary import.\n");
    }

    // ValidationErrorTest, DupeFile
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFile(
        "name: \"foo.proto\" "
        "message_type { name: \"Foo\" }");
      // Note:  We should *not* get redundant errors about "Foo" already being
      //   defined.
      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type { name: \"Foo\" } "
        // Add another type so that the files aren't identical (in which case there
        // would be no error).
        "enum_type { name: \"Bar\" }",

        "foo.proto: foo.proto: OTHER: A file with this name is already in the "
        "pool.\n");
    }

    // ValidationErrorTest, FieldInExtensionRange
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  field { name: \"foo\" number:  9 label:LABEL_OPTIONAL type:TYPE_INT32 }"
        "  field { name: \"bar\" number: 10 label:LABEL_OPTIONAL type:TYPE_INT32 }"
        "  field { name: \"baz\" number: 19 label:LABEL_OPTIONAL type:TYPE_INT32 }"
        "  field { name: \"qux\" number: 20 label:LABEL_OPTIONAL type:TYPE_INT32 }"
        "  extension_range { start: 10 end: 20 }"
        "}",

        "foo.proto: Foo.bar: NUMBER: Extension range 10 to 19 includes field "
        "\"bar\" (10).\n"
        "foo.proto: Foo.baz: NUMBER: Extension range 10 to 19 includes field "
        "\"baz\" (19).\n");
    }

    // ValidationErrorTest, OverlappingExtensionRanges
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  extension_range { start: 10 end: 20 }"
        "  extension_range { start: 20 end: 30 }"
        "  extension_range { start: 19 end: 21 }"
        "}",

        "foo.proto: Foo: NUMBER: Extension range 19 to 20 overlaps with "
        "already-defined range 10 to 19.\n"
        "foo.proto: Foo: NUMBER: Extension range 19 to 20 overlaps with "
        "already-defined range 20 to 29.\n");
    }

    // ValidationErrorTest, InvalidDefaults
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""

        // Invalid number.
        "  field { name: \"foo\" number: 1 label: LABEL_OPTIONAL type: TYPE_INT32"
        "          default_value: \"abc\" }"

        // Empty default value.
        "  field { name: \"bar\" number: 2 label: LABEL_OPTIONAL type: TYPE_INT32"
        "          default_value: \"\" }"

        // Invalid boolean.
        "  field { name: \"baz\" number: 3 label: LABEL_OPTIONAL type: TYPE_BOOL"
        "          default_value: \"abc\" }"

        // Messages can't have defaults.
        "  field { name: \"qux\" number: 4 label: LABEL_OPTIONAL type: TYPE_MESSAGE"
        "          default_value: \"abc\" type_name: \"Foo\" }"

        // Same thing, but we don't know that this field has message type until
        // we look up the type name.
        "  field { name: \"quux\" number: 5 label: LABEL_OPTIONAL"
        "          default_value: \"abc\" type_name: \"Foo\" }"

        // Repeateds can't have defaults.
        "  field { name: \"corge\" number: 6 label: LABEL_REPEATED type: TYPE_INT32"
        "          default_value: \"1\" }"
        "}",

        "foo.proto: Foo.foo: DEFAULT_VALUE: Couldn't parse default value \"abc\".\n"
        "foo.proto: Foo.bar: DEFAULT_VALUE: Couldn't parse default value \"\".\n"
        "foo.proto: Foo.baz: DEFAULT_VALUE: Boolean default must be true or "
        "false.\n"
        "foo.proto: Foo.qux: DEFAULT_VALUE: Messages can't have default values.\n"
        "foo.proto: Foo.corge: DEFAULT_VALUE: Repeated fields can't have default "
        "values.\n"
        // This ends up being reported later because the error is detected at
        // cross-linking time.
        "foo.proto: Foo.quux: DEFAULT_VALUE: Messages can't have default "
        "values.\n");
    }

    // ValidationErrorTest, NegativeFieldNumber
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  field { name: \"foo\" number: -1 label:LABEL_OPTIONAL type:TYPE_INT32 }"
        "}",

        "foo.proto: Foo.foo: NUMBER: Field numbers must be positive integers.\n");
    }

    // ValidationErrorTest, HugeFieldNumber
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  field { name: \"foo\" number: 0x70000000 "
        "          label:LABEL_OPTIONAL type:TYPE_INT32 }"
        "}",

        "foo.proto: Foo.foo: NUMBER: Field numbers cannot be greater than "
        "536870911.\n");
    }

    // ValidationErrorTest, ReservedFieldNumber
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  field {name:\"foo\" number: 18999 label:LABEL_OPTIONAL type:TYPE_INT32 }"
        "  field {name:\"bar\" number: 19000 label:LABEL_OPTIONAL type:TYPE_INT32 }"
        "  field {name:\"baz\" number: 19999 label:LABEL_OPTIONAL type:TYPE_INT32 }"
        "  field {name:\"qux\" number: 20000 label:LABEL_OPTIONAL type:TYPE_INT32 }"
        "}",

        "foo.proto: Foo.bar: NUMBER: Field numbers 19000 through 19999 are "
        "reserved for the protocol buffer library implementation.\n"
        "foo.proto: Foo.baz: NUMBER: Field numbers 19000 through 19999 are "
        "reserved for the protocol buffer library implementation.\n");
    }

    // ValidationErrorTest, ExtensionMissingExtendee
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  extension { name: \"foo\" number: 1 label: LABEL_OPTIONAL"
        "              type_name: \"Foo\" }"
        "}",

        "foo.proto: Foo.foo: EXTENDEE: FieldDescriptorProto.extendee not fun::set for "
        "extension field.\n");
    }

    // ValidationErrorTest, NonExtensionWithExtendee
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Bar\""
        "  extension_range { start: 1 end: 2 }"
        "}"
        "message_type {"
        "  name: \"Foo\""
        "  field { name: \"foo\" number: 1 label: LABEL_OPTIONAL"
        "          type_name: \"Foo\" extendee: \"Bar\" }"
        "}",

        "foo.proto: Foo.foo: EXTENDEE: FieldDescriptorProto.extendee fun::set for "
        "non-extension field.\n");
    }

    // ValidationErrorTest, FieldOneofIndexTooLarge
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_INT32 "
        "          oneof_index: 1 }"
        "  field { name:\"dummy\" number:2 label:LABEL_OPTIONAL type:TYPE_INT32 "
        "          oneof_index: 0 }"
        "  oneof_decl { name:\"bar\" }"
        "}",

        "foo.proto: Foo.foo: OTHER: FieldDescriptorProto.oneof_index 1 is out of "
        "range for type \"Foo\".\n");
    }

    // ValidationErrorTest, FieldOneofIndexNegative
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_INT32 "
        "          oneof_index: -1 }"
        "  field { name:\"dummy\" number:2 label:LABEL_OPTIONAL type:TYPE_INT32 "
        "          oneof_index: 0 }"
        "  oneof_decl { name:\"bar\" }"
        "}",

        "foo.proto: Foo.foo: OTHER: FieldDescriptorProto.oneof_index -1 is out of "
        "range for type \"Foo\".\n");
    }

    // ValidationErrorTest, FieldNumberConflict
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  field { name: \"foo\" number: 1 label:LABEL_OPTIONAL type:TYPE_INT32 }"
        "  field { name: \"bar\" number: 1 label:LABEL_OPTIONAL type:TYPE_INT32 }"
        "}",

        "foo.proto: Foo.bar: NUMBER: Field number 1 has already been used in "
        "\"Foo\" by field \"foo\".\n");
    }

    // ValidationErrorTest, BadMessageSetExtensionType
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"MessageSet\""
        "  options { message_set_wire_format: true }"
        "  extension_range { start: 4 end: 5 }"
        "}"
        "message_type {"
        "  name: \"Foo\""
        "  extension { name:\"foo\" number:4 label:LABEL_OPTIONAL type:TYPE_INT32"
        "              extendee: \"MessageSet\" }"
        "}",

        "foo.proto: Foo.foo: TYPE: Extensions of MessageSets must be optional "
        "messages.\n");
    }

    // ValidationErrorTest, BadMessageSetExtensionLabel
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"MessageSet\""
        "  options { message_set_wire_format: true }"
        "  extension_range { start: 4 end: 5 }"
        "}"
        "message_type {"
        "  name: \"Foo\""
        "  extension { name:\"foo\" number:4 label:LABEL_REPEATED type:TYPE_MESSAGE"
        "              type_name: \"Foo\" extendee: \"MessageSet\" }"
        "}",

        "foo.proto: Foo.foo: TYPE: Extensions of MessageSets must be optional "
        "messages.\n");
    }

    // ValidationErrorTest, FieldInMessageSet
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  options { message_set_wire_format: true }"
        "  field { name: \"foo\" number: 1 label:LABEL_OPTIONAL type:TYPE_INT32 }"
        "}",

        "foo.proto: Foo.foo: NAME: MessageSets cannot have fields, only "
        "extensions.\n");
    }

    // TValidationErrorTest, NegativeExtensionRangeNumber
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  extension_range { start: -10 end: -1 }"
        "}",

        "foo.proto: Foo: NUMBER: Extension numbers must be positive integers.\n");
    }

    // ValidationErrorTest, HugeExtensionRangeNumber
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  extension_range { start: 1 end: 0x70000000 }"
        "}",

        "foo.proto: Foo: NUMBER: Extension numbers cannot be greater than "
        "536870911.\n");
    }

    // ValidationErrorTest, ExtensionRangeEndBeforeStart
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  extension_range { start: 10 end: 10 }"
        "  extension_range { start: 10 end: 5 }"
        "}",

        "foo.proto: Foo: NUMBER: Extension range end number must be greater than "
        "start number.\n"
        "foo.proto: Foo: NUMBER: Extension range end number must be greater than "
        "start number.\n");
    }

    // ValidationErrorTest, EmptyEnum
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "enum_type { name: \"Foo\" }"
        // Also use the empty enum in a message to make sure there are no crashes
        // during validation (possible if the code attempts to derive a default
        // value for the field).
        "message_type {"
        "  name: \"Bar\""
        "  field { name: \"foo\" number: 1 label:LABEL_OPTIONAL type_name:\"Foo\" }"
        "  field { name: \"bar\" number: 2 label:LABEL_OPTIONAL type_name:\"Foo\" "
        "          default_value: \"NO_SUCH_VALUE\" }"
        "}",

        "foo.proto: Foo: NAME: Enums must contain at least one value.\n"
        "foo.proto: Bar.bar: DEFAULT_VALUE: Enum type \"Foo\" has no value named "
        "\"NO_SUCH_VALUE\".\n");
    }

    // ValidationErrorTest, UndefinedExtendee
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  extension { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_INT32"
        "              extendee: \"Bar\" }"
        "}",

        "foo.proto: Foo.foo: EXTENDEE: \"Bar\" is not defined.\n");
    }

    // ValidationErrorTest, NonMessageExtendee
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "enum_type { name: \"Bar\" value { name:\"DUMMY\" number:0 } }"
        "message_type {"
        "  name: \"Foo\""
        "  extension { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_INT32"
        "              extendee: \"Bar\" }"
        "}",

        "foo.proto: Foo.foo: EXTENDEE: \"Bar\" is not a message type.\n");
    }

    // ValidationErrorTest, NotAnExtensionNumber
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Bar\""
        "}"
        "message_type {"
        "  name: \"Foo\""
        "  extension { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_INT32"
        "              extendee: \"Bar\" }"
        "}",

        "foo.proto: Foo.foo: NUMBER: \"Bar\" does not declare 1 as an extension "
        "number.\n");
    }

    // ValidationErrorTest, RequiredExtension
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Bar\""
        "  extension_range { start: 1000 end: 10000 }"
        "}"
        "message_type {"
        "  name: \"Foo\""
        "  extension {"
        "    name:\"foo\""
        "    number:1000"
        "    label:LABEL_REQUIRED"
        "    type:TYPE_INT32"
        "    extendee: \"Bar\""
        "  }"
        "}",

        "foo.proto: Foo.foo: TYPE: Message extensions cannot have required "
        "fields.\n");
    }

    // ValidationErrorTest, UndefinedFieldType
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type_name:\"Bar\" }"
        "}",

        "foo.proto: Foo.foo: TYPE: \"Bar\" is not defined.\n");
    }

    // ValidationErrorTest, UndefinedFieldTypeWithDefault
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      // See b/12533582. Previously this failed because the default value was not
      // accepted by the parser, which assumed an enum type, leading to an unclear
      // error message. We want this input to yield a validation error instead,
      // since the unknown type is the primary problem.
      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type_name:\"int\" "
        "          default_value:\"1\" }"
        "}",

        "foo.proto: Foo.foo: TYPE: \"int\" is not defined.\n");
    }

    // ValidationErrorTest, UndefinedNestedFieldType
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  nested_type { name:\"Baz\" }"
        "  field { name:\"foo\" number:1"
        "          label:LABEL_OPTIONAL"
        "          type_name:\"Foo.Baz.Bar\" }"
        "}",

        "foo.proto: Foo.foo: TYPE: \"Foo.Baz.Bar\" is not defined.\n");
    }

    // ValidationErrorTest, FieldTypeDefinedInUndeclaredDependency
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFile(
        "name: \"bar.proto\" "
        "message_type { name: \"Bar\" } ");

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type_name:\"Bar\" }"
        "}",
        "foo.proto: Foo.foo: TYPE: \"Bar\" seems to be defined in \"bar.proto\", "
        "which is not imported by \"foo.proto\".  To use it here, please add the "
        "necessary import.\n");
    }

    // ValidationErrorTest, FieldTypeDefinedInIndirectDependency
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      // Test for hidden dependencies.
      //
      // // bar.proto
      // message Bar{}
      //
      // // forward.proto
      // import "bar.proto"
      //
      // // foo.proto
      // import "forward.proto"
      // message Foo {
      //   optional Bar foo = 1;  // Error, needs to import bar.proto explicitly.
      // }
      //
      BuildFile(
        "name: \"bar.proto\" "
        "message_type { name: \"Bar\" }");

      BuildFile(
        "name: \"forward.proto\""
        "dependency: \"bar.proto\"");

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"forward.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type_name:\"Bar\" }"
        "}",
        "foo.proto: Foo.foo: TYPE: \"Bar\" seems to be defined in \"bar.proto\", "
        "which is not imported by \"foo.proto\".  To use it here, please add the "
        "necessary import.\n");
    }

    // ValidationErrorTest, FieldTypeDefinedInPublicDependency
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      // Test for public dependencies.
      //
      // // bar.proto
      // message Bar{}
      //
      // // forward.proto
      // import public "bar.proto"
      //
      // // foo.proto
      // import "forward.proto"
      // message Foo {
      //   optional Bar foo = 1;  // Correct. "bar.proto" is public imported into
      //                          // forward.proto, so when "foo.proto" imports
      //                          // "forward.proto", it imports "bar.proto" too.
      // }
      //
      BuildFile(
        "name: \"bar.proto\" "
        "message_type { name: \"Bar\" }");

      BuildFile(
        "name: \"forward.proto\""
        "dependency: \"bar.proto\" "
        "public_dependency: 0");

      BuildFile(
        "name: \"foo.proto\" "
        "dependency: \"forward.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type_name:\"Bar\" }"
        "}");
    }

    // ValidationErrorTest, FieldTypeDefinedInTransitivePublicDependency
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      // Test for public dependencies.
      //
      // // bar.proto
      // message Bar{}
      //
      // // forward.proto
      // import public "bar.proto"
      //
      // // forward2.proto
      // import public "forward.proto"
      //
      // // foo.proto
      // import "forward2.proto"
      // message Foo {
      //   optional Bar foo = 1;  // Correct, public imports are transitive.
      // }
      //
      BuildFile(
        "name: \"bar.proto\" "
        "message_type { name: \"Bar\" }");

      BuildFile(
        "name: \"forward.proto\""
        "dependency: \"bar.proto\" "
        "public_dependency: 0");

      BuildFile(
        "name: \"forward2.proto\""
        "dependency: \"forward.proto\" "
        "public_dependency: 0");

      BuildFile(
        "name: \"foo.proto\" "
        "dependency: \"forward2.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type_name:\"Bar\" }"
        "}");
    }

    // ValidationErrorTest, FieldTypeDefinedInPrivateDependencyOfPublicDependency
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      // Test for public dependencies.
      //
      // // bar.proto
      // message Bar{}
      //
      // // forward.proto
      // import "bar.proto"
      //
      // // forward2.proto
      // import public "forward.proto"
      //
      // // foo.proto
      // import "forward2.proto"
      // message Foo {
      //   optional Bar foo = 1;  // Error, the "bar.proto" is not public imported
      //                          // into "forward.proto", so will not be imported
      //                          // into either "forward2.proto" or "foo.proto".
      // }
      //
      BuildFile(
        "name: \"bar.proto\" "
        "message_type { name: \"Bar\" }");

      BuildFile(
        "name: \"forward.proto\""
        "dependency: \"bar.proto\"");

      BuildFile(
        "name: \"forward2.proto\""
        "dependency: \"forward.proto\" "
        "public_dependency: 0");

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"forward2.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type_name:\"Bar\" }"
        "}",
        "foo.proto: Foo.foo: TYPE: \"Bar\" seems to be defined in \"bar.proto\", "
        "which is not imported by \"foo.proto\".  To use it here, please add the "
        "necessary import.\n");
    }


    // ValidationErrorTest, SearchMostLocalFirst
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      // The following should produce an error that Bar.Baz is resolved but
      // not defined:
      //   message Bar { message Baz {} }
      //   message Foo {
      //     message Bar {
      //       // Placing "message Baz{}" here, or removing Foo.Bar altogether,
      //       // would fix the error.
      //     }
      //     optional Bar.Baz baz = 1;
      //   }
      // An one point the lookup code incorrectly did not produce an error in this
      // case, because when looking for Bar.Baz, it would try "Foo.Bar.Baz" first,
      // fail, and ten try "Bar.Baz" and succeed, even though "Bar" should actually
      // refer to the inner Bar, not the outer one.
      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Bar\""
        "  nested_type { name: \"Baz\" }"
        "}"
        "message_type {"
        "  name: \"Foo\""
        "  nested_type { name: \"Bar\" }"
        "  field { name:\"baz\" number:1 label:LABEL_OPTIONAL"
        "          type_name:\"Bar.Baz\" }"
        "}",

        "foo.proto: Foo.baz: TYPE: \"Bar.Baz\" is resolved to \"Foo.Bar.Baz\","
        " which is not defined. The innermost scope is searched first in name "
        "resolution. Consider using a leading '.'(i.e., \".Bar.Baz\") to start "
        "from the outermost scope.\n");
    }

    // ValidationErrorTest, SearchMostLocalFirst2
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      // This test would find the most local "Bar" first, and does, but
      // proceeds to find the outer one because the inner one's not an
      // aggregate.
      BuildFile(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Bar\""
        "  nested_type { name: \"Baz\" }"
        "}"
        "message_type {"
        "  name: \"Foo\""
        "  field { name: \"Bar\" number:1 type:TYPE_BYTES } "
        "  field { name:\"baz\" number:2 label:LABEL_OPTIONAL"
        "          type_name:\"Bar.Baz\" }"
        "}");
    }

    // ValidationErrorTest, PackageOriginallyDeclaredInTransitiveDependent
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      // Imagine we have the following:
      //
      // foo.proto:
      //   package foo.bar;
      // bar.proto:
      //   package foo.bar;
      //   import "foo.proto";
      //   message Bar {}
      // baz.proto:
      //   package foo;
      //   import "bar.proto"
      //   message Baz { optional bar.Bar qux = 1; }
      //
      // When validating baz.proto, we will look up "bar.Bar".  As part of this
      // lookup, we first lookup "bar" then try to find "Bar" within it.  "bar"
      // should resolve to "foo.bar".  Note, though, that "foo.bar" was originally
      // defined in foo.proto, which is not a direct dependency of baz.proto.  The
      // implementation of FindSymbol() normally only returns symbols in direct
      // dependencies, not indirect ones.  This test insures that this does not
      // prevent it from finding "foo.bar".

      BuildFile(
        "name: \"foo.proto\" "
        "package: \"foo.bar\" ");
      BuildFile(
        "name: \"bar.proto\" "
        "package: \"foo.bar\" "
        "dependency: \"foo.proto\" "
        "message_type { name: \"Bar\" }");
      BuildFile(
        "name: \"baz.proto\" "
        "package: \"foo\" "
        "dependency: \"bar.proto\" "
        "message_type { "
        "  name: \"Baz\" "
        "  field { name:\"qux\" number:1 label:LABEL_OPTIONAL "
        "          type_name:\"bar.Bar\" }"
        "}");
    }

    // ValidationErrorTest, FieldTypeNotAType
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL "
        "          type_name:\".Foo.bar\" }"
        "  field { name:\"bar\" number:2 label:LABEL_OPTIONAL type:TYPE_INT32 }"
        "}",

        "foo.proto: Foo.foo: TYPE: \".Foo.bar\" is not a type.\n");
    }

    // ValidationErrorTest, RelativeFieldTypeNotAType
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  nested_type {"
        "    name: \"Bar\""
        "    field { name:\"Baz\" number:2 label:LABEL_OPTIONAL type:TYPE_INT32 }"
        "  }"
        "  name: \"Foo\""
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL "
        "          type_name:\"Bar.Baz\" }"
        "}",
        "foo.proto: Foo.foo: TYPE: \"Bar.Baz\" is not a type.\n");
    }

    // ValidationErrorTest, FieldTypeMayBeItsName
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFile(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Bar\""
        "}"
        "message_type {"
        "  name: \"Foo\""
        "  field { name:\"Bar\" number:1 label:LABEL_OPTIONAL type_name:\"Bar\" }"
        "}");
    }

    // ValidationErrorTest, EnumFieldTypeIsMessage
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type { name: \"Bar\" } "
        "message_type {"
        "  name: \"Foo\""
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_ENUM"
        "          type_name:\"Bar\" }"
        "}",

        "foo.proto: Foo.foo: TYPE: \"Bar\" is not an enum type.\n");
    }

    // ValidationErrorTest, MessageFieldTypeIsEnum
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "enum_type { name: \"Bar\" value { name:\"DUMMY\" number:0 } } "
        "message_type {"
        "  name: \"Foo\""
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_MESSAGE"
        "          type_name:\"Bar\" }"
        "}",

        "foo.proto: Foo.foo: TYPE: \"Bar\" is not a message type.\n");
    }

    // ValidationErrorTest, BadEnumDefaultValue
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "enum_type { name: \"Bar\" value { name:\"DUMMY\" number:0 } } "
        "message_type {"
        "  name: \"Foo\""
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type_name:\"Bar\""
        "          default_value:\"NO_SUCH_VALUE\" }"
        "}",

        "foo.proto: Foo.foo: DEFAULT_VALUE: Enum type \"Bar\" has no value named "
        "\"NO_SUCH_VALUE\".\n");
    }

    // ValidationErrorTest, EnumDefaultValueIsInteger
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "enum_type { name: \"Bar\" value { name:\"DUMMY\" number:0 } } "
        "message_type {"
        "  name: \"Foo\""
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type_name:\"Bar\""
        "          default_value:\"0\" }"
        "}",

        "foo.proto: Foo.foo: DEFAULT_VALUE: Default value for an enum field must "
        "be an identifier.\n");
    }

    // ValidationErrorTest, PrimitiveWithTypeName
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_INT32"
        "          type_name:\"Foo\" }"
        "}",

        "foo.proto: Foo.foo: TYPE: Field with primitive type has type_name.\n");
    }

    // ValidationErrorTest, NonPrimitiveWithoutTypeName
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_MESSAGE }"
        "}",

        "foo.proto: Foo.foo: TYPE: Field with message or enum type missing "
        "type_name.\n");
    }

    // ValidationErrorTest, OneofWithNoFields
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  oneof_decl { name:\"bar\" }"
        "}",

        "foo.proto: Foo.bar: NAME: Oneof must have at least one field.\n");
    }

    // ValidationErrorTest, OneofLabelMismatch
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"Foo\""
        "  field { name:\"foo\" number:1 label:LABEL_REPEATED type:TYPE_INT32 "
        "          oneof_index:0 }"
        "  oneof_decl { name:\"bar\" }"
        "}",

        "foo.proto: Foo.foo: NAME: Fields of oneofs must themselves have label "
        "LABEL_OPTIONAL.\n");
    }

    // ValidationErrorTest, InputTypeNotDefined
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type { name: \"Foo\" } "
        "service {"
        "  name: \"TestService\""
        "  method { name: \"A\" input_type: \"Bar\" output_type: \"Foo\" }"
        "}",

        "foo.proto: TestService.A: INPUT_TYPE: \"Bar\" is not defined.\n"
      );
    }

    // ValidationErrorTest, InputTypeNotAMessage
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type { name: \"Foo\" } "
        "enum_type { name: \"Bar\" value { name:\"DUMMY\" number:0 } } "
        "service {"
        "  name: \"TestService\""
        "  method { name: \"A\" input_type: \"Bar\" output_type: \"Foo\" }"
        "}",

        "foo.proto: TestService.A: INPUT_TYPE: \"Bar\" is not a message type.\n"
      );
    }

    // ValidationErrorTest, OutputTypeNotDefined
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type { name: \"Foo\" } "
        "service {"
        "  name: \"TestService\""
        "  method { name: \"A\" input_type: \"Foo\" output_type: \"Bar\" }"
        "}",

        "foo.proto: TestService.A: OUTPUT_TYPE: \"Bar\" is not defined.\n"
      );
    }

    // ValidationErrorTest, OutputTypeNotAMessage
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type { name: \"Foo\" } "
        "enum_type { name: \"Bar\" value { name:\"DUMMY\" number:0 } } "
        "service {"
        "  name: \"TestService\""
        "  method { name: \"A\" input_type: \"Foo\" output_type: \"Bar\" }"
        "}",

        "foo.proto: TestService.A: OUTPUT_TYPE: \"Bar\" is not a message type.\n"
      );
    }


    // ValidationErrorTest, IllegalPackedField
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {\n"
        "  name: \"Foo\""
        "  field { name:\"packed_string\" number:1 label:LABEL_REPEATED "
        "          type:TYPE_STRING "
        "          options { uninterpreted_option {"
        "            name { name_part: \"packed\" is_extension: false }"
        "            identifier_value: \"true\" }}}\n"
        "  field { name:\"packed_message\" number:3 label:LABEL_REPEATED "
        "          type_name: \"Foo\""
        "          options { uninterpreted_option {"
        "            name { name_part: \"packed\" is_extension: false }"
        "            identifier_value: \"true\" }}}\n"
        "  field { name:\"optional_int32\" number: 4 label: LABEL_OPTIONAL "
        "          type:TYPE_INT32 "
        "          options { uninterpreted_option {"
        "            name { name_part: \"packed\" is_extension: false }"
        "            identifier_value: \"true\" }}}\n"
        "}",

        "foo.proto: Foo.packed_string: TYPE: [packed = true] can only be "
        "specified for repeated primitive fields.\n"
        "foo.proto: Foo.packed_message: TYPE: [packed = true] can only be "
        "specified for repeated primitive fields.\n"
        "foo.proto: Foo.optional_int32: TYPE: [packed = true] can only be "
        "specified for repeated primitive fields.\n"
      );
    }

    // ValidationErrorTest, OptionWrongType
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type { "
        "  name: \"TestMessage\" "
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_STRING "
        "          options { uninterpreted_option { name { name_part: \"ctype\" "
        "                                                  is_extension: false }"
        "                                           positive_int_value: 1 }"
        "          }"
        "  }"
        "}\n",

        "foo.proto: TestMessage.foo: OPTION_VALUE: Value must be identifier for "
        "enum-valued option \"google.protobuf.FieldOptions.ctype\".\n");
    }

    // ValidationErrorTest, OptionExtendsAtomicType
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type { "
        "  name: \"TestMessage\" "
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_STRING "
        "          options { uninterpreted_option { name { name_part: \"ctype\" "
        "                                                  is_extension: false }"
        "                                           name { name_part: \"foo\" "
        "                                                  is_extension: true }"
        "                                           positive_int_value: 1 }"
        "          }"
        "  }"
        "}\n",

        "foo.proto: TestMessage.foo: OPTION_NAME: Option \"ctype\" is an "
        "atomic type, not a message.\n");
    }

    // ValidationErrorTest, DupOption
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type { "
        "  name: \"TestMessage\" "
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_UINT32 "
        "          options { uninterpreted_option { name { name_part: \"ctype\" "
        "                                                  is_extension: false }"
        "                                           identifier_value: \"CORD\" }"
        "                    uninterpreted_option { name { name_part: \"ctype\" "
        "                                                  is_extension: false }"
        "                                           identifier_value: \"CORD\" }"
        "          }"
        "  }"
        "}\n",

        "foo.proto: TestMessage.foo: OPTION_NAME: Option \"ctype\" was "
        "already set\n");
    }

    // ValidationErrorTest, InvalidOptionName
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type { "
        "  name: \"TestMessage\" "
        "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_BOOL "
        "          options { uninterpreted_option { "
        "                      name { name_part: \"uninterpreted_option\" "
        "                             is_extension: false }"
        "                      positive_int_value: 1 "
        "                    }"
        "          }"
        "  }"
        "}\n",

        "foo.proto: TestMessage.foo: OPTION_NAME: Option must not use "
        "reserved name \"uninterpreted_option\".\n");
    }

    // ValidationErrorTest, RepeatedMessageOption
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "message_type: { name: \"Bar\" field: { "
        "  name: \"foo\" number: 1 label: LABEL_OPTIONAL type: TYPE_INT32 } "
        "} "
        "extension { name: \"bar\" number: 7672757 label: LABEL_REPEATED "
        "            type: TYPE_MESSAGE type_name: \"Bar\" "
        "            extendee: \"google.protobuf.FileOptions\" }"
        "options { uninterpreted_option { name { name_part: \"bar\" "
        "                                        is_extension: true } "
        "                                 name { name_part: \"foo\" "
        "                                        is_extension: false } "
        "                                 positive_int_value: 1 } }",

        "foo.proto: foo.proto: OPTION_NAME: Option field \"(bar)\" is a "
        "repeated message. Repeated message options must be initialized "
        "using an aggregate value.\n");
    }

    // ValidationErrorTest, ResolveUndefinedOption
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      // The following should produce an eror that baz.bar is resolved but not
      // defined.
      // foo.proto:
      //   package baz
      //   import google/protobuf/descriptor.proto
      //   message Bar { optional int32 foo = 1; }
      //   extend FileOptions { optional Bar bar = 7672757; }
      //
      // qux.proto:
      //   package qux.baz
      //   option (baz.bar).foo = 1;
      //
      // Although "baz.bar" is already defined, the lookup code will try
      // "qux.baz.bar", since it's the match from the innermost scope, which will
      // cause a symbol not defined error.
      BuildDescriptorMessagesInTestPool();

      BuildFile(
        "name: \"foo.proto\" "
        "package: \"baz\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "message_type: { name: \"Bar\" field: { "
        "  name: \"foo\" number: 1 label: LABEL_OPTIONAL type: TYPE_INT32 } "
        "} "
        "extension { name: \"bar\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_MESSAGE type_name: \"Bar\" "
        "            extendee: \"google.protobuf.FileOptions\" }");

      BuildFileWithErrors(
        "name: \"qux.proto\" "
        "package: \"qux.baz\" "
        "options { uninterpreted_option { name { name_part: \"baz.bar\" "
        "                                        is_extension: true } "
        "                                 name { name_part: \"foo\" "
        "                                        is_extension: false } "
        "                                 positive_int_value: 1 } }",

        "qux.proto: qux.proto: OPTION_NAME: Option \"(baz.bar)\" is resolved to "
        "\"(qux.baz.bar)\","
        " which is not defined. The innermost scope is searched first in name "
        "resolution. Consider using a leading '.'(i.e., \"(.baz.bar)\") to start "
        "from the outermost scope.\n");
    }

    // ValidationErrorTest, UnknownOption
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"qux.proto\" "
        "package: \"qux.baz\" "
        "options { uninterpreted_option { name { name_part: \"baaz.bar\" "
        "                                        is_extension: true } "
        "                                 name { name_part: \"foo\" "
        "                                        is_extension: false } "
        "                                 positive_int_value: 1 } }",

        "qux.proto: qux.proto: OPTION_NAME: Option \"(baaz.bar)\" unknown.\n");
    }

    // ValidationErrorTest, CustomOptionConflictingFieldNumber
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "extension { name: \"foo1\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_INT32 extendee: \"google.protobuf.FieldOptions\" }"
        "extension { name: \"foo2\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_INT32 extendee: \"google.protobuf.FieldOptions\" }",

        "foo.proto: foo2: NUMBER: Extension number 7672757 has already been used "
        "in \"google.protobuf.FieldOptions\" by extension \"foo1\".\n");
    }

    // ValidationErrorTest, Int32OptionValueOutOfPositiveRange
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_INT32 extendee: \"google.protobuf.FileOptions\" }"
        "options { uninterpreted_option { name { name_part: \"foo\" "
        "                                        is_extension: true } "
        "                                 positive_int_value: 0x80000000 } "
        "}",

        "foo.proto: foo.proto: OPTION_VALUE: Value out of range "
        "for int32 option \"foo\".\n");
    }

    // ValidationErrorTest, Int32OptionValueOutOfNegativeRange
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_INT32 extendee: \"google.protobuf.FileOptions\" }"
        "options { uninterpreted_option { name { name_part: \"foo\" "
        "                                        is_extension: true } "
        "                                 negative_int_value: -0x80000001 } "
        "}",

        "foo.proto: foo.proto: OPTION_VALUE: Value out of range "
        "for int32 option \"foo\".\n");
    }

    // ValidationErrorTest, Int32OptionValueIsNotPositiveInt
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_INT32 extendee: \"google.protobuf.FileOptions\" }"
        "options { uninterpreted_option { name { name_part: \"foo\" "
        "                                        is_extension: true } "
        "                                 string_value: \"5\" } }",

        "foo.proto: foo.proto: OPTION_VALUE: Value must be integer "
        "for int32 option \"foo\".\n");
    }

    // ValidationErrorTest, Int64OptionValueOutOfRange
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_INT64 extendee: \"google.protobuf.FileOptions\" }"
        "options { uninterpreted_option { name { name_part: \"foo\" "
        "                                        is_extension: true } "
        "                                 positive_int_value: 0x8000000000000000 } "
        "}",

        "foo.proto: foo.proto: OPTION_VALUE: Value out of range "
        "for int64 option \"foo\".\n");
    }

    // ValidationErrorTest, Int64OptionValueIsNotPositiveInt
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_INT64 extendee: \"google.protobuf.FileOptions\" }"
        "options { uninterpreted_option { name { name_part: \"foo\" "
        "                                        is_extension: true } "
        "                                 identifier_value: \"5\" } }",

        "foo.proto: foo.proto: OPTION_VALUE: Value must be integer "
        "for int64 option \"foo\".\n");
    }

    // ValidationErrorTest, UInt32OptionValueOutOfRange
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_UINT32 extendee: \"google.protobuf.FileOptions\" }"
        "options { uninterpreted_option { name { name_part: \"foo\" "
        "                                        is_extension: true } "
        "                                 positive_int_value: 0x100000000 } }",

        "foo.proto: foo.proto: OPTION_VALUE: Value out of range "
        "for uint32 option \"foo\".\n");
    }

    // ValidationErrorTest, UInt32OptionValueIsNotPositiveInt
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_UINT32 extendee: \"google.protobuf.FileOptions\" }"
        "options { uninterpreted_option { name { name_part: \"foo\" "
        "                                        is_extension: true } "
        "                                 double_value: -5.6 } }",

        "foo.proto: foo.proto: OPTION_VALUE: Value must be non-negative integer "
        "for uint32 option \"foo\".\n");
    }

    // ValidationErrorTest, UInt64OptionValueIsNotPositiveInt
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_UINT64 extendee: \"google.protobuf.FileOptions\" }"
        "options { uninterpreted_option { name { name_part: \"foo\" "
        "                                        is_extension: true } "
        "                                 negative_int_value: -5 } }",

        "foo.proto: foo.proto: OPTION_VALUE: Value must be non-negative integer "
        "for uint64 option \"foo\".\n");
    }

    // ValidationErrorTest, FloatOptionValueIsNotNumber
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_FLOAT extendee: \"google.protobuf.FileOptions\" }"
        "options { uninterpreted_option { name { name_part: \"foo\" "
        "                                        is_extension: true } "
        "                                 string_value: \"bar\" } }",

        "foo.proto: foo.proto: OPTION_VALUE: Value must be number "
        "for float option \"foo\".\n");
    }

    // ValidationErrorTest, DoubleOptionValueIsNotNumber
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_DOUBLE extendee: \"google.protobuf.FileOptions\" }"
        "options { uninterpreted_option { name { name_part: \"foo\" "
        "                                        is_extension: true } "
        "                                 string_value: \"bar\" } }",

        "foo.proto: foo.proto: OPTION_VALUE: Value must be number "
        "for double option \"foo\".\n");
    }

    // ValidationErrorTest, BoolOptionValueIsNotTrueOrFalse
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_BOOL extendee: \"google.protobuf.FileOptions\" }"
        "options { uninterpreted_option { name { name_part: \"foo\" "
        "                                        is_extension: true } "
        "                                 identifier_value: \"bar\" } }",

        "foo.proto: foo.proto: OPTION_VALUE: Value must be \"true\" or \"false\" "
        "for boolean option \"foo\".\n");
    }

    // ValidationErrorTest, EnumOptionValueIsNotIdentifier
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "enum_type { name: \"FooEnum\" value { name: \"BAR\" number: 1 } "
        "                              value { name: \"BAZ\" number: 2 } }"
        "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_ENUM type_name: \"FooEnum\" "
        "            extendee: \"google.protobuf.FileOptions\" }"
        "options { uninterpreted_option { name { name_part: \"foo\" "
        "                                        is_extension: true } "
        "                                 string_value: \"QUUX\" } }",

        "foo.proto: foo.proto: OPTION_VALUE: Value must be identifier for "
        "enum-valued option \"foo\".\n");
    }

    // ValidationErrorTest, EnumOptionValueIsNotEnumValueName
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "enum_type { name: \"FooEnum\" value { name: \"BAR\" number: 1 } "
        "                              value { name: \"BAZ\" number: 2 } }"
        "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_ENUM type_name: \"FooEnum\" "
        "            extendee: \"google.protobuf.FileOptions\" }"
        "options { uninterpreted_option { name { name_part: \"foo\" "
        "                                        is_extension: true } "
        "                                 identifier_value: \"QUUX\" } }",

        "foo.proto: foo.proto: OPTION_VALUE: Enum type \"FooEnum\" has no value "
        "named \"QUUX\" for option \"foo\".\n");
    }

    // ValidationErrorTest, EnumOptionValueIsSiblingEnumValueName
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "enum_type { name: \"FooEnum1\" value { name: \"BAR\" number: 1 } "
        "                               value { name: \"BAZ\" number: 2 } }"
        "enum_type { name: \"FooEnum2\" value { name: \"QUX\" number: 1 } "
        "                               value { name: \"QUUX\" number: 2 } }"
        "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_ENUM type_name: \"FooEnum1\" "
        "            extendee: \"google.protobuf.FileOptions\" }"
        "options { uninterpreted_option { name { name_part: \"foo\" "
        "                                        is_extension: true } "
        "                                 identifier_value: \"QUUX\" } }",

        "foo.proto: foo.proto: OPTION_VALUE: Enum type \"FooEnum1\" has no value "
        "named \"QUUX\" for option \"foo\". This appears to be a value from a "
        "sibling type.\n");
    }

    // ValidationErrorTest, StringOptionValueIsNotString
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_STRING extendee: \"google.protobuf.FileOptions\" }"
        "options { uninterpreted_option { name { name_part: \"foo\" "
        "                                        is_extension: true } "
        "                                 identifier_value: \"QUUX\" } }",

        "foo.proto: foo.proto: OPTION_VALUE: Value must be quoted fun::string for "
        "fun::string option \"foo\".\n");
    }

    // ValidationErrorTest, DuplicateExtensionFieldNumber
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFile(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "extension { name: \"option1\" number: 1000 label: LABEL_OPTIONAL "
        "            type: TYPE_INT32 extendee: \"google.protobuf.FileOptions\" }");

      BuildFileWithWarnings(
        "name: \"bar.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "extension { name: \"option2\" number: 1000 label: LABEL_OPTIONAL "
        "            type: TYPE_INT32 extendee: \"google.protobuf.FileOptions\" }",
        "bar.proto: option2: NUMBER: Extension number 1000 has already been used "
        "in \"google.protobuf.FileOptions\" by extension \"option1\" defined in "
        "foo.proto.\n");
    }

    // Helper function for tests that check for aggregate value parsing
    // errors.  The "value" argument is embedded inside the
    // "uninterpreted_option" portion of the result.
    auto EmbedAggregateValue = [](const char* value) -> fun::string  {
      return google::protobuf::strings::Substitute(
        "name: \"foo.proto\" "
        "dependency: \"google/protobuf/descriptor.proto\" "
        "message_type { name: \"Foo\" } "
        "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
        "            type: TYPE_MESSAGE type_name: \"Foo\" "
        "            extendee: \"google.protobuf.FileOptions\" }"
        "options { uninterpreted_option { name { name_part: \"foo\" "
        "                                        is_extension: true } "
        "                                 $0 } }",
        value);
    };

    // ValidationErrorTest, AggregateValueNotFound
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        EmbedAggregateValue("string_value: \"\""),
        "foo.proto: foo.proto: OPTION_VALUE: Option \"foo\" is a message. "
        "To fun::set the entire message, use syntax like "
        "\"foo = { <proto text format> }\". To fun::set fields within it, use "
        "syntax like \"foo.foo = value\".\n");
    }

    // ValidationErrorTest, AggregateValueParseError
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        EmbedAggregateValue("aggregate_value: \"1+2\""),
        "foo.proto: foo.proto: OPTION_VALUE: Error while parsing option "
        "value for \"foo\": Expected identifier.\n");
    }

    // ValidationErrorTest, AggregateValueUnknownFields
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildDescriptorMessagesInTestPool();

      BuildFileWithErrors(
        EmbedAggregateValue("aggregate_value: \"x:100\""),
        "foo.proto: foo.proto: OPTION_VALUE: Error while parsing option "
        "value for \"foo\": Message type \"Foo\" has no field named \"x\".\n");
    }

    // ValidationErrorTest, NotLiteImportsLite
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFile(
        "name: \"bar.proto\" "
        "options { optimize_for: LITE_RUNTIME } ");

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"bar.proto\" ",

        "foo.proto: foo.proto: OTHER: Files that do not use optimize_for = "
        "LITE_RUNTIME cannot import files which do use this option.  This file "
        "is not lite, but it imports \"bar.proto\" which is.\n");
    }

    // ValidationErrorTest, LiteExtendsNotLite
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFile(
        "name: \"bar.proto\" "
        "message_type: {"
        "  name: \"Bar\""
        "  extension_range { start: 1 end: 1000 }"
        "}");

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "dependency: \"bar.proto\" "
        "options { optimize_for: LITE_RUNTIME } "
        "extension { name: \"ext\" number: 123 label: LABEL_OPTIONAL "
        "            type: TYPE_INT32 extendee: \"Bar\" }",

        "foo.proto: ext: EXTENDEE: Extensions to non-lite types can only be "
        "declared in non-lite files.  Note that you cannot extend a non-lite "
        "type to contain a lite type, but the reverse is allowed.\n");
    }

    // ValidationErrorTest, NoLiteServices
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "options {"
        "  optimize_for: LITE_RUNTIME"
        "  cc_generic_services: true"
        "  java_generic_services: true"
        "} "
        "service { name: \"Foo\" }",

        "foo.proto: Foo: NAME: Files with optimize_for = LITE_RUNTIME cannot "
        "define services unless you fun::set both options cc_generic_services and "
        "java_generic_sevices to false.\n");

      BuildFile(
        "name: \"bar.proto\" "
        "options {"
        "  optimize_for: LITE_RUNTIME"
        "  cc_generic_services: false"
        "  java_generic_services: false"
        "} "
        "service { name: \"Bar\" }");
    }

    // ValidationErrorTest, RollbackAfterError
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      // Build a file which contains every kind of construct but references an
      // undefined type.  All these constructs will be added to the symbol table
      // before the undefined type error is noticed.  The DescriptorPool will then
      // have to roll everything back.
      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"TestMessage\""
        "  field { name:\"foo\" label:LABEL_OPTIONAL type:TYPE_INT32 number:1 }"
        "} "
        "enum_type {"
        "  name: \"TestEnum\""
        "  value { name:\"BAR\" number:1 }"
        "} "
        "service {"
        "  name: \"TestService\""
        "  method {"
        "    name: \"Baz\""
        "    input_type: \"NoSuchType\""    // error
        "    output_type: \"TestMessage\""
        "  }"
        "}",

        "foo.proto: TestService.Baz: INPUT_TYPE: \"NoSuchType\" is not defined.\n"
      );

      // Make sure that if we build the same file again with the error fixed,
      // it works.  If the above rollback was incomplete, then some symbols will
      // be left defined, and this second attempt will fail since it tries to
      // re-define the same symbols.
      BuildFile(
        "name: \"foo.proto\" "
        "message_type {"
        "  name: \"TestMessage\""
        "  field { name:\"foo\" label:LABEL_OPTIONAL type:TYPE_INT32 number:1 }"
        "} "
        "enum_type {"
        "  name: \"TestEnum\""
        "  value { name:\"BAR\" number:1 }"
        "} "
        "service {"
        "  name: \"TestService\""
        "  method { name:\"Baz\""
        "           input_type:\"TestMessage\""
        "           output_type:\"TestMessage\" }"
        "}");
    }

    // ValidationErrorTest, ErrorsReportedToLogError
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      // Test that errors are reported to GOOGLE_LOG(ERROR) if no error collector is
      // provided.

      google::protobuf::FileDescriptorProto file_proto;
      verify(google::protobuf::TextFormat::ParseFromString(
        "name: \"foo.proto\" "
        "message_type { name: \"Foo\" } "
        "message_type { name: \"Foo\" } ",
        &file_proto));

      fun::vector<fun::string> errors;

      {
        google::protobuf::ScopedMemoryLog log;
        verify(pool.BuildFile(file_proto) == NULL);
        errors = log.GetMessages(google::protobuf::ERROR);
      }

      verify(2 == errors.size());

      verify("Invalid proto descriptor for file \"foo.proto\":" == errors[0]);
      verify("  Foo: \"Foo\" is already defined." == errors[1]);
    }

    // ValidationErrorTest, DisallowEnumAlias
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFileWithErrors(
        "name: \"foo.proto\" "
        "enum_type {"
        "  name: \"Bar\""
        "  value { name:\"ENUM_A\" number:0 }"
        "  value { name:\"ENUM_B\" number:0 }"
        "}",
        "foo.proto: Bar: NUMBER: "
        "\"ENUM_B\" uses the same enum value as \"ENUM_A\". "
        "If this is intended, fun::set 'option allow_alias = true;' to the enum "
        "definition.\n");
    }

    // ValidationErrorTest, AllowEnumAlias
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      BuildFile(
        "name: \"foo.proto\" "
        "enum_type {"
        "  name: \"Bar\""
        "  value { name:\"ENUM_A\" number:0 }"
        "  value { name:\"ENUM_B\" number:0 }"
        "  options { allow_alias: true }"
        "}");
    }

    // ValidationErrorTest, UnusedImportWarning
    {
      google::protobuf::DescriptorPool pool;
      pool_ = &pool;

      pool.AddUnusedImportTrackFile("bar.proto");
      BuildFile(
        "name: \"bar.proto\" "
        "message_type { name: \"Bar\" }");

      pool.AddUnusedImportTrackFile("base.proto");
      BuildFile(
        "name: \"base.proto\" "
        "message_type { name: \"Base\" }");

      pool.AddUnusedImportTrackFile("baz.proto");
      BuildFile(
        "name: \"baz.proto\" "
        "message_type { name: \"Baz\" }");

      pool.AddUnusedImportTrackFile("public.proto");
      BuildFile(
        "name: \"public.proto\" "
        "dependency: \"bar.proto\""
        "public_dependency: 0");

      // // forward.proto
      // import "base.proto"       // No warning: Base message is used.
      // import "bar.proto"        // Will log a warning.
      // import public "baz.proto" // No warning: Do not track import public.
      // import "public.proto"     // No warning: public.proto has import public.
      // message Forward {
      //   optional Base base = 1;
      // }
      //
      pool.AddUnusedImportTrackFile("forward.proto");
      BuildFile(
        "name: \"forward.proto\""
        "dependency: \"base.proto\""
        "dependency: \"bar.proto\""
        "dependency: \"baz.proto\""
        "dependency: \"public.proto\""
        "public_dependency: 2 "
        "message_type {"
        "  name: \"Forward\""
        "  field { name:\"base\" number:1 label:LABEL_OPTIONAL type_name:\"Base\" }"
        "}");
    }
  }
  // // // //

  // ===================================================================
  // DescriptorDatabase

  auto AddToDatabase = [](google::protobuf::SimpleDescriptorDatabase* database,
    const char* file_text)
  {
    google::protobuf::FileDescriptorProto file_proto;
    verify(google::protobuf::TextFormat::ParseFromString(file_text, &file_proto));
    database->Add(file_proto);
  };

  // // // //
  // DatabaseBackedPoolTest
  {
    google::protobuf::SimpleDescriptorDatabase database_;

    // SetUp()
    {
      AddToDatabase(&database_,
        "name: 'foo.proto' "
        "message_type { name:'Foo' extension_range { start: 1 end: 100 } } "
        "enum_type { name:'TestEnum' value { name:'DUMMY' number:0 } } "
        "service { name:'TestService' } ");
      AddToDatabase(&database_,
        "name: 'bar.proto' "
        "dependency: 'foo.proto' "
        "message_type { name:'Bar' } "
        "extension { name:'foo_ext' extendee: '.Foo' number:5 "
        "            label:LABEL_OPTIONAL type:TYPE_INT32 } ");
      // Baz has an undeclared dependency on Foo.
      AddToDatabase(&database_,
        "name: 'baz.proto' "
        "message_type { "
        "  name:'Baz' "
        "  field { name:'foo' number:1 label:LABEL_OPTIONAL type_name:'Foo' } "
        "}");
    }

    // We can't inject a file containing errors into a DescriptorPool, so we
    // need an actual mock DescriptorDatabase to test errors.
    class ErrorDescriptorDatabase : public google::protobuf::DescriptorDatabase {
    public:
      ErrorDescriptorDatabase() {}
      ~ErrorDescriptorDatabase() {}

      // implements DescriptorDatabase ---------------------------------
      bool FindFileByName(const fun::string& filename,
        google::protobuf::FileDescriptorProto* output) {
        // error.proto and error2.proto cyclically import each other.
        if (filename == "error.proto") {
          output->Clear();
          output->set_name("error.proto");
          output->add_dependency("error2.proto");
          return true;
        }
        else if (filename == "error2.proto") {
          output->Clear();
          output->set_name("error2.proto");
          output->add_dependency("error.proto");
          return true;
        }
        else {
          return false;
        }
      }
      bool FindFileContainingSymbol(const fun::string& symbol_name,
        google::protobuf::FileDescriptorProto* output) {
        return false;
      }
      bool FindFileContainingExtension(const fun::string& containing_type,
        int field_number,
        google::protobuf::FileDescriptorProto* output) {
        return false;
      }
    };

    // A DescriptorDatabase that counts how many times each method has been
    // called and forwards to some other DescriptorDatabase.
    class CallCountingDatabase : public google::protobuf::DescriptorDatabase {
    public:
      CallCountingDatabase(google::protobuf::DescriptorDatabase* wrapped_db)
        : wrapped_db_(wrapped_db) {
        Clear();
      }
      ~CallCountingDatabase() {}

      google::protobuf::DescriptorDatabase* wrapped_db_;

      int call_count_;

      void Clear() {
        call_count_ = 0;
      }

      // implements DescriptorDatabase ---------------------------------
      bool FindFileByName(const fun::string& filename,
        google::protobuf::FileDescriptorProto* output) {
        ++call_count_;
        return wrapped_db_->FindFileByName(filename, output);
      }
      bool FindFileContainingSymbol(const fun::string& symbol_name,
        google::protobuf::FileDescriptorProto* output) {
        ++call_count_;
        return wrapped_db_->FindFileContainingSymbol(symbol_name, output);
      }
      bool FindFileContainingExtension(const fun::string& containing_type,
        int field_number,
        google::protobuf::FileDescriptorProto* output) {
        ++call_count_;
        return wrapped_db_->FindFileContainingExtension(
          containing_type, field_number, output);
      }
    };

    // A DescriptorDatabase which falsely always returns foo.proto when searching
    // for any symbol or extension number.  This shouldn't cause the
    // DescriptorPool to reload foo.proto if it is already loaded.
    class FalsePositiveDatabase : public google::protobuf::DescriptorDatabase {
    public:
      FalsePositiveDatabase(google::protobuf::DescriptorDatabase* wrapped_db)
        : wrapped_db_(wrapped_db) {}
      ~FalsePositiveDatabase() {}

      google::protobuf::DescriptorDatabase* wrapped_db_;

      // implements DescriptorDatabase ---------------------------------
      bool FindFileByName(const fun::string& filename,
        google::protobuf::FileDescriptorProto* output) {
        return wrapped_db_->FindFileByName(filename, output);
      }
      bool FindFileContainingSymbol(const fun::string& symbol_name,
        google::protobuf::FileDescriptorProto* output) {
        return FindFileByName("foo.proto", output);
      }
      bool FindFileContainingExtension(const fun::string& containing_type,
        int field_number,
        google::protobuf::FileDescriptorProto* output) {
        return FindFileByName("foo.proto", output);
      }
    };

    // DatabaseBackedPoolTest, FindFileByName
    {
      google::protobuf::DescriptorPool pool(&database_);

      const google::protobuf::FileDescriptor* foo = pool.FindFileByName("foo.proto");
      verify(foo != NULL);
      verify("foo.proto" == foo->name());
      verify(1 == foo->message_type_count());
      verify("Foo" == foo->message_type(0)->name());

      verify(foo == pool.FindFileByName("foo.proto"));

      verify(pool.FindFileByName("no_such_file.proto") == NULL);
    }

    // DatabaseBackedPoolTest, FindDependencyBeforeDependent
    {
      google::protobuf::DescriptorPool pool(&database_);

      const google::protobuf::FileDescriptor* foo = pool.FindFileByName("foo.proto");
      verify(foo != NULL);
      verify("foo.proto" == foo->name());
      verify(1 == foo->message_type_count());
      verify("Foo" == foo->message_type(0)->name());

      const google::protobuf::FileDescriptor* bar = pool.FindFileByName("bar.proto");
      verify(bar != NULL);
      verify("bar.proto" == bar->name());
      verify(1 == bar->message_type_count());
      verify("Bar" == bar->message_type(0)->name());

      verify(1 == bar->dependency_count());
      verify(foo == bar->dependency(0));
    }

    // DatabaseBackedPoolTest, FindDependentBeforeDependency
    {
      google::protobuf::DescriptorPool pool(&database_);

      const google::protobuf::FileDescriptor* bar = pool.FindFileByName("bar.proto");
      verify(bar != NULL);
      verify("bar.proto" == bar->name());
      verify(1 == bar->message_type_count());
      verify("Bar" == bar->message_type(0)->name());

      const google::protobuf::FileDescriptor* foo = pool.FindFileByName("foo.proto");
      verify(foo != NULL);
      verify("foo.proto" == foo->name());
      verify(1 == foo->message_type_count());
      verify("Foo" == foo->message_type(0)->name());

      verify(1 == bar->dependency_count());
      verify(foo == bar->dependency(0));
    }

    // DatabaseBackedPoolTest, FindFileContainingSymbol
    {
      google::protobuf::DescriptorPool pool(&database_);

      const google::protobuf::FileDescriptor* file = pool.FindFileContainingSymbol("Foo");
      verify(file != NULL);
      verify("foo.proto" == file->name());
      verify(file == pool.FindFileByName("foo.proto"));

      verify(pool.FindFileContainingSymbol("NoSuchSymbol") == NULL);
    }

    // DatabaseBackedPoolTest, FindMessageTypeByName
    {
      google::protobuf::DescriptorPool pool(&database_);

      const google::protobuf::Descriptor* type = pool.FindMessageTypeByName("Foo");
      verify(type != NULL);
      verify("Foo" == type->name());
      verify(type->file() == pool.FindFileByName("foo.proto"));

      verify(pool.FindMessageTypeByName("NoSuchType") == NULL);
    }

    // DatabaseBackedPoolTest, FindExtensionByNumber
    {
      google::protobuf::DescriptorPool pool(&database_);

      const google::protobuf::Descriptor* foo = pool.FindMessageTypeByName("Foo");
      verify(foo != NULL);

      const google::protobuf::FieldDescriptor* extension = pool.FindExtensionByNumber(foo, 5);
      verify(extension != NULL);
      verify("foo_ext" == extension->name());
      verify(extension->file() == pool.FindFileByName("bar.proto"));

      verify(pool.FindExtensionByNumber(foo, 12) == NULL);
    }

    // DatabaseBackedPoolTest, FindAllExtensions
    {
      google::protobuf::DescriptorPool pool(&database_);

      const google::protobuf::Descriptor* foo = pool.FindMessageTypeByName("Foo");

      for (int i = 0; i < 2; ++i) {
        // Repeat the lookup twice, to check that we get consistent
        // results despite the fallback database lookup mutating the pool.
        fun::vector<const google::protobuf::FieldDescriptor*> extensions;
        pool.FindAllExtensions(foo, &extensions);
        verify(1 == extensions.size());
        verify(5 == extensions[0]->number());
      }
    }

    // DatabaseBackedPoolTest, ErrorWithoutErrorCollector
    {
      ErrorDescriptorDatabase error_database;
      google::protobuf::DescriptorPool pool(&error_database);

      fun::vector<fun::string> errors;

      {
        google::protobuf::ScopedMemoryLog log;
        verify(pool.FindFileByName("error.proto") == NULL);
        errors = log.GetMessages(google::protobuf::ERROR);
      }

      verify(false == errors.empty());
    }

    // DatabaseBackedPoolTest, ErrorWithErrorCollector
    {
      ErrorDescriptorDatabase error_database;
      MockErrorCollector error_collector;
      google::protobuf::DescriptorPool pool(&error_database, &error_collector);

      verify(pool.FindFileByName("error.proto") == NULL);
      verify(
        "error.proto: error.proto: OTHER: File recursively imports itself: "
        "error.proto -> error2.proto -> error.proto\n"
        "error2.proto: error2.proto: OTHER: Import \"error.proto\" was not "
        "found or had errors.\n"
        "error.proto: error.proto: OTHER: Import \"error2.proto\" was not "
        "found or had errors.\n" ==
        error_collector.text_);
    }

    // DatabaseBackedPoolTest, UndeclaredDependencyOnUnbuiltType
    {
      // Check that we find and report undeclared dependencies on types that exist
      // in the descriptor database but that have not not been built yet.
      MockErrorCollector error_collector;
      google::protobuf::DescriptorPool pool(&database_, &error_collector);
      verify(pool.FindMessageTypeByName("Baz") == NULL);
      verify(
        "baz.proto: Baz.foo: TYPE: \"Foo\" seems to be defined in \"foo.proto\", "
        "which is not imported by \"baz.proto\".  To use it here, please add "
        "the necessary import.\n" ==
        error_collector.text_);
    }

    // DatabaseBackedPoolTest, RollbackAfterError
    {
      // Make sure that all traces of bad types are removed from the pool. This used
      // to be b/4529436, due to the fact that a symbol resolution failure could
      // potentially cause another file to be recursively built, which would trigger
      // a checkpoint _past_ possibly invalid symbols.
      // Baz is defined in the database, but the file is invalid because it is
      // missing a necessary import.
      google::protobuf::DescriptorPool pool(&database_);
      verify(pool.FindMessageTypeByName("Baz") == NULL);
      // Make sure that searching again for the file or the type fails.
      verify(pool.FindFileByName("baz.proto") == NULL);
      verify(pool.FindMessageTypeByName("Baz") == NULL);
    }

    // DatabaseBackedPoolTest, UnittestProto
    {
      // Try to load all of unittest.proto from a DescriptorDatabase.  This should
      // thoroughly test all paths through DescriptorBuilder to insure that there
      // are no deadlocking problems when pool_->mutex_ is non-NULL.
      const  google::protobuf::FileDescriptor* original_file =
        protobuf_unittest::TestAllTypes::descriptor()->file();

      google::protobuf::DescriptorPoolDatabase database(*google::protobuf::DescriptorPool::generated_pool());
      google::protobuf::DescriptorPool pool(&database);
      const  google::protobuf::FileDescriptor* file_from_database =
        pool.FindFileByName(original_file->name());

      verify(file_from_database != NULL);

      google::protobuf::FileDescriptorProto original_file_proto;
      original_file->CopyTo(&original_file_proto);

      google::protobuf::FileDescriptorProto file_from_database_proto;
      file_from_database->CopyTo(&file_from_database_proto);

      verify(original_file_proto.DebugString() ==
        file_from_database_proto.DebugString());

      // Also verify that CopyTo() did not omit any information.
      verify(original_file->DebugString() ==
        file_from_database->DebugString());
    }

    // DatabaseBackedPoolTest, DoesntRetryDbUnnecessarily
    {
      // Searching for a child of an existing descriptor should never fall back
      // to the DescriptorDatabase even if it isn't found, because we know all
      // children are already loaded.
      CallCountingDatabase call_counter(&database_);
      google::protobuf::DescriptorPool pool(&call_counter);

      const google::protobuf::FileDescriptor* file = pool.FindFileByName("foo.proto");
      verify(file != NULL);
      const google::protobuf::Descriptor* foo = pool.FindMessageTypeByName("Foo");
      verify(foo != NULL);
      const google::protobuf::EnumDescriptor* test_enum = pool.FindEnumTypeByName("TestEnum");
      verify(test_enum != NULL);
      const google::protobuf::ServiceDescriptor* test_service = pool.FindServiceByName("TestService");
      verify(test_service != NULL);

      verify(0 != call_counter.call_count_);
      call_counter.Clear();

      verify(foo->FindFieldByName("no_such_field") == NULL);
      verify(foo->FindExtensionByName("no_such_extension") == NULL);
      verify(foo->FindNestedTypeByName("NoSuchMessageType") == NULL);
      verify(foo->FindEnumTypeByName("NoSuchEnumType") == NULL);
      verify(foo->FindEnumValueByName("NO_SUCH_VALUE") == NULL);
      verify(test_enum->FindValueByName("NO_SUCH_VALUE") == NULL);
      verify(test_service->FindMethodByName("NoSuchMethod") == NULL);

      verify(file->FindMessageTypeByName("NoSuchMessageType") == NULL);
      verify(file->FindEnumTypeByName("NoSuchEnumType") == NULL);
      verify(file->FindEnumValueByName("NO_SUCH_VALUE") == NULL);
      verify(file->FindServiceByName("NO_SUCH_VALUE") == NULL);
      verify(file->FindExtensionByName("no_such_extension") == NULL);

      verify(pool.FindFileContainingSymbol("Foo.no.such.field") == NULL);
      verify(pool.FindFileContainingSymbol("Foo.no_such_field") == NULL);
      verify(pool.FindMessageTypeByName("Foo.NoSuchMessageType") == NULL);
      verify(pool.FindFieldByName("Foo.no_such_field") == NULL);
      verify(pool.FindExtensionByName("Foo.no_such_extension") == NULL);
      verify(pool.FindEnumTypeByName("Foo.NoSuchEnumType") == NULL);
      verify(pool.FindEnumValueByName("Foo.NO_SUCH_VALUE") == NULL);
      verify(pool.FindMethodByName("TestService.NoSuchMethod") == NULL);

      verify(0 == call_counter.call_count_);
    }

    // DatabaseBackedPoolTest, DoesntReloadFilesUncesessarily
    {
      // If FindFileContainingSymbol() or FindFileContainingExtension() return a
      // file that is already in the DescriptorPool, it should not attempt to
      // reload the file.
      FalsePositiveDatabase false_positive_database(&database_);
      MockErrorCollector error_collector;
      google::protobuf::DescriptorPool pool(&false_positive_database, &error_collector);

      // First make sure foo.proto is loaded.
      const google::protobuf::Descriptor* foo = pool.FindMessageTypeByName("Foo");
      verify(foo != NULL);

      // Try inducing false positives.
      verify(pool.FindMessageTypeByName("NoSuchSymbol") == NULL);
      verify(pool.FindExtensionByNumber(foo, 22) == NULL);

      // No errors should have been reported.  (If foo.proto was incorrectly
      // loaded multiple times, errors would have been reported.)
      verify("" == error_collector.text_);
    }

    // DescriptorDatabase that attempts to induce exponentially-bad performance
    // in DescriptorPool. For every positive N, the database contains a file
    // fileN.proto, which defines a message MessageN, which contains fields of
    // type MessageK for all K in [0,N). Message0 is not defined anywhere
    // (file0.proto exists, but is empty), so every other file and message type
    // will fail to build.
    //
    // If the DescriptorPool is not careful to memoize errors, an attempt to
    // build a descriptor for MessageN can require O(2^N) time.
    class ExponentialErrorDatabase : public google::protobuf::DescriptorDatabase {
    public:
      ExponentialErrorDatabase() {}
      ~ExponentialErrorDatabase() {}

      // implements DescriptorDatabase ---------------------------------
      bool FindFileByName(const fun::string& filename,
        google::protobuf::FileDescriptorProto* output) {
        int file_num = -1;
        FullMatch(filename, "file", ".proto", &file_num);
        if (file_num > -1) {
          return PopulateFile(file_num, output);
        }
        else {
          return false;
        }
      }
      bool FindFileContainingSymbol(const fun::string& symbol_name,
        google::protobuf::FileDescriptorProto* output) {
        int file_num = -1;
        FullMatch(symbol_name, "Message", "", &file_num);
        if (file_num > 0) {
          return PopulateFile(file_num, output);
        }
        else {
          return false;
        }
      }
      bool FindFileContainingExtension(const fun::string& containing_type,
        int field_number,
        google::protobuf::FileDescriptorProto* output) {
        return false;
      }

    private:
      void FullMatch(const fun::string& name,
        const fun::string& begin_with,
        const fun::string& end_with,
        int* file_num) {
        int begin_size = begin_with.size();
        int end_size = end_with.size();
        if (name.substr(0, begin_size) != begin_with ||
          name.substr(name.size() - end_size, end_size) != end_with) {
          return;
        }
        google::protobuf::safe_strto32(name.substr(begin_size, name.size() - end_size - begin_size),
          file_num);
      }

      bool PopulateFile(int file_num, google::protobuf::FileDescriptorProto* output) {
        using google::protobuf::strings::Substitute;
        GOOGLE_CHECK_GE(file_num, 0);
        output->Clear();
        output->set_name(Substitute("file$0.proto", file_num));
        // file0.proto doesn't define Message0
        if (file_num > 0) {
          google::protobuf::DescriptorProto* message = output->add_message_type();
          message->set_name(Substitute("Message$0", file_num));
          for (int i = 0; i < file_num; ++i) {
            output->add_dependency(Substitute("file$0.proto", i));
            google::protobuf::FieldDescriptorProto* field = message->add_field();
            field->set_name(Substitute("field$0", i));
            field->set_number(i);
            field->set_label(google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL);
            field->set_type(google::protobuf::FieldDescriptorProto::TYPE_MESSAGE);
            field->set_type_name(Substitute("Message$0", i));
          }
        }
        return true;
      }
    };

    // DatabaseBackedPoolTest, DoesntReloadKnownBadFiles
    {
      ExponentialErrorDatabase error_database;
      google::protobuf::DescriptorPool pool(&error_database);

      GOOGLE_LOG(INFO) << "A timeout in this test probably indicates a real bug.";

      verify(pool.FindFileByName("file40.proto") == NULL);
      verify(pool.FindMessageTypeByName("Message40") == NULL);
    }

    // DatabaseBackedPoolTest, DoesntFallbackOnWrongType
    {
      // If a lookup finds a symbol of the wrong type (e.g. we pass a type name
      // to FindFieldByName()), we should fail fast, without checking the fallback
      // database.
      CallCountingDatabase call_counter(&database_);
      google::protobuf::DescriptorPool pool(&call_counter);

      const google::protobuf::FileDescriptor* file = pool.FindFileByName("foo.proto");
      verify(file != NULL);
      const google::protobuf::Descriptor* foo = pool.FindMessageTypeByName("Foo");
      verify(foo != NULL);
      const google::protobuf::EnumDescriptor* test_enum = pool.FindEnumTypeByName("TestEnum");
      verify(test_enum != NULL);

      verify(0 != call_counter.call_count_);
      call_counter.Clear();

      verify(pool.FindMessageTypeByName("TestEnum") == NULL);
      verify(pool.FindFieldByName("Foo") == NULL);
      verify(pool.FindExtensionByName("Foo") == NULL);
      verify(pool.FindEnumTypeByName("Foo") == NULL);
      verify(pool.FindEnumValueByName("Foo") == NULL);
      verify(pool.FindServiceByName("Foo") == NULL);
      verify(pool.FindMethodByName("Foo") == NULL);

      verify(0 == call_counter.call_count_);
    }
  }
  // // // //

  return true;
}

#endif