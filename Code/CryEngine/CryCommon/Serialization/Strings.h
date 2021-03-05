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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_STRINGS_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_STRINGS_H
#pragma once

#ifdef SERIALIZATION_STANDALONE
#include <string>
namespace Serialization {
    using std::string;
    using std::wstring;
}
#else
#include <platform.h>

namespace Serialization {
    typedef CryStringT<char> string;
    typedef CryStringT<wchar_t> wstring;
}
#endif // SERIALIZATION_STANDALONE
#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_STRINGS_H
