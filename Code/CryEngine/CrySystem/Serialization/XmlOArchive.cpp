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

#include "CrySystem_precompiled.h"
#include "CryExtension/Impl/ClassWeaver.h"

#include <Serialization/STL.h>
#include <Serialization/IClassFactory.h>

#include "XmlOArchive.h"

#include <Serialization/STLImpl.h>
#include <Serialization/ClassFactory.h>

namespace XmlUtil
{
    XmlNodeRef CreateChildNode(XmlNodeRef pParent, const char* const name)
    {
        CRY_ASSERT(pParent);
        CRY_ASSERT(name);
        CRY_ASSERT(name[ 0 ]);

        XmlNodeRef pChild = pParent->createNode(name);
        CRY_ASSERT(pChild);

        pParent->addChild(pChild);
        return pChild;
    }

    template < typename T, typename TIn >
    bool WriteChildNodeAs(XmlNodeRef pParent, const char* const name, const TIn& value)
    {
        XmlNodeRef pChild = XmlUtil::CreateChildNode(pParent, name);
        CRY_ASSERT(pChild);

        pChild->setAttr("value", static_cast< T >(value));
        return true;
    }

    template < typename T >
    bool WriteChildNode(XmlNodeRef pParent, const char* const name, const T& value)
    {
        return WriteChildNodeAs< T >(pParent, name, value);
    }
}

Serialization::CXmlOArchive::CXmlOArchive()
    : IArchive(OUTPUT | NO_EMPTY_NAMES)
{
}


Serialization::CXmlOArchive::CXmlOArchive(XmlNodeRef pRootNode)
    : IArchive(OUTPUT | NO_EMPTY_NAMES)
    , m_pRootNode(pRootNode)
{
    CRY_ASSERT(m_pRootNode);
}


Serialization::CXmlOArchive::~CXmlOArchive()
{
}


void Serialization::CXmlOArchive::SetXmlNode(XmlNodeRef pNode)
{
    m_pRootNode = pNode;
}


XmlNodeRef Serialization::CXmlOArchive::GetXmlNode() const
{
    return m_pRootNode;
}


bool Serialization::CXmlOArchive::operator()(bool& value, const char* name, [[maybe_unused]] const char* label)
{
    const char* const stringValue = value ? "true" : "false";
    return XmlUtil::WriteChildNode(m_pRootNode, name, stringValue);
}


bool Serialization::CXmlOArchive::operator()(IString& value, const char* name, [[maybe_unused]] const char* label)
{
    const char* const stringValue = value.get();
    return XmlUtil::WriteChildNode(m_pRootNode, name, stringValue);
}


bool Serialization::CXmlOArchive::operator()([[maybe_unused]] IWString& value, [[maybe_unused]] const char* name, [[maybe_unused]] const char* label)
{
    CryFatalError("CXmlOArchive::operator() with IWString is not implemented");
    return false;
}


bool Serialization::CXmlOArchive::operator()(float& value, const char* name, [[maybe_unused]] const char* label)
{
    return XmlUtil::WriteChildNode(m_pRootNode, name, value);
}


bool Serialization::CXmlOArchive::operator()(double& value, const char* name, [[maybe_unused]] const char* label)
{
    return XmlUtil::WriteChildNode(m_pRootNode, name, value);
}


bool Serialization::CXmlOArchive::operator()(int16& value, const char* name, [[maybe_unused]] const char* label)
{
    return XmlUtil::WriteChildNodeAs< int >(m_pRootNode, name, value);
}


bool Serialization::CXmlOArchive::operator()(uint16& value, const char* name, [[maybe_unused]] const char* label)
{
    return XmlUtil::WriteChildNodeAs< uint >(m_pRootNode, name, value);
}


bool Serialization::CXmlOArchive::operator()(int32& value, const char* name, [[maybe_unused]] const char* label)
{
    return XmlUtil::WriteChildNode(m_pRootNode, name, value);
}


bool Serialization::CXmlOArchive::operator()(uint32& value, const char* name, [[maybe_unused]] const char* label)
{
    return XmlUtil::WriteChildNode(m_pRootNode, name, value);
}


bool Serialization::CXmlOArchive::operator()(int64& value, const char* name, [[maybe_unused]] const char* label)
{
    return XmlUtil::WriteChildNode(m_pRootNode, name, value);
}


bool Serialization::CXmlOArchive::operator()(uint64& value, const char* name, [[maybe_unused]] const char* label)
{
    return XmlUtil::WriteChildNode(m_pRootNode, name, value);
}


bool Serialization::CXmlOArchive::operator()(int8& value, const char* name, [[maybe_unused]] const char* label)
{
    return XmlUtil::WriteChildNodeAs< int >(m_pRootNode, name, value);
}


bool Serialization::CXmlOArchive::operator()(uint8& value, const char* name, [[maybe_unused]] const char* label)
{
    return XmlUtil::WriteChildNodeAs< uint >(m_pRootNode, name, value);
}


bool Serialization::CXmlOArchive::operator()(char& value, const char* name, [[maybe_unused]] const char* label)
{
    return XmlUtil::WriteChildNodeAs< int >(m_pRootNode, name, value);
}


bool Serialization::CXmlOArchive::operator()(const SStruct& ser, const char* name, [[maybe_unused]] const char* label)
{
    CRY_ASSERT(name);
    CRY_ASSERT(name[ 0 ]);

    XmlNodeRef pChild = XmlUtil::CreateChildNode(m_pRootNode, name);
    CXmlOArchive childArchive(pChild);
    childArchive.SetFilter(GetFilter());
    childArchive.SetInnerContext(GetInnerContext());

    const bool serializeSuccess = ser(childArchive);

    return serializeSuccess;
}


bool Serialization::CXmlOArchive::operator()(IContainer& ser, const char* name, [[maybe_unused]] const char* label)
{
    CRY_ASSERT(name);
    CRY_ASSERT(name[ 0 ]);

    bool serializeSuccess = true;

    XmlNodeRef pChild = XmlUtil::CreateChildNode(m_pRootNode, name);
    CXmlOArchive childArchive(pChild);
    childArchive.SetFilter(GetFilter());
    childArchive.SetInnerContext(GetInnerContext());

    const size_t containerSize = ser.size();
    if (0 < containerSize)
    {
        do
        {
            serializeSuccess &= ser(childArchive, "Element", "Element");
        } while (ser.next());
    }

    return serializeSuccess;
}
