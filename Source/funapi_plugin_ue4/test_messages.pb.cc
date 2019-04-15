// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: test_messages.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "test_messages.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace {

const ::google::protobuf::Descriptor* PbufEchoMessage_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  PbufEchoMessage_reflection_ = NULL;
const ::google::protobuf::Descriptor* PbufAnotherMessage_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  PbufAnotherMessage_reflection_ = NULL;

}  // namespace


void protobuf_AssignDesc_test_5fmessages_2eproto() {
  protobuf_AddDesc_test_5fmessages_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "test_messages.proto");
  GOOGLE_CHECK(file != NULL);
  PbufEchoMessage_descriptor_ = file->message_type(0);
  static const int PbufEchoMessage_offsets_[1] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(PbufEchoMessage, msg_),
  };
  PbufEchoMessage_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      PbufEchoMessage_descriptor_,
      PbufEchoMessage::default_instance_,
      PbufEchoMessage_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(PbufEchoMessage, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(PbufEchoMessage, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(PbufEchoMessage));
  PbufAnotherMessage_descriptor_ = file->message_type(1);
  static const int PbufAnotherMessage_offsets_[1] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(PbufAnotherMessage, msg_),
  };
  PbufAnotherMessage_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      PbufAnotherMessage_descriptor_,
      PbufAnotherMessage::default_instance_,
      PbufAnotherMessage_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(PbufAnotherMessage, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(PbufAnotherMessage, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(PbufAnotherMessage));
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_test_5fmessages_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    PbufEchoMessage_descriptor_, &PbufEchoMessage::default_instance());
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    PbufAnotherMessage_descriptor_, &PbufAnotherMessage::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_test_5fmessages_2eproto() {
  delete PbufEchoMessage::default_instance_;
  delete PbufEchoMessage_reflection_;
  delete PbufAnotherMessage::default_instance_;
  delete PbufAnotherMessage_reflection_;
}

void protobuf_AddDesc_test_5fmessages_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::protobuf_AddDesc_funapi_2fnetwork_2ffun_5fmessage_2eproto();
  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\023test_messages.proto\032 funapi/network/fu"
    "n_message.proto\"\036\n\017PbufEchoMessage\022\013\n\003ms"
    "g\030\001 \002(\t\"!\n\022PbufAnotherMessage\022\013\n\003msg\030\001 \001"
    "(\t:0\n\tpbuf_echo\022\013.FunMessage\030\020 \001(\0132\020.Pbu"
    "fEchoMessage:6\n\014pbuf_another\022\013.FunMessag"
    "e\030\021 \001(\0132\023.PbufAnotherMessage", 228);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "test_messages.proto", &protobuf_RegisterTypes);
  PbufEchoMessage::default_instance_ = new PbufEchoMessage();
  PbufAnotherMessage::default_instance_ = new PbufAnotherMessage();
  ::google::protobuf::internal::ExtensionSet::RegisterMessageExtension(
    &::FunMessage::default_instance(),
    16, 11, false, false,
    &::PbufEchoMessage::default_instance());
  ::google::protobuf::internal::ExtensionSet::RegisterMessageExtension(
    &::FunMessage::default_instance(),
    17, 11, false, false,
    &::PbufAnotherMessage::default_instance());
  PbufEchoMessage::default_instance_->InitAsDefaultInstance();
  PbufAnotherMessage::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_test_5fmessages_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_test_5fmessages_2eproto {
  StaticDescriptorInitializer_test_5fmessages_2eproto() {
    protobuf_AddDesc_test_5fmessages_2eproto();
  }
} static_descriptor_initializer_test_5fmessages_2eproto_;

// ===================================================================

#ifndef _MSC_VER
const int PbufEchoMessage::kMsgFieldNumber;
#endif  // !_MSC_VER

PbufEchoMessage::PbufEchoMessage()
  : ::google::protobuf::Message() {
  SharedCtor();
  // @@protoc_insertion_point(constructor:PbufEchoMessage)
}

void PbufEchoMessage::InitAsDefaultInstance() {
}

PbufEchoMessage::PbufEchoMessage(const PbufEchoMessage& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
  // @@protoc_insertion_point(copy_constructor:PbufEchoMessage)
}

void PbufEchoMessage::SharedCtor() {
  ::google::protobuf::internal::GetEmptyString();
  _cached_size_ = 0;
  msg_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

PbufEchoMessage::~PbufEchoMessage() {
  // @@protoc_insertion_point(destructor:PbufEchoMessage)
  SharedDtor();
}

void PbufEchoMessage::SharedDtor() {
  if (msg_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete msg_;
  }
  if (this != default_instance_) {
  }
}

void PbufEchoMessage::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* PbufEchoMessage::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return PbufEchoMessage_descriptor_;
}

const PbufEchoMessage& PbufEchoMessage::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_test_5fmessages_2eproto();
  return *default_instance_;
}

PbufEchoMessage* PbufEchoMessage::default_instance_ = NULL;

PbufEchoMessage* PbufEchoMessage::New() const {
  return new PbufEchoMessage;
}

void PbufEchoMessage::Clear() {
  if (has_msg()) {
    if (msg_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
      msg_->clear();
    }
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool PbufEchoMessage::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:PbufEchoMessage)
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // required string msg = 1;
      case 1: {
        if (tag == 10) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_msg()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
            this->msg().data(), this->msg().length(),
            ::google::protobuf::internal::WireFormat::PARSE,
            "msg");
        } else {
          goto handle_unusual;
        }
        if (input->ExpectAtEnd()) goto success;
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0 ||
            ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:PbufEchoMessage)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:PbufEchoMessage)
  return false;
#undef DO_
}

