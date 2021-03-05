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

#include "CrySystem_precompiled.h"
#include <Serialization/IArchiveHost.h>
#include "JSONIArchive.h"
#include "JSONOArchive.h"
#include "BinArchive.h"
#include "XmlIArchive.h"
#include "XmlOArchive.h"
#include <Serialization/ClassFactoryImpl.h>

namespace Serialization
{
    bool LoadFile(std::vector<char>& content, const char* filename)
    {
        AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(filename, "rb");
        if (!fileHandle)
        {
            return false;
        }

        gEnv->pCryPak->FSeek(fileHandle, 0, SEEK_END);
        size_t size = gEnv->pCryPak->FTell(fileHandle);
        gEnv->pCryPak->FSeek(fileHandle, 0, SEEK_SET);

        content.resize(size);
        bool result = true;
        if (size != 0)
        {
            result = gEnv->pCryPak->FRead(&content[0], size, fileHandle) == size;
        }
        gEnv->pCryPak->FClose(fileHandle);
        return result;
    }

    class CArchiveHost
        : public IArchiveHost
    {
    public:
        bool LoadJsonFile(const SStruct& obj, const char* filename) override
        {
            std::vector<char> content;
            if (!LoadFile(content, filename))
            {
                return false;
            }
            JSONIArchive ia;
            if (!ia.open(content.data(), content.size()))
            {
                return false;
            }
            return ia(obj);
        }

        bool SaveJsonFile(const char* gameFilename, const SStruct& obj) override
        {
            char buffer[AZ::IO::IArchive::MaxPath];
            const char* filename = gEnv->pCryPak->AdjustFileName(gameFilename, buffer, AZ_ARRAY_SIZE(buffer), AZ::IO::IArchive::FLAGS_FOR_WRITING);
            JSONOArchive oa;
            if (!oa(obj))
            {
                return false;
            }
            return oa.save(filename);
        }

        bool LoadJsonBuffer(const SStruct& obj, const char* buffer, size_t bufferLength) override
        {
            if (bufferLength == 0)
            {
                return false;
            }
            JSONIArchive ia;
            if (!ia.open(buffer, bufferLength))
            {
                return false;
            }
            return ia(obj);
        }

        bool SaveJsonBuffer(DynArray<char>& buffer, const SStruct& obj) override
        {
            JSONOArchive oa;
            if (!oa(obj))
            {
                return false;
            }
            buffer.assign(oa.buffer(), oa.buffer() + oa.length());
            return true;
        }


        bool LoadBinaryFile(const SStruct& obj, const char* filename) override
        {
            std::vector<char> content;
            if (!LoadFile(content, filename))
            {
                return false;
            }
            BinIArchive ia;
            if (!ia.open(content.data(), content.size()))
            {
                return false;
            }
            return ia(obj);
        }

        bool SaveBinaryFile(const char* gameFilename, const SStruct& obj) override
        {
            char buffer[AZ::IO::IArchive::MaxPath];
            const char* filename = gEnv->pCryPak->AdjustFileName(gameFilename, buffer, AZ_ARRAY_SIZE(buffer), AZ::IO::IArchive::FLAGS_FOR_WRITING);
            BinOArchive oa;
            obj(oa);
            return oa.save(filename);
        }

        bool LoadBinaryBuffer(const SStruct& obj, const char* buffer, size_t bufferLength) override
        {
            if (bufferLength == 0)
            {
                return false;
            }
            BinIArchive ia;
            if (!ia.open(buffer, bufferLength))
            {
                return false;
            }
            return ia(obj);
        }

        bool SaveBinaryBuffer(DynArray<char>& buffer, const SStruct& obj) override
        {
            BinOArchive oa;
            obj(oa);
            buffer.assign(oa.buffer(), oa.buffer() + oa.length());
            return true;
        }

        bool CloneBinary(const SStruct& dest, const SStruct& src) override
        {
            BinOArchive oa;
            src(oa);
            BinIArchive ia;
            if (!ia.open(oa.buffer(), oa.length()))
            {
                return false;
            }
            dest(ia);
            return true;
        }

        bool CompareBinary(const SStruct& lhs, const SStruct& rhs) override
        {
            BinOArchive oa1;
            lhs(oa1);
            BinOArchive oa2;
            rhs(oa2);
            if (oa1.length() != oa2.length())
            {
                return false;
            }
            return memcmp(oa1.buffer(), oa2.buffer(), oa1.length()) == 0;
        }

        bool SaveXmlFile(const char* filename, const SStruct& obj, const char* rootNodeName) override
        {
            XmlNodeRef node = SaveXmlNode(obj, rootNodeName);
            if (!node)
            {
                return false;
            }
            return node->saveToFile(filename);
        }

        bool LoadXmlFile(const SStruct& obj, const char* filename) override
        {
            XmlNodeRef node = gEnv->pSystem->LoadXmlFromFile(filename);
            if (!node)
            {
                return false;
            }
            return LoadXmlNode(obj, node);
        }

        XmlNodeRef SaveXmlNode(const SStruct& obj, const char* nodeName) override
        {
            CXmlOArchive oa;
            XmlNodeRef node = gEnv->pSystem->CreateXmlNode(nodeName);
            if (!node)
            {
                return XmlNodeRef();
            }
            oa.SetXmlNode(node);
            if (!obj(oa))
            {
                return XmlNodeRef();
            }
            return oa.GetXmlNode();
        }

        bool SaveXmlNode(XmlNodeRef& node, const SStruct& obj) override
        {
            if (!node)
            {
                return false;
            }
            CXmlOArchive oa;
            oa.SetXmlNode(node);
            return obj(oa);
        }

        bool LoadXmlNode(const SStruct& obj, const XmlNodeRef& node) override
        {
            CXmlIArchive ia;
            ia.SetXmlNode(node);
            if (!obj(ia))
            {
                return false;
            }
            return true;
        }
    };

    IArchiveHost* CreateArchiveHost()
    {
        return new CArchiveHost;
    }
}
