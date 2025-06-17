// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

#pragma once

#include <OpenImageIO/ustring.h>

#include "pl/strong_param.h"
#include "pl/type_desc.h"

CCL_NAMESPACE_BEGIN

using OIIO::ustring;
using OIIO::ustringhash;

/// ParamValue holds a named parameter and typed data. Usually, it owns the
/// data (holding it in the struct itself if small enough, dynamically
/// allocated for larger things), but it can also refer to non-owned data.
///
/// The data is usually a single value of any type described by TypeDesc
/// (including arrays). It may also hold more than one value of the type --
/// this is usually only used in a geometric context, such as storing a value
/// for each vertex in a mesh. Please note the subtle distinction between the
/// value type itself being an array, versus having multiple values as a
/// parameter, versus the type of the value having multiple components (such
/// as a point or color). Any combination of these may be present.
///
/// To clarify, if you have an array of 4 colors for each of 15 mesh
/// vertices, that means:
///  - There are 15 VALUES (one for each vertex)
///  - Each value has an array of 4 ELEMENTS
///  - Each element is a color
///  - A color has 3 COMPONENTS (R, G, B)
/// Thus, it would be constructed as
/// `ParamValue("mycolor", TypeDesc(TypeDesc::COLOR, 4), 15, ptr_to_data)`
///
/// The main constructor is `ParamValue(name, type, nvalues, dataptr)`. It can
/// be confusing at first to remember that the data argument is a pointer to
/// the first values to copy, not the values themselves, even if the values
/// are themselves pointers, and even if the number of values is 1. In other
/// words, it's behaving as if you're always pointing it to an array even if
/// the "array" has only one element. This is extra confusing for strings,
/// because the strings themselves are `char*` (or ustring), so the pointer
/// you need to pass is `char**`. For this reason, there are also convenience
/// constructors for simple types such as a single int, float, or string.
///
/// So here are some examples:
///
///     // Single int:
///     int my_int = 42;
///     ParamValue A("foo", TypeDesc::INT, 1, &my_int);
///     // Three int values (say, one per vertex of a triangle):
///     int my_int_array[3] = { 1, 2, 3 };
///     ParamValue B("foo", TypeDesc::INT, 1, &my_int_array);
///     // A single value which is an array of 3 ints:
///     ParamValue C("foo", TypeDesc(TypeDesc::INT, 3), 1, &my_int_array);
///     // A string -- note the trick about treating it as an array:
///     const char* my_string = "hello";
///     ParamValue D("foo", TypeDesc::STRING, 1, &my_string);
///
///     // The most common cases also have simple "duck-typed" convenience
///     // constructors:
///     ParamValue A("foo", 42);           // single int
///     ParamValue B("foo", 42.0f);        // single float
///     ParamValue C("foo", "forty two");  // single string

class ParamValue {
 public:
  /// Interpolation types
  ///
  enum Interp {
    INTERP_CONSTANT = 0,  ///< Constant for all pieces/faces
    INTERP_PERPIECE = 1,  ///< Piecewise constant per piece/face
    INTERP_LINEAR = 2,    ///< Linearly interpolated across each piece/face
    INTERP_VERTEX = 3     ///< Interpolated like vertices
  };

  CCL_STRONG_PARAM_TYPE(Copy, bool);
  CCL_STRONG_PARAM_TYPE(FromUstring, bool);

  ParamValue() noexcept
  {
    m_data.ptr = nullptr;
  }

  ParamValue(const ustring &_name,
             TypeDesc _type,
             int _nvalues,
             const void *_value,
             Copy _copy = Copy(true)) noexcept
  {
    init_noclear(_name, _type, _nvalues, _value, _copy);
  }
  ParamValue(const ustring &_name,
             TypeDesc _type,
             int _nvalues,
             Interp _interp,
             const void *_value,
             Copy _copy = Copy(true)) noexcept
  {
    init_noclear(_name, _type, _nvalues, _interp, _value, _copy);
  }
  ParamValue(string_view _name,
             TypeDesc _type,
             int _nvalues,
             const void *_value,
             Copy _copy = Copy(true)) noexcept
  {
    init_noclear(ustring(_name), _type, _nvalues, _value, _copy);
  }
  ParamValue(string_view _name,
             TypeDesc _type,
             int _nvalues,
             Interp _interp,
             const void *_value,
             Copy _copy = Copy(true)) noexcept
  {
    init_noclear(ustring(_name), _type, _nvalues, _interp, _value, _copy);
  }

