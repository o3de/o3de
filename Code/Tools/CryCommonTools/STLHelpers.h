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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_STLHELPERS_H
#define CRYINCLUDE_CRYCOMMONTOOLS_STLHELPERS_H
#pragma once


#include <functional>

namespace STLHelpers
{
    template <class Type>
    inline const char* constchar_cast(const Type& type)
    {
        return type;
    }

    template <>
    inline const char* constchar_cast(const std::string& type)
    {
        return type.c_str();
    }

    template <class Type>
    struct less_strcmp
    {
        bool operator()(const Type& left, const Type& right) const
        {
            return strcmp(constchar_cast(left), constchar_cast(right)) < 0;
        }
    };

    template <class Type>
    struct less_stricmp
    {
        bool operator()(const Type& left, const Type& right) const
        {
            return _stricmp(constchar_cast(left), constchar_cast(right)) < 0;
        }
    };
}

#endif // CRYINCLUDE_CRYCOMMONTOOLS_STLHELPERS_H
