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

#include "CryXML_precompiled.h"
#include "XMLSerializer.h"
#include "XML/xml.h"
#include "IXMLSerializer.h"
#include "StringUtils.h"

XmlNodeRef XMLSerializer::CreateNode(const char* tag)
{
    return new CXmlNode(tag);
}

bool XMLSerializer::Write(XmlNodeRef root, const char* szFileName)
{
    return root->saveToFile(szFileName);
}

XmlNodeRef XMLSerializer::Read(const IXmlBufferSource& source, bool bRemoveNonessentialSpacesFromContent, int nErrorBufferSize, char* szErrorBuffer)
{
    XmlParser parser(bRemoveNonessentialSpacesFromContent);
    XmlNodeRef root = parser.parseSource(&source);
    if (nErrorBufferSize > 0 && szErrorBuffer)
    {
        const char* const err = parser.getErrorString();
        cry_strcpy(szErrorBuffer, nErrorBufferSize, err ? err : "");
    }
    return root;
}
