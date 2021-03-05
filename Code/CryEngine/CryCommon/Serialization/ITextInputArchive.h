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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_ITEXTINPUTARCHIVE_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_ITEXTINPUTARCHIVE_H
#pragma once
#include "Serialization/IArchive.h"
#include "CryExtension/ICryUnknown.h"
#include "CryExtension/CryCreateClassInstance.h"

namespace Serialization {
    class ITextInputArchive
        : public ICryUnknown
        , public IArchive
    {
    public:
        CRYINTERFACE_DECLARE(ITextInputArchive, 0x1845738b1dcc4168, 0xb440dba776b460c9)

        using IArchive::operator();

        virtual bool LoadFileUsingCRT(const char* filename) = 0;
        virtual bool AttachMemory(const char* buffer, size_t size) = 0;

    protected:
        ITextInputArchive(int caps)
            : IArchive(caps) {}
    };

    inline AZStd::shared_ptr<ITextInputArchive> CreateTextInputArchive()
    {
        AZStd::shared_ptr<ITextInputArchive> pArchive;
        CryCreateClassInstance(MAKE_CRYGUID(0x7a83a1c890054608, 0x9f8447a4b0ad6c3b), pArchive);
        return pArchive;
    }
}
#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_ITEXTINPUTARCHIVE_H
