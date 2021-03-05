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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_ITEXTOUTPUTARCHIVE_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_ITEXTOUTPUTARCHIVE_H
#pragma once
#include "Serialization/IArchive.h"
#include "CryExtension/ICryUnknown.h"
#include "CryExtension/CryCreateClassInstance.h"

namespace Serialization {
    class ITextOutputArchive
        : public ICryUnknown
        , public IArchive
    {
        CRYINTERFACE_DECLARE(ITextOutputArchive, 0xa273d6157a8b4f0d, 0x80ad6c8031bbfbf3)
    public:
        virtual bool SaveFileUsingCRT(const char* filename) = 0;

        // use precise but less readable way to write float/double types
        virtual void SetExponentFloatRepresentation(bool) = 0;

        // buffer is a null-terminated string
        virtual const char* GetBuffer() const = 0;
        virtual size_t GetBufferLength() const = 0;

        // by default nested structres are put in one line, unless the line length is
        // longer than 'textWidth'
        virtual void SetTextWidth(int textWidth) = 0;

        using IArchive::operator();
    protected:
        ITextOutputArchive(int caps)
            : IArchive(caps) {}
    };

    inline AZStd::shared_ptr<ITextOutputArchive> CreateTextOutputArchive()
    {
        AZStd::shared_ptr<ITextOutputArchive> pArchive;
        CryCreateClassInstance(MAKE_CRYGUID(0xd1f14adbc4e74e49, 0x9cea55d80a55cbdb), pArchive);
        return pArchive;
    }
}

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_ITEXTOUTPUTARCHIVE_H
