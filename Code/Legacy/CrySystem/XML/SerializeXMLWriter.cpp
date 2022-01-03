/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "CrySystem_precompiled.h"
#include "SerializeXMLWriter.h"

#include <AzCore/Time/ITime.h>

static const size_t MAX_NODE_STACK_DEPTH = 40;

#define TAG_SCRIPT_VALUE "v"
#define TAG_SCRIPT_TYPE "t"
#define TAG_SCRIPT_NAME "n"

CSerializeXMLWriterImpl::CSerializeXMLWriterImpl(const XmlNodeRef& nodeRef)
{
    const AZ::TimeMs elaspsedTimeMs = AZ::GetRealElapsedTimeMs();
    const double elaspedTimeSec = AZ::TimeMsToSecondsDouble(elaspsedTimeMs);
    m_curTime = CTimeValue(elaspedTimeSec);
    assert(!!nodeRef);
    m_nodeStack.push_back(nodeRef);

    m_luaSaveStack.reserve(10);
}

//////////////////////////////////////////////////////////////////////////
CSerializeXMLWriterImpl::~CSerializeXMLWriterImpl()
{
    if (m_nodeStack.size() != 1)
    {
        // Node stack is incorrect.
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "!BeginGroup/EndGroup mismatch in SaveGame");
    }
}

//////////////////////////////////////////////////////////////////////////
bool CSerializeXMLWriterImpl::Value(const char* name, CTimeValue value)
{
    if (value == CTimeValue(0.0f))
    {
        AddValue(name, "zero");
    }
    else
    {
        AddValue(name, (value - m_curTime).GetSeconds());
    }
    return true;
}

void CSerializeXMLWriterImpl::BeginGroup(const char* szName)
{
    if (strchr(szName, ' ') != 0)
    {
        assert(0 && "Spaces in group name not supported");
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "!Spaces in group name not supported: %s/%s", GetStackInfo().c_str(), szName);
    }
    XmlNodeRef node = CreateNodeNamed(szName);
    CurNode()->addChild(node);
    m_nodeStack.push_back(node);
    if (m_nodeStack.size() > MAX_NODE_STACK_DEPTH)
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "!Too Deep Node Stack:\r\n%s", GetStackInfo().c_str());
    }
}

bool CSerializeXMLWriterImpl::BeginOptionalGroup(const char* szName, bool condition)
{
    if (condition)
    {
        BeginGroup(szName);
        return true;
    }

    return condition;
}

XmlNodeRef CSerializeXMLWriterImpl::CreateNodeNamed(const char* name)
{
    XmlNodeRef newNode = CurNode()->createNode(name);
    return newNode;
}

void CSerializeXMLWriterImpl::EndGroup()
{
    if (m_nodeStack.size() == 1)
    {
        //
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "!Misplaced EndGroup() for BeginGroup(%s)", CurNode()->getTag());
    }
    assert(!m_nodeStack.empty());
    m_nodeStack.pop_back();
    assert(!m_nodeStack.empty());
}

//////////////////////////////////////////////////////////////////////////
AZStd::string CSerializeXMLWriterImpl::GetStackInfo() const
{
    AZStd::string str;
    for (int i = 0; i < (int)m_nodeStack.size(); i++)
    {
        const char* name = m_nodeStack[i]->getAttr(TAG_SCRIPT_NAME);
        if (name && name[0])
        {
            str += name;
        }
        else
        {
            str += m_nodeStack[i]->getTag();
        }
        if (i != m_nodeStack.size() - 1)
        {
            str += "/";
        }
    }
    return str;
}

//////////////////////////////////////////////////////////////////////////
AZStd::string CSerializeXMLWriterImpl::GetLuaStackInfo() const
{
    AZStd::string str;
    for (int i = 0; i < (int)m_luaSaveStack.size(); i++)
    {
        const char* name = m_luaSaveStack[i];
        str += name;
        if (i != m_luaSaveStack.size() - 1)
        {
            str += ".";
        }
    }
    return str;
}
