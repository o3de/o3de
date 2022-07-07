/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "SimpleSerialize.h"
#include <stack>
#include <IXml.h>
#include "xml.h"

class CSerializeXMLReaderImpl
    : public CSimpleSerializeImpl<true, ESerializationTarget::eST_SaveGame>
{
public:
    CSerializeXMLReaderImpl(const XmlNodeRef& nodeRef);

    template <class T_Value>
    ILINE bool GetAttr(const XmlNodeRef& node, const char* name, T_Value& value)
    {
        XmlStrCmpFunc pPrevCmpFunc = g_pXmlStrCmp;
        g_pXmlStrCmp = &strcmp; // Do case-sensitive compare
        bool bReturn = node->getAttr(name, value);
        g_pXmlStrCmp = pPrevCmpFunc;
        return bReturn;
    }
    ILINE bool GetAttr(const XmlNodeRef& node, const char* name, SSerializeString& value)
    {
        XmlStrCmpFunc pPrevCmpFunc = g_pXmlStrCmp;
        g_pXmlStrCmp = &strcmp; // Do case-sensitive compare
        bool bReturn = node->haveAttr(name);
        if (bReturn)
        {
            value = node->getAttr(name);
        }
        g_pXmlStrCmp = pPrevCmpFunc;
        return bReturn;
    }
    ILINE bool GetAttr([[maybe_unused]] XmlNodeRef& node, [[maybe_unused]] const char* name, [[maybe_unused]] const AZStd::string& value)
    {
        return false;
    }

    template <class T_Value>
    bool Value(const char* name, T_Value& value)
    {
        DefaultValue(value); // Set input value to default.
        if (m_nErrors)
        {
            return false;
        }

        if (!GetAttr(CurNode(), name, value))
        {
            //CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"Unable to read attribute %s (invalid type?)", name);
            //Failed();
            return false;
        }
        return true;
    }

    bool Value(const char* name, int8& value);
    bool Value(const char* name, AZStd::string& value);
    bool Value(const char* name, CTimeValue& value);

    void BeginGroup(const char* szName);
    bool BeginOptionalGroup(const char* szName, bool condition);
    void EndGroup();

private:
    XmlNodeRef CurNode() { return m_nodeStack.back().m_node; }
    XmlNodeRef NextOf(const char* name)
    {
        XmlStrCmpFunc pPrevCmpFunc = g_pXmlStrCmp;
        g_pXmlStrCmp = &strcmp; // Do case-sensitive compare
        assert(!m_nodeStack.empty());
        CParseState& ps = m_nodeStack.back();
        XmlNodeRef node = ps.GetNext(name);
        g_pXmlStrCmp = pPrevCmpFunc;
        return node;
    }

    class CParseState
    {
    public:
        CParseState() {}
        void Init(const XmlNodeRef& node)
        {
            m_node = node;
            m_nCurrent = 0;
        }

        XmlNodeRef GetNext(const char* name)
        {
            int i;
            int num = m_node->getChildCount();
            for (i = m_nCurrent; i < num; i++)
            {
                XmlNodeRef child = m_node->getChild(i);
                if (strcmp(child->getTag(), name) == 0)
                {
                    m_nCurrent = i + 1;
                    return child;
                }
            }
            int ncount = min(m_nCurrent, num);
            // Try searching from begining.
            for (i = 0; i < ncount; i++)
            {
                XmlNodeRef child = m_node->getChild(i);
                if (strcmp(child->getTag(), name) == 0)
                {
                    m_nCurrent = i + 1;
                    return child;
                }
            }
            return XmlNodeRef();
        }

    public:
        // TODO: make this much more efficient
        int m_nCurrent;
        XmlNodeRef m_node;
    };

    int m_nErrors;
    std::vector<CParseState> m_nodeStack;

    //////////////////////////////////////////////////////////////////////////
    // Set Defaults.
    //////////////////////////////////////////////////////////////////////////
    void DefaultValue(bool& v) const { v = false; }
    void DefaultValue(float& v) const { v = 0; }
    void DefaultValue(double& v) const { v = 0; }
    void DefaultValue(int8& v) const { v = 0; }
    void DefaultValue(uint8& v) const { v = 0; }
    void DefaultValue(int16& v) const { v = 0; }
    void DefaultValue(uint16& v) const { v = 0; }
    void DefaultValue(int32& v) const { v = 0; }
    void DefaultValue(uint32& v) const { v = 0; }
    void DefaultValue(int64& v) const { v = 0; }
    void DefaultValue(uint64& v) const { v = 0; }
    void DefaultValue(Vec2& v) const { v.x = 0; v.y = 0; }
    void DefaultValue(Vec3& v) const { v.x = 0; v.y = 0; v.z = 0; }
    void DefaultValue(Ang3& v) const { v.x = 0; v.y = 0; v.z = 0; }
    void DefaultValue(Quat& v) const { v.w = 1.0f; v.v.x = 0; v.v.y = 0; v.v.z = 0; }
    void DefaultValue(CTimeValue& v) const { v.SetValue(0); }
    void DefaultValue(AZStd::string& str) const { str = ""; }
    void DefaultValue([[maybe_unused]] const AZStd::string& str) const {}
    void DefaultValue([[maybe_unused]] SSerializeString& str) const {}
    void DefaultValue(XmlNodeRef& ref) const { ref = NULL; }
    //////////////////////////////////////////////////////////////////////////
};
