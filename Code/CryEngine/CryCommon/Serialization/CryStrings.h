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

#pragma once

#include "CryFixedString.h"

#include "Serialization/Serializer.h"

namespace Serialization
{
    class IArchive;
}

// Note : if you are looking for the CryStringT serialization, it is handled in Serialization/STL.h

template< size_t N >
bool Serialize(Serialization::IArchive& ar, CryFixedStringT< N >& value, const char* name, const char* label);

template< size_t N >
bool Serialize(Serialization::IArchive& ar, CryStackStringT< char, N >& value, const char* name, const char* label);

template< size_t N >
bool Serialize(Serialization::IArchive& ar, CryStackStringT< wchar_t, N >& value, const char* name, const char* label);

#include "Serialization/CryStringsImpl.h"
