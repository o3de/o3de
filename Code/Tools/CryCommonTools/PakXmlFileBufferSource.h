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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_PAKXMLFILEBUFFERSOURCE_H
#define CRYINCLUDE_CRYCOMMONTOOLS_PAKXMLFILEBUFFERSOURCE_H
#pragma once


#include "../CryXML/IXMLSerializer.h"
#include "IPakSystem.h"

class PakXmlFileBufferSource
    : public IXmlBufferSource
{
public:
    PakXmlFileBufferSource(IPakSystem* pakSystem, const char* path)
        : pakSystem(pakSystem)
    {
        file = pakSystem->Open(path, "r");
    }
    ~PakXmlFileBufferSource()
    {
        if (file)
        {
            pakSystem->Close(file);
        }
    }

    virtual int Read(void* buffer, int size) const
    {
        return pakSystem->Read(file, buffer, size);
    };

    IPakSystem* pakSystem;
    PakSystemFile* file;
};

class PakXmlBufferSource
    : public IXmlBufferSource
{
public:
    PakXmlBufferSource(const char* buffer, size_t length)
        : position(buffer)
        , end(buffer + length)
    {
    }

    virtual int Read(void* output, int size) const
    {
        size_t bytesLeft = end - position;
        size_t bytesToCopy = size < bytesLeft ? size : bytesLeft;
        if (bytesToCopy > 0)
        {
            memcpy(output, position, bytesToCopy);
            position += bytesToCopy;
        }
        return bytesToCopy;
    };

    mutable const char* position;
    const char* end;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_PAKXMLFILEBUFFERSOURCE_H
