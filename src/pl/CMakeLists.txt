# SPDX-FileCopyrightText: 2025 Primate Labs Inc.
#
# SPDX-License-Identifier: Apache-2.0

set(INC
  ..
)

set(INC_SYS
)

set(SRC
  error.cpp
  image_buf.cpp
  image_buf_algo.cpp
  image_input.cpp
  image_output.cpp
  image_spec.cpp
  param_value.cpp
  png_image_output.cpp
  strong_param.cpp
  type_desc.cpp
)

set(SRC_HEADERS
  error.h
  image_buf.h
  image_buf_algo.h
  image_input.h
  image_io.h
  image_output.h
  image_spec.h
  param_value.h
  png_image_output.h
  strong_param.h
  type_desc.h
)

include_directories(${INC})
include_directories(SYSTEM ${INC_SYS})

cycles_add_library(cycles_pl "${LIB}" ${SRC} ${SRC_HEADERS})
