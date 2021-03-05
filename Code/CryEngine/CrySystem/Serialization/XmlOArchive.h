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

#ifndef __XML_O_ARCHIVE__H__
#define __XML_O_ARCHIVE__H__

#include <Serialization/IArchive.h>

namespace Serialization
{
    class CXmlOArchive
        : public IArchive
    {
    public:
        CXmlOArchive();
        CXmlOArchive(XmlNodeRef pRootNode);
        ~CXmlOArchive();

        void SetXmlNode(XmlNodeRef pNode);
        XmlNodeRef GetXmlNode() const;

        // IArchive
        bool operator()(bool& value, const char* name = "", const char* label = 0) override;
        bool operator()(IString& value, const char* name = "", const char* label = 0) override;
        bool operator()(IWString& value, const char* name = "", const char* label = 0) override;
        bool operator()(float& value, const char* name = "", const char* label = 0) override;
        bool operator()(double& value, const char* name = "", const char* label = 0) override;
        bool operator()(int16& value, const char* name = "", const char* label = 0) override;
        bool operator()(uint16& value, const char* name = "", const char* label = 0) override;
        bool operator()(int32& value, const char* name = "", const char* label = 0) override;
        bool operator()(uint32& value, const char* name = "", const char* label = 0) override;
        bool operator()(int64& value, const char* name = "", const char* label = 0) override;
        bool operator()(uint64& value, const char* name = "", const char* label = 0) override;

        bool operator()(int8& value, const char* name = "", const char* label = 0) override;
        bool operator()(uint8& value, const char* name = "", const char* label = 0) override;
        bool operator()(char& value, const char* name = "", const char* label = 0) override;

        bool operator()(const SStruct& ser, const char* name = "", const char* label = 0) override;
        bool operator()(IContainer& ser, const char* name = "", const char* label = 0) override;
        // ~IArchive

        using IArchive::operator();

    private:
        XmlNodeRef m_pRootNode;
    };
}

#endif
