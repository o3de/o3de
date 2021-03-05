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

#ifndef CRYINCLUDE_CRYSYSTEM_XML_XMLBINARYWRITER_H
#define CRYINCLUDE_CRYSYSTEM_XML_XMLBINARYWRITER_H
#pragma once


#include "IXml.h"
#include "XMLBinaryHeaders.h"
#include <vector>
#include <map>

class IXMLDataSink;

namespace XMLBinary
{
    class CXMLBinaryWriter
    {
    public:
        CXMLBinaryWriter();
        bool WriteNode(IDataWriter* pFile, XmlNodeRef node, bool bNeedSwapEndian, XMLBinary::IFilter* pFilter, string& error);

    private:
        bool CompileTables(XmlNodeRef node, XMLBinary::IFilter* pFilter, string& error);

        bool CompileTablesForNode(XmlNodeRef node, int nParentIndex, XMLBinary::IFilter* pFilter, string& error);
        bool CompileChildTable(XmlNodeRef node, XMLBinary::IFilter* pFilter, string& error);
        int AddString(const XmlString& sString);

    private:
        // tables.
        typedef std::map<IXmlNode*, int> NodesMap;
        typedef std::map<string, uint> StringMap;

        std::vector<Node> m_nodes;
        NodesMap m_nodesMap;
        std::vector<Attribute> m_attributes;
        std::vector<NodeIndex> m_childs;
        std::vector<string> m_strings;
        StringMap m_stringMap;

        uint m_nStringDataSize;
    };
}

#endif // CRYINCLUDE_CRYSYSTEM_XML_XMLBINARYWRITER_H