void PbufEchoMessage::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:PbufEchoMessage)
  // required string msg = 1;
  if (has_msg()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->msg().data(), this->msg().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "msg");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      1, this->msg(), output);
  }

  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
  // @@protoc_insertion_point(serialize_end:PbufEchoMessage)
}

::google::protobuf::uint8* PbufEchoMessage::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // @@protoc_insertion_point(serialize_to_array_start:PbufEchoMessage)
  // required string msg = 1;
  if (has_msg()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->msg().data(), this->msg().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "msg");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        1, this->msg(), target);
  }

  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:PbufEchoMessage)
  return target;
}

int PbufEchoMessage::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // required string msg = 1;
    if (has_msg()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->msg());
    }

  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void PbufEchoMessage::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const PbufEchoMessage* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const PbufEchoMessage*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void PbufEchoMessage::MergeFrom(const PbufEchoMessage& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_msg()) {
      set_msg(from.msg());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void PbufEchoMessage::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void PbufEchoMessage::CopyFrom(const PbufEchoMessage& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool PbufEchoMessage::IsInitialized() const {
  if ((_has_bits_[0] & 0x00000001) != 0x00000001) return false;

  return true;
}

void PbufEchoMessage::Swap(PbufEchoMessage* other) {
  if (other != this) {
    std::swap(msg_, other->msg_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata PbufEchoMessage::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = PbufEchoMessage_descriptor_;
  metadata.reflection = PbufEchoMessage_reflection_;
  return metadata;
}


// ===================================================================

#ifndef _MSC_VER
const int PbufAnotherMessage::kMsgFieldNumber;
#endif  // !_MSC_VER

PbufAnotherMessage::PbufAnotherMessage()
  : ::google::protobuf::Message() {
  SharedCtor();
  // @@protoc_insertion_point(constructor:PbufAnotherMessage)
}

void PbufAnotherMessage::InitAsDefaultInstance() {
}

PbufAnotherMessage::PbufAnotherMessage(const PbufAnotherMessage& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
  // @@protoc_insertion_point(copy_constructor:PbufAnotherMessage)
}

void PbufAnotherMessage::SharedCtor() {
  ::google::protobuf::internal::GetEmptyString();
  _cached_size_ = 0;
  msg_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

PbufAnotherMessage::~PbufAnotherMessage() {
  // @@protoc_insertion_point(destructor:PbufAnotherMessage)
  SharedDtor();
}

void PbufAnotherMessage::SharedDtor() {
  if (msg_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete msg_;
  }
  if (this != default_instance_) {
  }
}

void PbufAnotherMessage::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* PbufAnotherMessage::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return PbufAnotherMessage_descriptor_;
}

const PbufAnotherMessage& PbufAnotherMessage::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_test_5fmessages_2eproto();
  return *default_instance_;
}

PbufAnotherMessage* PbufAnotherMessage::default_instance_ = NULL;

PbufAnotherMessage* PbufAnotherMessage::New() const {
  return new PbufAnotherMessage;
}

void PbufAnotherMessage::Clear() {
  if (has_msg()) {
    if (msg_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
      msg_->clear();
    }
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool PbufAnotherMessage::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:PbufAnotherMessage)
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional string msg = 1;
      case 1: {
        if (tag == 10) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_msg()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
            this->msg().data(), this->msg().length(),
            ::google::protobuf::internal::WireFormat::PARSE,
            "msg");
        } else {
          goto handle_unusual;
        }
        if (input->ExpectAtEnd()) goto success;
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0 ||
            ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:PbufAnotherMessage)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:PbufAnotherMessage)
  return false;
#undef DO_
}

void PbufAnotherMessage::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:PbufAnotherMessage)
  // optional string msg = 1;
  if (has_msg()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->msg().data(), this->msg().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "msg");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      1, this->msg(), output);
  }

  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
  // @@protoc_insertion_point(serialize_end:PbufAnotherMessage)
}

::google::protobuf::uint8* PbufAnotherMessage::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // @@protoc_insertion_point(serialize_to_array_start:PbufAnotherMessage)
  // optional string msg = 1;
  if (has_msg()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->msg().data(), this->msg().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "msg");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        1, this->msg(), target);
  }

  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:PbufAnotherMessage)
  return target;
}

int PbufAnotherMessage::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // optional string msg = 1;
    if (has_msg()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->msg());
    }

  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void PbufAnotherMessage::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const PbufAnotherMessage* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const PbufAnotherMessage*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void PbufAnotherMessage::MergeFrom(const PbufAnotherMessage& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_msg()) {
      set_msg(from.msg());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void PbufAnotherMessage::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void PbufAnotherMessage::CopyFrom(const PbufAnotherMessage& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool PbufAnotherMessage::IsInitialized() const {

  return true;
}

void PbufAnotherMessage::Swap(PbufAnotherMessage* other) {
  if (other != this) {
    std::swap(msg_, other->msg_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata PbufAnotherMessage::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = PbufAnotherMessage_descriptor_;
  metadata.reflection = PbufAnotherMessage_reflection_;
  return metadata;
}

::google::protobuf::internal::ExtensionIdentifier< ::FunMessage,
    ::google::protobuf::internal::MessageTypeTraits< ::PbufEchoMessage >, 11, false >
  pbuf_echo(kPbufEchoFieldNumber, ::PbufEchoMessage::default_instance());
::google::protobuf::internal::ExtensionIdentifier< ::FunMessage,
    ::google::protobuf::internal::MessageTypeTraits< ::PbufAnotherMessage >, 11, false >
  pbuf_another(kPbufAnotherFieldNumber, ::PbufAnotherMessage::default_instance());

// @@protoc_insertion_point(namespace_scope)

// @@protoc_insertion_point(global_scope)
