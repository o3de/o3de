/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYSYSTEM_XML_SERIALIZEXMLWRITER_H
#define CRYINCLUDE_CRYSYSTEM_XML_SERIALIZEXMLWRITER_H
#pragma once


#include <ISystem.h>
#include <IXml.h>
#include "SimpleSerialize.h"

class CSerializeXMLWriterImpl
    : public CSimpleSerializeImpl<false, ESerializationTarget::eST_SaveGame>
{
public:
    CSerializeXMLWriterImpl(const XmlNodeRef& nodeRef);
    ~CSerializeXMLWriterImpl();

    template <class T_Value>
    bool Value(const char* name, T_Value& value)
    {
        AddValue(name, value);
        return true;
    }

    bool Value(const char* name, CTimeValue value);

    void BeginGroup(const char* szName);
    bool BeginOptionalGroup(const char* szName, bool condition);
    void EndGroup();

private:
    //////////////////////////////////////////////////////////////////////////
    // Vars.
    //////////////////////////////////////////////////////////////////////////
    CTimeValue m_curTime;

    std::vector<XmlNodeRef> m_nodeStack;
    std::vector<const char*> m_luaSaveStack;
    //////////////////////////////////////////////////////////////////////////


    ILINE const XmlNodeRef& CurNode()
    {
        assert(!m_nodeStack.empty());
        if (m_nodeStack.empty())
        {
            static XmlNodeRef temp = GetISystem()->CreateXmlNode("Error");
            return temp;
        }
        return m_nodeStack.back();
    }

    XmlNodeRef CreateNodeNamed(const char* name);

    template <class T>
    void AddValue(const char* name, const T& value)
    {
        if (strchr(name, ' ') != 0)
        {
            assert(0 && "Spaces in Value name not supported");
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "!Spaces in Value name not supported: %s in Group %s", name, GetStackInfo().c_str());
            return;
        }
        if (GetISystem()->IsDevMode() && CurNode())
        {
            // Check if this attribute already added.
            if (CurNode()->haveAttr(name))
            {
                assert(0);
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "!Duplicate tag Value( \"%s\" ) in Group %s", name, GetStackInfo().c_str());
            }
        }

        if (!IsDefaultValue(value))
        {
            CurNode()->setAttr(name, value);
        }
    }
    void AddValue(const char* name, const SSerializeString& value)
    {
        AddValue(name, value.c_str());
    }
    template <class T>
    void AddTypedValue(const char* name, const T& value, const char* type)
    {
        if (!IsDefaultValue(value))
        {
            XmlNodeRef newNode = CreateNodeNamed(name);
            newNode->setAttr("v", value);
            newNode->setAttr("t", type);
        }
    }

    // Used for printing currebnt stack info for warnings.
    AZStd::string GetStackInfo() const;
    AZStd::string GetLuaStackInfo() const;

    //////////////////////////////////////////////////////////////////////////
    // Check For Defaults.
    //////////////////////////////////////////////////////////////////////////
    bool IsDefaultValue(bool v) const { return v == false; };
    bool IsDefaultValue(float v) const { return v == 0; };
    bool IsDefaultValue(double v) const { return v == 0; };
    bool IsDefaultValue(int8 v) const { return v == 0; };
    bool IsDefaultValue(uint8 v) const { return v == 0; };
    bool IsDefaultValue(int16 v) const { return v == 0; };
    bool IsDefaultValue(uint16 v) const { return v == 0; };
    bool IsDefaultValue(int32 v) const { return v == 0; };
    bool IsDefaultValue(uint32 v) const { return v == 0; };
    bool IsDefaultValue(int64 v) const { return v == 0; };
    bool IsDefaultValue(uint64 v) const { return v == 0; };
    bool IsDefaultValue(const Vec2& v) const { return v.x == 0 && v.y == 0; };
    bool IsDefaultValue(const Vec3& v) const { return v.x == 0 && v.y == 0 && v.z == 0; };
    bool IsDefaultValue(const Ang3& v) const { return v.x == 0 && v.y == 0 && v.z == 0; };
    bool IsDefaultValue(const Quat& v) const { return v.w == 1.0f && v.v.x == 0 && v.v.y == 0 && v.v.z == 0; };
    bool IsDefaultValue(const CTimeValue& v) const { return v.GetValue() == 0; };
    bool IsDefaultValue(const char* str) const { return !str || !*str; };
    bool IsDefaultValue(const AZStd::string& str) const { return str.empty(); };
    bool IsDefaultValue(const SSerializeString& str) const { return str.empty(); };
    //////////////////////////////////////////////////////////////////////////

    /*

    template <class T>
    bool IsDefaultValue( const T& v ) const { return false; };
    */
};

#endif // CRYINCLUDE_CRYSYSTEM_XML_SERIALIZEXMLWRITER_H
