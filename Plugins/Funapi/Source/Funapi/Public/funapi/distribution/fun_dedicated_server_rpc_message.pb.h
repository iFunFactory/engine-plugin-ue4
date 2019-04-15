// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: funapi/distribution/fun_dedicated_server_rpc_message.proto

#ifndef PROTOBUF_funapi_2fdistribution_2ffun_5fdedicated_5fserver_5frpc_5fmessage_2eproto__INCLUDED
#define PROTOBUF_funapi_2fdistribution_2ffun_5fdedicated_5fserver_5frpc_5fmessage_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2006000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2006001 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)

// Internal implementation detail -- do not call these.
void FUNAPI_API protobuf_AddDesc_funapi_2fdistribution_2ffun_5fdedicated_5fserver_5frpc_5fmessage_2eproto();
void protobuf_AssignDesc_funapi_2fdistribution_2ffun_5fdedicated_5fserver_5frpc_5fmessage_2eproto();
void protobuf_ShutdownFile_funapi_2fdistribution_2ffun_5fdedicated_5fserver_5frpc_5fmessage_2eproto();

class FunDedicatedServerRpcMessage;
class FunDedicatedServerRpcSystemMessage;

// ===================================================================

class FUNAPI_API FunDedicatedServerRpcMessage : public ::google::protobuf::Message {
 public:
  FunDedicatedServerRpcMessage();
  virtual ~FunDedicatedServerRpcMessage();

  FunDedicatedServerRpcMessage(const FunDedicatedServerRpcMessage& from);

  inline FunDedicatedServerRpcMessage& operator=(const FunDedicatedServerRpcMessage& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const FunDedicatedServerRpcMessage& default_instance();

  void Swap(FunDedicatedServerRpcMessage* other);

  // implements Message ----------------------------------------------

  FunDedicatedServerRpcMessage* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const FunDedicatedServerRpcMessage& from);
  void MergeFrom(const FunDedicatedServerRpcMessage& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required bytes xid = 1;
  inline bool has_xid() const;
  inline void clear_xid();
  static const int kXidFieldNumber = 1;
  inline const ::std::string& xid() const;
  inline void set_xid(const ::std::string& value);
  inline void set_xid(const char* value);
  inline void set_xid(const void* value, size_t size);
  inline ::std::string* mutable_xid();
  inline ::std::string* release_xid();
  inline void set_allocated_xid(::std::string* xid);

  // required string type = 2;
  inline bool has_type() const;
  inline void clear_type();
  static const int kTypeFieldNumber = 2;
  inline const ::std::string& type() const;
  inline void set_type(const ::std::string& value);
  inline void set_type(const char* value);
  inline void set_type(const char* value, size_t size);
  inline ::std::string* mutable_type();
  inline ::std::string* release_type();
  inline void set_allocated_type(::std::string* type);

  // required bool is_request = 3;
  inline bool has_is_request() const;
  inline void clear_is_request();
  static const int kIsRequestFieldNumber = 3;
  inline bool is_request() const;
  inline void set_is_request(bool value);

  GOOGLE_PROTOBUF_EXTENSION_ACCESSORS(FunDedicatedServerRpcMessage)
  // @@protoc_insertion_point(class_scope:FunDedicatedServerRpcMessage)
 private:
  inline void set_has_xid();
  inline void clear_has_xid();
  inline void set_has_type();
  inline void clear_has_type();
  inline void set_has_is_request();
  inline void clear_has_is_request();

  ::google::protobuf::internal::ExtensionSet _extensions_;

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::std::string* xid_;
  ::std::string* type_;
  bool is_request_;
  friend void FUNAPI_API protobuf_AddDesc_funapi_2fdistribution_2ffun_5fdedicated_5fserver_5frpc_5fmessage_2eproto();
  friend void protobuf_AssignDesc_funapi_2fdistribution_2ffun_5fdedicated_5fserver_5frpc_5fmessage_2eproto();
  friend void protobuf_ShutdownFile_funapi_2fdistribution_2ffun_5fdedicated_5fserver_5frpc_5fmessage_2eproto();

  void InitAsDefaultInstance();
  static FunDedicatedServerRpcMessage* default_instance_;
};
// -------------------------------------------------------------------

class FUNAPI_API FunDedicatedServerRpcSystemMessage : public ::google::protobuf::Message {
 public:
  FunDedicatedServerRpcSystemMessage();
  virtual ~FunDedicatedServerRpcSystemMessage();

  FunDedicatedServerRpcSystemMessage(const FunDedicatedServerRpcSystemMessage& from);

