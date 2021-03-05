/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_ASSERT_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_ASSERT_H
#pragma once

#ifdef SERIALIZATION_STANDALONE
#include <assert.h>
#else
#include <platform.h>
#endif

#ifdef YASLI_ASSERT
# undef YASLI_ASSERT
#endif

#ifdef YASLI_VERIFY
# undef YASLI_VERIFY
#endif

#ifdef YASLI_ESCAPE
# undef YASLI_ESCAPE
#endif

#ifdef SERIALIZATION_STANDALONE
#define YASLI_ASSERT(x) assert(x)
#define YASLI_ASSERT_STR(x, str) assert(x && str)
#define YASLI_ESCAPE(x, action) if (!(x)) { YASLI_ASSERT(0 && #x); action; };
#else
#define YASLI_ASSERT(x) CRY_ASSERT(x)
#define YASLI_ASSERT_STR(x, str) CRY_ASSERT_MESSAGE(x, str)
#define YASLI_ESCAPE(x, action) if (!(x)) { YASLI_ASSERT(0 && #x); action; };
#endif // SERIALIZATION_STANDALONE

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_ASSERT_H
