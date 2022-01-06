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

    virtual IXmlSerializer* CreateXmlSerializer();

    // Create XML Table reader.
    virtual IXmlTableReader* CreateXmlTableReader();
    //////////////////////////////////////////////////////////////////////////
private:
    ISystem* m_pSystem;
};

#endif // CRYINCLUDE_CRYSYSTEM_XML_XMLUTILS_H
