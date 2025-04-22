/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <platform.h>
#include "Cry_Color.h"
#include "XMLBinaryNode.h"

#include <AzCore/Serialization/Locale.h>
//////////////////////////////////////////////////////////////////////////
CBinaryXmlData::~CBinaryXmlData()
{
    if (bOwnsFileContentsMemory)
    {
        delete [] pFileContents;
    }
    pFileContents = nullptr;

    delete [] pBinaryNodes;
    pBinaryNodes = nullptr;
}

//////////////////////////////////////////////////////////////////////////
// CBinaryXmlNode implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CBinaryXmlNode::getParent() const
{
    const XMLBinary::Node* const pNode = _node();
    if (pNode->nParentIndex != (XMLBinary::NodeIndex)-1)
    {
        return &m_pData->pBinaryNodes[pNode->nParentIndex];
    }
    return XmlNodeRef();
}

XmlNodeRef CBinaryXmlNode::createNode([[maybe_unused]] const char* tag)
{
    assert(0);
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
bool CBinaryXmlNode::isTag(const char* tag) const
{
    return g_pXmlStrCmp(tag, getTag()) == 0;
}

const char* CBinaryXmlNode::getAttr(const char* key) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        return svalue;
    }
    return "";
}

bool CBinaryXmlNode::getAttr(const char* key, const char** value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        *value = svalue;
        return true;
    }
    else
    {
        *value = "";
        return false;
    }
}


bool CBinaryXmlNode::haveAttr(const char* key) const
{
    return (GetValue(key) != nullptr);
}

//////////////////////////////////////////////////////////////////////////
bool CBinaryXmlNode::getAttr(const char* key, int& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // for parsing to be culture invariant

        value = atoi(svalue);
        return true;
    }
    return false;
}

bool CBinaryXmlNode::getAttr(const char* key, unsigned int& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // for parsing to be culture invariant

        value = static_cast<unsigned int>(strtoul(svalue, nullptr, 10));
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CBinaryXmlNode::getAttr(const char* key, int64& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // for parsing to be culture invariant

        value = strtoll(svalue, nullptr, 10);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CBinaryXmlNode::getAttr(const char* key, uint64& value, bool useHexFormat) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // for parsing to be culture invariant

        value = strtoull(svalue, nullptr, useHexFormat ? 16 : 10);
        return true;
    }
    return false;
}

bool CBinaryXmlNode::getAttr(const char* key, bool& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // for parsing to be culture invariant

        value = atoi(svalue) != 0;
        return true;
    }
    return false;
}

bool CBinaryXmlNode::getAttr(const char* key, float& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // for parsing to be culture invariant

        value = (float)atof(svalue);
        return true;
    }
    return false;
}

bool CBinaryXmlNode::getAttr(const char* key, double& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // for parsing to be culture invariant

        value = atof(svalue);
        return true;
    }
    return false;
}

bool CBinaryXmlNode::getAttr(const char* key, Ang3& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // for parsing to be culture invariant

        float x, y, z;
        if (azsscanf(svalue, "%f,%f,%f", &x, &y, &z) == 3)
        {
            value(x, y, z);
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CBinaryXmlNode::getAttr(const char* key, Vec3& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // for parsing to be culture invariant

        float x, y, z;
        if (azsscanf(svalue, "%f,%f,%f", &x, &y, &z) == 3)
        {
            value = Vec3(x, y, z);
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CBinaryXmlNode::getAttr(const char* key, Vec4& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // for parsing to be culture invariant

        float x, y, z, w;
        if (azsscanf(svalue, "%f,%f,%f,%f", &x, &y, &z, &w) == 4)
        {
            value = Vec4(x, y, z, w);
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CBinaryXmlNode::getAttr(const char* key, Vec2& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // for parsing to be culture invariant

        float x, y;
        if (azsscanf(svalue, "%f,%f", &x, &y) == 2)
        {
            value = Vec2(x, y);
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CBinaryXmlNode::getAttr(const char* key, Quat& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // for parsing to be culture invariant

        float w, x, y, z;
        if (azsscanf(svalue, "%f,%f,%f,%f", &w, &x, &y, &z) == 4)
        {
            value = Quat(w, x, y, z);
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CBinaryXmlNode::getAttr(const char* key, ColorB& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // for parsing to be culture invariant

        unsigned int r, g, b, a = 255;
        int numFound = azsscanf(svalue, "%u,%u,%u,%u", &r, &g, &b, &a);
        if (numFound == 3 || numFound == 4)
        {
            // If we only found 3 values, a should be unchanged, and still be 255
            if (r < 256 && g < 256 && b < 256 && a < 256)
            {
                value = ColorB(static_cast<uint8>(r), static_cast<uint8>(g), static_cast<uint8>(b), static_cast<uint8>(a));
                return true;
            }
        }
    }
    return false;
}


XmlNodeRef CBinaryXmlNode::findChild(const char* tag) const
{
    const XMLBinary::Node* const pNode = _node();
    const uint32 nFirst = pNode->nFirstChildIndex;
    const uint32 nAfterLast = pNode->nFirstChildIndex + pNode->nChildCount;
    for (uint32 i = nFirst; i < nAfterLast; ++i)
    {
        const char* sChildTag = m_pData->pStringData + m_pData->pNodes[m_pData->pChildIndices[i]].nTagStringOffset;
        if (g_pXmlStrCmp(tag, sChildTag) == 0)
        {
            return m_pData->pBinaryNodes + m_pData->pChildIndices[i];
        }
    }
    return nullptr;
}

//! Get XML Node child nodes.
XmlNodeRef CBinaryXmlNode::getChild(int i) const
{
    const XMLBinary::Node* const pNode = _node();
    assert(i >= 0 && i < (int)pNode->nChildCount);
    return m_pData->pBinaryNodes + m_pData->pChildIndices[pNode->nFirstChildIndex + i];
}

//////////////////////////////////////////////////////////////////////////
bool CBinaryXmlNode::getAttributeByIndex(int index, const char** key, const char** value)
{
    const XMLBinary::Node* const pNode = _node();
    if (index >= 0 && index < pNode->nAttributeCount)
    {
        const XMLBinary::Attribute& attr = m_pData->pAttributes[pNode->nFirstAttributeIndex + index];
        *key = _string(attr.nKeyStringOffset);
        *value = _string(attr.nValueStringOffset);
        return true;
    }
    return false;
}
//////////////////////////////////////////////////////////////////////////
bool CBinaryXmlNode::getAttributeByIndex(int index, XmlString& key, XmlString& value)
{
    const XMLBinary::Node* const pNode = _node();
    if (index >= 0 && index < pNode->nAttributeCount)
    {
        const XMLBinary::Attribute& attr = m_pData->pAttributes[pNode->nFirstAttributeIndex + index];
        key = _string(attr.nKeyStringOffset);
        value = _string(attr.nValueStringOffset);
        return true;
    }
    return false;
}
//////////////////////////////////////////////////////////////////////////
