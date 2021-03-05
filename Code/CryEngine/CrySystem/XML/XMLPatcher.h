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

#ifndef CRYINCLUDE_CRYSYSTEM_XML_XMLPATCHER_H
#define CRYINCLUDE_CRYSYSTEM_XML_XMLPATCHER_H
#pragma once


#if defined(WIN32) && !defined(_RELEASE)
#define DATA_PATCH_DEBUG    1
#else
#define DATA_PATCH_DEBUG    0
#endif

class CXMLPatcher
{
protected:
#if DATA_PATCH_DEBUG
    ICVar                                               * m_pDumpFilesCVar;
#endif

    XmlNodeRef                                  m_patchXML;
    const char* m_pFileBeingPatched;
    bool                                                m_patchingEnabled;

    void                                                PatchFail(
        const char* pInReason);
    XmlNodeRef                                  ApplyPatchToNode(
        const XmlNodeRef& inNode,
        const XmlNodeRef& inPatch);

    XmlNodeRef                                  DuplicateForPatching(
        const XmlNodeRef& inOrig,
        bool                                                inShareChildren);

    bool                                                CompareTags(
        const XmlNodeRef& inA,
        const XmlNodeRef& inB);
    XmlNodeRef                                  GetMatchTag(
        const XmlNodeRef& inNode);
    XmlNodeRef                                  GetReplaceTag(
        const XmlNodeRef& inNode,
        bool* outShouldReplaceChildren);
    XmlNodeRef                                  GetInsertTag(
        const XmlNodeRef& inNode);
    XmlNodeRef                                  GetDeleteTag(
        const XmlNodeRef& inNode);
    XmlNodeRef                                  FindPatchForFile(
        const char* pInFileToPatch);

#if DATA_PATCH_DEBUG
    void                                                DumpXMLNodes(
        AZ::IO::HandleType                                  inFileHandle,
        int                                                 inIndent,
        const XmlNodeRef& inNode,
        CryFixedStringT<512>* ioTempString);
    void                                                DumpFiles(
        const char* pInXMLFileName,
        const XmlNodeRef& inBefore,
        const XmlNodeRef& inAfter);
    void                                                DumpXMLFile(
        const char* pInFilePath,
        const XmlNodeRef& inNode);
#endif

public:
    CXMLPatcher(XmlNodeRef& patchXML);
    ~CXMLPatcher();

    XmlNodeRef                                  ApplyXMLDataPatch(
        const XmlNodeRef& inNode,
        const char* pInXMLFileName);
};

#endif // CRYINCLUDE_CRYSYSTEM_XML_XMLPATCHER_H
