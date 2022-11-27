// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: declvol/v1/declvol.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_declvol_2fv1_2fdeclvol_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_declvol_2fv1_2fdeclvol_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3021000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3021004 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_bases.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_declvol_2fv1_2fdeclvol_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_declvol_2fv1_2fdeclvol_2eproto {
  static const uint32_t offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_declvol_2fv1_2fdeclvol_2eproto;
namespace declvol {
namespace v1 {
class SwitchProfileRequest;
struct SwitchProfileRequestDefaultTypeInternal;
extern SwitchProfileRequestDefaultTypeInternal _SwitchProfileRequest_default_instance_;
class SwitchProfileResponse;
struct SwitchProfileResponseDefaultTypeInternal;
extern SwitchProfileResponseDefaultTypeInternal _SwitchProfileResponse_default_instance_;
}  // namespace v1
}  // namespace declvol
PROTOBUF_NAMESPACE_OPEN
template<> ::declvol::v1::SwitchProfileRequest* Arena::CreateMaybeMessage<::declvol::v1::SwitchProfileRequest>(Arena*);
template<> ::declvol::v1::SwitchProfileResponse* Arena::CreateMaybeMessage<::declvol::v1::SwitchProfileResponse>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace declvol {
namespace v1 {

// ===================================================================

class SwitchProfileRequest final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:declvol.v1.SwitchProfileRequest) */ {
 public:
  inline SwitchProfileRequest() : SwitchProfileRequest(nullptr) {}
  ~SwitchProfileRequest() override;
  explicit PROTOBUF_CONSTEXPR SwitchProfileRequest(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  SwitchProfileRequest(const SwitchProfileRequest& from);
  SwitchProfileRequest(SwitchProfileRequest&& from) noexcept
    : SwitchProfileRequest() {
    *this = ::std::move(from);
  }

  inline SwitchProfileRequest& operator=(const SwitchProfileRequest& from) {
    CopyFrom(from);
    return *this;
  }
  inline SwitchProfileRequest& operator=(SwitchProfileRequest&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const SwitchProfileRequest& default_instance() {
    return *internal_default_instance();
  }
  static inline const SwitchProfileRequest* internal_default_instance() {
    return reinterpret_cast<const SwitchProfileRequest*>(
               &_SwitchProfileRequest_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(SwitchProfileRequest& a, SwitchProfileRequest& b) {
    a.Swap(&b);
  }
  inline void Swap(SwitchProfileRequest* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(SwitchProfileRequest* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  SwitchProfileRequest* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<SwitchProfileRequest>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const SwitchProfileRequest& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const SwitchProfileRequest& from) {
    SwitchProfileRequest::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  uint8_t* _InternalSerialize(
      uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::PROTOBUF_NAMESPACE_ID::Arena* arena, bool is_message_owned);
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(SwitchProfileRequest* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "declvol.v1.SwitchProfileRequest";
  }
  protected:
  explicit SwitchProfileRequest(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kProfileFieldNumber = 1,
    kConfigPathFieldNumber = 2,
  };
  // string profile = 1;
  void clear_profile();
  const std::string& profile() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_profile(ArgT0&& arg0, ArgT... args);
  std::string* mutable_profile();
  PROTOBUF_NODISCARD std::string* release_profile();
  void set_allocated_profile(std::string* profile);
  private:
  const std::string& _internal_profile() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_profile(const std::string& value);
  std::string* _internal_mutable_profile();
  public:

  // string config_path = 2;
  void clear_config_path();
  const std::string& config_path() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_config_path(ArgT0&& arg0, ArgT... args);
  std::string* mutable_config_path();
  PROTOBUF_NODISCARD std::string* release_config_path();
  void set_allocated_config_path(std::string* config_path);
  private:
  const std::string& _internal_config_path() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_config_path(const std::string& value);
  std::string* _internal_mutable_config_path();
  public:

  // @@protoc_insertion_point(class_scope:declvol.v1.SwitchProfileRequest)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr profile_;
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr config_path_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_declvol_2fv1_2fdeclvol_2eproto;
};
// -------------------------------------------------------------------

class SwitchProfileResponse final :
    public ::PROTOBUF_NAMESPACE_ID::internal::ZeroFieldsBase /* @@protoc_insertion_point(class_definition:declvol.v1.SwitchProfileResponse) */ {
 public:
  inline SwitchProfileResponse() : SwitchProfileResponse(nullptr) {}
  explicit PROTOBUF_CONSTEXPR SwitchProfileResponse(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  SwitchProfileResponse(const SwitchProfileResponse& from);
  SwitchProfileResponse(SwitchProfileResponse&& from) noexcept
    : SwitchProfileResponse() {
    *this = ::std::move(from);
  }

  inline SwitchProfileResponse& operator=(const SwitchProfileResponse& from) {
    CopyFrom(from);
    return *this;
  }
  inline SwitchProfileResponse& operator=(SwitchProfileResponse&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const SwitchProfileResponse& default_instance() {
    return *internal_default_instance();
  }
  static inline const SwitchProfileResponse* internal_default_instance() {
    return reinterpret_cast<const SwitchProfileResponse*>(
               &_SwitchProfileResponse_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  friend void swap(SwitchProfileResponse& a, SwitchProfileResponse& b) {
    a.Swap(&b);
  }
  inline void Swap(SwitchProfileResponse* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(SwitchProfileResponse* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  SwitchProfileResponse* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<SwitchProfileResponse>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::internal::ZeroFieldsBase::CopyFrom;
  inline void CopyFrom(const SwitchProfileResponse& from) {
    ::PROTOBUF_NAMESPACE_ID::internal::ZeroFieldsBase::CopyImpl(*this, from);
  }
  using ::PROTOBUF_NAMESPACE_ID::internal::ZeroFieldsBase::MergeFrom;
  void MergeFrom(const SwitchProfileResponse& from) {
    ::PROTOBUF_NAMESPACE_ID::internal::ZeroFieldsBase::MergeImpl(*this, from);
  }
  public:

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "declvol.v1.SwitchProfileResponse";
  }
  protected:
  explicit SwitchProfileResponse(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // @@protoc_insertion_point(class_scope:declvol.v1.SwitchProfileResponse)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
  };
  friend struct ::TableStruct_declvol_2fv1_2fdeclvol_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// SwitchProfileRequest

// string profile = 1;
inline void SwitchProfileRequest::clear_profile() {
  _impl_.profile_.ClearToEmpty();
}
inline const std::string& SwitchProfileRequest::profile() const {
  // @@protoc_insertion_point(field_get:declvol.v1.SwitchProfileRequest.profile)
  return _internal_profile();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void SwitchProfileRequest::set_profile(ArgT0&& arg0, ArgT... args) {
 
 _impl_.profile_.Set(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:declvol.v1.SwitchProfileRequest.profile)
}
inline std::string* SwitchProfileRequest::mutable_profile() {
  std::string* _s = _internal_mutable_profile();
  // @@protoc_insertion_point(field_mutable:declvol.v1.SwitchProfileRequest.profile)
  return _s;
}
inline const std::string& SwitchProfileRequest::_internal_profile() const {
  return _impl_.profile_.Get();
}
inline void SwitchProfileRequest::_internal_set_profile(const std::string& value) {
  
  _impl_.profile_.Set(value, GetArenaForAllocation());
}
inline std::string* SwitchProfileRequest::_internal_mutable_profile() {
  
  return _impl_.profile_.Mutable(GetArenaForAllocation());
}
inline std::string* SwitchProfileRequest::release_profile() {
  // @@protoc_insertion_point(field_release:declvol.v1.SwitchProfileRequest.profile)
  return _impl_.profile_.Release();
}
inline void SwitchProfileRequest::set_allocated_profile(std::string* profile) {
  if (profile != nullptr) {
    
  } else {
    
  }
  _impl_.profile_.SetAllocated(profile, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.profile_.IsDefault()) {
    _impl_.profile_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:declvol.v1.SwitchProfileRequest.profile)
}

// string config_path = 2;
inline void SwitchProfileRequest::clear_config_path() {
  _impl_.config_path_.ClearToEmpty();
}
inline const std::string& SwitchProfileRequest::config_path() const {
  // @@protoc_insertion_point(field_get:declvol.v1.SwitchProfileRequest.config_path)
  return _internal_config_path();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void SwitchProfileRequest::set_config_path(ArgT0&& arg0, ArgT... args) {
 
 _impl_.config_path_.Set(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:declvol.v1.SwitchProfileRequest.config_path)
}
inline std::string* SwitchProfileRequest::mutable_config_path() {
  std::string* _s = _internal_mutable_config_path();
  // @@protoc_insertion_point(field_mutable:declvol.v1.SwitchProfileRequest.config_path)
  return _s;
}
inline const std::string& SwitchProfileRequest::_internal_config_path() const {
  return _impl_.config_path_.Get();
}
inline void SwitchProfileRequest::_internal_set_config_path(const std::string& value) {
  
  _impl_.config_path_.Set(value, GetArenaForAllocation());
}
inline std::string* SwitchProfileRequest::_internal_mutable_config_path() {
  
  return _impl_.config_path_.Mutable(GetArenaForAllocation());
}
inline std::string* SwitchProfileRequest::release_config_path() {
  // @@protoc_insertion_point(field_release:declvol.v1.SwitchProfileRequest.config_path)
  return _impl_.config_path_.Release();
}
inline void SwitchProfileRequest::set_allocated_config_path(std::string* config_path) {
  if (config_path != nullptr) {
    
  } else {
    
  }
  _impl_.config_path_.SetAllocated(config_path, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.config_path_.IsDefault()) {
    _impl_.config_path_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:declvol.v1.SwitchProfileRequest.config_path)
}

// -------------------------------------------------------------------

// SwitchProfileResponse

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__
// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace v1
}  // namespace declvol

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_declvol_2fv1_2fdeclvol_2eproto
