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

#pragma once

#include <AzCore/XML/rapidxml.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

using namespace AZ::rapidxml;

namespace ScriptCanvasDeveloperEditor
{
    class XMLDoc;
    using XMLDocPtr = AZStd::shared_ptr<XMLDoc>;

    class XMLDoc
    {
    public:
        static XMLDocPtr Alloc(const AZStd::string& contextName);
        static XMLDocPtr LoadFromDisk(const AZStd::string& fileName);

        XMLDoc();
        virtual ~XMLDoc() {}

        bool WriteToDisk(const AZStd::string & filename);
        bool StartContext(const AZStd::string& contextName);
        void AddToContext(const AZStd::string& id, const AZStd::string& translation = "", const AZStd::string& comment = "", const AZStd::string& source = "");
        bool MethodFamilyExists(const AZStd::string& familyName) const;
        AZStd::string ToString() const;

    private:
        void CreateTSDoc(const AZStd::string& contextName);
        bool LoadTSDoc(const AZStd::string& contextName);
        bool IsValidTSDoc();
        const char *AllocString(const AZStd::string& str);

    private:
        xml_document<>          m_doc;
        xml_node<> *            m_tsNode;
        xml_node<> *            m_context;
        AZStd::vector<char>     m_readBuffer;
    };
}
