#pragma once

// Feature check macro
#define GOOGLE_PROTOBUF_HAS_DONATED_STRING 1

#include "google/protobuf/arena.h"
#if defined(__has_include) && __has_include("google/protobuf/config.h")
#include "google/protobuf/config.h"
#endif

#include "absl/strings/internal/resize_uninitialized.h"
#include "absl/strings/str_format.h"
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

namespace internal {

#if __GLIBCXX__
#if _GLIBCXX_USE_CXX11_ABI
struct StdStringRep {
  char* data;
  uint64_t size;
  union {
    uint64_t capacity;
    char local[16];
  };
};
#else                  // !_GLIBCXX_USE_CXX11_ABI
struct StdStringRep {
  uint64_t size;
  uint64_t capacity;
  int32_t refcount;
  uint32_t gap;
  char data[0];
};
#endif                 // !_GLIBCXX_USE_CXX11_ABI
#elif _LIBCPP_VERSION  // && !__GLIBCXX__
#if _LIBCPP_ABI_ALTERNATE_STRING_LAYOUT
static_assert(false, "don not support _LIBCPP_ABI_ALTERNATE_STRING_LAYOUT yet");
#endif  // _LIBCPP_ABI_ALTERNATE_STRING_LAYOUT
#if _LIBCPP_BIG_ENDIAN
static_assert(false, "don not support _LIBCPP_BIG_ENDIAN yet");
#endif  // _LIBCPP_BIG_ENDIAN
union StdStringRep {
  struct {
    typename ::std::string::size_type capacity;
    typename ::std::string::size_type size;
    typename ::std::string::pointer data;
  } long_format;
  struct {
    uint8_t size;
    typename ::std::string::value_type data[0];
  } shot_format;

  inline bool is_long() const noexcept { return shot_format.size & 0x01; }

  inline ::std::string::size_type long_capacity() const noexcept {
    return long_format.capacity & ~static_cast<::std::string::size_type>(0x01);
  }
};
#endif  // _LIBCPP_VERSION && !__GLIBCXX__
}  // namespace internal

// Wrap a full arenastring pointer and arena it belongs to.
// provide function make it look like a string*
//
// Full arenastring itself and it's dynamic content both placed on arena
class ArenaStringAccessor {
 public:
  using value_type = ::std::string::value_type;
  using traits_type = ::std::string::traits_type;
  using allocator_type = ::std::string::allocator_type;
  using size_type = ::std::string::size_type;
  using difference_type = ::std::string::difference_type;
  using reference = ::std::string::reference;
  using const_reference = ::std::string::const_reference;
  using pointer = ::std::string::pointer;
  using const_pointer = ::std::string::const_pointer;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = ::std::reverse_iterator<iterator>;
  using const_reverse_iterator = ::std::reverse_iterator<const_iterator>;

  // Disable default constructor and default operator=
  ArenaStringAccessor() = delete;
  inline ArenaStringAccessor(ArenaStringAccessor&&) noexcept = default;
  inline ArenaStringAccessor(const ArenaStringAccessor&) noexcept = default;
  ArenaStringAccessor& operator=(ArenaStringAccessor&&) = delete;
  ArenaStringAccessor& operator=(const ArenaStringAccessor&) = delete;
  inline ~ArenaStringAccessor() noexcept = default;

  // Assign
  inline ArenaStringAccessor& operator=(::absl::string_view other) noexcept;
  inline ArenaStringAccessor& assign(::absl::string_view other) noexcept;
  inline ArenaStringAccessor& assign(const_pointer data,
                                     size_type size) noexcept;

  // Element access
  inline reference operator[](size_type position) noexcept;
  inline const_reference operator[](size_type position) const noexcept;
  inline const_pointer data() const noexcept;
  inline const_pointer c_str() const noexcept;
  inline operator ::absl::string_view() const noexcept;
  inline operator const ::std::string&() const noexcept;

  // Iterators
  inline iterator begin() noexcept;
  inline const_iterator begin() const noexcept;
  inline const_iterator cbegin() const noexcept;
  inline iterator end() noexcept;
  inline const_iterator end() const noexcept;
  inline const_iterator cend() const noexcept;