  inline FunDedicatedServerRpcSystemMessage& operator=(const FunDedicatedServerRpcSystemMessage& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const FunDedicatedServerRpcSystemMessage& default_instance();

  void Swap(FunDedicatedServerRpcSystemMessage* other);

  // implements Message ----------------------------------------------

  FunDedicatedServerRpcSystemMessage* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const FunDedicatedServerRpcSystemMessage& from);
  void MergeFrom(const FunDedicatedServerRpcSystemMessage& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional string data = 1;
  inline bool has_data() const;
  inline void clear_data();
  static const int kDataFieldNumber = 1;
  inline const ::std::string& data() const;
  inline void set_data(const ::std::string& value);
  inline void set_data(const char* value);
  inline void set_data(const char* value, size_t size);
  inline ::std::string* mutable_data();
  inline ::std::string* release_data();
  inline void set_allocated_data(::std::string* data);

  // @@protoc_insertion_point(class_scope:FunDedicatedServerRpcSystemMessage)
 private:
  inline void set_has_data();
  inline void clear_has_data();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::std::string* data_;
  friend void FUNAPI_API protobuf_AddDesc_funapi_2fdistribution_2ffun_5fdedicated_5fserver_5frpc_5fmessage_2eproto();
  friend void protobuf_AssignDesc_funapi_2fdistribution_2ffun_5fdedicated_5fserver_5frpc_5fmessage_2eproto();
  friend void protobuf_ShutdownFile_funapi_2fdistribution_2ffun_5fdedicated_5fserver_5frpc_5fmessage_2eproto();

  void InitAsDefaultInstance();
  static FunDedicatedServerRpcSystemMessage* default_instance_;
};
// ===================================================================

static const int kDsRpcSysFieldNumber = 8;
FUNAPI_API extern ::google::protobuf::internal::ExtensionIdentifier< ::FunDedicatedServerRpcMessage,
    ::google::protobuf::internal::MessageTypeTraits< ::FunDedicatedServerRpcSystemMessage >, 11, false >
  ds_rpc_sys;

// ===================================================================

// FunDedicatedServerRpcMessage

// required bytes xid = 1;
inline bool FunDedicatedServerRpcMessage::has_xid() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void FunDedicatedServerRpcMessage::set_has_xid() {
  _has_bits_[0] |= 0x00000001u;
}
inline void FunDedicatedServerRpcMessage::clear_has_xid() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void FunDedicatedServerRpcMessage::clear_xid() {
  if (xid_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    xid_->clear();
  }
  clear_has_xid();
}
inline const ::std::string& FunDedicatedServerRpcMessage::xid() const {
  // @@protoc_insertion_point(field_get:FunDedicatedServerRpcMessage.xid)
  return *xid_;
}
inline void FunDedicatedServerRpcMessage::set_xid(const ::std::string& value) {
  set_has_xid();
  if (xid_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    xid_ = new ::std::string;
  }
  xid_->assign(value);
  // @@protoc_insertion_point(field_set:FunDedicatedServerRpcMessage.xid)
}
inline void FunDedicatedServerRpcMessage::set_xid(const char* value) {
  set_has_xid();
  if (xid_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    xid_ = new ::std::string;
  }
  xid_->assign(value);
  // @@protoc_insertion_point(field_set_char:FunDedicatedServerRpcMessage.xid)
}
inline void FunDedicatedServerRpcMessage::set_xid(const void* value, size_t size) {
  set_has_xid();
  if (xid_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    xid_ = new ::std::string;
  }
  xid_->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:FunDedicatedServerRpcMessage.xid)
}
inline ::std::string* FunDedicatedServerRpcMessage::mutable_xid() {
  set_has_xid();
  if (xid_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    xid_ = new ::std::string;
  }
  // @@protoc_insertion_point(field_mutable:FunDedicatedServerRpcMessage.xid)
  return xid_;
}
inline ::std::string* FunDedicatedServerRpcMessage::release_xid() {
  clear_has_xid();
  if (xid_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    return NULL;
  } else {
    ::std::string* temp = xid_;
    xid_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
    return temp;
  }
}
inline void FunDedicatedServerRpcMessage::set_allocated_xid(::std::string* xid) {
  if (xid_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete xid_;
  }
  if (xid) {
    set_has_xid();
    xid_ = xid;
  } else {
    clear_has_xid();
    xid_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  }
  // @@protoc_insertion_point(field_set_allocated:FunDedicatedServerRpcMessage.xid)
}

// required string type = 2;
inline bool FunDedicatedServerRpcMessage::has_type() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void FunDedicatedServerRpcMessage::set_has_type() {
  _has_bits_[0] |= 0x00000002u;
}
inline void FunDedicatedServerRpcMessage::clear_has_type() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void FunDedicatedServerRpcMessage::clear_type() {
  if (type_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    type_->clear();
  }
  clear_has_type();
}
inline const ::std::string& FunDedicatedServerRpcMessage::type() const {
  // @@protoc_insertion_point(field_get:FunDedicatedServerRpcMessage.type)
  return *type_;
}
inline void FunDedicatedServerRpcMessage::set_type(const ::std::string& value) {
  set_has_type();
  if (type_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    type_ = new ::std::string;
  }
  type_->assign(value);
  // @@protoc_insertion_point(field_set:FunDedicatedServerRpcMessage.type)
}
inline void FunDedicatedServerRpcMessage::set_type(const char* value) {
  set_has_type();
  if (type_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    type_ = new ::std::string;
  }
  type_->assign(value);
  // @@protoc_insertion_point(field_set_char:FunDedicatedServerRpcMessage.type)
}
inline void FunDedicatedServerRpcMessage::set_type(const char* value, size_t size) {
  set_has_type();
  if (type_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    type_ = new ::std::string;
  }
  type_->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:FunDedicatedServerRpcMessage.type)
}
inline ::std::string* FunDedicatedServerRpcMessage::mutable_type() {
  set_has_type();
  if (type_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    type_ = new ::std::string;
  }
  // @@protoc_insertion_point(field_mutable:FunDedicatedServerRpcMessage.type)
  return type_;
}
inline ::std::string* FunDedicatedServerRpcMessage::release_type() {
  clear_has_type();
  if (type_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    return NULL;
  } else {
    ::std::string* temp = type_;
    type_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
    return temp;
  }
}
inline void FunDedicatedServerRpcMessage::set_allocated_type(::std::string* type) {
  if (type_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete type_;
  }
  if (type) {
    set_has_type();
    type_ = type;
  } else {
    clear_has_type();
    type_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  }
  // @@protoc_insertion_point(field_set_allocated:FunDedicatedServerRpcMessage.type)
}

