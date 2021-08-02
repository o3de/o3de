/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/XML/rapidxml.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

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