  // Capacity
  inline bool empty() const noexcept;
  inline size_type size() const noexcept;
  inline void reserve(size_type required_capacity) noexcept;
  inline size_type capacity() const noexcept;

  // Modifiers
  inline void clear() noexcept;
  inline void push_back(value_type c) noexcept;
  inline ArenaStringAccessor& append(::absl::string_view sv) noexcept;
  inline ArenaStringAccessor& append(const_pointer append_data,
                                     size_type append_size) noexcept;
  inline ArenaStringAccessor& operator+=(char ch) noexcept;
  inline ArenaStringAccessor& operator+=(::absl::string_view sv) noexcept;
  inline void resize(size_type new_size) noexcept;
  inline void resize(size_type new_size, value_type c) noexcept;
  inline void swap(ArenaStringAccessor other) noexcept;

  // Operations
  inline int compare(::absl::string_view other) const noexcept;

  ////////////////////////////////////////////////////////////////////////////
  // Special functions
  // Create string on arena
  inline static ArenaStringAccessor create(Arena* arena) noexcept;
  template <typename T>
  inline static ArenaStringAccessor create(Arena* arena, T&& value) noexcept;
  // Swap strings both on **same** arena or both not on arena
  inline static void swap(::std::string* left, ::std::string* right) noexcept;
  // Wrap constructor
  inline ArenaStringAccessor(Arena* arena, ::std::string* ptr) noexcept;
  inline Arena* arena() const noexcept;
  inline ::std::string* underlying() const noexcept;
  // Support absl::strings_internal::STLStringResizeUninitialized
  inline char* __resize_default_init(size_type new_size) noexcept;
  ////////////////////////////////////////////////////////////////////////////

 private:
  inline internal::StdStringRep& representation() noexcept;

  inline pointer qualified_buffer(size_type required_capacity) noexcept;
  inline pointer qualified_buffer(size_type required_capacity,
                                  size_type predict_capacity) noexcept;
  inline pointer writable_buffer() noexcept;
  inline pointer recreate_buffer(size_type capacity) noexcept;

  inline void set_size(size_type size) noexcept;
  inline void set_size_and_terminator(size_type size) noexcept;

  Arena* _arena{nullptr};
  ::std::string* _ptr{nullptr};
};

class MaybeArenaStringAccessor : public ArenaStringAccessor {
 public:
  using ArenaStringAccessor::ArenaStringAccessor;
  inline MaybeArenaStringAccessor(const ArenaStringAccessor& other) noexcept;

  // Assign
  template <typename T>
  inline MaybeArenaStringAccessor& operator=(T&& other) noexcept;
  // Deal with assign string specially. Try to keep copy on write state when
  // using old abi.
  inline MaybeArenaStringAccessor& operator=(
      const ::std::string& other) noexcept;
  inline MaybeArenaStringAccessor& operator=(::std::string& other) noexcept;
  inline MaybeArenaStringAccessor& operator=(::std::string&& other) noexcept;
  // Support api of ArenaStringPtr and InlinedStringField
  template <typename T>
  inline MaybeArenaStringAccessor& operator=(
      ::std::reference_wrapper<T> other) noexcept;
  template <typename T>
  inline MaybeArenaStringAccessor& assign(T&& other) noexcept;
  inline MaybeArenaStringAccessor& assign(const_pointer data,
                                          size_type size) noexcept;
  inline MaybeArenaStringAccessor& assign(const ::std::string& other) noexcept;
  inline MaybeArenaStringAccessor& assign(::std::string& other) noexcept;
  inline MaybeArenaStringAccessor& assign(::std::string&& other) noexcept;
  template <typename T>
  inline MaybeArenaStringAccessor& assign(
      ::std::reference_wrapper<T> other) noexcept;

  // Element access
  inline reference operator[](size_type position) noexcept;
  using ArenaStringAccessor::operator[];
  using ArenaStringAccessor::c_str;
  using ArenaStringAccessor::data;
  using ArenaStringAccessor::operator ::absl::string_view;
  using ArenaStringAccessor::operator const ::std::string&;