  ParamValue(string_view _name, int value) noexcept
  {
    init_noclear(ustring(_name), TypeDesc::INT, 1, &value);
  }
  ParamValue(string_view _name, float value) noexcept
  {
    init_noclear(ustring(_name), TypeDesc::FLOAT, 1, &value);
  }
  ParamValue(string_view _name, ustring value) noexcept
  {
    init_noclear(ustring(_name), TypeDesc::STRING, 1, &value, Copy(true), FromUstring(true));
  }
  ParamValue(string_view _name, string_view value) noexcept : ParamValue(_name, ustring(value)) {}
  ParamValue(string_view _name, ustringhash value) noexcept
  {
    init_noclear(ustring(_name), TypeDesc::USTRINGHASH, 1, &value, Copy(true));
  }

  // Set from string -- parse
  ParamValue(string_view _name, TypeDesc type, string_view value);

  // Copy constructor
  ParamValue(const ParamValue &p) noexcept
  {
    init_noclear(
        p.name(), p.type(), p.nvalues(), p.interp(), p.data(), Copy(true), FromUstring(true));
  }
  ParamValue(const ParamValue &p, Copy _copy) noexcept
  {
    init_noclear(p.name(), p.type(), p.nvalues(), p.interp(), p.data(), _copy, FromUstring(true));
  }

  // Rvalue (move) constructor
  ParamValue(ParamValue &&p) noexcept
  {
    init_noclear(
        p.name(), p.type(), p.nvalues(), p.interp(), p.data(), Copy(false), FromUstring(true));
    m_copy = p.m_copy;
    m_nonlocal = p.m_nonlocal;
    p.m_data.ptr = nullptr;  // make sure the old one won't free
  }

  ~ParamValue() noexcept
  {
    clear_value();
  }

  void init(ustring _name,
            TypeDesc _type,
            int _nvalues,
            Interp _interp,
            const void *_value,
            Copy _copy) noexcept
  {
    clear_value();
    init_noclear(_name, _type, _nvalues, _interp, _value, _copy);
  }
  void init(ustring _name,
            TypeDesc _type,
            int _nvalues,
            const void *_value,
            Copy _copy = Copy(true)) noexcept
  {
    init(_name, _type, _nvalues, INTERP_CONSTANT, _value, _copy);
  }
  void init(string_view _name,
            TypeDesc _type,
            int _nvalues,
            const void *_value,
            Copy _copy = Copy(true)) noexcept
  {
    init(ustring(_name), _type, _nvalues, _value, _copy);
  }
  void init(string_view _name,
            TypeDesc _type,
            int _nvalues,
            Interp _interp,
            const void *_value,
            Copy _copy = Copy(true)) noexcept
  {
    init(ustring(_name), _type, _nvalues, _interp, _value, _copy);
  }

  // Assignment
  const ParamValue &operator=(const ParamValue &p) noexcept;
  const ParamValue &operator=(ParamValue &&p) noexcept;

  // FIXME -- some time in the future (after more cleanup), we should make
  // name() return a string_view, and use uname() for the rare time when
  // the caller truly requires the ustring.
  const ustring &name() const noexcept
  {
    return m_name;
  }
  const ustring &uname() const noexcept
  {
    return m_name;
  }
  TypeDesc type() const noexcept
  {
    return m_type;
  }
  int nvalues() const noexcept
  {
    return m_nvalues;
  }
  const void *data() const noexcept
  {
    return m_nonlocal ? m_data.ptr : &m_data;
  }
  int datasize() const noexcept
  {
    return m_nvalues * static_cast<int>(m_type.size());
  }
  Interp interp() const noexcept
  {
    return (Interp)m_interp;
  }
  void interp(Interp i) noexcept
  {
    m_interp = (unsigned char)i;
  }
  bool is_nonlocal() const noexcept
  {
    return m_nonlocal;
  }

