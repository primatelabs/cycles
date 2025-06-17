// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

#include "pl/param_value.h"

#include "pl/error.h"

CCL_NAMESPACE_BEGIN

void ParamValue::init_noclear(ustring _name,
                              TypeDesc _type,
                              int _nvalues,
                              const void *_value,
                              Copy _copy,
                              FromUstring _from_ustring) noexcept
{
  init_noclear(_name, _type, _nvalues, INTERP_CONSTANT, _value, _copy, _from_ustring);
}

void ParamValue::init_noclear(ustring _name,
                              TypeDesc _type,
                              int _nvalues,
                              Interp _interp,
                              const void *_value,
                              Copy _copy,
                              FromUstring _from_ustring) noexcept
{
  m_name = _name;
  m_type = _type;
  m_nvalues = _nvalues;
  m_interp = _interp;
  size_t size = (size_t)(m_nvalues * m_type.size());
  bool small = (size <= sizeof(m_data));

  if (_copy || small) {
    if (small) {
      if (_value)
        memcpy(&m_data, _value, size);
      else
        memset(&m_data, 0, sizeof(m_data));
      m_copy = false;
      m_nonlocal = false;
    }
    else {
      void *ptr = malloc(size);
      if (_value)
        memcpy(ptr, _value, size);  // NOSONAR
      else
        memset(ptr, 0, size);
      m_data.ptr = ptr;
      m_copy = true;
      m_nonlocal = true;
    }
    // FIXME: Implement
    if (m_type.basetype == TypeDesc::STRING && !_from_ustring) {
      exit(-1);
    }
#if 0
    if (m_type.basetype == TypeDesc::STRING && !_from_ustring) {
      // Convert non-ustrings to ustrings
      for (ustring &u : as_span<ustring>())
        u = ustring(u.c_str());
    }
#endif
  }
  else {
    // Big enough to warrant a malloc, but the caller said don't
    // make a copy
    m_data.ptr = _value;
    m_copy = false;
    m_nonlocal = true;
  }
}

const ParamValue &ParamValue::operator=(const ParamValue &p) noexcept
{
  if (this != &p) {
    clear_value();
    init_noclear(
        p.name(), p.type(), p.nvalues(), p.interp(), p.data(), Copy(p.m_copy), FromUstring(true));
  }
  return *this;
}

const ParamValue &ParamValue::operator=(ParamValue &&p) noexcept
{
  if (this != &p) {
    clear_value();
    init_noclear(
        p.name(), p.type(), p.nvalues(), p.interp(), p.data(), Copy(false), FromUstring(true));
    m_copy = p.m_copy;
    m_nonlocal = p.m_nonlocal;
    p.m_data.ptr = nullptr;  // make sure the old one won't free
  }
  return *this;
}

void ParamValue::clear_value() noexcept
{
  PL_NOT_IMPLEMENTED();
}

CCL_NAMESPACE_END