  // Iterators
  inline iterator begin() noexcept;
  using ArenaStringAccessor::begin;
  using ArenaStringAccessor::cbegin;
  inline iterator end() noexcept;
  using ArenaStringAccessor::cend;
  using ArenaStringAccessor::end;

  // Capacity
  using ArenaStringAccessor::empty;
  using ArenaStringAccessor::size;
  inline void reserve(size_type required_capacity) noexcept;
  using ArenaStringAccessor::capacity;

  // Modifiers
  inline void clear() noexcept;
  inline void push_back(value_type c) noexcept;
  inline MaybeArenaStringAccessor& append(::absl::string_view sv) noexcept;
  inline MaybeArenaStringAccessor& append(const_pointer append_data,
                                          size_type append_size) noexcept;
  inline MaybeArenaStringAccessor& operator+=(char ch) noexcept;
  inline MaybeArenaStringAccessor& operator+=(::absl::string_view sv) noexcept;
  inline void resize(size_type new_size) noexcept;
  inline void resize(size_type new_size, value_type c) noexcept;
  using ArenaStringAccessor::swap;

  ////////////////////////////////////////////////////////////////////////////
  // Special functions
  // Create string maybe on arena
  inline static MaybeArenaStringAccessor create(Arena* arena) noexcept;
  template <typename T>
  inline static MaybeArenaStringAccessor create(Arena* arena,
                                                T&& value) noexcept;
  // Clear string maybe on arena
  inline static void clear(::std::string* ptr) noexcept;
  // Wrap constructor
  inline MaybeArenaStringAccessor(::std::string* string) noexcept;
  using ArenaStringAccessor::arena;
  using ArenaStringAccessor::underlying;
  // Support absl::strings_internal::STLStringResizeUninitialized
  inline void __resize_default_init(size_type new_size) noexcept;
  // Make operator* and operator-> both to self to imitate a string*
  inline MaybeArenaStringAccessor* operator->() noexcept;
  inline const MaybeArenaStringAccessor* operator->() const noexcept;
  inline MaybeArenaStringAccessor& operator*() noexcept;
  inline const MaybeArenaStringAccessor& operator*() const noexcept;
  // Destroy string if not on arena
  inline void destroy() noexcept;
  // Also support absl::Format(MaybeArenaStringAccessor, ...)
  inline operator ::absl::FormatRawSink() noexcept;
  ////////////////////////////////////////////////////////////////////////////

 private:
  // Support absl::Format(MaybeArenaStringAccessor*, ...)
  friend inline void AbslFormatFlush(MaybeArenaStringAccessor* accessor,
                                     ::absl::string_view sv) noexcept {
    accessor->append(sv.data(), sv.size());
  }
};

#if GOOGLE_PROTOBUF_MUTABLE_DONATED_STRING
using MutableStringType = MaybeArenaStringAccessor;
using MutableStringReferenceType = MaybeArenaStringAccessor;
#else   // !GOOGLE_PROTOBUF_MUTABLE_DONATED_STRING
using MutableStringType = ::std::string*;
using MutableStringReferenceType = ::std::string&;
#endif  // !GOOGLE_PROTOBUF_MUTABLE_DONATED_STRING

////////////////////////////////////////////////////////////////////////////////
// ArenaStringAccessor begin
inline ArenaStringAccessor& ArenaStringAccessor::operator=(
    ::absl::string_view other) noexcept {
  return assign(other.data(), other.size());
}

inline ArenaStringAccessor& ArenaStringAccessor::assign(
    ::absl::string_view other) noexcept {
  return assign(other.data(), other.size());
}

inline ArenaStringAccessor& ArenaStringAccessor::assign(
    const_pointer data, size_type size) noexcept {
  auto buffer = qualified_buffer(size);
  set_size(size);
  __builtin_memcpy(buffer, data, size);
  buffer[size] = '\0';
  return *this;
}

inline ArenaStringAccessor::reference ArenaStringAccessor::operator[](
    size_type position) noexcept {
  return writable_buffer()[position];
}