// required bool is_request = 3;
inline bool FunDedicatedServerRpcMessage::has_is_request() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void FunDedicatedServerRpcMessage::set_has_is_request() {
  _has_bits_[0] |= 0x00000004u;
}
inline void FunDedicatedServerRpcMessage::clear_has_is_request() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void FunDedicatedServerRpcMessage::clear_is_request() {
  is_request_ = false;
  clear_has_is_request();
}
inline bool FunDedicatedServerRpcMessage::is_request() const {
  // @@protoc_insertion_point(field_get:FunDedicatedServerRpcMessage.is_request)
  return is_request_;
}
inline void FunDedicatedServerRpcMessage::set_is_request(bool value) {
  set_has_is_request();
  is_request_ = value;
  // @@protoc_insertion_point(field_set:FunDedicatedServerRpcMessage.is_request)
}

// -------------------------------------------------------------------

// FunDedicatedServerRpcSystemMessage

// optional string data = 1;
inline bool FunDedicatedServerRpcSystemMessage::has_data() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void FunDedicatedServerRpcSystemMessage::set_has_data() {
  _has_bits_[0] |= 0x00000001u;
}
inline void FunDedicatedServerRpcSystemMessage::clear_has_data() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void FunDedicatedServerRpcSystemMessage::clear_data() {
  if (data_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    data_->clear();
  }
  clear_has_data();
}
inline const ::std::string& FunDedicatedServerRpcSystemMessage::data() const {
  // @@protoc_insertion_point(field_get:FunDedicatedServerRpcSystemMessage.data)
  return *data_;
}
inline void FunDedicatedServerRpcSystemMessage::set_data(const ::std::string& value) {
  set_has_data();
  if (data_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    data_ = new ::std::string;
  }
  data_->assign(value);
  // @@protoc_insertion_point(field_set:FunDedicatedServerRpcSystemMessage.data)
}
inline void FunDedicatedServerRpcSystemMessage::set_data(const char* value) {
  set_has_data();
  if (data_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    data_ = new ::std::string;
  }
  data_->assign(value);
  // @@protoc_insertion_point(field_set_char:FunDedicatedServerRpcSystemMessage.data)
}
inline void FunDedicatedServerRpcSystemMessage::set_data(const char* value, size_t size) {
  set_has_data();
  if (data_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    data_ = new ::std::string;
  }
  data_->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:FunDedicatedServerRpcSystemMessage.data)
}
inline ::std::string* FunDedicatedServerRpcSystemMessage::mutable_data() {
  set_has_data();
  if (data_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    data_ = new ::std::string;
  }
  // @@protoc_insertion_point(field_mutable:FunDedicatedServerRpcSystemMessage.data)
  return data_;
}
inline ::std::string* FunDedicatedServerRpcSystemMessage::release_data() {
  clear_has_data();
  if (data_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    return NULL;
  } else {
    ::std::string* temp = data_;
    data_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
    return temp;
  }
}
inline void FunDedicatedServerRpcSystemMessage::set_allocated_data(::std::string* data) {
  if (data_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete data_;
  }
  if (data) {
    set_has_data();
    data_ = data;
  } else {
    clear_has_data();
    data_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  }
  // @@protoc_insertion_point(field_set_allocated:FunDedicatedServerRpcSystemMessage.data)
}


// @@protoc_insertion_point(namespace_scope)

#ifndef SWIG
namespace google {
namespace protobuf {


}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_funapi_2fdistribution_2ffun_5fdedicated_5fserver_5frpc_5fmessage_2eproto__INCLUDED