  friend void swap(ParamValue &a, ParamValue &b) noexcept
  {
    auto tmp = std::move(a);
    a = std::move(b);
    b = std::move(tmp);
  }

#if 0
    /// Return a `cspan<T>` pointing to the bounded data. If the type `T` is
    /// not the actual underlying base type, return an empty span.
    template<typename T> cspan<T> as_cspan() const noexcept
    {
        return BaseTypeFromC<T>::value == m_type.basetype
                   ? make_cspan(reinterpret_cast<const T*>(data()),
                                size_t(m_nvalues * m_type.basevalues()))
                   : cspan<T>();
    }

    /// Return a `span<T>` pointing to the bounded data. If the type `T` is
    /// not the actual underlying base type, return an empty span.
    template<typename T> span<T> as_span() noexcept
    {
        return BaseTypeFromC<T>::value == m_type.basetype
                   ? make_span(reinterpret_cast<T*>(const_cast<void*>(data())),
                               size_t(m_nvalues * m_type.basevalues()))
                   : span<T>();
    }
#endif

  // Use with extreme caution! This is just doing a cast. You'd better
  // be really sure you are asking for the right type. Note that for
  // "string" data, you can get<ustring> or get<char*>, but it's not
  // a std::string.
  template<typename T> const T &get(int i = 0) const noexcept
  {
    OIIO_DASSERT(i >= 0 && i < int(m_nvalues * m_type.basevalues()) &&
                 "OIIO::ParamValue::get() range check");
    return (reinterpret_cast<const T *>(data()))[i];
  }

  /// Retrieve an integer, with conversions from a wide variety of type
  /// cases, including unsigned, short, byte. Not float. It will retrieve
  /// from a string, but only if the string is entirely a valid int
  /// format. Unconvertible types return the default value.
  int get_int(int defaultval = 0) const;
  int get_int_indexed(int index, int defaultval = 0) const;

  /// Retrieve a float, with conversions from a wide variety of type
  /// cases, including integers. It will retrieve from a string, but only
  /// if the string is entirely a valid float format. Unconvertible types
  /// return the default value.
  float get_float(float defaultval = 0) const;
  float get_float_indexed(int index, float defaultval = 0) const;

  /// Convert any type to a string value. An optional maximum number of
  /// elements is also passed. In the case of a single string, just the
  /// string directly is returned. But for an array of strings, the array
  /// is returned as one string that's a comma-separated list of double-
  /// quoted, escaped strings. For an array or aggregate, at most `maxsize`
  /// elements are returned (if `maxsize` is 0, all elements are returned,
  /// no matter how large it is).
  std::string get_string(int maxsize = 64) const;
  std::string get_string_indexed(int index) const;
  /// Convert any type to a ustring value. An optional maximum number of
  /// elements is also passed. Same behavior as get_string, but returning a
  /// ustring. For an array or aggregate, at most `maxsize` elements are
  /// returned (if `maxsize` is 0, all elements are returned, no matter how
  /// large it is).
  ustring get_ustring(int maxsize = 64) const;
  ustring get_ustring_indexed(int index) const;

 private:
  ustring m_name;   ///< data name
  TypeDesc m_type;  ///< data type, which may itself be an array
  union {
    char localval[16];
    const void *ptr;
  } m_data;                                  ///< Our data, either a pointer or small local value
  int m_nvalues = 0;                         ///< number of values of the given type
  unsigned char m_interp = INTERP_CONSTANT;  ///< Interpolation type
  bool m_copy = false;
  bool m_nonlocal = false;

  void init_noclear(ustring _name,
                    TypeDesc _type,
                    int _nvalues,
                    const void *_value,
                    Copy _copy = Copy(true),
                    FromUstring _from_ustring = FromUstring(false)) noexcept;
  void init_noclear(ustring _name,
                    TypeDesc _type,
                    int _nvalues,
                    Interp _interp,
                    const void *_value,
                    Copy _copy = Copy(true),
                    FromUstring _from_ustring = FromUstring(false)) noexcept;
  void clear_value() noexcept;
};

CCL_NAMESPACE_END