inline ArenaStringAccessor::const_reference ArenaStringAccessor::operator[](
    size_type position) const noexcept {
  return data()[position];
}

inline ArenaStringAccessor::const_pointer ArenaStringAccessor::data()
    const noexcept {
  return c_str();
}

inline ArenaStringAccessor::const_pointer ArenaStringAccessor::c_str()
    const noexcept {
  return static_cast<const ::std::string*>(_ptr)->c_str();
}

inline ArenaStringAccessor::operator ::absl::string_view() const noexcept {
  return ::absl::string_view(data(), size());
}

inline ArenaStringAccessor::operator const ::std::string&() const noexcept {
  return *_ptr;
}

inline ArenaStringAccessor::iterator ArenaStringAccessor::begin() noexcept {
  return iterator(writable_buffer());
}

inline ArenaStringAccessor::const_iterator ArenaStringAccessor::begin()
    const noexcept {
  return cbegin();
}

inline ArenaStringAccessor::const_iterator ArenaStringAccessor::cbegin()
    const noexcept {
  return const_iterator(data());
}

inline ArenaStringAccessor::iterator ArenaStringAccessor::end() noexcept {
  return iterator(writable_buffer() + size());
}

inline ArenaStringAccessor::const_iterator ArenaStringAccessor::end()
    const noexcept {
  return cend();
}

inline ArenaStringAccessor::const_iterator ArenaStringAccessor::cend()
    const noexcept {
  return const_iterator(data() + size());
}

inline bool ArenaStringAccessor::empty() const noexcept {
  return _ptr->empty();
}

inline ArenaStringAccessor::size_type ArenaStringAccessor::size()
    const noexcept {
  return _ptr->size();
}

inline void ArenaStringAccessor::reserve(size_type required_capacity) noexcept {
  if (required_capacity > capacity()) {
    auto origin_size = size();
    recreate_buffer(required_capacity);
    set_size_and_terminator(origin_size);
  }
}

inline ArenaStringAccessor::size_type ArenaStringAccessor::capacity()
    const noexcept {
  return _ptr->capacity();
}

inline void ArenaStringAccessor::clear() noexcept {
  set_size_and_terminator(0);
}

inline void ArenaStringAccessor::push_back(value_type c) noexcept {
  auto origin_size = size();
  auto buffer = qualified_buffer(origin_size + 1, origin_size << 1);
  set_size(origin_size + 1);
  buffer[origin_size] = c;
  buffer[origin_size + 1] = '\0';
}

inline ArenaStringAccessor& ArenaStringAccessor::append(
    ::absl::string_view sv) noexcept {
  return append(sv.data(), sv.size());
}

inline ArenaStringAccessor& ArenaStringAccessor::append(
    const_pointer append_data, size_type append_size) noexcept {
  auto origin_size = size();
  auto buffer = qualified_buffer(origin_size + append_size);
  set_size(origin_size + append_size);
  __builtin_memcpy(buffer + origin_size, append_data, append_size);
  buffer[origin_size + append_size] = '\0';
  return *this;
}

inline ArenaStringAccessor& ArenaStringAccessor::operator+=(char ch) noexcept {
  push_back(ch);
  return *this;
}

inline ArenaStringAccessor& ArenaStringAccessor::operator+=(
    ::absl::string_view sv) noexcept {
  return append(sv.data(), sv.size());
}

inline void ArenaStringAccessor::resize(size_type new_size) noexcept {
  resize(new_size, '\0');
}

inline void ArenaStringAccessor::resize(size_type new_size,
                                        value_type c) noexcept {
  auto origin_size = size();
  auto buffer = qualified_buffer(new_size);
  set_size_and_terminator(new_size);
  if (new_size > origin_size) {
    __builtin_memset(buffer + origin_size, c, new_size - origin_size);
  }
}

inline void ArenaStringAccessor::swap(ArenaStringAccessor other) noexcept {
#if __GLIBCXX__ && !_GLIBCXX_USE_CXX11_ABI
  auto tmp = representation().data;
  *reinterpret_cast<pointer*>(_ptr) = other.representation().data;
  *reinterpret_cast<pointer*>(other._ptr) = tmp;
#else   // !__GLIBCXX__ || _GLIBCXX_USE_CXX11_ABI
  _ptr->swap(*other._ptr);
#endif  // !__GLIBCXX__ || _GLIBCXX_USE_CXX11_ABI
}

