/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "CrySystem_precompiled.h"
#include "SerializeXMLReader.h"
#include <ISystem.h>

#define TAG_SCRIPT_VALUE "v"
#define TAG_SCRIPT_TYPE "t"
#define TAG_SCRIPT_NAME "n"

//#define LOG_SERIALIZE_STACK(tag,szName) CryLogAlways( "<%s> %s/%s",tag,GetStackInfo().c_str(), szName);
#define LOG_SERIALIZE_STACK(tag, szName)

CSerializeXMLReaderImpl::CSerializeXMLReaderImpl(const XmlNodeRef& nodeRef)
    : m_nErrors(0)
{
    //m_curTime = gEnv->pTimer->GetFrameStartTime();
    assert(!!nodeRef);
    m_nodeStack.push_back(CParseState());
    m_nodeStack.back().Init(nodeRef);
}

bool CSerializeXMLReaderImpl::Value(const char* name, int8& value)
{
    DefaultValue(value); // Set input value to default.
    if (m_nErrors)
    {
        return false;
    }
    int temp;
    bool bResult = Value(name, temp);
    if (temp < -128 || temp > 127)
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Attribute %s is out of range (%d)", name, temp);
        Failed();
        bResult = false;
    }
    else
    {
        value = static_cast<int8>(temp);
    }
    return bResult;
}

bool CSerializeXMLReaderImpl::Value(const char* name, AZStd::string& value)
{
    DefaultValue(value); // Set input value to default.
    if (m_nErrors)
    {
        return false;
    }
    if (!CurNode()->haveAttr(name))
    {
        //CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"No such attribute %s (invalid type?)", name);
        //Failed();
        return false;
    }
    else
    {
        value = CurNode()->getAttr(name);
    }
    return true;
}

bool CSerializeXMLReaderImpl::Value(const char* name, CTimeValue& value)
{
    DefaultValue(value); // Set input value to default.
    if (m_nErrors)
    {
        return false;
    }
    XmlNodeRef nodeRef = CurNode();
    if (!nodeRef)
    {
        return false;
    }
    if (0 == strcmp("zero", nodeRef->getAttr(name)))
    {
        value = CTimeValue(0.0f);
    }
    else
    {
        float delta;
        if (!GetAttr(nodeRef, name, delta))
        {
            //CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"Failed to read time value %s", name);
            //Failed();
            value = gEnv->pTimer->GetFrameStartTime(); // in case we don't find the node, it was assumed to be the default value (0.0)
            // 0.0 means current time, whereas "zero" really means CTimeValue(0.0), see above
            return false;
        }
        else
        {
            value = CTimeValue(gEnv->pTimer->GetFrameStartTime() + delta);
        }
    }
    return true;
}

void CSerializeXMLReaderImpl::BeginGroup(const char* szName)
{
    if (m_nErrors)
    {
        //CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"BeginGroup %s called on non-existant group", szName);
        m_nErrors++;
    }
    else if (XmlNodeRef node = NextOf(szName))
    {
        m_nodeStack.push_back(CParseState());
        m_nodeStack.back().Init(node);
        LOG_SERIALIZE_STACK("BeginGroup:ok", szName);
    }
    else
    {
        LOG_SERIALIZE_STACK("BeginGroup:fail", szName);
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "!BeginGroup( %s ) not found", szName);
        m_nErrors++;
    }
}

bool CSerializeXMLReaderImpl::BeginOptionalGroup(const char* szName, [[maybe_unused]] bool condition)
{
    if (m_nErrors)
    {
        //CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"BeginOptionalGroup %s called on non-existant group", szName);
        m_nErrors++;
    }
    else if (XmlNodeRef node = NextOf(szName))
    {
        m_nodeStack.push_back(CParseState());
        m_nodeStack.back().Init(node);
        LOG_SERIALIZE_STACK("BeginOptionalGroup:ok", szName);
        return true;
    }
    LOG_SERIALIZE_STACK("BeginOptionalGroup:fail", szName);
    return false;
}

void CSerializeXMLReaderImpl::EndGroup()
{
    if (m_nErrors)
    {
        m_nErrors--;
    }
    else
    {
        LOG_SERIALIZE_STACK("EndGroup", "");
        m_nodeStack.pop_back();
    }
    assert(!m_nodeStack.empty());
}
