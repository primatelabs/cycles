// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

/* SPDX-FileCopyrightText: 2025 Primate Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include <limits>
#include <string_view>

#include "pl/error.h"

namespace ccl {

using std::string_view;

struct TypeDesc {
  /// BASETYPE is a simple enum describing the base data types that
  /// correspond (mostly) to the C/C++ built-in types.
  enum BASETYPE {
    UNKNOWN,  ///< unknown type
    NONE,     ///< void/no type
    UINT8,    ///< 8-bit unsigned int values ranging from 0..255,
              ///<   (C/C++ `unsigned char`).
    UCHAR = UINT8,
    INT8,  ///< 8-bit int values ranging from -128..127,
           ///<   (C/C++ `char`).
    CHAR = INT8,
    UINT16,  ///< 16-bit int values ranging from 0..65535,
             ///<   (C/C++ `unsigned short`).
    USHORT = UINT16,
    INT16,  ///< 16-bit int values ranging from -32768..32767,
            ///<   (C/C++ `short`).
    SHORT = INT16,
    UINT32,  ///< 32-bit unsigned int values (C/C++ `unsigned int`).
    UINT = UINT32,
    INT32,  ///< signed 32-bit int values (C/C++ `int`).
    INT = INT32,
    UINT64,  ///< 64-bit unsigned int values (C/C++
             ///<   `unsigned long long` on most architectures).
    ULONGLONG = UINT64,
    INT64,  ///< signed 64-bit int values (C/C++ `long long`
            ///<   on most architectures).
    LONGLONG = INT64,
    HALF,         ///< 16-bit IEEE floating point values (OpenEXR `half`).
    FLOAT,        ///< 32-bit IEEE floating point values, (C/C++ `float`).
    DOUBLE,       ///< 64-bit IEEE floating point values, (C/C++ `double`).
    STRING,       ///< Character string.
    PTR,          ///< A pointer value.
    LASTBASE
  };

  /// AGGREGATE describes whether our TypeDesc is a simple scalar of one
  /// of the BASETYPE's, or one of several simple aggregates.
  ///
  /// Note that aggregates and arrays are different. A `TypeDesc(FLOAT,3)`
  /// is an array of three floats, a `TypeDesc(FLOAT,VEC3)` is a single
  /// 3-component vector comprised of floats, and `TypeDesc(FLOAT,3,VEC3)`
  /// is an array of 3 vectors, each of which is comprised of 3 floats.
  enum AGGREGATE {
    SCALAR = 1,    ///< A single scalar value (such as a raw `int` or
                   ///<   `float` in C).  This is the default.
    VEC2 = 2,      ///< 2 values representing a 2D vector.
    VEC3 = 3,      ///< 3 values representing a 3D vector.
    VEC4 = 4,      ///< 4 values representing a 4D vector.
    MATRIX33 = 9,  ///< 9 values representing a 3x3 matrix.
    MATRIX44 = 16  ///< 16 values representing a 4x4 matrix.
  };

  /// VECSEMANTICS gives hints about what the data represent (for example,
  /// if a spatial vector quantity should transform as a point, direction
  /// vector, or surface normal).
  enum VECSEMANTICS {
    NOXFORM = 0,      ///< No semantic hints.
    NOSEMANTICS = 0,  ///< No semantic hints.
    COLOR,            ///< Color
    POINT,            ///< Point: a spatial location
    VECTOR,           ///< Vector: a spatial direction
    NORMAL,           ///< Normal: a surface normal
    TIMECODE,         ///< indicates an `int[2]` representing the standard
                      ///<   4-byte encoding of an SMPTE timecode.
    KEYCODE,          ///< indicates an `int[7]` representing the standard
                      ///<   28-byte encoding of an SMPTE keycode.
    RATIONAL,         ///< A VEC2 representing a rational number `val[0] / val[1]`
    BOX,              ///< A VEC2[2] or VEC3[2] that represents a 2D or 3D bounds (min/max)
  };

  unsigned char basetype;      ///< C data type at the heart of our type
  unsigned char aggregate;     ///< What kind of AGGREGATE is it?
  unsigned char vecsemantics;  ///< Hint: What does the aggregate represent?
  unsigned char reserved;      ///< Reserved for future expansion
  int arraylen;                ///< Array length, 0 = not array, -1 = unsized

  /// Construct from a BASETYPE and optional aggregateness, semantics,
  /// and arrayness.
   constexpr TypeDesc(BASETYPE btype = UNKNOWN,
                                     AGGREGATE agg = SCALAR,
                                     VECSEMANTICS semantics = NOSEMANTICS,
                                     int arraylen = 0) noexcept
      : basetype(static_cast<unsigned char>(btype)),
        aggregate(static_cast<unsigned char>(agg)),
        vecsemantics(static_cast<unsigned char>(semantics)),
        reserved(0),
        arraylen(arraylen)
  {
  }

  /// Construct an array of a non-aggregate BASETYPE.
   constexpr TypeDesc(BASETYPE btype, int arraylen) noexcept
      : TypeDesc(btype, SCALAR, NOSEMANTICS, arraylen)
  {
  }

  /// Construct an array from BASETYPE, AGGREGATE, and array length,
  /// with unspecified (or moot) semantic hints.
   constexpr TypeDesc(BASETYPE btype, AGGREGATE agg, int arraylen) noexcept
      : TypeDesc(btype, agg, NOSEMANTICS, arraylen)
  {
  }

  /// Construct from a string (e.g., "float[3]").  If no valid
  /// type could be assembled, set base to UNKNOWN.
  ///
  /// Examples:
  /// ```
  ///      TypeDesc("int") == TypeDesc(TypeDesc::INT)            // C++ int32_t
  ///      TypeDesc("float") == TypeDesc(TypeDesc::FLOAT)        // C++ float
  ///      TypeDesc("uint16") == TypeDesc(TypeDesc::UINT16)      // C++ uint16_t
  ///      TypeDesc("float[4]") == TypeDesc(TypeDesc::FLOAT, 4)  // array
  ///      TypeDesc("point") == TypeDesc(TypeDesc::FLOAT,
  ///                                    TypeDesc::VEC3, TypeDesc::POINT)
  /// ```
  ///
  TypeDesc(string_view typestring);

  /// Copy constructor.
  constexpr TypeDesc(const TypeDesc &t) noexcept = default;

  /// Return the name, for printing and whatnot.  For example,
  /// "float", "int[5]", "normal"
  const char *c_str() const;

  friend std::ostream &operator<<(std::ostream &o, const TypeDesc &t)
  {
    o << "{" << (int)t.basetype << ", " << (int)t.aggregate << ", " << (int)t.vecsemantics << ", "
      << (int)t.arraylen << "}";
    return o;
  }

  /// Return the number of elements: 1 if not an array, or the array
  /// length. Invalid to call this for arrays of undetermined size.
   constexpr size_t numelements() const noexcept
  {
    PL_CHECK(arraylen >= 0);
    return (arraylen >= 1 ? arraylen : 1);
  }

  /// Return the number of basetype values: the aggregate count multiplied
  /// by the array length (or 1 if not an array). Invalid to call this
  /// for arrays of undetermined size.
   constexpr size_t basevalues() const noexcept
  {
    return numelements() * aggregate;
  }

  /// Does this TypeDesc describe an array?
   constexpr bool is_array() const noexcept
  {
    return (arraylen != 0);
  }

  /// Does this TypeDesc describe an array, but whose length is not
  /// specified?
   constexpr bool is_unsized_array() const noexcept
  {
    return (arraylen < 0);
  }

  /// Does this TypeDesc describe an array, whose length is specified?
   constexpr bool is_sized_array() const noexcept
  {
    return (arraylen > 0);
  }

  /// Return the size, in bytes, of this type.
  ///
   size_t size() const noexcept
  {
    PL_CHECK(arraylen >= 0);
    size_t a = (size_t)(arraylen > 0 ? arraylen : 1);
    if (sizeof(size_t) > sizeof(int)) {
      // size_t has plenty of room for this multiplication
      return a * elementsize();
    }
    else {
      // need overflow protection
      unsigned long long s = (unsigned long long)a * elementsize();
      const size_t toobig = std::numeric_limits<size_t>::max();
      return s < toobig ? (size_t)s : toobig;
    }
  }

  /// Return the type of one element, i.e., strip out the array-ness.
  ///
   constexpr TypeDesc elementtype() const noexcept
  {
    TypeDesc t(*this);
    t.arraylen = 0;
    return t;
  }

  /// Return the size, in bytes, of one element of this type (that is,
  /// ignoring whether it's an array).
   size_t elementsize() const noexcept
  {
    return aggregate * basesize();
  }

  /// Return just the underlying C scalar type, i.e., strip out the
  /// array-ness and the aggregateness.
   constexpr TypeDesc scalartype() const
  {
    return TypeDesc(BASETYPE(basetype));
  }

  /// Return the base type size, i.e., stripped of both array-ness
  /// and aggregateness.
  size_t basesize() const noexcept;

  /// True if it's a floating-point type (versus a fundamentally
  /// integral type or something else like a string).
  bool is_floating_point() const noexcept;

  /// True if it's a signed type that allows for negative values.
  bool is_signed() const noexcept;

  /// Shortcut: is it UNKNOWN?
   constexpr bool is_unknown() const noexcept
  {
    return (basetype == UNKNOWN);
  }

  /// if (typedesc) is the same as asking whether it's not UNKNOWN.
   constexpr operator bool() const noexcept
  {
    return (basetype != UNKNOWN);
  }

  /// Set *this to the type described in the string.  Return the
  /// length of the part of the string that describes the type.  If
  /// no valid type could be assembled, return 0 and do not modify
  /// *this.
  size_t fromstring(string_view typestring);

  /// Compare two TypeDesc values for equality.
  ///
   constexpr bool operator==(const TypeDesc &t) const noexcept
  {
    return basetype == t.basetype && aggregate == t.aggregate && vecsemantics == t.vecsemantics &&
           arraylen == t.arraylen;
  }

  /// Compare two TypeDesc values for inequality.
  ///
   constexpr bool operator!=(const TypeDesc &t) const noexcept
  {
    return !(*this == t);
  }

  /// Compare a TypeDesc to a basetype (it's the same if it has the
  /// same base type and is not an aggregate or an array).
   friend constexpr bool operator==(const TypeDesc &t, BASETYPE b) noexcept
  {
    return (BASETYPE)t.basetype == b && (AGGREGATE)t.aggregate == SCALAR && !t.is_array();
  }
   friend constexpr bool operator==(BASETYPE b, const TypeDesc &t) noexcept
  {
    return (BASETYPE)t.basetype == b && (AGGREGATE)t.aggregate == SCALAR && !t.is_array();
  }

  /// Compare a TypeDesc to a basetype (it's the same if it has the
  /// same base type and is not an aggregate or an array).
   friend constexpr bool operator!=(const TypeDesc &t, BASETYPE b) noexcept
  {
    return (BASETYPE)t.basetype != b || (AGGREGATE)t.aggregate != SCALAR || t.is_array();
  }
   friend constexpr bool operator!=(BASETYPE b, const TypeDesc &t) noexcept
  {
    return (BASETYPE)t.basetype != b || (AGGREGATE)t.aggregate != SCALAR || t.is_array();
  }

  /// TypeDesc's are equivalent if they are equal, or if their only
  /// inequality is differing vector semantics.
   friend constexpr bool equivalent(const TypeDesc &a, const TypeDesc &b) noexcept
  {
    return a.basetype == b.basetype && a.aggregate == b.aggregate &&
           (a.arraylen == b.arraylen || (a.is_unsized_array() && b.is_sized_array()) ||
            (a.is_sized_array() && b.is_unsized_array()));
  }
  /// Member version of equivalent
   constexpr bool equivalent(const TypeDesc &b) const noexcept
  {
    return this->basetype == b.basetype && this->aggregate == b.aggregate &&
           (this->arraylen == b.arraylen || (this->is_unsized_array() && b.is_sized_array()) ||
            (this->is_sized_array() && b.is_unsized_array()));
  }

  /// Is this a 2-vector aggregate (of the given type, float by default)?
   constexpr bool is_vec2(BASETYPE b = FLOAT) const noexcept
  {
    return this->aggregate == VEC2 && this->basetype == b && !is_array();
  }

  /// Is this a 3-vector aggregate (of the given type, float by default)?
   constexpr bool is_vec3(BASETYPE b = FLOAT) const noexcept
  {
    return this->aggregate == VEC3 && this->basetype == b && !is_array();
  }

  /// Is this a 4-vector aggregate (of the given type, float by default)?
   constexpr bool is_vec4(BASETYPE b = FLOAT) const noexcept
  {
    return this->aggregate == VEC4 && this->basetype == b && !is_array();
  }

  /// Is this an array of aggregates that represents a 2D bounding box?
   constexpr bool is_box2(BASETYPE b = FLOAT) const noexcept
  {
    return this->aggregate == VEC2 && this->basetype == b && arraylen == 2 &&
           this->vecsemantics == BOX;
  }

  /// Is this an array of aggregates that represents a 3D bounding box?
   constexpr bool is_box3(BASETYPE b = FLOAT) const noexcept
  {
    return this->aggregate == VEC3 && this->basetype == b && arraylen == 2 &&
           this->vecsemantics == BOX;
  }

  /// Demote the type to a non-array
  ///
   void unarray(void) noexcept
  {
    arraylen = 0;
  }

  /// Test for lexicographic 'less', comes in handy for lots of STL
  /// containers and algorithms.
  bool operator<(const TypeDesc &x) const noexcept;

  /// Given base data types of a and b, return a basetype that is a best
  /// guess for one that can handle both without any loss of range or
  /// precision.
  static BASETYPE basetype_merge(TypeDesc a, TypeDesc b);
  static BASETYPE basetype_merge(TypeDesc a, TypeDesc b, TypeDesc c)
  {
    return basetype_merge(basetype_merge(a, b), c);
  }
};

// Static values for commonly used types. Because these are constexpr,
// they should incur no runtime construction cost and should optimize nicely
// in various ways.
inline constexpr TypeDesc TypeColor(TypeDesc::FLOAT, TypeDesc::VEC3, TypeDesc::COLOR);
inline constexpr TypeDesc TypeFloat(TypeDesc::FLOAT);
inline constexpr TypeDesc TypeFloat2(TypeDesc::FLOAT, TypeDesc::VEC2);
inline constexpr TypeDesc TypeFloat4(TypeDesc::FLOAT, TypeDesc::VEC4);
inline constexpr TypeDesc TypeInt(TypeDesc::INT);
inline constexpr TypeDesc TypeMatrix44(TypeDesc::FLOAT, TypeDesc::MATRIX44);
inline constexpr TypeDesc TypeMatrix = TypeMatrix44;
inline constexpr TypeDesc TypeNormal(TypeDesc::FLOAT, TypeDesc::VEC3, TypeDesc::NORMAL);
inline constexpr TypeDesc TypePoint(TypeDesc::FLOAT, TypeDesc::VEC3, TypeDesc::POINT);
inline constexpr TypeDesc TypeString(TypeDesc::STRING);
inline constexpr TypeDesc TypeUnknown(TypeDesc::UNKNOWN);
inline constexpr TypeDesc TypeVector(TypeDesc::FLOAT, TypeDesc::VEC3, TypeDesc::VECTOR);

/// A template mechanism for getting the a base type from C type
///
template<typename T> struct BaseTypeFromC {};
template<> struct BaseTypeFromC<unsigned char> {
  static const TypeDesc::BASETYPE value = TypeDesc::UINT8;
};
template<> struct BaseTypeFromC<char> {
  static const TypeDesc::BASETYPE value = TypeDesc::INT8;
};
template<> struct BaseTypeFromC<uint16_t> {
  static const TypeDesc::BASETYPE value = TypeDesc::UINT16;
};
template<> struct BaseTypeFromC<int16_t> {
  static const TypeDesc::BASETYPE value = TypeDesc::INT16;
};
template<> struct BaseTypeFromC<uint32_t> {
  static const TypeDesc::BASETYPE value = TypeDesc::UINT;
};
template<> struct BaseTypeFromC<int32_t> {
  static const TypeDesc::BASETYPE value = TypeDesc::INT;
};
template<> struct BaseTypeFromC<uint64_t> {
  static const TypeDesc::BASETYPE value = TypeDesc::UINT64;
};
template<> struct BaseTypeFromC<int64_t> {
  static const TypeDesc::BASETYPE value = TypeDesc::INT64;
};
#if defined(__GNUC__) && __WORDSIZE == 64 && !(defined(__APPLE__) && defined(__MACH__))
// Some platforms consider int64_t and long long to be different types, even
// though they are actually the same size.
static_assert(!std::is_same_v<unsigned long long, uint64_t>);
static_assert(!std::is_same_v<long long, int64_t>);
template<> struct BaseTypeFromC<unsigned long long> {
  static const TypeDesc::BASETYPE value = TypeDesc::UINT64;
};
template<> struct BaseTypeFromC<long long> {
  static const TypeDesc::BASETYPE value = TypeDesc::INT64;
};
#endif
#if defined(_HALF_H_) || defined(IMATH_HALF_H_)
template<> struct BaseTypeFromC<half> {
  static const TypeDesc::BASETYPE value = TypeDesc::HALF;
};
#endif
template<> struct BaseTypeFromC<float> {
  static const TypeDesc::BASETYPE value = TypeDesc::FLOAT;
};
template<> struct BaseTypeFromC<double> {
  static const TypeDesc::BASETYPE value = TypeDesc::DOUBLE;
};
template<> struct BaseTypeFromC<const char *> {
  static const TypeDesc::BASETYPE value = TypeDesc::STRING;
};
template<> struct BaseTypeFromC<char *> {
  static const TypeDesc::BASETYPE value = TypeDesc::STRING;
};
template<> struct BaseTypeFromC<std::string> {
  static const TypeDesc::BASETYPE value = TypeDesc::STRING;
};
template<> struct BaseTypeFromC<string_view> {
  static const TypeDesc::BASETYPE value = TypeDesc::STRING;
};

// template<> struct BaseTypeFromC<string> { static const TypeDesc::BASETYPE value =
// TypeDesc::STRING; }; template<size_t S> struct BaseTypeFromC<char[S]> { static const
// TypeDesc::BASETYPE value = TypeDesc::STRING; }; template<size_t S> struct BaseTypeFromC<const
// char[S]> { static const TypeDesc::BASETYPE value = TypeDesc::STRING; };
template<typename P> struct BaseTypeFromC<P *> {
  static const TypeDesc::BASETYPE value = TypeDesc::PTR;
};

}