inline int ArenaStringAccessor::compare(
    ::absl::string_view other) const noexcept {
  return static_cast<::absl::string_view>(*this).compare(other);
}

inline ArenaStringAccessor ArenaStringAccessor::create(Arena* arena) noexcept {
  auto ptr = reinterpret_cast<::std::string*>(
      arena->AllocateAligned(sizeof(::std::string)));
  new (ptr)::std::string();
  return ArenaStringAccessor(arena, ptr);
}

template <typename T>
inline ArenaStringAccessor ArenaStringAccessor::create(Arena* arena,
                                                       T&& value) noexcept {
  return create(arena) = ::std::forward<T>(value);
}

inline void ArenaStringAccessor::swap(::std::string* left,
                                      ::std::string* right) noexcept {
  ArenaStringAccessor(nullptr, left).swap(ArenaStringAccessor(nullptr, right));
}

inline ArenaStringAccessor::ArenaStringAccessor(Arena* arena,
                                                ::std::string* ptr) noexcept
    : _arena(arena), _ptr(ptr) {}

inline Arena* ArenaStringAccessor::arena() const noexcept { return _arena; }
inline ::std::string* ArenaStringAccessor::underlying() const noexcept {
  return _ptr;
}

inline char* ArenaStringAccessor::__resize_default_init(
    size_type new_size) noexcept {
  auto buffer = qualified_buffer(new_size);
  set_size_and_terminator(new_size);
  return buffer;
}

inline internal::StdStringRep& ArenaStringAccessor::representation() noexcept {
#if __GLIBCXX__ && !_GLIBCXX_USE_CXX11_ABI
  return *(*reinterpret_cast<internal::StdStringRep**>(_ptr) - 1);
#else   // !__GLIBCXX__ || _GLIBCXX_USE_CXX11_ABI
  return *reinterpret_cast<internal::StdStringRep*>(_ptr);
#endif  // !__GLIBCXX__ || _GLIBCXX_USE_CXX11_ABI
}

inline ArenaStringAccessor::pointer ArenaStringAccessor::qualified_buffer(
    size_type required_capacity, size_type predict_capacity) noexcept {
  return required_capacity <= capacity() ? writable_buffer()
                                         : recreate_buffer(predict_capacity);
}

inline ArenaStringAccessor::pointer ArenaStringAccessor::qualified_buffer(
    size_type required_capacity) noexcept {
  return qualified_buffer(required_capacity, required_capacity);
}

inline ArenaStringAccessor::pointer
ArenaStringAccessor::writable_buffer() noexcept {
#if __GLIBCXX__ && !_GLIBCXX_USE_CXX11_ABI
  return representation().data;
#else   // !__GLIBCXX__ || _GLIBCXX_USE_CXX11_ABI
  return &(*_ptr)[0];
#endif  // !__GLIBCXX__ || _GLIBCXX_USE_CXX11_ABI
}

inline ArenaStringAccessor::pointer ArenaStringAccessor::recreate_buffer(
    size_type capacity) noexcept {
#if __GLIBCXX__
  size_t buffer_size = capacity + 1;
  buffer_size = (buffer_size + 7) & static_cast<size_t>(-8);
  capacity = buffer_size - 1;
#if _GLIBCXX_USE_CXX11_ABI
  auto buffer = reinterpret_cast<char*>(_arena->AllocateAligned(buffer_size));
  __builtin_memcpy(buffer, data(), size());
  auto& rep = representation();
  rep.data = buffer;
  rep.capacity = capacity;
  return buffer;
#else   // !_GLIBCXX_USE_CXX11_ABI
  auto rep = reinterpret_cast<StdStringRep*>(
      _arena->AllocateAligned(sizeof(StdStringRep) + buffer_size));
  rep->capacity = capacity;
  rep->refcount = -1;
  __builtin_memcpy(rep->data, data(), size());
  *reinterpret_cast<pointer*>(_ptr) = rep->data;
  return rep->data;
#endif  // !_GLIBCXX_USE_CXX11_ABI
#else   // !__GLIBCXX__
  capacity = (capacity + 16) & static_cast<size_type>(-16);
  auto buffer = reinterpret_cast<pointer>(_arena->AllocateAligned(capacity));
  __builtin_memcpy(buffer, data(), size());
  auto& rep = representation();
  rep.long_format.data = buffer;
  rep.long_format.capacity = capacity + 1;
  return rep.long_format.data;
#endif  // !__GLIBCXX__
}

