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


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFunapiLibProtobufGeneratedMessageReflectionTest, "LibProtobuf.GeneratedMessageReflectionTest", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFunapiLibProtobufGeneratedMessageReflectionTest::RunTest(const FString& Parameters)
{
  // Shorthand to get a FieldDescriptor for a field of unittest::TestAllTypes.
  auto F = [](const std::string& name) -> const google::protobuf::FieldDescriptor* {
    const google::protobuf::FieldDescriptor* result =
    google::protobuf::unittest::TestAllTypes::descriptor()->FindFieldByName(name);
    GOOGLE_CHECK(result != NULL);
    return result;
  };

  // GeneratedMessageReflectionTest, Defaults
  {
    // Check that all default values are set correctly in the initial message.
    google::protobuf::unittest::TestAllTypes message;
    google::protobuf::TestUtil::ReflectionTester reflection_tester(
      google::protobuf::unittest::TestAllTypes::descriptor());

    reflection_tester.ExpectClearViaReflection(message);

    const google::protobuf::Reflection* reflection = message.GetReflection();

    // Messages should return pointers to default instances until first use.
    // (This is not checked by ExpectClear() since it is not actually true after
    // the fields have been set and then cleared.)
    verify(&google::protobuf::unittest::TestAllTypes::OptionalGroup::default_instance() ==
      &reflection->GetMessage(message, F("optionalgroup")));
    verify(&google::protobuf::unittest::TestAllTypes::NestedMessage::default_instance() ==
      &reflection->GetMessage(message, F("optional_nested_message")));
    verify(&google::protobuf::unittest::ForeignMessage::default_instance() ==
      &reflection->GetMessage(message, F("optional_foreign_message")));
    verify(&google::protobuf::unittest_import::ImportMessage::default_instance() ==
      &reflection->GetMessage(message, F("optional_import_message")));
  }

  // GeneratedMessageReflectionTest, Accessors
  {
    // Set every field to a unique value then go back and check all those
    // values.
    google::protobuf::unittest::TestAllTypes message;
    google::protobuf::TestUtil::ReflectionTester reflection_tester(
      google::protobuf::unittest::TestAllTypes::descriptor());

    reflection_tester.SetAllFieldsViaReflection(&message);
    google::protobuf::TestUtil::ExpectAllFieldsSet(message);
    reflection_tester.ExpectAllFieldsSetViaReflection(message);

    reflection_tester.ModifyRepeatedFieldsViaReflection(&message);
    google::protobuf::TestUtil::ExpectRepeatedFieldsModified(message);
  }

  // GeneratedMessageReflectionTest, GetStringReference
  {
    // Test that GetStringReference() returns the underlying string when it is
    // a normal string field.
    google::protobuf::unittest::TestAllTypes message;
    message.set_optional_string("foo");
    message.add_repeated_string("foo");

    const google::protobuf::Reflection* reflection = message.GetReflection();
    std::string scratch;

    verify(&message.optional_string() ==
      &reflection->GetStringReference(message, F("optional_string"), &scratch))
      // << "For simple string fields, GetStringReference() should return a "
      // "reference to the underlying string.";
    verify(&message.repeated_string(0) ==
      &reflection->GetRepeatedStringReference(message, F("repeated_string"),
        0, &scratch))
      // << "For simple string fields, GetRepeatedStringReference() should return "
      //"a reference to the underlying string.";
  }


  // GeneratedMessageReflectionTest, DefaultsAfterClear
  {
    // Check that after setting all fields and then clearing, getting an
    // embedded message does NOT return the default instance.
    google::protobuf::unittest::TestAllTypes message;
    google::protobuf::TestUtil::ReflectionTester reflection_tester(
      google::protobuf::unittest::TestAllTypes::descriptor());

    google::protobuf::TestUtil::SetAllFields(&message);
    message.Clear();

    const google::protobuf::Reflection* reflection = message.GetReflection();

    verify(&google::protobuf::unittest::TestAllTypes::OptionalGroup::default_instance() !=
      &reflection->GetMessage(message, F("optionalgroup")));
    verify(&google::protobuf::unittest::TestAllTypes::NestedMessage::default_instance() !=
      &reflection->GetMessage(message, F("optional_nested_message")));
    verify(&google::protobuf::unittest::ForeignMessage::default_instance() !=
      &reflection->GetMessage(message, F("optional_foreign_message")));
    verify(&google::protobuf::unittest_import::ImportMessage::default_instance() !=
      &reflection->GetMessage(message, F("optional_import_message")));
  }


  // GeneratedMessageReflectionTest, Swap
  {
    google::protobuf::unittest::TestAllTypes message1;
    google::protobuf::unittest::TestAllTypes message2;

    google::protobuf::TestUtil::SetAllFields(&message1);

    const google::protobuf::Reflection* reflection = message1.GetReflection();
    reflection->Swap(&message1, &message2);

    google::protobuf::TestUtil::ExpectClear(message1);
    google::protobuf::TestUtil::ExpectAllFieldsSet(message2);
  }

  // GeneratedMessageReflectionTest, SwapWithBothSet
  {
    google::protobuf::unittest::TestAllTypes message1;
    google::protobuf::unittest::TestAllTypes message2;

    google::protobuf::TestUtil::SetAllFields(&message1);
    google::protobuf::TestUtil::SetAllFields(&message2);
    google::protobuf::TestUtil::ModifyRepeatedFields(&message2);

    const google::protobuf::Reflection* reflection = message1.GetReflection();
    reflection->Swap(&message1, &message2);

    google::protobuf::TestUtil::ExpectRepeatedFieldsModified(message1);
    google::protobuf::TestUtil::ExpectAllFieldsSet(message2);

    message1.set_optional_int32(532819);

    reflection->Swap(&message1, &message2);

    verify(532819 == message2.optional_int32());
  }

  // GeneratedMessageReflectionTest, SwapExtensions
  {
    google::protobuf::unittest::TestAllExtensions message1;
    google::protobuf::unittest::TestAllExtensions message2;

    google::protobuf::TestUtil::SetAllExtensions(&message1);

    const google::protobuf::Reflection* reflection = message1.GetReflection();
    reflection->Swap(&message1, &message2);

    google::protobuf::TestUtil::ExpectExtensionsClear(message1);
    google::protobuf::TestUtil::ExpectAllExtensionsSet(message2);
  }

  // GeneratedMessageReflectionTest, SwapUnknown
  {
    google::protobuf::unittest::TestEmptyMessage message1, message2;

    message1.mutable_unknown_fields()->AddVarint(1234, 1);

    verify(1 == message1.unknown_fields().field_count());
    verify(0 == message2.unknown_fields().field_count());
    const  google::protobuf::Reflection* reflection = message1.GetReflection();
    reflection->Swap(&message1, &message2);
    verify(0 == message1.unknown_fields().field_count());
    verify(1 == message2.unknown_fields().field_count());
  }

  // GeneratedMessageReflectionTest, SwapFields
  {
    google::protobuf::unittest::TestAllTypes message1, message2;
    message1.set_optional_double(12.3);
    message1.mutable_repeated_int32()->Add(10);
    message1.mutable_repeated_int32()->Add(20);

    message2.set_optional_string("hello");
    message2.mutable_repeated_int64()->Add(30);

    std::vector<const google::protobuf::FieldDescriptor*> fields;
    const google::protobuf::Descriptor* descriptor = message1.GetDescriptor();
    fields.push_back(descriptor->FindFieldByName("optional_double"));
    fields.push_back(descriptor->FindFieldByName("repeated_int32"));
    fields.push_back(descriptor->FindFieldByName("optional_string"));
    fields.push_back(descriptor->FindFieldByName("optional_uint64"));

    const google::protobuf::Reflection* reflection = message1.GetReflection();
    reflection->SwapFields(&message1, &message2, fields);

    verify(false == message1.has_optional_double());
    verify(0 == message1.repeated_int32_size());
    verify(message1.has_optional_string());
    verify("hello" == message1.optional_string());
    verify(0 == message1.repeated_int64_size());
    verify(false == message1.has_optional_uint64());

    verify(message2.has_optional_double());
    verify(12.3 == message2.optional_double());
    verify(2 == message2.repeated_int32_size());
    verify(10 == message2.repeated_int32(0));
    verify(20 == message2.repeated_int32(1));
    verify(false == message2.has_optional_string());
    verify(1 == message2.repeated_int64_size());
    verify(false == message2.has_optional_uint64());
  }

  // GeneratedMessageReflectionTest, SwapFieldsAll
  {
    google::protobuf::unittest::TestAllTypes message1;
    google::protobuf::unittest::TestAllTypes message2;

    google::protobuf::TestUtil::SetAllFields(&message2);

    std::vector<const google::protobuf::FieldDescriptor*> fields;
    const google::protobuf::Reflection* reflection = message1.GetReflection();
    reflection->ListFields(message2, &fields);
    reflection->SwapFields(&message1, &message2, fields);

    google::protobuf::TestUtil::ExpectAllFieldsSet(message1);
    google::protobuf::TestUtil::ExpectClear(message2);
  }

  // GeneratedMessageReflectionTest, SwapFieldsAllExtension
  {
    google::protobuf::unittest::TestAllExtensions message1;
    google::protobuf::unittest::TestAllExtensions message2;

    google::protobuf::TestUtil::SetAllExtensions(&message1);

    std::vector<const google::protobuf::FieldDescriptor*> fields;
    const google::protobuf::Reflection* reflection = message1.GetReflection();
    reflection->ListFields(message1, &fields);
    reflection->SwapFields(&message1, &message2, fields);

    google::protobuf::TestUtil::ExpectExtensionsClear(message1);
    google::protobuf::TestUtil::ExpectAllExtensionsSet(message2);
  }

  // GeneratedMessageReflectionTest, SwapOneof
  {
    google::protobuf::unittest::TestOneof2 message1, message2;
    google::protobuf::TestUtil::SetOneof1(&message1);

    const google::protobuf::Reflection* reflection = message1.GetReflection();
    reflection->Swap(&message1, &message2);

    google::protobuf::TestUtil::ExpectOneofClear(message1);
    google::protobuf::TestUtil::ExpectOneofSet1(message2);
  }

  // GeneratedMessageReflectionTest, SwapOneofBothSet
  {
    google::protobuf::unittest::TestOneof2 message1, message2;
    google::protobuf::TestUtil::SetOneof1(&message1);
    google::protobuf::TestUtil::SetOneof2(&message2);

    const google::protobuf::Reflection* reflection = message1.GetReflection();
    reflection->Swap(&message1, &message2);

    google::protobuf::TestUtil::ExpectOneofSet2(message1);
    google::protobuf::TestUtil::ExpectOneofSet1(message2);
  }

  // GeneratedMessageReflectionTest, SwapFieldsOneof
  {
    google::protobuf::unittest::TestOneof2 message1, message2;
    google::protobuf::TestUtil::SetOneof1(&message1);

    std::vector<const google::protobuf::FieldDescriptor*> fields;
    const google::protobuf::Descriptor* descriptor = message1.GetDescriptor();
    for (int i = 0; i < descriptor->field_count(); i++) {
      fields.push_back(descriptor->field(i));
    }
    const google::protobuf::Reflection* reflection = message1.GetReflection();
    reflection->SwapFields(&message1, &message2, fields);

    google::protobuf::TestUtil::ExpectOneofClear(message1);
    google::protobuf::TestUtil::ExpectOneofSet1(message2);
  }

  // GeneratedMessageReflectionTest, RemoveLast
  {
    google::protobuf::unittest::TestAllTypes message;
    google::protobuf::TestUtil::ReflectionTester reflection_tester(
      google::protobuf::unittest::TestAllTypes::descriptor());

    google::protobuf::TestUtil::SetAllFields(&message);

    reflection_tester.RemoveLastRepeatedsViaReflection(&message);

    google::protobuf::TestUtil::ExpectLastRepeatedsRemoved(message);
  }

  // GeneratedMessageReflectionTest, RemoveLastExtensions
  {
    google::protobuf::unittest::TestAllExtensions message;
    google::protobuf::TestUtil::ReflectionTester reflection_tester(
      google::protobuf::unittest::TestAllExtensions::descriptor());

    google::protobuf::TestUtil::SetAllExtensions(&message);

    reflection_tester.RemoveLastRepeatedsViaReflection(&message);

    google::protobuf::TestUtil::ExpectLastRepeatedExtensionsRemoved(message);
  }

  // TEST(GeneratedMessageReflectionTest, ReleaseLast)
  {
    google::protobuf::unittest::TestAllTypes message;
    const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
    google::protobuf::TestUtil::ReflectionTester reflection_tester(descriptor);

    google::protobuf::TestUtil::SetAllFields(&message);

    reflection_tester.ReleaseLastRepeatedsViaReflection(&message, false);

    google::protobuf::TestUtil::ExpectLastRepeatedsReleased(message);

    // Now test that we actually release the right message.
    message.Clear();
    google::protobuf::TestUtil::SetAllFields(&message);
    verify(2 == message.repeated_foreign_message_size());
    const protobuf_unittest::ForeignMessage* expected =
      message.mutable_repeated_foreign_message(1);
    google::protobuf::scoped_ptr<google::protobuf::Message> released(message.GetReflection()->ReleaseLast(
      &message, descriptor->FindFieldByName("repeated_foreign_message")));
    verify(expected == released.get());
  }

  // TEST(GeneratedMessageReflectionTest, ReleaseLastExtensions)
  {
    google::protobuf::unittest::TestAllExtensions message;
    const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
    google::protobuf::TestUtil::ReflectionTester reflection_tester(descriptor);

    google::protobuf::TestUtil::SetAllExtensions(&message);

    reflection_tester.ReleaseLastRepeatedsViaReflection(&message, true);

    google::protobuf::TestUtil::ExpectLastRepeatedExtensionsReleased(message);

    // Now test that we actually release the right message.
    message.Clear();
    google::protobuf::TestUtil::SetAllExtensions(&message);
    verify(2 == message.ExtensionSize(
      google::protobuf::unittest::repeated_foreign_message_extension));
    const protobuf_unittest::ForeignMessage* expected = message.MutableExtension(
      google::protobuf::unittest::repeated_foreign_message_extension, 1);
    google::protobuf::scoped_ptr<google::protobuf::Message> released(message.GetReflection()->ReleaseLast(
      &message, descriptor->file()->FindExtensionByName(
        "repeated_foreign_message_extension")));
    verify(expected == released.get());

  }

  // TEST(GeneratedMessageReflectionTest, SwapRepeatedElements)
  {
    google::protobuf::unittest::TestAllTypes message;
    google::protobuf::TestUtil::ReflectionTester reflection_tester(
      google::protobuf::unittest::TestAllTypes::descriptor());

    google::protobuf::TestUtil::SetAllFields(&message);

    // Swap and test that fields are all swapped.
    reflection_tester.SwapRepeatedsViaReflection(&message);
    google::protobuf::TestUtil::ExpectRepeatedsSwapped(message);

    // Swap back and test that fields are all back to original values.
    reflection_tester.SwapRepeatedsViaReflection(&message);
    google::protobuf::TestUtil::ExpectAllFieldsSet(message);
  }

  // TEST(GeneratedMessageReflectionTest, SwapRepeatedElementsExtension)
  {
    google::protobuf::unittest::TestAllExtensions message;
    google::protobuf::TestUtil::ReflectionTester reflection_tester(
      google::protobuf::unittest::TestAllExtensions::descriptor());

    google::protobuf::TestUtil::SetAllExtensions(&message);

    // Swap and test that fields are all swapped.
    reflection_tester.SwapRepeatedsViaReflection(&message);
    google::protobuf::TestUtil::ExpectRepeatedExtensionsSwapped(message);

    // Swap back and test that fields are all back to original values.
    reflection_tester.SwapRepeatedsViaReflection(&message);
    google::protobuf::TestUtil::ExpectAllExtensionsSet(message);
  }

  // TEST(GeneratedMessageReflectionTest, Extensions)
  {
    // Set every extension to a unique value then go back and check all those
    // values.
    google::protobuf::unittest::TestAllExtensions message;
    google::protobuf::TestUtil::ReflectionTester reflection_tester(
      google::protobuf::unittest::TestAllExtensions::descriptor());

    reflection_tester.SetAllFieldsViaReflection(&message);
    google::protobuf::TestUtil::ExpectAllExtensionsSet(message);
    reflection_tester.ExpectAllFieldsSetViaReflection(message);

    reflection_tester.ModifyRepeatedFieldsViaReflection(&message);
    google::protobuf::TestUtil::ExpectRepeatedExtensionsModified(message);
  }

  // TEST(GeneratedMessageReflectionTest, FindExtensionTypeByNumber)
  {
    const google::protobuf::Reflection* reflection =
      google::protobuf::unittest::TestAllExtensions::default_instance().GetReflection();

    const google::protobuf::FieldDescriptor* extension1 =
      google::protobuf::unittest::TestAllExtensions::descriptor()->file()->FindExtensionByName(
        "optional_int32_extension");
    const google::protobuf::FieldDescriptor* extension2 =
      google::protobuf::unittest::TestAllExtensions::descriptor()->file()->FindExtensionByName(
        "repeated_string_extension");

    verify(extension1 ==
      reflection->FindKnownExtensionByNumber(extension1->number()));
    verify(extension2 ==
      reflection->FindKnownExtensionByNumber(extension2->number()));

    // Non-existent extension.
    verify(reflection->FindKnownExtensionByNumber(62341) == NULL);

    // Extensions of TestAllExtensions should not show up as extensions of
    // other types.
    verify(google::protobuf::unittest::TestAllTypes::default_instance().GetReflection()->
      FindKnownExtensionByNumber(extension1->number()) == NULL);
  }

  // GeneratedMessageReflectionTest, FindKnownExtensionByName
  {
    const google::protobuf::Reflection* reflection =
      google::protobuf::unittest::TestAllExtensions::default_instance().GetReflection();

    const google::protobuf::FieldDescriptor* extension1 =
      google::protobuf::unittest::TestAllExtensions::descriptor()->file()->FindExtensionByName(
        "optional_int32_extension");
    const google::protobuf::FieldDescriptor* extension2 =
      google::protobuf::unittest::TestAllExtensions::descriptor()->file()->FindExtensionByName(
        "repeated_string_extension");

    verify(extension1 ==
      reflection->FindKnownExtensionByName(extension1->full_name()));
    verify(extension2 ==
      reflection->FindKnownExtensionByName(extension2->full_name()));

    // Non-existent extension.
    verify(reflection->FindKnownExtensionByName("no_such_ext") == NULL);

    // Extensions of TestAllExtensions should not show up as extensions of
    // other types.
    verify(google::protobuf::unittest::TestAllTypes::default_instance().GetReflection()->
      FindKnownExtensionByName(extension1->full_name()) == NULL);
  }

  // TEST(GeneratedMessageReflectionTest, SetAllocatedMessageTest)
  {
    google::protobuf::unittest::TestAllTypes from_message1;
    google::protobuf::unittest::TestAllTypes from_message2;
    google::protobuf::unittest::TestAllTypes to_message;
    google::protobuf::TestUtil::ReflectionTester reflection_tester(
      google::protobuf::unittest::TestAllTypes::descriptor());
    reflection_tester.SetAllFieldsViaReflection(&from_message1);
    reflection_tester.SetAllFieldsViaReflection(&from_message2);

    // Before moving fields, we expect the nested messages to be NULL.
    reflection_tester.ExpectMessagesReleasedViaReflection(
      &to_message, google::protobuf::TestUtil::ReflectionTester::IS_NULL);

    // After fields are moved we should get non-NULL releases.
    reflection_tester.SetAllocatedOptionalMessageFieldsToMessageViaReflection(
      &from_message1, &to_message);
    reflection_tester.ExpectMessagesReleasedViaReflection(
      &to_message, google::protobuf::TestUtil::ReflectionTester::NOT_NULL);

    // Another move to make sure that we can SetAllocated several times.
    reflection_tester.SetAllocatedOptionalMessageFieldsToMessageViaReflection(
      &from_message2, &to_message);
    reflection_tester.ExpectMessagesReleasedViaReflection(
      &to_message, google::protobuf::TestUtil::ReflectionTester::NOT_NULL);

    // After SetAllocatedOptionalMessageFieldsToNullViaReflection() we expect the
    // releases to be NULL again.
    reflection_tester.SetAllocatedOptionalMessageFieldsToNullViaReflection(
      &to_message);
    reflection_tester.ExpectMessagesReleasedViaReflection(
      &to_message, google::protobuf::TestUtil::ReflectionTester::IS_NULL);
  }

  // TEST(GeneratedMessageReflectionTest, SetAllocatedExtensionMessageTest)
  {
    google::protobuf::unittest::TestAllExtensions from_message1;
    google::protobuf::unittest::TestAllExtensions from_message2;
    google::protobuf::unittest::TestAllExtensions to_message;
    google::protobuf::TestUtil::ReflectionTester reflection_tester(
      google::protobuf::unittest::TestAllExtensions::descriptor());
    reflection_tester.SetAllFieldsViaReflection(&from_message1);
    reflection_tester.SetAllFieldsViaReflection(&from_message2);

    // Before moving fields, we expect the nested messages to be NULL.
    reflection_tester.ExpectMessagesReleasedViaReflection(
      &to_message, google::protobuf::TestUtil::ReflectionTester::IS_NULL);

    // After fields are moved we should get non-NULL releases.
    reflection_tester.SetAllocatedOptionalMessageFieldsToMessageViaReflection(
      &from_message1, &to_message);
    reflection_tester.ExpectMessagesReleasedViaReflection(
      &to_message, google::protobuf::TestUtil::ReflectionTester::NOT_NULL);

    // Another move to make sure that we can SetAllocated several times.
    reflection_tester.SetAllocatedOptionalMessageFieldsToMessageViaReflection(
      &from_message2, &to_message);
    reflection_tester.ExpectMessagesReleasedViaReflection(
      &to_message, google::protobuf::TestUtil::ReflectionTester::NOT_NULL);

    // After SetAllocatedOptionalMessageFieldsToNullViaReflection() we expect the
    // releases to be NULL again.
    reflection_tester.SetAllocatedOptionalMessageFieldsToNullViaReflection(
      &to_message);
    reflection_tester.ExpectMessagesReleasedViaReflection(
      &to_message, google::protobuf::TestUtil::ReflectionTester::IS_NULL);
  }

  // TEST(GeneratedMessageReflectionTest, ListFieldsOneOf)
  {
    google::protobuf::unittest::TestOneof2 message;
    google::protobuf::TestUtil::SetOneof1(&message);

    const google::protobuf::Reflection* reflection = message.GetReflection();
    std::vector<const google::protobuf::FieldDescriptor*> fields;
    reflection->ListFields(message, &fields);
    verify(4 == fields.size());
  }

  // GeneratedMessageReflectionTest, Oneof
  {
    google::protobuf::unittest::TestOneof2 message;
    const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
    const google::protobuf::Reflection* reflection = message.GetReflection();

    // Check default values.
    verify(0 == reflection->GetInt32(
      message, descriptor->FindFieldByName("foo_int")));
    verify("" == reflection->GetString(
      message, descriptor->FindFieldByName("foo_string")));
    verify("" == reflection->GetString(
      message, descriptor->FindFieldByName("foo_cord")));
    verify("" == reflection->GetString(
      message, descriptor->FindFieldByName("foo_string_piece")));
    verify("" == reflection->GetString(
      message, descriptor->FindFieldByName("foo_bytes")));
    verify(google::protobuf::unittest::TestOneof2::FOO == reflection->GetEnum(
      message, descriptor->FindFieldByName("foo_enum"))->number());
    verify(&google::protobuf::unittest::TestOneof2::NestedMessage::default_instance() ==
      &reflection->GetMessage(
        message, descriptor->FindFieldByName("foo_message")));
    verify(&google::protobuf::unittest::TestOneof2::FooGroup::default_instance() ==
      &reflection->GetMessage(
        message, descriptor->FindFieldByName("foogroup")));
    verify(&google::protobuf::unittest::TestOneof2::FooGroup::default_instance() !=
      &reflection->GetMessage(
        message, descriptor->FindFieldByName("foo_lazy_message")));
    verify(5 == reflection->GetInt32(
      message, descriptor->FindFieldByName("bar_int")));
    verify("STRING" == reflection->GetString(
      message, descriptor->FindFieldByName("bar_string")));
    verify("CORD" == reflection->GetString(
      message, descriptor->FindFieldByName("bar_cord")));
    verify("SPIECE" == reflection->GetString(
      message, descriptor->FindFieldByName("bar_string_piece")));
    verify("BYTES" == reflection->GetString(
      message, descriptor->FindFieldByName("bar_bytes")));
    verify(google::protobuf::unittest::TestOneof2::BAR == reflection->GetEnum(
      message, descriptor->FindFieldByName("bar_enum"))->number());

    // Check Set functions.
    reflection->SetInt32(
      &message, descriptor->FindFieldByName("foo_int"), 123);
    verify(123 == reflection->GetInt32(
      message, descriptor->FindFieldByName("foo_int")));
    reflection->SetString(
      &message, descriptor->FindFieldByName("foo_string"), "abc");
    verify("abc" == reflection->GetString(
      message, descriptor->FindFieldByName("foo_string")));
    reflection->SetString(
      &message, descriptor->FindFieldByName("foo_bytes"), "bytes");
    verify("bytes" == reflection->GetString(
      message, descriptor->FindFieldByName("foo_bytes")));
    reflection->SetString(
      &message, descriptor->FindFieldByName("bar_cord"), "change_cord");
    verify("change_cord" == reflection->GetString(
      message, descriptor->FindFieldByName("bar_cord")));
    reflection->SetString(
      &message, descriptor->FindFieldByName("bar_string_piece"),
      "change_spiece");
    verify("change_spiece" == reflection->GetString(
      message, descriptor->FindFieldByName("bar_string_piece")));
  }

  // TEST(GeneratedMessageReflectionTest, SetAllocatedOneofMessageTest)
  {
    google::protobuf::unittest::TestOneof2 from_message1;
    google::protobuf::unittest::TestOneof2 from_message2;
    google::protobuf::unittest::TestOneof2 to_message;
    const google::protobuf::Descriptor* descriptor = google::protobuf::unittest::TestOneof2::descriptor();
    const google::protobuf::Reflection* reflection = to_message.GetReflection();

    google::protobuf::Message* released = reflection->ReleaseMessage(
      &to_message, descriptor->FindFieldByName("foo_lazy_message"));
    verify(released == NULL);
    released = reflection->ReleaseMessage(
      &to_message, descriptor->FindFieldByName("foo_message"));
    verify(released == NULL);

    google::protobuf::TestUtil::ReflectionTester::SetOneofViaReflection(&from_message1);
    google::protobuf::TestUtil::ReflectionTester::ExpectOneofSetViaReflection(from_message1);

    google::protobuf::TestUtil::ReflectionTester::
      SetAllocatedOptionalMessageFieldsToMessageViaReflection(
        &from_message1, &to_message);
    const google::protobuf::Message& sub_message = reflection->GetMessage(
      to_message, descriptor->FindFieldByName("foo_lazy_message"));
    released = reflection->ReleaseMessage(
      &to_message, descriptor->FindFieldByName("foo_lazy_message"));
    verify(released != NULL);
    verify(&sub_message == released);
    delete released;

    google::protobuf::TestUtil::ReflectionTester::SetOneofViaReflection(&from_message2);

    reflection->MutableMessage(
      &from_message2, descriptor->FindFieldByName("foo_message"));

    google::protobuf::TestUtil::ReflectionTester::
      SetAllocatedOptionalMessageFieldsToMessageViaReflection(
        &from_message2, &to_message);

    const google::protobuf::Message& sub_message2 = reflection->GetMessage(
      to_message, descriptor->FindFieldByName("foo_message"));
    released = reflection->ReleaseMessage(
      &to_message, descriptor->FindFieldByName("foo_message"));
    verify(released != NULL);
    verify(&sub_message2 == released);
    delete released;
  }

  // TEST(GeneratedMessageReflectionTest, ReleaseMessageTest)
  {
    google::protobuf::unittest::TestAllTypes message;
    google::protobuf::TestUtil::ReflectionTester reflection_tester(
      google::protobuf::unittest::TestAllTypes::descriptor());

    // When nothing is set, we expect all released messages to be NULL.
    reflection_tester.ExpectMessagesReleasedViaReflection(
      &message, google::protobuf::TestUtil::ReflectionTester::IS_NULL);

    // After fields are set we should get non-NULL releases.
    reflection_tester.SetAllFieldsViaReflection(&message);
    reflection_tester.ExpectMessagesReleasedViaReflection(
      &message, google::protobuf::TestUtil::ReflectionTester::NOT_NULL);

    // After Clear() we may or may not get a message from ReleaseMessage().
    // This is implementation specific.
    reflection_tester.SetAllFieldsViaReflection(&message);
    message.Clear();
    reflection_tester.ExpectMessagesReleasedViaReflection(
      &message, google::protobuf::TestUtil::ReflectionTester::CAN_BE_NULL);

    // Test a different code path for setting after releasing.
    google::protobuf::TestUtil::SetAllFields(&message);
    google::protobuf::TestUtil::ExpectAllFieldsSet(message);
  }

  // TEST(GeneratedMessageReflectionTest, ReleaseExtensionMessageTest)
  {
    google::protobuf::unittest::TestAllExtensions message;
    google::protobuf::TestUtil::ReflectionTester reflection_tester(
      google::protobuf::unittest::TestAllExtensions::descriptor());

    // When nothing is set, we expect all released messages to be NULL.
    reflection_tester.ExpectMessagesReleasedViaReflection(
      &message, google::protobuf::TestUtil::ReflectionTester::IS_NULL);

    // After fields are set we should get non-NULL releases.
    reflection_tester.SetAllFieldsViaReflection(&message);
    reflection_tester.ExpectMessagesReleasedViaReflection(
      &message, google::protobuf::TestUtil::ReflectionTester::NOT_NULL);

    // After Clear() we may or may not get a message from ReleaseMessage().
    // This is implementation specific.
    reflection_tester.SetAllFieldsViaReflection(&message);
    message.Clear();
    reflection_tester.ExpectMessagesReleasedViaReflection(
      &message, google::protobuf::TestUtil::ReflectionTester::CAN_BE_NULL);

    // Test a different code path for setting after releasing.
    google::protobuf::TestUtil::SetAllExtensions(&message);
    google::protobuf::TestUtil::ExpectAllExtensionsSet(message);
  }

  // TEST(GeneratedMessageReflectionTest, ReleaseOneofMessageTest)
  {
    google::protobuf::unittest::TestOneof2 message;
    google::protobuf::TestUtil::ReflectionTester::SetOneofViaReflection(&message);

    const google::protobuf::Descriptor* descriptor = google::protobuf::unittest::TestOneof2::descriptor();
    const google::protobuf::Reflection* reflection = message.GetReflection();
    const google::protobuf::Message& sub_message = reflection->GetMessage(
      message, descriptor->FindFieldByName("foo_lazy_message"));
    google::protobuf::Message* released = reflection->ReleaseMessage(
      &message, descriptor->FindFieldByName("foo_lazy_message"));

    verify(released != NULL);
    verify(&sub_message == released);
    delete released;

    released = reflection->ReleaseMessage(
      &message, descriptor->FindFieldByName("foo_lazy_message"));
    verify(released == NULL);
  }

  return true;
}
