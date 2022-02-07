/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
        bool WriteNode(IDataWriter* pFile, XmlNodeRef node, XMLBinary::IFilter* pFilter, AZStd::string & error);

    private:
        bool CompileTables(XmlNodeRef node, XMLBinary::IFilter* pFilter, AZStd::string& error);

        bool CompileTablesForNode(XmlNodeRef node, int nParentIndex, XMLBinary::IFilter* pFilter, AZStd::string& error);
        bool CompileChildTable(XmlNodeRef node, XMLBinary::IFilter* pFilter, AZStd::string& error);
        int AddString(const XmlString& sString);

    private:
        // tables.
        typedef std::map<IXmlNode*, int> NodesMap;
        typedef std::map<AZStd::string, uint> StringMap;

        std::vector<Node> m_nodes;
        NodesMap m_nodesMap;
        std::vector<Attribute> m_attributes;
        std::vector<NodeIndex> m_childs;
        std::vector<AZStd::string> m_strings;
        StringMap m_stringMap;

        uint m_nStringDataSize;
    };
}

#endif // CRYINCLUDE_CRYSYSTEM_XML_XMLBINARYWRITER_H
