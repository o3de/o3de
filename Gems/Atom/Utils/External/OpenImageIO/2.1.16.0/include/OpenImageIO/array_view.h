// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md


#pragma once

#include <OpenImageIO/span.h>

OIIO_NAMESPACE_BEGIN

// Backwards-compatibility: define array_view<> (our old name) as a
// synonym for span<> (which is the nomenclature favored by C++20).

template<typename T> using array_view = span<T>;

template<typename T> using array_view_strided = span_strided<T>;


OIIO_NAMESPACE_END
