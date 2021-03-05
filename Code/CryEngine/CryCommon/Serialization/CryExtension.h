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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_CRYEXTENSION_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_CRYEXTENSION_H
#pragma once

#ifdef GetClassName
#undef GetClassName
#endif
#include <CryExtension/ICryFactory.h>

namespace Serialization
{
    // Allows to have AZStd::shared_ptr<TPointer> but serialize it by
    // interface-casting to TSerializable, i.e. implementing Serialization through
    // separate interface.
    template<class TPointer, class TSerializable = TPointer>
    struct CryExtensionPointer
    {
        AZStd::shared_ptr<TPointer>& ptr;

        CryExtensionPointer(AZStd::shared_ptr<TPointer>& _ptr)
            : ptr(_ptr) {}
        void Serialize(Serialization::IArchive& ar);
    };
}

// This function treats T as a type derived from CryUnknown type.
template<class T>
bool Serialize(Serialization::IArchive& ar, AZStd::shared_ptr<T>& ptr, const char* name, const char* label);

#include "CryExtensionImpl.h"
#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_CRYEXTENSION_H