inline void ArenaStringAccessor::set_size(size_type size) noexcept {
#if __GLIBCXX__
  auto& rep = representation();
  rep.size = size;
#else   // !__GLIBCXX__
  auto& rep = representation();
  if (rep.is_long()) {
    rep.long_format.size = size;
  } else {
    rep.shot_format.size = size << 1;
  }
#endif  // !__GLIBCXX__
}

inline void ArenaStringAccessor::set_size_and_terminator(
    size_type size) noexcept {
#if __GLIBCXX__
  auto& rep = representation();
  rep.size = size;
  rep.data[size] = '\0';
#else   // !__GLIBCXX__
  auto& rep = representation();
  if (rep.is_long()) {
    rep.long_format.size = size;
    rep.long_format.data[size] = '\0';
  } else {
    rep.shot_format.size = size << 1;
    rep.shot_format.data[size] = '\0';
  }
#endif  // !__GLIBCXX__
}

inline bool operator==(const ArenaStringAccessor& left,
                       const ArenaStringAccessor& right) noexcept {
  return *left.underlying() == *right.underlying();
}

inline bool operator==(::absl::string_view left,
                       const ArenaStringAccessor& right) noexcept {
  return left == *right.underlying();
}

inline bool operator==(const ArenaStringAccessor& left,
                       ::absl::string_view right) noexcept {
  return *left.underlying() == right;
}

inline bool operator!=(const ArenaStringAccessor& left,
                       const ArenaStringAccessor& right) noexcept {
  return !(left == right);
}

inline bool operator!=(::absl::string_view left,
                       const ArenaStringAccessor& right) noexcept {
  return !(left == right);
}

inline bool operator!=(const ArenaStringAccessor& left,
                       ::absl::string_view right) noexcept {
  return !(left == right);
}

inline bool operator<(const ArenaStringAccessor& left,
                      const ArenaStringAccessor& right) noexcept {
  return *left.underlying() < *right.underlying();
}

inline bool operator<(::absl::string_view left,
                      const ArenaStringAccessor& right) noexcept {
  return left < *right.underlying();
}

inline bool operator<(const ArenaStringAccessor& left,
                      ::absl::string_view right) noexcept {
  return *left.underlying() < right;
}

inline bool operator<=(const ArenaStringAccessor& left,
                       const ArenaStringAccessor& right) noexcept {
  return *left.underlying() <= *right.underlying();
}

inline bool operator<=(::absl::string_view left,
                       const ArenaStringAccessor& right) noexcept {
  return left <= *right.underlying();
}

inline bool operator<=(const ArenaStringAccessor& left,
                       ::absl::string_view right) noexcept {
  return *left.underlying() <= right;
}

inline bool operator>(const ArenaStringAccessor& left,
                      const ArenaStringAccessor& right) noexcept {
  return *left.underlying() > *right.underlying();
}

inline bool operator>(::absl::string_view left,
                      const ArenaStringAccessor& right) noexcept {
  return left > *right.underlying();
}

inline bool operator>(const ArenaStringAccessor& left,
                      ::absl::string_view right) noexcept {
  return *left.underlying() > right;
}

inline bool operator>=(const ArenaStringAccessor& left,
                       const ArenaStringAccessor& right) noexcept {
  return *left.underlying() >= *right.underlying();
}

inline bool operator>=(::absl::string_view left,
                       const ArenaStringAccessor& right) noexcept {
  return left >= *right.underlying();
}

