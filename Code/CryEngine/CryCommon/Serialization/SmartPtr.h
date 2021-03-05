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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_SMARTPTR_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_SMARTPTR_H
#pragma once

template <class T>
class _smart_ptr;

namespace Serialization
{
    class IArchive;
};

template<class T>
bool Serialize(Serialization::IArchive& ar, _smart_ptr<T>& ptr, const char* name, const char* label);

#include "SmartPtrImpl.h"

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_SMARTPTR_H
