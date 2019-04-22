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

#if !defined(FUNAPI_UE4_PLATFORM_PS4) && !defined(FUNAPI_UE4_PLATFORM_ANDROID) && !defined(FUNAPI_UE4_PLATFORM_LINUX)

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

// N.B.: We do not test range-based for here because we remain C++03 compatible.
template<typename T, typename M, typename ID>
inline T SumAllExtensions(const M& message, ID extension, T zero) {
  T sum = zero;
  typename google::protobuf::RepeatedField<T>::const_iterator iter =
    message.GetRepeatedExtension(extension).begin();
  typename google::protobuf::RepeatedField<T>::const_iterator end =
    message.GetRepeatedExtension(extension).end();
  for (; iter != end; ++iter) {
    sum += *iter;
  }
  return sum;
}

template<typename T, typename M, typename ID>
inline void IncAllExtensions(M* message, ID extension,
  T val) {
  typename google::protobuf::RepeatedField<T>::iterator iter =
    message->MutableRepeatedExtension(extension)->begin();
  typename google::protobuf::RepeatedField<T>::iterator end =
    message->MutableRepeatedExtension(extension)->end();
  for (; iter != end; ++iter) {
    *iter += val;
  }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFunapiLibProtobufExtensionSetTest, "LibProtobuf.ExtensionSetTest", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFunapiLibProtobufExtensionSetTest::RunTest(const FString& Parameters)
{
  // ExtensionSetTest, Defaults
  {
    // Check that all default values are fun::set correctly in the initial message.
    google::protobuf::unittest::TestAllExtensions message;

    google::protobuf::TestUtil::ExpectExtensionsClear(message);

    // Messages should return pointers to default instances until first use.
    // (This is not checked by ExpectClear() since it is not actually true after
    // the fields have been fun::set and then cleared.)
    verify(&google::protobuf::unittest::OptionalGroup_extension::default_instance() ==
      &message.GetExtension(google::protobuf::unittest::optionalgroup_extension));
    verify(&google::protobuf::unittest::TestAllTypes::NestedMessage::default_instance() ==
      &message.GetExtension(google::protobuf::unittest::optional_nested_message_extension));
    verify(&google::protobuf::unittest::ForeignMessage::default_instance() ==
      &message.GetExtension(
        google::protobuf::unittest::optional_foreign_message_extension));
    verify(&google::protobuf::unittest_import::ImportMessage::default_instance() ==
      &message.GetExtension(google::protobuf::unittest::optional_import_message_extension));
  }

  // ExtensionSetTest, Accessors
  {
    // Set every field to a unique value then go back and check all those
    // values.
    google::protobuf::unittest::TestAllExtensions message;

    google::protobuf::TestUtil::SetAllExtensions(&message);
    google::protobuf::TestUtil::ExpectAllExtensionsSet(message);

    google::protobuf::TestUtil::ModifyRepeatedExtensions(&message);
    google::protobuf::TestUtil::ExpectRepeatedExtensionsModified(message);
  }

  // ExtensionSetTest, Clear
  {
    // Set every field to a unique value, clear the message, then check that
    // it is cleared.
    google::protobuf::unittest::TestAllExtensions message;

    google::protobuf::TestUtil::SetAllExtensions(&message);
    message.Clear();
    google::protobuf::TestUtil::ExpectExtensionsClear(message);

    // Unlike with the defaults test, we do NOT expect that requesting embedded
    // messages will return a pointer to the default instance.  Instead, they
    // should return the objects that were created when mutable_blah() was
    // called.
    verify(&google::protobuf::unittest::OptionalGroup_extension::default_instance() !=
      &message.GetExtension(google::protobuf::unittest::optionalgroup_extension));
    verify(&google::protobuf::unittest::TestAllTypes::NestedMessage::default_instance() !=
      &message.GetExtension(google::protobuf::unittest::optional_nested_message_extension));
    verify(&google::protobuf::unittest::ForeignMessage::default_instance() !=
      &message.GetExtension(
        google::protobuf::unittest::optional_foreign_message_extension));
    verify(&google::protobuf::unittest_import::ImportMessage::default_instance() !=
      &message.GetExtension(google::protobuf::unittest::optional_import_message_extension));

    // Make sure setting stuff again after clearing works.  (This takes slightly
    // different code paths since the objects are reused.)
    google::protobuf::TestUtil::SetAllExtensions(&message);
    google::protobuf::TestUtil::ExpectAllExtensionsSet(message);
  }

  // ExtensionSetTest, ClearOneField
  {
    // Set every field to a unique value, then clear one value and insure that
    // only that one value is cleared.
    google::protobuf::unittest::TestAllExtensions message;

    google::protobuf::TestUtil::SetAllExtensions(&message);
    int64 original_value =
      message.GetExtension(google::protobuf::unittest::optional_int64_extension);

    // Clear the field and make sure it shows up as cleared.
    message.ClearExtension(google::protobuf::unittest::optional_int64_extension);
    verify(false == message.HasExtension(google::protobuf::unittest::optional_int64_extension));
    verify(0 == message.GetExtension(google::protobuf::unittest::optional_int64_extension));

    // Other adjacent fields should not be cleared.
    verify(message.HasExtension(google::protobuf::unittest::optional_int32_extension));
    verify(message.HasExtension(google::protobuf::unittest::optional_uint32_extension));

    // Make sure if we fun::set it again, then all fields are set
    message.SetExtension(google::protobuf::unittest::optional_int64_extension, original_value);
    google::protobuf::TestUtil::ExpectAllExtensionsSet(message);
  }

  // ExtensionSetTest, SetAllocatedExtension
  {
    google::protobuf::unittest::TestAllExtensions message;
    verify(false == message.HasExtension(
      google::protobuf::unittest::optional_foreign_message_extension));
    // Add a extension using SetAllocatedExtension
    google::protobuf::unittest::ForeignMessage* foreign_message = new google::protobuf::unittest::ForeignMessage();
    message.SetAllocatedExtension(google::protobuf::unittest::optional_foreign_message_extension,
      foreign_message);
    verify(message.HasExtension(
      google::protobuf::unittest::optional_foreign_message_extension));
    verify(foreign_message ==
      message.MutableExtension(
        google::protobuf::unittest::optional_foreign_message_extension));
    verify(foreign_message ==
      &message.GetExtension(
        google::protobuf::unittest::optional_foreign_message_extension));

    // SetAllocatedExtension should delete the previously existing extension.
    // (We reply on unittest to check memory leaks for this case)
    message.SetAllocatedExtension(google::protobuf::unittest::optional_foreign_message_extension,
      new google::protobuf::unittest::ForeignMessage());

    // SetAllocatedExtension with a NULL parameter is equivalent to ClearExtenion.
    message.SetAllocatedExtension(google::protobuf::unittest::optional_foreign_message_extension,
      NULL);
    verify(false == message.HasExtension(
      google::protobuf::unittest::optional_foreign_message_extension));
  }

  // ExtensionSetTest, ReleaseExtension
  {
    google::protobuf::unittest::TestMessageSet message;
    verify(false == message.HasExtension(
      google::protobuf::unittest::TestMessageSetExtension1::message_set_extension));
    // Add a extension using SetAllocatedExtension
    google::protobuf::unittest::TestMessageSetExtension1* extension =
      new  google::protobuf::unittest::TestMessageSetExtension1();
    message.SetAllocatedExtension(
      google::protobuf::unittest::TestMessageSetExtension1::message_set_extension,
      extension);
    verify(message.HasExtension(
      google::protobuf::unittest::TestMessageSetExtension1::message_set_extension));
    // Release the extension using ReleaseExtension
    google::protobuf::unittest::TestMessageSetExtension1* released_extension =
      message.ReleaseExtension(
        google::protobuf::unittest::TestMessageSetExtension1::message_set_extension);
    verify(extension == released_extension);
    verify(false == message.HasExtension(
      google::protobuf::unittest::TestMessageSetExtension1::message_set_extension));
    // ReleaseExtension will return the underlying object even after
    // ClearExtension is called.
    message.SetAllocatedExtension(
      google::protobuf::unittest::TestMessageSetExtension1::message_set_extension,
      extension);
    message.ClearExtension(
      google::protobuf::unittest::TestMessageSetExtension1::message_set_extension);
    released_extension = message.ReleaseExtension(
      google::protobuf::unittest::TestMessageSetExtension1::message_set_extension);
    verify(released_extension != NULL);
    delete released_extension;
  }


  // ExtensionSetTest, CopyFrom
  {
    google::protobuf::unittest::TestAllExtensions message1, message2;

    google::protobuf::TestUtil::SetAllExtensions(&message1);
    message2.CopyFrom(message1);
    google::protobuf::TestUtil::ExpectAllExtensionsSet(message2);
    message2.CopyFrom(message1);  // exercise copy when fields already exist
    google::protobuf::TestUtil::ExpectAllExtensionsSet(message2);
  }

  // ExtensioSetTest, CopyFromPacked
  {
    google::protobuf::unittest::TestPackedExtensions message1, message2;

    google::protobuf::TestUtil::SetPackedExtensions(&message1);
    message2.CopyFrom(message1);
    google::protobuf::TestUtil::ExpectPackedExtensionsSet(message2);
    message2.CopyFrom(message1);  // exercise copy when fields already exist
    google::protobuf::TestUtil::ExpectPackedExtensionsSet(message2);
  }

  // ExtensionSetTest, CopyFromUpcasted
  {
    google::protobuf::unittest::TestAllExtensions message1, message2;
    const google::protobuf::Message& upcasted_message = message1;

    google::protobuf::TestUtil::SetAllExtensions(&message1);
    message2.CopyFrom(upcasted_message);
    google::protobuf::TestUtil::ExpectAllExtensionsSet(message2);
    // exercise copy when fields already exist
    message2.CopyFrom(upcasted_message);
    google::protobuf::TestUtil::ExpectAllExtensionsSet(message2);
  }

  // ExtensionSetTest, SwapWithEmpty
  {
    google::protobuf::unittest::TestAllExtensions message1, message2;
    google::protobuf::TestUtil::SetAllExtensions(&message1);

    google::protobuf::TestUtil::ExpectAllExtensionsSet(message1);
    google::protobuf::TestUtil::ExpectExtensionsClear(message2);
    message1.Swap(&message2);
    google::protobuf::TestUtil::ExpectAllExtensionsSet(message2);
    google::protobuf::TestUtil::ExpectExtensionsClear(message1);
  }

  // ExtensionSetTest, SwapWithSelf
  {
    google::protobuf::unittest::TestAllExtensions message;
    google::protobuf::TestUtil::SetAllExtensions(&message);

    google::protobuf::TestUtil::ExpectAllExtensionsSet(message);
    message.Swap(&message);
    google::protobuf::TestUtil::ExpectAllExtensionsSet(message);
  }

  // ExtensionSetTest, SwapExtension
  {
    google::protobuf::unittest::TestAllExtensions message1;
    google::protobuf::unittest::TestAllExtensions message2;

    google::protobuf::TestUtil::SetAllExtensions(&message1);
    fun::vector<const google::protobuf::FieldDescriptor*> fields;

    // Swap empty fields.
    const google::protobuf::Reflection* reflection = message1.GetReflection();
    reflection->SwapFields(&message1, &message2, fields);
    google::protobuf::TestUtil::ExpectAllExtensionsSet(message1);
    google::protobuf::TestUtil::ExpectExtensionsClear(message2);

    // Swap two extensions.
    fields.push_back(
      reflection->FindKnownExtensionByNumber(12));
    fields.push_back(
      reflection->FindKnownExtensionByNumber(25));
    reflection->SwapFields(&message1, &message2, fields);

    verify(message1.HasExtension(google::protobuf::unittest::optional_int32_extension));
    verify(false == message1.HasExtension(google::protobuf::unittest::optional_double_extension));
    verify(false == message1.HasExtension(google::protobuf::unittest::optional_cord_extension));

    verify(false == message2.HasExtension(google::protobuf::unittest::optional_int32_extension));
    verify(message2.HasExtension(google::protobuf::unittest::optional_double_extension));
    verify(message2.HasExtension(google::protobuf::unittest::optional_cord_extension));
  }

  // ExtensionSetTest, SwapExtensionWithEmpty
  {
    google::protobuf::unittest::TestAllExtensions message1;
    google::protobuf::unittest::TestAllExtensions message2;
    google::protobuf::unittest::TestAllExtensions message3;

    google::protobuf::TestUtil::SetAllExtensions(&message3);

    const google::protobuf::Reflection* reflection = message3.GetReflection();
    fun::vector<const google::protobuf::FieldDescriptor*> fields;
    reflection->ListFields(message3, &fields);

    reflection->SwapFields(&message1, &message2, fields);

    google::protobuf::TestUtil::ExpectExtensionsClear(message1);
    google::protobuf::TestUtil::ExpectExtensionsClear(message2);
  }

  // ExtensionSetTest, SwapExtensionBothFull
  {
    google::protobuf::unittest::TestAllExtensions message1;
    google::protobuf::unittest::TestAllExtensions message2;

    google::protobuf::TestUtil::SetAllExtensions(&message1);
    google::protobuf::TestUtil::SetAllExtensions(&message2);

    const google::protobuf::Reflection* reflection = message1.GetReflection();
    fun::vector<const google::protobuf::FieldDescriptor*> fields;
    reflection->ListFields(message1, &fields);

    reflection->SwapFields(&message1, &message2, fields);

    google::protobuf::TestUtil::ExpectAllExtensionsSet(message1);
    google::protobuf::TestUtil::ExpectAllExtensionsSet(message2);
  }

  // ExtensionSetTest, SwapExtensionWithSelf
  {
    google::protobuf::unittest::TestAllExtensions message1;

    google::protobuf::TestUtil::SetAllExtensions(&message1);

    fun::vector<const google::protobuf::FieldDescriptor*> fields;
    const google::protobuf::Reflection* reflection = message1.GetReflection();
    reflection->ListFields(message1, &fields);
    reflection->SwapFields(&message1, &message1, fields);

    google::protobuf::TestUtil::ExpectAllExtensionsSet(message1);
  }

  // ExtensionSetTest, SerializationToArray
  {
    // Serialize as TestAllExtensions and parse as TestAllTypes to insure wire
    // compatibility of extensions.
    //
    // This checks serialization to a flat array by explicitly reserving space in
    // the fun::string and calling the generated message's
    // SerializeWithCachedSizesToArray.
    google::protobuf::unittest::TestAllExtensions source;
    google::protobuf::unittest::TestAllTypes destination;
    google::protobuf::TestUtil::SetAllExtensions(&source);
    int size = source.ByteSize();
    fun::string data;
    data.resize(size);
    uint8* target = reinterpret_cast<uint8*>(google::protobuf::string_as_array(&data));
    uint8* end = source.SerializeWithCachedSizesToArray(target);
    verify(size == end - target);
    verify(destination.ParseFromString(data));
    google::protobuf::TestUtil::ExpectAllFieldsSet(destination);
  }

  // ExtensionSetTest, SerializationToStream
  {
    // Serialize as TestAllExtensions and parse as TestAllTypes to insure wire
    // compatibility of extensions.
    //
    // This checks serialization to an output stream by creating an array output
    // stream that can only buffer 1 byte at a time - this prevents the message
    // from ever jumping to the fast path, ensuring that serialization happens via
    // the CodedOutputStream.
    google::protobuf::unittest::TestAllExtensions source;
    google::protobuf::unittest::TestAllTypes destination;
    google::protobuf::TestUtil::SetAllExtensions(&source);
    int size = source.ByteSize();
    fun::string data;
    data.resize(size);
    {
      google::protobuf::io::ArrayOutputStream array_stream(google::protobuf::string_as_array(&data), size, 1);
      google::protobuf::io::CodedOutputStream output_stream(&array_stream);
      source.SerializeWithCachedSizes(&output_stream);
      verify(false == output_stream.HadError());
    }
    verify(destination.ParseFromString(data));
    google::protobuf::TestUtil::ExpectAllFieldsSet(destination);
  }

  // ExtensionSetTest, PackedSerializationToArray
  {
    // Serialize as TestPackedExtensions and parse as TestPackedTypes to insure
    // wire compatibility of extensions.
    //
    // This checks serialization to a flat array by explicitly reserving space in
    // the fun::string and calling the generated message's
    // SerializeWithCachedSizesToArray.
    google::protobuf::unittest::TestPackedExtensions source;
    google::protobuf::unittest::TestPackedTypes destination;
    google::protobuf::TestUtil::SetPackedExtensions(&source);
    int size = source.ByteSize();
    fun::string data;
    data.resize(size);
    uint8* target = reinterpret_cast<uint8*>(google::protobuf::string_as_array(&data));
    uint8* end = source.SerializeWithCachedSizesToArray(target);
    verify(size == end - target);
    verify(destination.ParseFromString(data));
    google::protobuf::TestUtil::ExpectPackedFieldsSet(destination);
  }

  // ExtensionSetTest, PackedSerializationToStream
  {
    // Serialize as TestPackedExtensions and parse as TestPackedTypes to insure
    // wire compatibility of extensions.
    //
    // This checks serialization to an output stream by creating an array output
    // stream that can only buffer 1 byte at a time - this prevents the message
    // from ever jumping to the fast path, ensuring that serialization happens via
    // the CodedOutputStream.
    google::protobuf::unittest::TestPackedExtensions source;
    google::protobuf::unittest::TestPackedTypes destination;
    google::protobuf::TestUtil::SetPackedExtensions(&source);
    int size = source.ByteSize();
    fun::string data;
    data.resize(size);
    {
      google::protobuf::io::ArrayOutputStream array_stream(google::protobuf::string_as_array(&data), size, 1);
      google::protobuf::io::CodedOutputStream output_stream(&array_stream);
      source.SerializeWithCachedSizes(&output_stream);
      verify(false == output_stream.HadError());
    }
    verify(destination.ParseFromString(data));
    google::protobuf::TestUtil::ExpectPackedFieldsSet(destination);
  }

  // ExtensionSetTest, Parsing
  {
    // Serialize as TestAllTypes and parse as TestAllExtensions.
    google::protobuf::unittest::TestAllTypes source;
    google::protobuf::unittest::TestAllExtensions destination;
    fun::string data;

    google::protobuf::TestUtil::SetAllFields(&source);
    source.SerializeToString(&data);
    verify(destination.ParseFromString(data));
    google::protobuf::TestUtil::SetOneofFields(&destination);
    google::protobuf::TestUtil::ExpectAllExtensionsSet(destination);
  }

  // ExtensionSetTest, PackedParsing
  {
    // Serialize as TestPackedTypes and parse as TestPackedExtensions.
    google::protobuf::unittest::TestPackedTypes source;
    google::protobuf::unittest::TestPackedExtensions destination;
    fun::string data;

    google::protobuf::TestUtil::SetPackedFields(&source);
    source.SerializeToString(&data);
    verify(destination.ParseFromString(data));
    google::protobuf::TestUtil::ExpectPackedExtensionsSet(destination);
  }

  // ExtensionSetTest, PackedToUnpackedParsing
  {
    google::protobuf::unittest::TestPackedTypes source;
    google::protobuf::unittest::TestUnpackedExtensions destination;
    fun::string data;

    google::protobuf::TestUtil::SetPackedFields(&source);
    source.SerializeToString(&data);
    verify(destination.ParseFromString(data));
    google::protobuf::TestUtil::ExpectUnpackedExtensionsSet(destination);

    // Reserialize
    google::protobuf::unittest::TestUnpackedTypes unpacked;
    google::protobuf::TestUtil::SetUnpackedFields(&unpacked);
    verify(unpacked.SerializeAsString() == destination.SerializeAsString());

    // Make sure we can add extensions.
    destination.AddExtension(google::protobuf::unittest::unpacked_int32_extension, 1);
    destination.AddExtension(google::protobuf::unittest::unpacked_enum_extension,
      protobuf_unittest::FOREIGN_BAR);
  }

  // ExtensionSetTest, UnpackedToPackedParsing
  {
    google::protobuf::unittest::TestUnpackedTypes source;
    google::protobuf::unittest::TestPackedExtensions destination;
    fun::string data;

    google::protobuf::TestUtil::SetUnpackedFields(&source);
    source.SerializeToString(&data);
    verify(destination.ParseFromString(data));
    google::protobuf::TestUtil::ExpectPackedExtensionsSet(destination);

    // Reserialize
    google::protobuf::unittest::TestPackedTypes packed;
    google::protobuf::TestUtil::SetPackedFields(&packed);
    verify(packed.SerializeAsString() == destination.SerializeAsString());

    // Make sure we can add extensions.
    destination.AddExtension(google::protobuf::unittest::packed_int32_extension, 1);
    destination.AddExtension(google::protobuf::unittest::packed_enum_extension,
      protobuf_unittest::FOREIGN_BAR);
  }

  // ExtensionSetTest, IsInitialized
  {
    // Test that IsInitialized() returns false if required fields in nested
    // extensions are missing.
    google::protobuf::unittest::TestAllExtensions message;

    verify(message.IsInitialized());

    message.MutableExtension(google::protobuf::unittest::TestRequired::single);
    verify(false == message.IsInitialized());

    message.MutableExtension(google::protobuf::unittest::TestRequired::single)->set_a(1);
    verify(false == message.IsInitialized());
    message.MutableExtension(google::protobuf::unittest::TestRequired::single)->set_b(2);
    verify(false == message.IsInitialized());
    message.MutableExtension(google::protobuf::unittest::TestRequired::single)->set_c(3);
    verify(message.IsInitialized());

    message.AddExtension(google::protobuf::unittest::TestRequired::multi);
    verify(false == message.IsInitialized());

    message.MutableExtension(google::protobuf::unittest::TestRequired::multi, 0)->set_a(1);
    verify(false == message.IsInitialized());
    message.MutableExtension(google::protobuf::unittest::TestRequired::multi, 0)->set_b(2);
    verify(false == message.IsInitialized());
    message.MutableExtension(google::protobuf::unittest::TestRequired::multi, 0)->set_c(3);
    verify(message.IsInitialized());
  }

  // ExtensionSetTest, MutableString
  {
    // Test the mutable fun::string accessors.
    google::protobuf::unittest::TestAllExtensions message;

    message.MutableExtension(google::protobuf::unittest::optional_string_extension)->assign("foo");
    verify(message.HasExtension(google::protobuf::unittest::optional_string_extension));
    verify("foo" == message.GetExtension(google::protobuf::unittest::optional_string_extension));

    message.AddExtension(google::protobuf::unittest::repeated_string_extension)->assign("bar");
    verify(1 == message.ExtensionSize(google::protobuf::unittest::repeated_string_extension));
    verify("bar" ==
      message.GetExtension(google::protobuf::unittest::repeated_string_extension, 0));
  }

  // ExtensionSetTest, SpaceUsedExcludingSelf {
    // Scalar primitive extensions should increase the extension fun::set size by a
    // minimum of the size of the primitive type.
#define TEST_SCALAR_EXTENSIONS_SPACE_USED(type, value)                        \
  do {                                                                        \
    google::protobuf::unittest::TestAllExtensions message;                                      \
    const int base_size = message.SpaceUsed();                                \
    message.SetExtension(google::protobuf::unittest::optional_##type##_extension, value);       \
    int min_expected_size = base_size +                                       \
        sizeof(message.GetExtension(google::protobuf::unittest::optional_##type##_extension));  \
    verify(min_expected_size <= message.SpaceUsed());                        \
  } while (0)

  TEST_SCALAR_EXTENSIONS_SPACE_USED(int32, 101);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(int64, 102);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(uint32, 103);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(uint64, 104);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(sint32, 105);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(sint64, 106);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(fixed32, 107);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(fixed64, 108);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(sfixed32, 109);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(sfixed64, 110);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(float, 111);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(double, 112);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(bool, true);
#undef TEST_SCALAR_EXTENSIONS_SPACE_USED
  {
    google::protobuf::unittest::TestAllExtensions message;
    const int base_size = message.SpaceUsed();
    message.SetExtension(google::protobuf::unittest::optional_nested_enum_extension,
      google::protobuf::unittest::TestAllTypes::FOO);
    int min_expected_size = base_size +
      sizeof(message.GetExtension(google::protobuf::unittest::optional_nested_enum_extension));
    verify(min_expected_size <= message.SpaceUsed());
  }
  {
    // Strings may cause extra allocations depending on their length; ensure
    // that gets included as well.
    google::protobuf::unittest::TestAllExtensions message;
    const int base_size = message.SpaceUsed();
    const fun::string s("this is a fairly large fun::string that will cause some "
      "allocation in order to store it in the extension");
    message.SetExtension(google::protobuf::unittest::optional_string_extension, s);
    int min_expected_size = base_size + s.length();
    verify(min_expected_size <= message.SpaceUsed());
  }
  {
    // Messages also have additional allocation that need to be counted.
    google::protobuf::unittest::TestAllExtensions message;
    const int base_size = message.SpaceUsed();
    google::protobuf::unittest::ForeignMessage foreign;
    foreign.set_c(42);
    message.MutableExtension(google::protobuf::unittest::optional_foreign_message_extension)->
      CopyFrom(foreign);
    int min_expected_size = base_size + foreign.SpaceUsed();
    verify(min_expected_size <= message.SpaceUsed());
  }

  // Repeated primitive extensions will increase space used by at least a
  // RepeatedField<T>, and will cause additional allocations when the array
  // gets too big for the initial space.
  // This macro:
  //   - Adds a value to the repeated extension, then clears it, establishing
  //     the base size.
  //   - Adds a small number of values, testing that it doesn't increase the
  //     SpaceUsed()
  //   - Adds a large number of values (requiring allocation in the repeated
  //     field), and ensures that that allocation is included in SpaceUsed()
#define TEST_REPEATED_EXTENSIONS_SPACE_USED(type, cpptype, value)              \
  do {                                                                         \
    google::protobuf::unittest::TestAllExtensions message;                                       \
    const int base_size = message.SpaceUsed();                                 \
    int min_expected_size = sizeof(google::protobuf::RepeatedField<cpptype>) + base_size;        \
    message.AddExtension(google::protobuf::unittest::repeated_##type##_extension, value);        \
    message.ClearExtension(google::protobuf::unittest::repeated_##type##_extension);             \
    const int empty_repeated_field_size = message.SpaceUsed();                 \
    verify(min_expected_size <= empty_repeated_field_size); \
    message.AddExtension(google::protobuf::unittest::repeated_##type##_extension, value);        \
    message.AddExtension(google::protobuf::unittest::repeated_##type##_extension, value);        \
    verify(empty_repeated_field_size == message.SpaceUsed());        \
    message.ClearExtension(google::protobuf::unittest::repeated_##type##_extension);             \
    for (int i = 0; i < 16; ++i) {                                             \
      message.AddExtension(google::protobuf::unittest::repeated_##type##_extension, value);      \
    }                                                                          \
    int expected_size = sizeof(cpptype) * (16 -                                \
        google::protobuf::internal::kMinRepeatedFieldAllocationSize) + empty_repeated_field_size;          \
    verify(expected_size == message.SpaceUsed());                     \
  } while (0)

  TEST_REPEATED_EXTENSIONS_SPACE_USED(int32, int32, 101);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(int64, int64, 102);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(uint32, uint32, 103);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(uint64, uint64, 104);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(sint32, int32, 105);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(sint64, int64, 106);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(fixed32, uint32, 107);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(fixed64, uint64, 108);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(sfixed32, int32, 109);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(sfixed64, int64, 110);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(float, float, 111);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(double, double, 112);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(bool, bool, true);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(nested_enum, int,
    google::protobuf::unittest::TestAllTypes::FOO);
#undef TEST_REPEATED_EXTENSIONS_SPACE_USED
  // Repeated strings
  {
    google::protobuf::unittest::TestAllExtensions message;
    const int base_size = message.SpaceUsed();
    int min_expected_size = sizeof(google::protobuf::RepeatedPtrField<fun::string>) + base_size;
    const fun::string value(256, 'x');
    // Once items are allocated, they may stick around even when cleared so
    // without the hardcore memory management accessors there isn't a notion of
    // the empty repeated field memory usage as there is with primitive types.
    for (int i = 0; i < 16; ++i) {
      message.AddExtension(google::protobuf::unittest::repeated_string_extension, value);
    }
    min_expected_size += (sizeof(value) + value.size()) *
      (16 - google::protobuf::internal::kMinRepeatedFieldAllocationSize);
    verify(min_expected_size <= message.SpaceUsed());
  }
  // Repeated messages
  {
    google::protobuf::unittest::TestAllExtensions message;
    const int base_size = message.SpaceUsed();
    int min_expected_size = sizeof(google::protobuf::RepeatedPtrField<google::protobuf::unittest::ForeignMessage>) +
      base_size;
    google::protobuf::unittest::ForeignMessage prototype;
    prototype.set_c(2);
    for (int i = 0; i < 16; ++i) {
      message.AddExtension(google::protobuf::unittest::repeated_foreign_message_extension)->
        CopyFrom(prototype);
    }
    min_expected_size +=
      (16 - google::protobuf::internal::kMinRepeatedFieldAllocationSize) * prototype.SpaceUsed();
    verify(min_expected_size <= message.SpaceUsed());
  }

  // ExtensionSetTest, RepeatedFields
  {
    google::protobuf::unittest::TestAllExtensions message;

    // Test empty repeated-field case (b/12926163)
    verify(0 == message.GetRepeatedExtension(
      google::protobuf::unittest::repeated_int32_extension).size());
    verify(0 == message.GetRepeatedExtension(
      google::protobuf::unittest::repeated_nested_enum_extension).size());
    verify(0 == message.GetRepeatedExtension(
      google::protobuf::unittest::repeated_string_extension).size());
    verify(0 == message.GetRepeatedExtension(
      google::protobuf::unittest::repeated_nested_message_extension).size());

    google::protobuf::unittest::TestAllTypes::NestedMessage nested_message;
    nested_message.set_bb(42);
    google::protobuf::unittest::TestAllTypes::NestedEnum nested_enum =
      google::protobuf::unittest::TestAllTypes::NestedEnum_MIN;

    for (int i = 0; i < 10; ++i) {
      message.AddExtension(google::protobuf::unittest::repeated_int32_extension, 1);
      message.AddExtension(google::protobuf::unittest::repeated_int64_extension, 2);
      message.AddExtension(google::protobuf::unittest::repeated_uint32_extension, 3);
      message.AddExtension(google::protobuf::unittest::repeated_uint64_extension, 4);
      message.AddExtension(google::protobuf::unittest::repeated_sint32_extension, 5);
      message.AddExtension(google::protobuf::unittest::repeated_sint64_extension, 6);
      message.AddExtension(google::protobuf::unittest::repeated_fixed32_extension, 7);
      message.AddExtension(google::protobuf::unittest::repeated_fixed64_extension, 8);
      message.AddExtension(google::protobuf::unittest::repeated_sfixed32_extension, 7);
      message.AddExtension(google::protobuf::unittest::repeated_sfixed64_extension, 8);
      message.AddExtension(google::protobuf::unittest::repeated_float_extension, 9.0);
      message.AddExtension(google::protobuf::unittest::repeated_double_extension, 10.0);
      message.AddExtension(google::protobuf::unittest::repeated_bool_extension, true);
      message.AddExtension(google::protobuf::unittest::repeated_nested_enum_extension, nested_enum);
      message.AddExtension(google::protobuf::unittest::repeated_string_extension,
        ::fun::string("test"));
      message.AddExtension(google::protobuf::unittest::repeated_bytes_extension,
        ::fun::string("test\xFF"));
      message.AddExtension(
        google::protobuf::unittest::repeated_nested_message_extension)->CopyFrom(nested_message);
      message.AddExtension(google::protobuf::unittest::repeated_nested_enum_extension,
        nested_enum);
    }

    verify(10 == SumAllExtensions<int32>(
      message, google::protobuf::unittest::repeated_int32_extension, 0));
    IncAllExtensions<int32>(
      &message, google::protobuf::unittest::repeated_int32_extension, 1);
    verify(20 == SumAllExtensions<int32>(
      message, google::protobuf::unittest::repeated_int32_extension, 0));

    verify(20 == SumAllExtensions<int64>(
      message, google::protobuf::unittest::repeated_int64_extension, 0));
    IncAllExtensions<int64>(
      &message, google::protobuf::unittest::repeated_int64_extension, 1);
    verify(30 == SumAllExtensions<int64>(
      message, google::protobuf::unittest::repeated_int64_extension, 0));

    verify(30 == SumAllExtensions<uint32>(
      message, google::protobuf::unittest::repeated_uint32_extension, 0));
    IncAllExtensions<uint32>(
      &message, google::protobuf::unittest::repeated_uint32_extension, 1);
    verify(40 == SumAllExtensions<uint32>(
      message, google::protobuf::unittest::repeated_uint32_extension, 0));

    verify(40 == SumAllExtensions<uint64>(
      message, google::protobuf::unittest::repeated_uint64_extension, 0));
    IncAllExtensions<uint64>(
      &message, google::protobuf::unittest::repeated_uint64_extension, 1);
    verify(50 == SumAllExtensions<uint64>(
      message, google::protobuf::unittest::repeated_uint64_extension, 0));

    verify(50 == SumAllExtensions<int32>(
      message, google::protobuf::unittest::repeated_sint32_extension, 0));
    IncAllExtensions<int32>(
      &message, google::protobuf::unittest::repeated_sint32_extension, 1);
    verify(60 == SumAllExtensions<int32>(
      message, google::protobuf::unittest::repeated_sint32_extension, 0));

    verify(60 == SumAllExtensions<int64>(
      message, google::protobuf::unittest::repeated_sint64_extension, 0));
    IncAllExtensions<int64>(
      &message, google::protobuf::unittest::repeated_sint64_extension, 1);
    verify(70 == SumAllExtensions<int64>(
      message, google::protobuf::unittest::repeated_sint64_extension, 0));

    verify(70 == SumAllExtensions<uint32>(
      message, google::protobuf::unittest::repeated_fixed32_extension, 0));
    IncAllExtensions<uint32>(
      &message, google::protobuf::unittest::repeated_fixed32_extension, 1);
    verify(80 == SumAllExtensions<uint32>(
      message, google::protobuf::unittest::repeated_fixed32_extension, 0));

    verify(80 == SumAllExtensions<uint64>(
      message, google::protobuf::unittest::repeated_fixed64_extension, 0));
    IncAllExtensions<uint64>(
      &message, google::protobuf::unittest::repeated_fixed64_extension, 1);
    verify(90 == SumAllExtensions<uint64>(
      message, google::protobuf::unittest::repeated_fixed64_extension, 0));

    // Usually, floating-point arithmetic cannot be trusted to be exact, so it is
    // a Bad Idea to assert equality in a test like this. However, we're dealing
    // with integers with a small number of significant mantissa bits, so we
    // should actually have exact precision here.
    verify(90 == SumAllExtensions<float>(
      message, google::protobuf::unittest::repeated_float_extension, 0));
    IncAllExtensions<float>(
      &message, google::protobuf::unittest::repeated_float_extension, 1);
    verify(100 == SumAllExtensions<float>(
      message, google::protobuf::unittest::repeated_float_extension, 0));

    verify(100 == SumAllExtensions<double>(
      message, google::protobuf::unittest::repeated_double_extension, 0));
    IncAllExtensions<double>(
      &message, google::protobuf::unittest::repeated_double_extension, 1);
    verify(110 == SumAllExtensions<double>(
      message, google::protobuf::unittest::repeated_double_extension, 0));

    google::protobuf::RepeatedPtrField< ::fun::string>::iterator string_iter;
    google::protobuf::RepeatedPtrField< ::fun::string>::iterator string_end;
    for (string_iter = message.MutableRepeatedExtension(
      google::protobuf::unittest::repeated_string_extension)->begin(),
      string_end = message.MutableRepeatedExtension(
        google::protobuf::unittest::repeated_string_extension)->end();
      string_iter != string_end; ++string_iter) {
      *string_iter += "test";
    }
    google::protobuf::RepeatedPtrField< ::fun::string>::const_iterator string_const_iter;
    google::protobuf::RepeatedPtrField< ::fun::string>::const_iterator string_const_end;
    for (string_const_iter = message.GetRepeatedExtension(
      google::protobuf::unittest::repeated_string_extension).begin(),
      string_const_end = message.GetRepeatedExtension(
        google::protobuf::unittest::repeated_string_extension).end();
      string_iter != string_end; ++string_iter) {
      verify(*string_iter == "testtest");
    }

    google::protobuf::RepeatedField<google::protobuf::unittest::TestAllTypes_NestedEnum>::iterator enum_iter;
    google::protobuf::RepeatedField<google::protobuf::unittest::TestAllTypes_NestedEnum>::iterator enum_end;
    for (enum_iter = message.MutableRepeatedExtension(
      google::protobuf::unittest::repeated_nested_enum_extension)->begin(),
      enum_end = message.MutableRepeatedExtension(
        google::protobuf::unittest::repeated_nested_enum_extension)->end();
      enum_iter != enum_end; ++enum_iter) {
      *enum_iter = google::protobuf::unittest::TestAllTypes::NestedEnum_MAX;
    }
    google::protobuf::RepeatedField<google::protobuf::unittest::TestAllTypes_NestedEnum>::const_iterator
      enum_const_iter;
    google::protobuf::RepeatedField<google::protobuf::unittest::TestAllTypes_NestedEnum>::const_iterator
      enum_const_end;
    for (enum_const_iter = message.GetRepeatedExtension(
      google::protobuf::unittest::repeated_nested_enum_extension).begin(),
      enum_const_end = message.GetRepeatedExtension(
        google::protobuf::unittest::repeated_nested_enum_extension).end();
      enum_iter != enum_end; ++enum_iter) {
      verify(*enum_const_iter == google::protobuf::unittest::TestAllTypes::NestedEnum_MAX);
    }

    google::protobuf::RepeatedPtrField<google::protobuf::unittest::TestAllTypes_NestedMessage>::iterator
      msg_iter;
    google::protobuf::RepeatedPtrField<google::protobuf::unittest::TestAllTypes_NestedMessage>::iterator
      msg_end;
    for (msg_iter = message.MutableRepeatedExtension(
      google::protobuf::unittest::repeated_nested_message_extension)->begin(),
      msg_end = message.MutableRepeatedExtension(
        google::protobuf::unittest::repeated_nested_message_extension)->end();
      msg_iter != msg_end; ++msg_iter) {
      msg_iter->set_bb(1234);
    }
    google::protobuf::RepeatedPtrField<google::protobuf::unittest::TestAllTypes_NestedMessage>::
      const_iterator msg_const_iter;
    google::protobuf::RepeatedPtrField<google::protobuf::unittest::TestAllTypes_NestedMessage>::
      const_iterator msg_const_end;
    for (msg_const_iter = message.GetRepeatedExtension(
      google::protobuf::unittest::repeated_nested_message_extension).begin(),
      msg_const_end = message.GetRepeatedExtension(
        google::protobuf::unittest::repeated_nested_message_extension).end();
      msg_const_iter != msg_const_end; ++msg_const_iter) {
      verify(msg_const_iter->bb() == 1234);
    }

    // Test range-based for as well, but only if compiled as C++11.
// #if __cplusplus >= 201103L
    // Test one primitive field.
    for (auto& x : *message.MutableRepeatedExtension(
      google::protobuf::unittest::repeated_int32_extension)) {
      x = 4321;
    }
    for (const auto& x : message.GetRepeatedExtension(
      google::protobuf::unittest::repeated_int32_extension)) {
      verify(x == 4321);
    }
    // Test one fun::string field.
    for (auto& x : *message.MutableRepeatedExtension(
      google::protobuf::unittest::repeated_string_extension)) {
      x = "test_range_based_for";
    }
    for (const auto& x : message.GetRepeatedExtension(
      google::protobuf::unittest::repeated_string_extension)) {
      verify(x == "test_range_based_for");
    }
    // Test one message field.
    for (auto& x : *message.MutableRepeatedExtension(
      google::protobuf::unittest::repeated_nested_message_extension)) {
      x.set_bb(4321);
    }
    for (const auto& x : *message.MutableRepeatedExtension(
      google::protobuf::unittest::repeated_nested_message_extension)) {
      verify(x.bb() == 4321);
    }
// #endif
  }

  // From b/12926163
  // ExtensionSetTest, AbsentExtension
  {
    google::protobuf::unittest::TestAllExtensions message;
    message.MutableRepeatedExtension(google::protobuf::unittest::repeated_nested_message_extension)
      ->Add()->set_bb(123);
    verify(1 == message.ExtensionSize(
      google::protobuf::unittest::repeated_nested_message_extension));
    verify(
      123 == message.GetExtension(
        google::protobuf::unittest::repeated_nested_message_extension, 0).bb());
  }

  // ExtensionSetTest, DynamicExtensions
  {
    // Test adding a dynamic extension to a compiled-in message object.

    google::protobuf::FileDescriptorProto dynamic_proto;
    dynamic_proto.set_name("dynamic_extensions_test.proto");
    dynamic_proto.add_dependency(
      google::protobuf::unittest::TestAllExtensions::descriptor()->file()->name());
    dynamic_proto.set_package("dynamic_extensions");

    // Copy the fields and nested types from TestDynamicExtensions into our new
    // proto, converting the fields into extensions.
    const google::protobuf::Descriptor* template_descriptor =
      google::protobuf::unittest::TestDynamicExtensions::descriptor();
    google::protobuf::DescriptorProto template_descriptor_proto;
    template_descriptor->CopyTo(&template_descriptor_proto);
    dynamic_proto.mutable_message_type()->MergeFrom(
      template_descriptor_proto.nested_type());
    dynamic_proto.mutable_enum_type()->MergeFrom(
      template_descriptor_proto.enum_type());
    dynamic_proto.mutable_extension()->MergeFrom(
      template_descriptor_proto.field());

    // For each extension that we added...
    for (int i = 0; i < dynamic_proto.extension_size(); i++) {
      // Set its extendee to TestAllExtensions.
      google::protobuf::FieldDescriptorProto* extension = dynamic_proto.mutable_extension(i);
      extension->set_extendee(
        google::protobuf::unittest::TestAllExtensions::descriptor()->full_name());

      // If the field refers to one of the types nested in TestDynamicExtensions,
      // make it refer to the type in our dynamic proto instead.
      fun::string prefix = "." + template_descriptor->full_name() + ".";
      if (extension->has_type_name()) {
        fun::string* type_name = extension->mutable_type_name();
        if (google::protobuf::HasPrefixString(*type_name, prefix)) {
          type_name->replace(0, prefix.size(), ".dynamic_extensions.");
        }
      }
    }

    // Now build the file, using the generated pool as an underlay.
    google::protobuf::DescriptorPool dynamic_pool(google::protobuf::DescriptorPool::generated_pool());
    const google::protobuf::FileDescriptor* file = dynamic_pool.BuildFile(dynamic_proto);
    verify(file != NULL);
    google::protobuf::DynamicMessageFactory dynamic_factory(&dynamic_pool);
    dynamic_factory.SetDelegateToGeneratedFactory(true);

    // Construct a message that we can parse with the extensions we defined.
    // Since the extensions were based off of the fields of TestDynamicExtensions,
    // we can use that message to create this test message.
    fun::string data;
    {
      google::protobuf::unittest::TestDynamicExtensions message;
      message.set_scalar_extension(123);
      message.set_enum_extension(google::protobuf::unittest::FOREIGN_BAR);
      message.set_dynamic_enum_extension(
        google::protobuf::unittest::TestDynamicExtensions::DYNAMIC_BAZ);
      message.mutable_message_extension()->set_c(456);
      message.mutable_dynamic_message_extension()->set_dynamic_field(789);
      message.add_repeated_extension("foo");
      message.add_repeated_extension("bar");
      message.add_packed_extension(12);
      message.add_packed_extension(-34);
      message.add_packed_extension(56);
      message.add_packed_extension(-78);

      // Also add some unknown fields.

      // An unknown enum value (for a known field).
      message.mutable_unknown_fields()->AddVarint(
        google::protobuf::unittest::TestDynamicExtensions::kDynamicEnumExtensionFieldNumber,
        12345);
      // A regular unknown field.
      message.mutable_unknown_fields()->AddLengthDelimited(54321, "unknown");

      message.SerializeToString(&data);
    }

    // Now we can parse this using our dynamic extension definitions...
    google::protobuf::unittest::TestAllExtensions message;
    {
      google::protobuf::io::ArrayInputStream raw_input(data.data(), data.size());
      google::protobuf::io::CodedInputStream input(&raw_input);
      input.SetExtensionRegistry(&dynamic_pool, &dynamic_factory);
      verify(message.ParseFromCodedStream(&input));
      verify(input.ConsumedEntireMessage());
    }

    // Can we print it?
    verify(
      "[dynamic_extensions.scalar_extension]: 123\n"
      "[dynamic_extensions.enum_extension]: FOREIGN_BAR\n"
      "[dynamic_extensions.dynamic_enum_extension]: DYNAMIC_BAZ\n"
      "[dynamic_extensions.message_extension] {\n"
      "  c: 456\n"
      "}\n"
      "[dynamic_extensions.dynamic_message_extension] {\n"
      "  dynamic_field: 789\n"
      "}\n"
      "[dynamic_extensions.repeated_extension]: \"foo\"\n"
      "[dynamic_extensions.repeated_extension]: \"bar\"\n"
      "[dynamic_extensions.packed_extension]: 12\n"
      "[dynamic_extensions.packed_extension]: -34\n"
      "[dynamic_extensions.packed_extension]: 56\n"
      "[dynamic_extensions.packed_extension]: -78\n"
      "2002: 12345\n"
      "54321: \"unknown\"\n" ==
      message.DebugString());

    // Can we serialize it?
    // (Don't use EXPECT_EQ because we don't want to dump raw binary data to the
    // terminal on failure.)
    verify(message.SerializeAsString() == data);

    // What if we parse using the reflection-based parser?
    {
      google::protobuf::unittest::TestAllExtensions message2;
      google::protobuf::io::ArrayInputStream raw_input(data.data(), data.size());
      google::protobuf::io::CodedInputStream input(&raw_input);
      input.SetExtensionRegistry(&dynamic_pool, &dynamic_factory);
      verify(google::protobuf::internal::WireFormat::ParseAndMergePartial(&input, &message2));
      verify(input.ConsumedEntireMessage());
      verify(message.DebugString() == message2.DebugString());
    }

    // Are the embedded generated types actually using the generated objects?
    {
      const google::protobuf::FieldDescriptor* message_extension =
        file->FindExtensionByName("message_extension");
      verify(message_extension != NULL);
      const google::protobuf::Message& sub_message =
        message.GetReflection()->GetMessage(message, message_extension);
      const google::protobuf::unittest::ForeignMessage* typed_sub_message =
#ifdef GOOGLE_PROTOBUF_NO_RTTI
        static_cast<const google::protobuf::unittest::ForeignMessage*>(&sub_message);
#else
        dynamic_cast<const google::protobuf::unittest::ForeignMessage*>(&sub_message);
#endif
      verify(typed_sub_message != NULL);
      verify(456 == typed_sub_message->c());
    }

    // What does GetMessage() return for the embedded dynamic type if it isn't
    // present?
    {
      const google::protobuf::FieldDescriptor* dynamic_message_extension =
        file->FindExtensionByName("dynamic_message_extension");
      verify(dynamic_message_extension != NULL);
      const google::protobuf::Message& parent = google::protobuf::unittest::TestAllExtensions::default_instance();
      const google::protobuf::Message& sub_message =
        parent.GetReflection()->GetMessage(parent, dynamic_message_extension,
          &dynamic_factory);
      const google::protobuf::Message* prototype =
        dynamic_factory.GetPrototype(dynamic_message_extension->message_type());
      verify(prototype == &sub_message);
    }
  }

  return true;
}

#endif