inline bool operator>=(const ArenaStringAccessor& left,
                       ::absl::string_view right) noexcept {
  return *left.underlying() >= right;
}
// ArenaStringAccessor end
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// MaybeArenaStringAccessor begin
inline MaybeArenaStringAccessor::MaybeArenaStringAccessor(
    const ArenaStringAccessor& other) noexcept
    : ArenaStringAccessor(other) {}

template <typename T>
inline MaybeArenaStringAccessor& MaybeArenaStringAccessor::operator=(
    T&& other) noexcept {
  return assign(::std::forward<T>(other));
}
template <typename T>
inline MaybeArenaStringAccessor& MaybeArenaStringAccessor::assign(
    T&& other) noexcept {
  ::absl::string_view sv(::std::forward<T>(other));
  return assign(sv.data(), sv.size());
}
inline MaybeArenaStringAccessor& MaybeArenaStringAccessor::assign(
    const_pointer data, size_type size) noexcept {
  if (arena() != nullptr) {
    ArenaStringAccessor::assign(data, size);
  } else {
    underlying()->assign(data, size);
  }
  return *this;
}

// Deal with assign string specially. Try to keep copy on write state when
// using old abi
inline MaybeArenaStringAccessor& MaybeArenaStringAccessor::operator=(
    const ::std::string& other) noexcept {
  return assign(other);
}

inline MaybeArenaStringAccessor& MaybeArenaStringAccessor::assign(
    const ::std::string& other) noexcept {
  if (arena() != nullptr) {
    ArenaStringAccessor::assign(other);
  } else {
    underlying()->assign(other);
  }
  return *this;
}

inline MaybeArenaStringAccessor& MaybeArenaStringAccessor::operator=(
    ::std::string& other) noexcept {
  return assign(static_cast<const ::std::string&>(other));
}

inline MaybeArenaStringAccessor& MaybeArenaStringAccessor::assign(
    ::std::string& other) noexcept {
  return assign(static_cast<const ::std::string&>(other));
}

inline MaybeArenaStringAccessor& MaybeArenaStringAccessor::operator=(
    ::std::string&& other) noexcept {
  return assign(::std::move(other));
}

inline MaybeArenaStringAccessor& MaybeArenaStringAccessor::assign(
    ::std::string&& other) noexcept {
  if (arena() != nullptr) {
    ArenaStringAccessor::assign(other);
  } else {
    underlying()->assign(::std::move(other));
  }
  return *this;
}

template <typename T>
inline MaybeArenaStringAccessor& MaybeArenaStringAccessor::operator=(
    ::std::reference_wrapper<T> other) noexcept {
  return assign(other);
}

template <typename T>
inline MaybeArenaStringAccessor& MaybeArenaStringAccessor::assign(
    ::std::reference_wrapper<T> other) noexcept {
  return assign(other.get());
}

inline MaybeArenaStringAccessor::reference MaybeArenaStringAccessor::operator[](
    size_type position) noexcept {
#if __GLIBCXX__ && !_GLIBCXX_USE_CXX11_ABI
  if (representation().refcount >= 0) {
    return underlying()->operator[](position);
  }
#endif  // !__GLIBCXX__ || _GLIBCXX_USE_CXX11_ABI
  return ArenaStringAccessor::operator[](position);
}

inline MaybeArenaStringAccessor::iterator
MaybeArenaStringAccessor::begin() noexcept {
#if __GLIBCXX__ && !_GLIBCXX_USE_CXX11_ABI
  if (representation().refcount >= 0) {
    return underlying()->begin();
  }
#endif  // !__GLIBCXX__ || _GLIBCXX_USE_CXX11_ABI
  return ArenaStringAccessor::begin();
}

inline MaybeArenaStringAccessor::iterator
MaybeArenaStringAccessor::end() noexcept {
#if __GLIBCXX__ && !_GLIBCXX_USE_CXX11_ABI
  if (representation().refcount >= 0) {
    return underlying()->end();
  }
#endif  // !__GLIBCXX__ || _GLIBCXX_USE_CXX11_ABI
  return ArenaStringAccessor::end();
}

