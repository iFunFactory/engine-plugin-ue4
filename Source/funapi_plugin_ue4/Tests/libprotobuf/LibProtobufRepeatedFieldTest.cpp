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

#include <algorithm>
#include <limits>
#include <list>
#include <vector>

#include <google/protobuf/repeated_field.h>

#include <google/protobuf/stubs/common.h>
#include "unittest.pb.h"
#include <google/protobuf/stubs/strutil.h>
#include "googletest.h"
// #include <gtest/gtest.h>
#include <google/protobuf/stubs/stl_util.h>

// Determines how much space was reserved by the given field by adding elements
// to it until it re-allocates its space.
int ReservedSpace(google::protobuf::RepeatedField<int>* field) {
  const int* ptr = field->data();
  do {
    field->Add(0);
  } while (field->data() == ptr);

  return field->size() - 1;
};

int ReservedSpace(google::protobuf::RepeatedPtrField<std::string>* field) {
  const std::string* const* ptr = field->data();
  do {
    field->Add();
  } while (field->data() == ptr);

  return field->size() - 1;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFunapiLibProtobufRepeatedFieldTest, "LibProtobuf.RepeatedFieldTest", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFunapiLibProtobufRepeatedFieldTest::RunTest(const FString& Parameters)
{
  using protobuf_unittest::TestAllTypes;

  // Test operations on a small RepeatedField.
  // TEST(RepeatedField, Small)
  {
    google::protobuf::RepeatedField<int> field;

    verify(field.empty());
    verify(field.size() == 0);

    field.Add(5);

    verify(false == field.empty());
    verify(field.size() == 1);
    verify(field.Get(0) == 5);

    field.Add(42);

    verify(false == field.empty());
    verify(field.size() == 2);
    verify(field.Get(0) == 5);
    verify(field.Get(1) == 42);

    field.Set(1, 23);

    verify(false == field.empty());
    verify(field.size() == 2);
    verify(field.Get(0) == 5);
    verify(field.Get(1) == 23);

    field.RemoveLast();

    verify(false == field.empty());
    verify(field.size() == 1);
    verify(field.Get(0) == 5);

    field.Clear();

    verify(field.empty());
    verify(field.size() == 0);
    int expected_usage = 4 * sizeof(int);
    verify(field.SpaceUsedExcludingSelf() == expected_usage);
  }


  // Test operations on a RepeatedField which is large enough to allocate a
  // separate array.
  // TEST(RepeatedField, Large)
  {
    google::protobuf::RepeatedField<int> field;

    for (int i = 0; i < 16; i++) {
      field.Add(i * i);
    }

    verify(false == field.empty());
    verify(field.size() == 16);

    for (int i = 0; i < 16; i++) {
      verify(field.Get(i) == i * i);
    }

    int expected_usage = 16 * sizeof(int);
    verify(field.SpaceUsedExcludingSelf() >= expected_usage);
  }

  // Test swapping between various types of RepeatedFields.
  // TEST(RepeatedField, SwapSmallSmall)
  {
    google::protobuf::RepeatedField<int> field1;
    google::protobuf::RepeatedField<int> field2;

    field1.Add(5);
    field1.Add(42);

    verify(false == field1.empty());
    verify(field1.size() == 2);
    verify(field1.Get(0) == 5);
    verify(field1.Get(1) == 42);

    verify(field2.empty());
    verify(field2.size() == 0);

    field1.Swap(&field2);

    verify(field1.empty());
    verify(field1.size() == 0);

    verify(false == field2.empty());
    verify(field2.size() == 2);
    verify(field2.Get(0) == 5);
    verify(field2.Get(1) == 42);
  }

  // TEST(RepeatedField, SwapLargeSmall)
  {
    google::protobuf::RepeatedField<int> field1;
    google::protobuf::RepeatedField<int> field2;

    for (int i = 0; i < 16; i++) {
      field1.Add(i * i);
    }
    field2.Add(5);
    field2.Add(42);
    field1.Swap(&field2);

    verify(field1.size() == 2);
    verify(field1.Get(0) == 5);
    verify(field1.Get(1) == 42);
    verify(field2.size() == 16);
    for (int i = 0; i < 16; i++) {
      verify(field2.Get(i) == i * i);
    }
  }

  // TEST(RepeatedField, SwapLargeLarge)
  {
    google::protobuf::RepeatedField<int> field1;
    google::protobuf::RepeatedField<int> field2;

    field1.Add(5);
    field1.Add(42);
    for (int i = 0; i < 16; i++) {
      field1.Add(i);
      field2.Add(i * i);
    }
    field2.Swap(&field1);

    verify(field1.size() == 16);
    for (int i = 0; i < 16; i++) {
      verify(field1.Get(i) == i * i);
    }
    verify(field2.size() == 18);
    verify(field2.Get(0) == 5);
    verify(field2.Get(1) == 42);
    for (int i = 2; i < 18; i++) {
      verify(field2.Get(i) == i - 2);
    }
  }

  // TEST(RepeatedField, ReserveMoreThanDouble)
  {
    // Reserve more than double the previous space in the field and expect the
    // field to reserve exactly the amount specified.
    google::protobuf::RepeatedField<int> field;
    field.Reserve(20);

    verify(20 == ReservedSpace(&field));
  }

  // TEST(RepeatedField, ReserveLessThanDouble)
  {
    // Reserve less than double the previous space in the field and expect the
    // field to grow by double instead.
    google::protobuf::RepeatedField<int> field;
    field.Reserve(20);
    field.Reserve(30);

    verify(40 == ReservedSpace(&field));
  }

  // TEST(RepeatedField, ReserveLessThanExisting)
  {
    // Reserve less than the previous space in the field and expect the
    // field to not re-allocate at all.
    google::protobuf::RepeatedField<int> field;
    field.Reserve(20);
    const int* previous_ptr = field.data();
    field.Reserve(10);

    verify(previous_ptr == field.data());
    verify(20 == ReservedSpace(&field));
  }

  // TEST(RepeatedField, Resize)
  {
    google::protobuf::RepeatedField<int> field;
    field.Resize(2, 1);
    verify(2 == field.size());
    field.Resize(5, 2);
    verify(5 == field.size());
    field.Resize(4, 3);
    verify(4 == field.size());
    verify(1 == field.Get(0));
    verify(1 == field.Get(1));
    verify(2 == field.Get(2));
    verify(2 == field.Get(3));
    field.Resize(0, 4);
    verify(field.empty());
  }

  // TEST(RepeatedField, MergeFrom)
  {
    google::protobuf::RepeatedField<int> source, destination;
    source.Add(4);
    source.Add(5);
    destination.Add(1);
    destination.Add(2);
    destination.Add(3);

    destination.MergeFrom(source);

    verify(5 == destination.size());
    verify(1 == destination.Get(0));
    verify(2 == destination.Get(1));
    verify(3 == destination.Get(2));
    verify(4 == destination.Get(3));
    verify(5 == destination.Get(4));
  }

#ifdef PROTOBUF_HAS_DEATH_TEST
  TEST(RepeatedField, MergeFromSelf) {
    RepeatedField<int> me;
    me.Add(3);
    EXPECT_DEATH(me.MergeFrom(me), "");
  }
#endif  // PROTOBUF_HAS_DEATH_TEST

  // TEST(RepeatedField, CopyFrom)
  {
    google::protobuf::RepeatedField<int> source, destination;
    source.Add(4);
    source.Add(5);
    destination.Add(1);
    destination.Add(2);
    destination.Add(3);

    destination.CopyFrom(source);

    verify(2 == destination.size());
    verify(4 == destination.Get(0));
    verify(5 == destination.Get(1));
  }

  // TEST(RepeatedField, CopyFromSelf)
  {
    google::protobuf::RepeatedField<int> me;
    me.Add(3);
    me.CopyFrom(me);
    verify(1 == me.size());
    verify(3 == me.Get(0));
  }

  // TEST(RepeatedField, CopyConstruct)
  {
    google::protobuf::RepeatedField<int> source;
    source.Add(1);
    source.Add(2);

    google::protobuf::RepeatedField<int> destination(source);

    verify(2 == destination.size());
    verify(1 == destination.Get(0));
    verify(2 == destination.Get(1));
  }

  // TEST(RepeatedField, IteratorConstruct)
  {
    std::vector<int> values;
    values.push_back(1);
    values.push_back(2);

    google::protobuf::RepeatedField<int> field(values.begin(), values.end());
    verify(values.size() == field.size());
    verify(values[0] == field.Get(0));
    verify(values[1] == field.Get(1));

    google::protobuf::RepeatedField<int> other(field.begin(), field.end());
    verify(values.size() == other.size());
    verify(values[0] == other.Get(0));
    verify(values[1] == other.Get(1));
  }

  // TEST(RepeatedField, CopyAssign)
  {
    google::protobuf::RepeatedField<int> source, destination;
    source.Add(4);
    source.Add(5);
    destination.Add(1);
    destination.Add(2);
    destination.Add(3);

    destination = source;

    verify(2 == destination.size());
    verify(4 == destination.Get(0));
    verify(5 == destination.Get(1));
  }

  // TEST(RepeatedField, SelfAssign)
  {
    // Verify that assignment to self does not destroy data.
    google::protobuf::RepeatedField<int> source, *p;
    p = &source;
    source.Add(7);
    source.Add(8);

    *p = source;

    verify(2 == source.size());
    verify(7 == source.Get(0));
    verify(8 == source.Get(1));
  }

  // TEST(RepeatedField, MutableDataIsMutable)
  {
    google::protobuf::RepeatedField<int> field;
    field.Add(1);
    verify(1 == field.Get(0));
    // The fact that this line compiles would be enough, but we'll check the
    // value anyway.
    *field.mutable_data() = 2;
    verify(2 == field.Get(0));
  }

  // TEST(RepeatedField, Truncate)
  {
    google::protobuf::RepeatedField<int> field;

    field.Add(12);
    field.Add(34);
    field.Add(56);
    field.Add(78);
    verify(4 == field.size());

    field.Truncate(3);
    verify(3 == field.size());

    field.Add(90);
    verify(4 == field.size());
    verify(90 == field.Get(3));

    // Truncations that don't change the size are allowed, but growing is not
    // allowed.
    field.Truncate(field.size());
  }


  // TEST(RepeatedField, ExtractSubrange)
  {
    // Exhaustively test every subrange in arrays of all sizes from 0 through 9.
    for (int sz = 0; sz < 10; ++sz) {
      for (int num = 0; num <= sz; ++num) {
        for (int start = 0; start < sz - num; ++start) {
          // Create RepeatedField with sz elements having values 0 through sz-1.
          google::protobuf::RepeatedField<int32> field;
          for (int i = 0; i < sz; ++i)
            field.Add(i);
          verify(field.size() == sz);

          // Create a catcher array and call ExtractSubrange.
          int32 catcher[10];
          for (int i = 0; i < 10; ++i)
            catcher[i] = -1;
          field.ExtractSubrange(start, num, catcher);

          // Does the resulting array have the right size?
          verify(field.size() == sz - num);

          // Were the removed elements extracted into the catcher array?
          for (int i = 0; i < num; ++i)
            verify(catcher[i] == start + i);
          verify(catcher[num] == -1);

          // Does the resulting array contain the right values?
          for (int i = 0; i < start; ++i)
            verify(field.Get(i) == i);
          for (int i = start; i < field.size(); ++i)
            verify(field.Get(i) == i + num);
        }
      }
    }
  }

  // ===================================================================
  // RepeatedPtrField tests.  These pretty much just mirror the RepeatedField
  // tests above.

  // TEST(RepeatedPtrField, Small)
  {
    google::protobuf::RepeatedPtrField<std::string> field;

    verify(field.empty());
    verify(field.size() == 0);

    field.Add()->assign("foo");

    verify(false == field.empty());
    verify(field.size() == 1);
    verify(field.Get(0) == "foo");

    field.Add()->assign("bar");

    verify(false == field.empty());
    verify(field.size() == 2);
    verify(field.Get(0) == "foo");
    verify(field.Get(1) == "bar");

    field.Mutable(1)->assign("baz");

    verify(false == field.empty());
    verify(field.size() == 2);
    verify(field.Get(0) == "foo");
    verify(field.Get(1) == "baz");

    field.RemoveLast();

    verify(false == field.empty());
    verify(field.size() == 1);
    verify(field.Get(0) == "foo");

    field.Clear();

    verify(field.empty());
    verify(field.size() == 0);
  }

  // TEST(RepeatedPtrField, Large)
  {
    google::protobuf::RepeatedPtrField<std::string> field;

    for (int i = 0; i < 16; i++) {
      *field.Add() += 'a' + i;
    }

    verify(field.size() == 16);

    for (int i = 0; i < 16; i++) {
      verify(field.Get(i).size() == 1);
      verify(field.Get(i)[0] == 'a' + i);
    }

    int min_expected_usage = 16 * sizeof(std::string);
    verify(field.SpaceUsedExcludingSelf() >= min_expected_usage);
  }

  // TEST(RepeatedPtrField, SwapSmallSmall)
  {
    google::protobuf::RepeatedPtrField<std::string> field1;
    google::protobuf::RepeatedPtrField<std::string> field2;

    verify(field1.empty());
    verify(field1.size() == 0);
    verify(field2.empty());
    verify(field2.size() == 0);

    field1.Add()->assign("foo");
    field1.Add()->assign("bar");

    verify(false == field1.empty());
    verify(field1.size() == 2);
    verify(field1.Get(0) == "foo");
    verify(field1.Get(1) == "bar");

    verify(field2.empty());
    verify(field2.size() == 0);

    field1.Swap(&field2);

    verify(field1.empty());
    verify(field1.size() == 0);

    verify(field2.size() == 2);
    verify(field2.Get(0) == "foo");
    verify(field2.Get(1) == "bar");
  }

  // TEST(RepeatedPtrField, SwapLargeSmall)
  {
    google::protobuf::RepeatedPtrField<std::string> field1;
    google::protobuf::RepeatedPtrField<std::string> field2;

    field2.Add()->assign("foo");
    field2.Add()->assign("bar");
    for (int i = 0; i < 16; i++) {
      *field1.Add() += 'a' + i;
    }
    field1.Swap(&field2);

    verify(field1.size() == 2);
    verify(field1.Get(0) == "foo");
    verify(field1.Get(1) == "bar");
    verify(field2.size() == 16);
    for (int i = 0; i < 16; i++) {
      verify(field2.Get(i).size() == 1);
      verify(field2.Get(i)[0] == 'a' + i);
    }
  }

  // TEST(RepeatedPtrField, SwapLargeLarge)
  {
    google::protobuf::RepeatedPtrField<std::string> field1;
    google::protobuf::RepeatedPtrField<std::string> field2;

    field1.Add()->assign("foo");
    field1.Add()->assign("bar");
    for (int i = 0; i < 16; i++) {
      *field1.Add() += 'A' + i;
      *field2.Add() += 'a' + i;
    }
    field2.Swap(&field1);

    verify(field1.size() == 16);
    for (int i = 0; i < 16; i++) {
      verify(field1.Get(i).size() == 1);
      verify(field1.Get(i)[0] == 'a' + i);
    }
    verify(field2.size() == 18);
    verify(field2.Get(0) == "foo");
    verify(field2.Get(1) == "bar");
    for (int i = 2; i < 18; i++) {
      verify(field2.Get(i).size() == 1);
      verify(field2.Get(i)[0] == 'A' + i - 2);
    }
  }

  // TEST(RepeatedPtrField, ReserveMoreThanDouble)
  {
    google::protobuf::RepeatedPtrField<std::string> field;
    field.Reserve(20);

    verify(20 == ReservedSpace(&field));
  }

  // TEST(RepeatedPtrField, ReserveLessThanDouble)
  {
    google::protobuf::RepeatedPtrField<std::string> field;
    field.Reserve(20);
    field.Reserve(30);

    verify(40 == ReservedSpace(&field));
  }

  // TEST(RepeatedPtrField, ReserveLessThanExisting)
  {
    google::protobuf::RepeatedPtrField<std::string> field;
    field.Reserve(20);
    const std::string* const* previous_ptr = field.data();
    field.Reserve(10);

    verify(previous_ptr == field.data());
    verify(20 == ReservedSpace(&field));
  }

  // TEST(RepeatedPtrField, ReserveDoesntLoseAllocated)
  {
    // Check that a bug is fixed:  An earlier implementation of Reserve()
    // failed to copy pointers to allocated-but-cleared objects, possibly
    // leading to segfaults.
    google::protobuf::RepeatedPtrField<std::string> field;
    std::string* first = field.Add();
    field.RemoveLast();

    field.Reserve(20);
    verify(first == field.Add());
  }

  // Clearing elements is tricky with RepeatedPtrFields since the memory for
  // the elements is retained and reused.
  // TEST(RepeatedPtrField, ClearedElements)
  {
    google::protobuf::RepeatedPtrField<std::string> field;

    std::string* original = field.Add();
    *original = "foo";

    verify(field.ClearedCount() == 0);

    field.RemoveLast();
    verify(original->empty());
    verify(field.ClearedCount() == 1);

    verify(field.Add() == original);  // Should return same string for reuse.

    verify(field.ReleaseLast() == original);  // We take ownership.
    verify(field.ClearedCount() == 0);

    verify(field.Add() != original);  // Should NOT return the same string.
    verify(field.ClearedCount() == 0);

    field.AddAllocated(original);  // Give ownership back.
    verify(field.ClearedCount() == 0);
    verify(field.Mutable(1) == original);

    field.Clear();
    verify(field.ClearedCount() == 2);
    verify(field.ReleaseCleared() == original);  // Take ownership again.
    verify(field.ClearedCount() == 1);
    verify(field.Add() != original);
    verify(field.ClearedCount() == 0);
    verify(field.Add() != original);
    verify(field.ClearedCount() == 0);

    field.AddCleared(original);  // Give ownership back, but as a cleared object.
    verify(field.ClearedCount() == 1);
    verify(field.Add() == original);
    verify(field.ClearedCount() == 0);
  }

  // Test all code paths in AddAllocated().
  // TEST(RepeatedPtrField, AddAlocated)
  {
    google::protobuf::RepeatedPtrField<std::string> field;
    while (field.size() < field.Capacity()) {
      field.Add()->assign("filler");
    }

    int index = field.size();

    // First branch:  Field is at capacity with no cleared objects.
    std::string* foo = new std::string("foo");
    field.AddAllocated(foo);
    verify(index + 1 == field.size());
    verify(0 == field.ClearedCount());
    verify(foo == &field.Get(index));

    // Last branch:  Field is not at capacity and there are no cleared objects.
    std::string* bar = new std::string("bar");
    field.AddAllocated(bar);
    ++index;
    verify(index + 1 == field.size());
    verify(0 == field.ClearedCount());
    verify(bar == &field.Get(index));

    // Third branch:  Field is not at capacity and there are no cleared objects.
    field.RemoveLast();
    std::string* baz = new std::string("baz");
    field.AddAllocated(baz);
    verify(index + 1 == field.size());
    verify(1 == field.ClearedCount());
    verify(baz == &field.Get(index));

    // Second branch:  Field is at capacity but has some cleared objects.
    while (field.size() < field.Capacity()) {
      field.Add()->assign("filler2");
    }
    field.RemoveLast();
    index = field.size();
    std::string* qux = new std::string("qux");
    field.AddAllocated(qux);
    verify(index + 1 == field.size());
    // We should have discarded the cleared object.
    verify(0 == field.ClearedCount());
    verify(qux == &field.Get(index));
  }

  // TEST(RepeatedPtrField, MergeFrom)
  {
    google::protobuf::RepeatedPtrField<std::string> source, destination;
    source.Add()->assign("4");
    source.Add()->assign("5");
    destination.Add()->assign("1");
    destination.Add()->assign("2");
    destination.Add()->assign("3");

    destination.MergeFrom(source);

    verify(5 == destination.size());
    verify("1" == destination.Get(0));
    verify("2" == destination.Get(1));
    verify("3" == destination.Get(2));
    verify("4" == destination.Get(3));
    verify("5" == destination.Get(4));
  }

  // TEST(RepeatedPtrField, CopyFrom)
  {
    google::protobuf::RepeatedPtrField<std::string> source, destination;
    source.Add()->assign("4");
    source.Add()->assign("5");
    destination.Add()->assign("1");
    destination.Add()->assign("2");
    destination.Add()->assign("3");

    destination.CopyFrom(source);

    verify(2 == destination.size());
    verify("4" == destination.Get(0));
    verify("5" == destination.Get(1));
  }

  // TEST(RepeatedPtrField, CopyFromSelf)
  {
    google::protobuf::RepeatedPtrField<std::string> me;
    me.Add()->assign("1");
    me.CopyFrom(me);
    verify(1 == me.size());
    verify("1" == me.Get(0));
  }

  // TEST(RepeatedPtrField, CopyConstruct)
  {
    google::protobuf::RepeatedPtrField<std::string> source;
    source.Add()->assign("1");
    source.Add()->assign("2");

    google::protobuf::RepeatedPtrField<std::string> destination(source);

    verify(2 == destination.size());
    verify("1" == destination.Get(0));
    verify("2" == destination.Get(1));
  }

  // TEST(RepeatedPtrField, IteratorConstruct_String)
  {
    std::vector<std::string> values;
    values.push_back("1");
    values.push_back("2");

    google::protobuf::RepeatedPtrField<std::string> field(values.begin(), values.end());
    verify(values.size() == field.size());
    verify(values[0] == field.Get(0));
    verify(values[1] == field.Get(1));

    google::protobuf::RepeatedPtrField<std::string> other(field.begin(), field.end());
    verify(values.size() == other.size());
    verify(values[0] == other.Get(0));
    verify(values[1] == other.Get(1));
  }

  // TEST(RepeatedPtrField, IteratorConstruct_Proto)
  {
    typedef TestAllTypes::NestedMessage Nested;
    std::vector<Nested> values;
    values.push_back(Nested());
    values.back().set_bb(1);
    values.push_back(Nested());
    values.back().set_bb(2);

    google::protobuf::RepeatedPtrField<Nested> field(values.begin(), values.end());
    verify(values.size() == field.size());
    verify(values[0].bb() == field.Get(0).bb());
    verify(values[1].bb() == field.Get(1).bb());

    google::protobuf::RepeatedPtrField<Nested> other(field.begin(), field.end());
    verify(values.size() == other.size());
    verify(values[0].bb() == other.Get(0).bb());
    verify(values[1].bb() == other.Get(1).bb());
  }

  // TEST(RepeatedPtrField, CopyAssign)
  {
    google::protobuf::RepeatedPtrField<std::string> source, destination;
    source.Add()->assign("4");
    source.Add()->assign("5");
    destination.Add()->assign("1");
    destination.Add()->assign("2");
    destination.Add()->assign("3");

    destination = source;

    verify(2 == destination.size());
    verify("4" == destination.Get(0));
    verify("5" == destination.Get(1));
  }

  // TEST(RepeatedPtrField, SelfAssign)
  {
    // Verify that assignment to self does not destroy data.
    google::protobuf::RepeatedPtrField<std::string> source, *p;
    p = &source;
    source.Add()->assign("7");
    source.Add()->assign("8");

    *p = source;

    verify(2 == source.size());
    verify("7" == source.Get(0));
    verify("8" == source.Get(1));
  }

  // TEST(RepeatedPtrField, MutableDataIsMutable)
  {
    google::protobuf::RepeatedPtrField<std::string> field;
    *field.Add() = "1";
    verify("1" == field.Get(0));
    // The fact that this line compiles would be enough, but we'll check the
    // value anyway.
    std::string** data = field.mutable_data();
    **data = "2";
    verify("2" == field.Get(0));
  }

  // TEST(RepeatedPtrField, ExtractSubrange)
  {
    // Exhaustively test every subrange in arrays of all sizes from 0 through 9
    // with 0 through 3 cleared elements at the end.
    for (int sz = 0; sz < 10; ++sz) {
      for (int num = 0; num <= sz; ++num) {
        for (int start = 0; start < sz - num; ++start) {
          for (int extra = 0; extra < 4; ++extra) {
            std::vector<std::string*> subject;

            // Create an array with "sz" elements and "extra" cleared elements.
            google::protobuf::RepeatedPtrField<std::string> field;
            for (int i = 0; i < sz + extra; ++i) {
              subject.push_back(new std::string());
              field.AddAllocated(subject[i]);
            }
            verify(field.size() == sz + extra);
            for (int i = 0; i < extra; ++i)
              field.RemoveLast();
            verify(field.size() == sz);
            verify(field.ClearedCount() == extra);

            // Create a catcher array and call ExtractSubrange.
            std::string* catcher[10];
            for (int i = 0; i < 10; ++i)
              catcher[i] = NULL;
            field.ExtractSubrange(start, num, catcher);

            // Does the resulting array have the right size?
            verify(field.size() == sz - num);

            // Were the removed elements extracted into the catcher array?
            for (int i = 0; i < num; ++i)
              verify(catcher[i] == subject[start + i]);
            verify(NULL == catcher[num]);

            // Does the resulting array contain the right values?
            for (int i = 0; i < start; ++i)
              verify(field.Mutable(i) == subject[i]);
            for (int i = start; i < field.size(); ++i)
              verify(field.Mutable(i) == subject[i + num]);

            // Reinstate the cleared elements.
            verify(field.ClearedCount() == extra);
            for (int i = 0; i < extra; ++i)
              field.Add();
            verify(field.ClearedCount() == 0);
            verify(field.size() == sz - num + extra);

            // Make sure the extra elements are all there (in some order).
            for (int i = sz; i < sz + extra; ++i) {
              int count = 0;
              for (int j = sz; j < sz + extra; ++j) {
                if (field.Mutable(j - num) == subject[i])
                  count += 1;
              }
              verify(count == 1);
            }

            // Release the caught elements.
            for (int i = 0; i < num; ++i)
              delete catcher[i];
          }
        }
      }
    }
  }

  // TEST(RepeatedPtrField, DeleteSubrange)
  {
    // DeleteSubrange is a trivial extension of ExtendSubrange.
  }

  // ===================================================================
  // RepeatedFieldIteratorTest

  // Iterator tests stolen from net/proto/proto-array_unittest.
  {
    google::protobuf::RepeatedField<int> proto_array_;

    // SetUp()
    {
      for (int i = 0; i < 3; ++i) {
        proto_array_.Add(i);
      }
    }

    // TEST_F(RepeatedFieldIteratorTest, Convertible)
    {
      google::protobuf::RepeatedField<int>::iterator iter = proto_array_.begin();
      google::protobuf::RepeatedField<int>::const_iterator c_iter = iter;
      google::protobuf::RepeatedField<int>::value_type value = *c_iter;
      verify(0 == value);
    }

    // TEST_F(RepeatedFieldIteratorTest, MutableIteration)
    {
      google::protobuf::RepeatedField<int>::iterator iter = proto_array_.begin();
      verify(0 == *iter);
      ++iter;
      verify(1 == *iter++);
      verify(2 == *iter);
      ++iter;
      verify(proto_array_.end() == iter);

      verify(2 == *(proto_array_.end() - 1));
    }

    // TEST_F(RepeatedFieldIteratorTest, ConstIteration)
    {
      const google::protobuf::RepeatedField<int>& const_proto_array = proto_array_;
      google::protobuf::RepeatedField<int>::const_iterator iter = const_proto_array.begin();
      verify(0 == *iter);
      ++iter;
      verify(1 == *iter++);
      verify(2 == *iter);
      ++iter;
      verify(proto_array_.end() == iter);
      verify(2 == *(proto_array_.end() - 1));
    }

    // TEST_F(RepeatedFieldIteratorTest, Mutation)
    {
      google::protobuf::RepeatedField<int>::iterator iter = proto_array_.begin();
      *iter = 7;
      verify(7 == proto_array_.Get(0));
    }
  }

  // -------------------------------------------------------------------
  // RepeatedPtrFieldIteratorTest
  {
    google::protobuf::RepeatedPtrField<std::string> proto_array_;

    // SetUp()
    {
      proto_array_.Add()->assign("foo");
      proto_array_.Add()->assign("bar");
      proto_array_.Add()->assign("baz");
    }

    // TEST_F(RepeatedPtrFieldIteratorTest, Convertible)
    {
      google::protobuf::RepeatedPtrField<std::string>::iterator iter = proto_array_.begin();
      google::protobuf::RepeatedPtrField<std::string>::const_iterator c_iter = iter;
      google::protobuf::RepeatedPtrField<std::string>::value_type value = *c_iter;
      verify("foo" == value);
    }

    // TEST_F(RepeatedPtrFieldIteratorTest, MutableIteration)
    {
      google::protobuf::RepeatedPtrField<std::string>::iterator iter = proto_array_.begin();
      verify("foo" == *iter);
      ++iter;
      verify("bar" == *(iter++));
      verify("baz" == *iter);
      ++iter;
      verify(proto_array_.end() == iter);
      verify("baz" == *(--proto_array_.end()));
    }

    // TEST_F(RepeatedPtrFieldIteratorTest, ConstIteration)
    {
      const google::protobuf::RepeatedPtrField<std::string>& const_proto_array = proto_array_;
      google::protobuf::RepeatedPtrField<std::string>::const_iterator iter = const_proto_array.begin();
      verify("foo" == *iter);
      ++iter;
      verify("bar" == *(iter++));
      verify("baz" == *iter);
      ++iter;
      verify(const_proto_array.end() == iter);
      verify("baz" == *(--const_proto_array.end()));
    }

    // TEST_F(RepeatedPtrFieldIteratorTest, MutableReverseIteration)
    {
      google::protobuf::RepeatedPtrField<std::string>::reverse_iterator iter = proto_array_.rbegin();
      verify("baz" == *iter);
      ++iter;
      verify("bar" == *(iter++));
      verify("foo" == *iter);
      ++iter;
      verify(proto_array_.rend() == iter);
      verify("foo" == *(--proto_array_.rend()));
    }

    // TEST_F(RepeatedPtrFieldIteratorTest, ConstReverseIteration)
    {
      const google::protobuf::RepeatedPtrField<std::string>& const_proto_array = proto_array_;
      google::protobuf::RepeatedPtrField<std::string>::const_reverse_iterator iter
        = const_proto_array.rbegin();
      verify("baz" == *iter);
      ++iter;
      verify("bar" == *(iter++));
      verify("foo" == *iter);
      ++iter;
      verify(const_proto_array.rend() == iter);
      verify("foo" == *(--const_proto_array.rend()));
    }

    // TEST_F(RepeatedPtrFieldIteratorTest, RandomAccess)
    {
      google::protobuf::RepeatedPtrField<std::string>::iterator iter = proto_array_.begin();
      google::protobuf::RepeatedPtrField<std::string>::iterator iter2 = iter;
      ++iter2;
      ++iter2;
      verify(iter + 2 == iter2);
      verify(iter == iter2 - 2);
      verify("baz" == iter[2]);
      verify("baz" == *(iter + 2));
      verify(3 == proto_array_.end() - proto_array_.begin());
    }

    // TEST_F(RepeatedPtrFieldIteratorTest, Comparable)
    {
      google::protobuf::RepeatedPtrField<std::string>::const_iterator iter = proto_array_.begin();
      google::protobuf::RepeatedPtrField<std::string>::const_iterator iter2 = iter + 1;
      verify(iter == iter);
      verify(iter != iter2);
      verify(iter < iter2);
      verify(iter <= iter2);
      verify(iter <= iter);
      verify(iter2 > iter);
      verify(iter2 >= iter);
      verify(iter >= iter);
    }

    // Uninitialized iterator does not point to any of the RepeatedPtrField.
    // TEST_F(RepeatedPtrFieldIteratorTest, UninitializedIterator)
    {
      google::protobuf::RepeatedPtrField<std::string>::iterator iter;
      verify(iter != proto_array_.begin());
      verify(iter != proto_array_.begin() + 1);
      verify(iter != proto_array_.begin() + 2);
      verify(iter != proto_array_.begin() + 3);
      verify(iter != proto_array_.end());
    }

    // TEST_F(RepeatedPtrFieldIteratorTest, STLAlgorithms_lower_bound)
    {
      proto_array_.Clear();
      proto_array_.Add()->assign("a");
      proto_array_.Add()->assign("c");
      proto_array_.Add()->assign("d");
      proto_array_.Add()->assign("n");
      proto_array_.Add()->assign("p");
      proto_array_.Add()->assign("x");
      proto_array_.Add()->assign("y");

      std::string v = "f";
      google::protobuf::RepeatedPtrField <std::string> ::const_iterator it =
        lower_bound(proto_array_.begin(), proto_array_.end(), v);

      verify(*it == "n");
      verify(it == proto_array_.begin() + 3);
    }

    // TEST_F(RepeatedPtrFieldIteratorTest, Mutation)
    {
      google::protobuf::RepeatedPtrField<std::string>::iterator iter = proto_array_.begin();
      *iter = "qux";
      verify("qux" == proto_array_.Get(0));
    }
  }

  // -------------------------------------------------------------------
  // RepeatedPtrFieldPtrsIteratorTest
  {
    google::protobuf::RepeatedPtrField<std::string> proto_array_;
    const google::protobuf::RepeatedPtrField<std::string>* const_proto_array_;

    auto SetUp = [&proto_array_, &const_proto_array_]()
    {
      proto_array_.Clear();

      proto_array_.Add()->assign("foo");
      proto_array_.Add()->assign("bar");
      proto_array_.Add()->assign("baz");
      const_proto_array_ = &proto_array_;
    };

    // TEST_F(RepeatedPtrFieldPtrsIteratorTest, ConvertiblePtr)
    SetUp();
    {
      google::protobuf::RepeatedPtrField<std::string>::pointer_iterator iter =
        proto_array_.pointer_begin();
      static_cast<void>(iter);
    }

    // TEST_F(RepeatedPtrFieldPtrsIteratorTest, ConvertibleConstPtr)
    SetUp();
    {
      google::protobuf::RepeatedPtrField<std::string>::const_pointer_iterator iter =
        const_proto_array_->pointer_begin();
      static_cast<void>(iter);
    }

    // TEST_F(RepeatedPtrFieldPtrsIteratorTest, MutablePtrIteration)
    SetUp();
    {
      google::protobuf::RepeatedPtrField<std::string>::pointer_iterator iter =
        proto_array_.pointer_begin();
      verify("foo" == **iter);
      ++iter;
      verify("bar" == **(iter++));
      verify("baz" == **iter);
      ++iter;
      verify(proto_array_.pointer_end() == iter);
      verify("baz" == **(--proto_array_.pointer_end()));
    }

    // TEST_F(RepeatedPtrFieldPtrsIteratorTest, MutableConstPtrIteration)
    SetUp();
    {
      google::protobuf::RepeatedPtrField<std::string>::const_pointer_iterator iter =
        const_proto_array_->pointer_begin();
      verify("foo" == **iter);
      ++iter;
      verify("bar" == **(iter++));
      verify("baz" == **iter);
      ++iter;
      verify(const_proto_array_->pointer_end() == iter);
      verify("baz" == **(--const_proto_array_->pointer_end()));
    }

    // TEST_F(RepeatedPtrFieldPtrsIteratorTest, RandomPtrAccess)
    SetUp();
    {
      google::protobuf::RepeatedPtrField<std::string>::pointer_iterator iter =
        proto_array_.pointer_begin();
      google::protobuf::RepeatedPtrField<std::string>::pointer_iterator iter2 = iter;
      ++iter2;
      ++iter2;
      verify(iter + 2 == iter2);
      verify(iter == iter2 - 2);
      verify("baz" == *iter[2]);
      verify("baz" == **(iter + 2));
      verify(3 == proto_array_.end() - proto_array_.begin());
    }

    // TEST_F(RepeatedPtrFieldPtrsIteratorTest, RandomConstPtrAccess)
    SetUp();
    {
      google::protobuf::RepeatedPtrField<std::string>::const_pointer_iterator iter =
        const_proto_array_->pointer_begin();
      google::protobuf::RepeatedPtrField<std::string>::const_pointer_iterator iter2 = iter;
      ++iter2;
      ++iter2;
      verify(iter + 2 == iter2);
      verify(iter == iter2 - 2);
      verify("baz" == *iter[2]);
      verify("baz" == **(iter + 2));
      verify(3 == const_proto_array_->end() - const_proto_array_->begin());
    }

    // TEST_F(RepeatedPtrFieldPtrsIteratorTest, ComparablePtr)
    SetUp();
    {
      google::protobuf::RepeatedPtrField<std::string>::pointer_iterator iter =
        proto_array_.pointer_begin();
      google::protobuf::RepeatedPtrField<std::string>::pointer_iterator iter2 = iter + 1;
      verify(iter == iter);
      verify(iter != iter2);
      verify(iter < iter2);
      verify(iter <= iter2);
      verify(iter <= iter);
      verify(iter2 > iter);
      verify(iter2 >= iter);
      verify(iter >= iter);
    }

    // TEST_F(RepeatedPtrFieldPtrsIteratorTest, ComparableConstPtr)
    SetUp();
    {
      google::protobuf::RepeatedPtrField<std::string>::const_pointer_iterator iter =
        const_proto_array_->pointer_begin();
      google::protobuf::RepeatedPtrField<std::string>::const_pointer_iterator iter2 = iter + 1;
      verify(iter == iter);
      verify(iter != iter2);
      verify(iter < iter2);
      verify(iter <= iter2);
      verify(iter <= iter);
      verify(iter2 > iter);
      verify(iter2 >= iter);
      verify(iter >= iter);
    }

    // Uninitialized iterator does not point to any of the RepeatedPtrOverPtrs.
    // Dereferencing an uninitialized iterator crashes the process.
    // TEST_F(RepeatedPtrFieldPtrsIteratorTest, UninitializedPtrIterator)
    SetUp();
    {
      google::protobuf::RepeatedPtrField<std::string>::pointer_iterator iter;
      verify(iter != proto_array_.pointer_begin());
      verify(iter != proto_array_.pointer_begin() + 1);
      verify(iter != proto_array_.pointer_begin() + 2);
      verify(iter != proto_array_.pointer_begin() + 3);
      verify(iter != proto_array_.pointer_end());
    }

    // TEST_F(RepeatedPtrFieldPtrsIteratorTest, UninitializedConstPtrIterator)
    SetUp();
    {
      google::protobuf::RepeatedPtrField<std::string>::const_pointer_iterator iter;
      verify(iter != const_proto_array_->pointer_begin());
      verify(iter != const_proto_array_->pointer_begin() + 1);
      verify(iter != const_proto_array_->pointer_begin() + 2);
      verify(iter != const_proto_array_->pointer_begin() + 3);
      verify(iter != const_proto_array_->pointer_end());
    }

    // This comparison functor is required by the tests for RepeatedPtrOverPtrs.
    // They operate on strings and need to compare strings as strings in
    // any stl algorithm, even though the iterator returns a pointer to a string
    // - i.e. *iter has type string*.
    struct StringLessThan {
      bool operator()(const std::string* z, const std::string& y) {
        return *z < y;
      }
      bool operator()(const std::string* z, const std::string* y) const { return *z < *y; }
    };

    // TEST_F(RepeatedPtrFieldPtrsIteratorTest, PtrSTLAlgorithms_lower_bound)
    SetUp();
    {
      proto_array_.Clear();
      proto_array_.Add()->assign("a");
      proto_array_.Add()->assign("c");
      proto_array_.Add()->assign("d");
      proto_array_.Add()->assign("n");
      proto_array_.Add()->assign("p");
      proto_array_.Add()->assign("x");
      proto_array_.Add()->assign("y");

      {
        std::string v = "f";
        google::protobuf::RepeatedPtrField<std::string>::pointer_iterator it =
          lower_bound(proto_array_.pointer_begin(), proto_array_.pointer_end(),
            &v, StringLessThan());

        GOOGLE_CHECK(*it != NULL);

        verify(**it == "n");
        verify(it == proto_array_.pointer_begin() + 3);
      }
      {
        std::string v = "f";
        google::protobuf::RepeatedPtrField<std::string>::const_pointer_iterator it =
          lower_bound(const_proto_array_->pointer_begin(),
            const_proto_array_->pointer_end(),
            &v, StringLessThan());

        GOOGLE_CHECK(*it != NULL);

        verify(**it == "n");
        verify(it == const_proto_array_->pointer_begin() + 3);
      }
    }

    // TEST_F(RepeatedPtrFieldPtrsIteratorTest, PtrMutation)
    SetUp();
    {
      google::protobuf::RepeatedPtrField<std::string>::pointer_iterator iter =
        proto_array_.pointer_begin();
      **iter = "qux";
      verify("qux" == proto_array_.Get(0));

      verify("bar" == proto_array_.Get(1));
      verify("baz" == proto_array_.Get(2));
      ++iter;
      delete *iter;
      *iter = new std::string("a");
      ++iter;
      delete *iter;
      *iter = new std::string("b");
      verify("a" == proto_array_.Get(1));
      verify("b" == proto_array_.Get(2));
    }

    // TEST_F(RepeatedPtrFieldPtrsIteratorTest, Sort)
    SetUp();
    {
      proto_array_.Add()->assign("c");
      proto_array_.Add()->assign("d");
      proto_array_.Add()->assign("n");
      proto_array_.Add()->assign("p");
      proto_array_.Add()->assign("a");
      proto_array_.Add()->assign("y");
      proto_array_.Add()->assign("x");
      verify("foo" == proto_array_.Get(0));
      verify("n" == proto_array_.Get(5));
      verify("x" == proto_array_.Get(9));
      sort(proto_array_.pointer_begin(),
        proto_array_.pointer_end(),
        StringLessThan());
      verify("a" == proto_array_.Get(0));
      verify("baz" == proto_array_.Get(2));
      verify("y" == proto_array_.Get(9));
    }
  }


  // -----------------------------------------------------------------------------
  // Unit-tests for the insert iterators
  // google::protobuf::RepeatedFieldBackInserter,
  // google::protobuf::AllocatedRepeatedPtrFieldBackInserter
  // Ported from util/gtl/proto-array-iterators_unittest.

  // RepeatedFieldInsertionIteratorsTest
  {
    std::list<double> halves;
    std::list<int> fibonacci;
    std::vector<std::string> words;
    typedef TestAllTypes::NestedMessage Nested;
    Nested nesteds[2];
    std::vector<Nested*> nested_ptrs;
    TestAllTypes protobuffer;

    // SetUp()
    {
      fibonacci.push_back(1);
      fibonacci.push_back(1);
      fibonacci.push_back(2);
      fibonacci.push_back(3);
      fibonacci.push_back(5);
      fibonacci.push_back(8);
      std::copy(fibonacci.begin(), fibonacci.end(),
        RepeatedFieldBackInserter(protobuffer.mutable_repeated_int32()));

      halves.push_back(1.0);
      halves.push_back(0.5);
      halves.push_back(0.25);
      halves.push_back(0.125);
      halves.push_back(0.0625);
      std::copy(halves.begin(), halves.end(),
        RepeatedFieldBackInserter(protobuffer.mutable_repeated_double()));

      words.push_back("Able");
      words.push_back("was");
      words.push_back("I");
      words.push_back("ere");
      words.push_back("I");
      words.push_back("saw");
      words.push_back("Elba");
      std::copy(words.begin(), words.end(),
        RepeatedFieldBackInserter(protobuffer.mutable_repeated_string()));

      nesteds[0].set_bb(17);
      nesteds[1].set_bb(4711);
      std::copy(&nesteds[0], &nesteds[2],
        RepeatedFieldBackInserter(
          protobuffer.mutable_repeated_nested_message()));

      nested_ptrs.push_back(new Nested);
      nested_ptrs.back()->set_bb(170);
      nested_ptrs.push_back(new Nested);
      nested_ptrs.back()->set_bb(47110);
      std::copy(nested_ptrs.begin(), nested_ptrs.end(),
        RepeatedFieldBackInserter(
          protobuffer.mutable_repeated_nested_message()));

    }

    auto TearDown = [&nested_ptrs]() {
      STLDeleteContainerPointers(nested_ptrs.begin(), nested_ptrs.end());
    };

    // TEST_F(RepeatedFieldInsertionIteratorsTest, Fibonacci)
    {
      verify(std::equal(fibonacci.begin(),
        fibonacci.end(),
        protobuffer.repeated_int32().begin()));
      verify(std::equal(protobuffer.repeated_int32().begin(),
        protobuffer.repeated_int32().end(),
        fibonacci.begin()));
    }

    // TEST_F(RepeatedFieldInsertionIteratorsTest, Halves)
    {
      verify(std::equal(halves.begin(),
        halves.end(),
        protobuffer.repeated_double().begin()));
      verify(std::equal(protobuffer.repeated_double().begin(),
        protobuffer.repeated_double().end(),
        halves.begin()));
    }

    // TEST_F(RepeatedFieldInsertionIteratorsTest, Words)
    {
      verify(words.size() == protobuffer.repeated_string_size());
      for (int i = 0; i < words.size(); ++i)
        verify(words.at(i) == protobuffer.repeated_string(i));
    }

    // TEST_F(RepeatedFieldInsertionIteratorsTest, Words2)
    {
      words.clear();
      words.push_back("sing");
      words.push_back("a");
      words.push_back("song");
      words.push_back("of");
      words.push_back("six");
      words.push_back("pence");
      protobuffer.mutable_repeated_string()->Clear();
      std::copy(words.begin(), words.end(), RepeatedPtrFieldBackInserter(
        protobuffer.mutable_repeated_string()));
      verify(words.size() == protobuffer.repeated_string_size());
      for (int i = 0; i < words.size(); ++i)
        verify(words.at(i) == protobuffer.repeated_string(i));
    }

    // TEST_F(RepeatedFieldInsertionIteratorsTest, Nesteds)
    {
      verify(protobuffer.repeated_nested_message_size() == 4);
      verify(protobuffer.repeated_nested_message(0).bb() == 17);
      verify(protobuffer.repeated_nested_message(1).bb() == 4711);
      verify(protobuffer.repeated_nested_message(2).bb() == 170);
      verify(protobuffer.repeated_nested_message(3).bb() == 47110);
    }

    // TEST_F(RepeatedFieldInsertionIteratorsTest, AllocatedRepeatedPtrFieldWithStringIntData)
    {
      std::vector<Nested*> data;
      TestAllTypes goldenproto;
      for (int i = 0; i < 10; ++i) {
        Nested* new_data = new Nested;
        new_data->set_bb(i);
        data.push_back(new_data);

        new_data = goldenproto.add_repeated_nested_message();
        new_data->set_bb(i);
      }
      TestAllTypes testproto;
      copy(data.begin(), data.end(),
        AllocatedRepeatedPtrFieldBackInserter(
          testproto.mutable_repeated_nested_message()));
      verify(testproto.DebugString() == goldenproto.DebugString());
    }

    // TEST_F(RepeatedFieldInsertionIteratorsTest, AllocatedRepeatedPtrFieldWithString)
    {
      std::vector<std::string*> data;
      TestAllTypes goldenproto;
      for (int i = 0; i < 10; ++i) {
        std::string* new_data = new std::string;
        *new_data = "name-" + google::protobuf::SimpleItoa(i);
        data.push_back(new_data);

        new_data = goldenproto.add_repeated_string();
        *new_data = "name-" + google::protobuf::SimpleItoa(i);
      }
      TestAllTypes testproto;
      copy(data.begin(), data.end(),
        AllocatedRepeatedPtrFieldBackInserter(
          testproto.mutable_repeated_string()));
      verify(testproto.DebugString() == goldenproto.DebugString());
    }
  }

  return true;
}
