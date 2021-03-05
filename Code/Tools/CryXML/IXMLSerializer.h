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

#ifndef CRYINCLUDE_CRYXML_IXMLSERIALIZER_H
#define CRYINCLUDE_CRYXML_IXMLSERIALIZER_H
#pragma once


#include "IXml.h"
#include <cstdio>
class IXMLDataSink;
class IXMLDataSource;

struct IXmlBufferSource
{
    virtual int Read(void* buffer, int size) const = 0;
};

class FileXmlBufferSource
    : public IXmlBufferSource
{
public:
    FileXmlBufferSource(const char* path)
    {
        file = nullptr;
        azfopen(&file, path, "r");
    }
    ~FileXmlBufferSource()
    {
        if (file)
        {
            std::fclose(file);
        }
    }

    virtual int Read(void* buffer, int size) const
    {
        if (!file)
        {
            return 0;
        }
        return check_cast<int>(std::fread(buffer, 1, size, file));
    }

private:
    mutable std::FILE* file;
};

class IXMLSerializer
{
public:
    virtual XmlNodeRef CreateNode(const char* tag) = 0;
    virtual bool Write(XmlNodeRef root, const char* szFileName) = 0;

    virtual XmlNodeRef Read(const IXmlBufferSource& source, bool bRemoveNonessentialSpacesFromContent, int nErrorBufferSize, char* szErrorBuffer) = 0;
};

#endif // CRYINCLUDE_CRYXML_IXMLSERIALIZER_H
