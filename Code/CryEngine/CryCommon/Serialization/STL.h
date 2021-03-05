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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_STL_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_STL_H
#pragma once


#include <vector>
#include <list>
#include <map>

#include "Serialization/Serializer.h"

namespace Serialization {
    class IArchive;
}

namespace std
{
    template<class K, class V, class Alloc>
    bool Serialize(Serialization::IArchive& ar, std::pair<K, V>& pair, const char* name, const char* label);

    template<class T, class Alloc>
    bool Serialize(Serialization::IArchive& ar, std::vector<T, Alloc>& container, const char* name, const char* label);

    template<class T, class Alloc>
    bool Serialize(Serialization::IArchive& ar, std::list<T, Alloc>& container, const char* name, const char* label);

    template<class K, class V, class C, class Alloc>
    bool Serialize(Serialization::IArchive& ar, std::map<K, V, C, Alloc>& container, const char* name, const char* label);
}

namespace Serialization
{
    bool Serialize(Serialization::IArchive& ar, Serialization::string& value, const char* name, const char* label);
    bool Serialize(Serialization::IArchive& ar, Serialization::wstring& value, const char* name, const char* label);
}

#include "STLImpl.h"

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_STL_H
