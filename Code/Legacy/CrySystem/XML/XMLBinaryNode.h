/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYSYSTEM_XML_XMLBINARYNODE_H
#define CRYINCLUDE_CRYSYSTEM_XML_XMLBINARYNODE_H
#pragma once


#include <algorithm>
#include "IXml.h"
#include "XMLBinaryHeaders.h"

// Compare function for string comparison, can be strcmp or _stricmp
typedef int (__cdecl * XmlStrCmpFunc)(const char* str1, const char* str2);
extern XmlStrCmpFunc g_pXmlStrCmp;

class CBinaryXmlNode;

//////////////////////////////////////////////////////////////////////////
class CBinaryXmlData
{
public:
    const XMLBinary::Node*       pNodes;
    const XMLBinary::Attribute*  pAttributes;
    const XMLBinary::NodeIndex*  pChildIndices;
    const char*                  pStringData;

    const char*                  pFileContents;
    size_t                       nFileSize;
    bool                         bOwnsFileContentsMemory;

    CBinaryXmlNode*              pBinaryNodes;

    int                          nRefCount;

    CBinaryXmlData();
    ~CBinaryXmlData();
};

// forward declaration
namespace XMLBinary
{
    class XMLBinaryReader;
};

//////////////////////////////////////////////////////////////////////////
// CBinaryXmlNode class only used for fast read only binary XML import
//////////////////////////////////////////////////////////////////////////
class CBinaryXmlNode
    : public IXmlNode
{
public:

    //////////////////////////////////////////////////////////////////////////
    // Custom new/delete with pool allocator.
    //////////////////////////////////////////////////////////////////////////
    //void* operator new( size_t nSize );
    //void operator delete( void *ptr );

    void DeleteThis() override { }

    //! Create new XML node.
    XmlNodeRef createNode(const char* tag) override;

    // Summary:
    //   Reference counting.
    void AddRef() override { ++m_pData->nRefCount; };
    // Notes:
    //   When ref count reach zero XML node dies.
    void Release() override
    {
        if (--m_pData->nRefCount <= 0)
        {
            delete m_pData;
        }
    };

    //! Get XML node tag.
    const char* getTag() const override { return _string(_node()->nTagStringOffset); };
    void    setTag([[maybe_unused]] const char* tag) override { assert(0); };

    //! Return true if given tag is equal to node tag.
    bool isTag(const char* tag) const override;

    //! Get XML Node attributes.
    int getNumAttributes() const override { return (int)_node()->nAttributeCount; };
    //! Return attribute key and value by attribute index.
    bool getAttributeByIndex(int index, const char** key, const char** value) override;
    //! Return attribute key and value by attribute index, string version.
    virtual bool getAttributeByIndex(int index, XmlString& key, XmlString& value);


    void shareChildren([[maybe_unused]] const XmlNodeRef& fromNode) override { assert(0); };
    void copyAttributes(XmlNodeRef fromNode) override { assert(0); };

    //! Get XML Node attribute for specified key.
    const char* getAttr(const char* key) const override;

    //! Get XML Node attribute for specified key.
    // Returns true if the attribute exists, false otherwise.
    bool getAttr(const char* key, const char** value) const override;

    //! Check if attributes with specified key exist.
    bool haveAttr(const char* key) const override;

    XmlNodeRef newChild([[maybe_unused]] const char* tagName) override { assert(0); return 0; };
    void replaceChild([[maybe_unused]] int inChild, [[maybe_unused]] const XmlNodeRef& node) override { assert(0); };
    void insertChild([[maybe_unused]] int inChild, [[maybe_unused]] const XmlNodeRef& node) override { assert(0); };
    void addChild([[maybe_unused]] const XmlNodeRef& node) override { assert(0); };
    void removeChild([[maybe_unused]] const XmlNodeRef& node) override { assert(0); };

    //! Remove all child nodes.
    void removeAllChilds() override { assert(0); };

    //! Get number of child XML nodes.
    int getChildCount() const override { return (int)_node()->nChildCount; };

    //! Get XML Node child nodes.
    XmlNodeRef getChild(int i) const override;

    //! Find node with specified tag.
    XmlNodeRef findChild(const char* tag) const override;
    void deleteChild([[maybe_unused]] const char* tag) { assert(0); };
    void deleteChildAt([[maybe_unused]] int nIndex) override { assert(0); };

    //! Get parent XML node.
    XmlNodeRef  getParent() const override;

    //! Returns content of this node.
    const char* getContent() const override { return _string(_node()->nContentStringOffset); };
    void setContent([[maybe_unused]] const char* str) override { assert(0); };

    XmlNodeRef  clone() override  { assert(0); return 0; };

    //! Returns line number for XML tag.
    int getLine() const override { return 0; };
    //! Set line number in xml.
    void setLine([[maybe_unused]] int line) override { assert(0); };

    //! Returns XML of this node and sub nodes.
    IXmlStringData* getXMLData([[maybe_unused]] int nReserveMem = 0) const override { assert(0); return 0; };
    XmlString getXML([[maybe_unused]] int level = 0) const override { assert(0); return ""; };
    bool saveToFile([[maybe_unused]] const char* fileName) override { assert(0); return false; };   // saves in one huge chunk
    bool saveToFile([[maybe_unused]] const char* fileName, [[maybe_unused]] size_t chunkSizeBytes, [[maybe_unused]] AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle) override { assert(0); return false; };   // save in small memory chunks

    //! Set new XML Node attribute (or override attribute with same key).
    using IXmlNode::setAttr;
    void setAttr([[maybe_unused]] const char* key, [[maybe_unused]] const char* value) override { assert(0); };
    void setAttr([[maybe_unused]] const char* key, [[maybe_unused]] int value) override { assert(0); };
    void setAttr([[maybe_unused]] const char* key, [[maybe_unused]] unsigned int value) override { assert(0); };
    void setAttr([[maybe_unused]] const char* key, [[maybe_unused]] int64 value) override { assert(0); };
    void setAttr([[maybe_unused]] const char* key, [[maybe_unused]] uint64 value, [[maybe_unused]] bool useHexFormat = true /* ignored */) override { assert(0); };
    void setAttr([[maybe_unused]] const char* key, [[maybe_unused]] float value) override { assert(0); };
    void setAttr([[maybe_unused]] const char* key, [[maybe_unused]] f64 value) override { assert(0); };
    void setAttr([[maybe_unused]] const char* key, [[maybe_unused]] const Vec2& value) override { assert(0); };
    void setAttr([[maybe_unused]] const char* key, [[maybe_unused]] const Ang3& value) override { assert(0); };
    void setAttr([[maybe_unused]] const char* key, [[maybe_unused]] const Vec3& value) override { assert(0); };
    void setAttr([[maybe_unused]] const char* key, [[maybe_unused]] const Vec4& value) override { assert(0); };
    void setAttr([[maybe_unused]] const char* key, [[maybe_unused]] const Quat& value) override { assert(0); };
    void delAttr([[maybe_unused]] const char* key) override { assert(0); };
    void removeAllAttributes() override { assert(0); };

    //! Get attribute value of node.
    bool getAttr(const char* key, int& value) const override;
    bool getAttr(const char* key, unsigned int& value) const override;
    bool getAttr(const char* key, int64& value) const override;
    bool getAttr(const char* key, uint64& value, bool useHexFormat = true /* ignored */) const override;
    bool getAttr(const char* key, float& value) const override;
    bool getAttr(const char* key, f64& value) const override;
    bool getAttr(const char* key, bool& value) const override;
    bool getAttr(const char* key, XmlString& value) const override  {const char*    v(NULL); bool  boHasAttribute(getAttr(key, &v)); value = v; return boHasAttribute; }
    bool getAttr(const char* key, Vec2& value) const override;
    bool getAttr(const char* key, Ang3& value) const override;
    bool getAttr(const char* key, Vec3& value) const override;
    bool getAttr(const char* key, Vec4& value) const override;
    bool getAttr(const char* key, Quat& value) const override;
    bool getAttr(const char* key, ColorB& value) const override;

private:
    //////////////////////////////////////////////////////////////////////////
    // INTERNAL METHODS
    //////////////////////////////////////////////////////////////////////////
    const char* GetValue(const char* key) const
    {
        const XMLBinary::Attribute* const pAttributes = m_pData->pAttributes;
        const char* const pStringData = m_pData->pStringData;

        const int nFirst = _node()->nFirstAttributeIndex;
        const int nLast = nFirst + _node()->nAttributeCount;
        for (int i = nFirst; i < nLast; i++)
        {
            const char* const attrKey = pStringData + pAttributes[i].nKeyStringOffset;
            if (g_pXmlStrCmp(key, attrKey) == 0)
            {
                const char* attrValue = pStringData + pAttributes[i].nValueStringOffset;
                return attrValue;
            }
        }
        return 0;
    }

    // Return current node in binary data.
    const XMLBinary::Node* _node() const
    {
        return &m_pData->pNodes[this - m_pData->pBinaryNodes];
    }

    const char* _string(int nIndex) const
    {
        return m_pData->pStringData + nIndex;
    }

protected:
    void setParent([[maybe_unused]] const XmlNodeRef& inRef) override     { assert(0); }

    //////////////////////////////////////////////////////////////////////////
private:
    CBinaryXmlData* m_pData;

    friend class XMLBinary::XMLBinaryReader;
};

#endif // CRYINCLUDE_CRYSYSTEM_XML_XMLBINARYNODE_H