inline void MaybeArenaStringAccessor::reserve(
    size_type required_capacity) noexcept {
  if (arena() != nullptr) {
    ArenaStringAccessor::reserve(required_capacity);
  } else if (required_capacity > capacity()) {
    underlying()->reserve(required_capacity);
  }
}

inline void MaybeArenaStringAccessor::clear() noexcept {
#if __GLIBCXX__ && !_GLIBCXX_USE_CXX11_ABI
  if (representation().refcount <= 0) {
    ArenaStringAccessor::clear();
    return;
  }
#endif  // !__GLIBCXX__ || _GLIBCXX_USE_CXX11_ABI
  underlying()->clear();
}

inline void MaybeArenaStringAccessor::push_back(value_type c) noexcept {
  if (arena() != nullptr) {
    ArenaStringAccessor::push_back(c);
  } else {
    underlying()->push_back(c);
  }
}

inline MaybeArenaStringAccessor& MaybeArenaStringAccessor::append(
    ::absl::string_view sv) noexcept {
  return append(sv.data(), sv.size());
}

inline MaybeArenaStringAccessor& MaybeArenaStringAccessor::append(
    const_pointer data, size_type size) noexcept {
  if (arena() != nullptr) {
    ArenaStringAccessor::append(data, size);
  } else {
    underlying()->append(data, size);
  }
  return *this;
}

inline MaybeArenaStringAccessor& MaybeArenaStringAccessor::operator+=(
    char ch) noexcept {
  push_back(ch);
  return *this;
}

inline MaybeArenaStringAccessor& MaybeArenaStringAccessor::operator+=(
    ::absl::string_view sv) noexcept {
  return append(sv.data(), sv.size());
}

inline void MaybeArenaStringAccessor::resize(size_type size) noexcept {
  if (arena() != nullptr) {
    ArenaStringAccessor::resize(size);
  } else {
    underlying()->resize(size);
  }
}

inline void MaybeArenaStringAccessor::resize(size_type size,
                                             value_type c) noexcept {
  if (arena() != nullptr) {
    ArenaStringAccessor::resize(size, c);
  } else {
    underlying()->resize(size, c);
  }
}

inline MaybeArenaStringAccessor MaybeArenaStringAccessor::create(
    Arena* arena) noexcept {
  if (arena != nullptr) {
    return ArenaStringAccessor::create(arena);
  } else {
    return MaybeArenaStringAccessor(new ::std::string);
  }
}

template <typename T>
inline MaybeArenaStringAccessor MaybeArenaStringAccessor::create(
    Arena* arena, T&& value) noexcept {
  return create(arena) = ::std::forward<T>(value);
}

inline void MaybeArenaStringAccessor::clear(::std::string* ptr) noexcept {
  MaybeArenaStringAccessor(ptr).clear();
}

inline MaybeArenaStringAccessor::MaybeArenaStringAccessor(
    ::std::string* string) noexcept
    : ArenaStringAccessor(nullptr, string) {}

inline void MaybeArenaStringAccessor::__resize_default_init(
    size_type new_size) noexcept {
  if (arena() != nullptr) {
    ArenaStringAccessor::__resize_default_init(new_size);
  } else {
    ::absl::strings_internal::STLStringResizeUninitialized(underlying(),
                                                           new_size);
  }
}

inline MaybeArenaStringAccessor*
MaybeArenaStringAccessor::operator->() noexcept {
  return this;
}

inline const MaybeArenaStringAccessor* MaybeArenaStringAccessor::operator->()
    const noexcept {
  return this;
}

inline MaybeArenaStringAccessor&
MaybeArenaStringAccessor::operator*() noexcept {
  return *this;
}

inline const MaybeArenaStringAccessor& MaybeArenaStringAccessor::operator*()
    const noexcept {
  return *this;
}

inline void MaybeArenaStringAccessor::destroy() noexcept {
  if (arena() == nullptr) {
    delete underlying();
  }
}

inline MaybeArenaStringAccessor::operator ::absl::FormatRawSink() noexcept {
  return ::absl::FormatRawSink(this);
}
// MaybeArenaStringAccessor end
////////////////////////////////////////////////////////////////////////////////

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
