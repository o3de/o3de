/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYSYSTEM_XML_XMLUTILS_H
#define CRYINCLUDE_CRYSYSTEM_XML_XMLUTILS_H
#pragma once

#include "ISystem.h"

#ifdef _RELEASE
#define CHECK_STATS_THREAD_OWNERSHIP()
#else
#define CHECK_STATS_THREAD_OWNERSHIP() if (m_statsThreadOwner != CryGetCurrentThreadId()) {__debugbreak(); }
#endif

class CXmlNodePool;

//////////////////////////////////////////////////////////////////////////
// Implements IXmlUtils interface.
//////////////////////////////////////////////////////////////////////////
class CXmlUtils
    : public IXmlUtils
{
public:
    CXmlUtils(ISystem* pSystem);
    virtual ~CXmlUtils();

    //////////////////////////////////////////////////////////////////////////
    // IXmlUtils
    //////////////////////////////////////////////////////////////////////////

    virtual IXmlParser* CreateXmlParser();

    // Load xml from file, returns 0 if load failed.
    virtual XmlNodeRef LoadXmlFromFile(const char* sFilename, bool bReuseStrings = false);
    // Load xml from memory buffer, returns 0 if load failed.
    virtual XmlNodeRef LoadXmlFromBuffer(const char* buffer, size_t size, bool bReuseStrings = false, bool bSuppressWarnings = false);

    // create an MD5 hash of an XML file
    virtual const char* HashXml(XmlNodeRef node);

    virtual IXmlSerializer* CreateXmlSerializer();

    virtual bool SaveBinaryXmlFile(const char* sFilename, XmlNodeRef root);
    virtual XmlNodeRef LoadBinaryXmlFile(const char* sFilename, bool bEnablePatching = true);

    virtual bool EnableBinaryXmlLoading(bool bEnable);

    // Create XML Table reader.
    virtual IXmlTableReader* CreateXmlTableReader();
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Init xml stats nodes pool
    virtual void InitStatsXmlNodePool(uint32 nPoolSize = 1024*1024);

    // Create new xml node for statistics
    virtual XmlNodeRef CreateStatsXmlNode(const char* sNodeName = "");

    // Set owner thread
    virtual void SetStatsOwnerThread(threadID threadId);

    // Free memory if stats xml node pool is empty
    virtual void FlushStatsXmlNodePool();

    // Set the XML Patcher. This is an XML object that modifies named XML files as they are loaded
    // EXCEPT for xml files loaded from a buffer, for which names aren't passed in
    virtual void SetXMLPatcher(XmlNodeRef* pPatcher);

private:
    ISystem* m_pSystem;
    CXmlNodePool*                   m_pStatsXmlNodePool;
#ifndef _RELEASE
    threadID m_statsThreadOwner;
#endif
};

#endif // CRYINCLUDE_CRYSYSTEM_XML_XMLUTILS_H
