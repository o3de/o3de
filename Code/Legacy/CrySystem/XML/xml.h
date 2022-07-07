/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <algorithm>
#include <stack>

#include "IXml.h"

// track some XML stats. only to find persistent XML nodes in the system
// slow, so disable by default
//#define CRY_COLLECT_XML_NODE_STATS
//#undef CRY_COLLECT_XML_NODE_STATS


struct IXmlStringPool
{
public:
    IXmlStringPool() { m_refCount = 0; }
    virtual ~IXmlStringPool() {};
    void AddRef() { m_refCount++; };
    void Release()
    {
        if (--m_refCount <= 0)
        {
            delete this;
        }
    };
    virtual const char* AddString(const char* str) = 0;
private:
    int m_refCount;
};

/************************************************************************/
/* XmlParser class, Parse xml and return root xml node if success.      */
/************************************************************************/
class XmlParser
    : public IXmlParser
{
public:
    explicit XmlParser(bool bReuseStrings);
    ~XmlParser() override;

    void AddRef() override
    {
        ++m_nRefCount;
    }

    void Release() override
    {
        if (--m_nRefCount <= 0)
        {
            delete this;
        }
    }

    XmlNodeRef ParseFile(const char* filename, bool bCleanPools) override;

    XmlNodeRef ParseBuffer(const char* buffer, int nBufLen, bool bCleanPools, bool bSuppressWarnings = false) override;

    const char* getErrorString() const { return m_errorString; }

private:
    int m_nRefCount;
    XmlString m_errorString;
    class XmlParserImp* m_pImpl;
};

// Compare function for string comparasion, can be strcmp or _stricmp
typedef int (__cdecl * XmlStrCmpFunc)(const char* str1, const char* str2);
extern XmlStrCmpFunc g_pXmlStrCmp;

//////////////////////////////////////////////////////////////////////////
// XmlAttribute class
//////////////////////////////////////////////////////////////////////////
struct XmlAttribute
{
    const char* key;
    const char* value;

    bool operator<(const XmlAttribute& attr) const { return g_pXmlStrCmp(key, attr.key) < 0; }
    bool operator>(const XmlAttribute& attr) const { return g_pXmlStrCmp(key, attr.key) > 0; }
    bool operator==(const XmlAttribute& attr) const { return g_pXmlStrCmp(key, attr.key) == 0; }
    bool operator!=(const XmlAttribute& attr) const { return g_pXmlStrCmp(key, attr.key) != 0; }
};

//! Xml node attributes class.

typedef std::vector<XmlAttribute>   XmlAttributes;
typedef XmlAttributes::iterator XmlAttrIter;
typedef XmlAttributes::const_iterator XmlAttrConstIter;

/**
******************************************************************************
* CXmlNode class
* Never use CXmlNode directly instead use reference counted XmlNodeRef.
******************************************************************************
*/

class CXmlNode
    : public IXmlNode
{
public:
    //! Constructor.
    CXmlNode();
    CXmlNode(const char* tag, bool bReuseStrings, bool bIsProcessingInstruction = false);
    //! Destructor.
    ~CXmlNode() override;

    //////////////////////////////////////////////////////////////////////////
    // Custom new/delete with pool allocator.
    //////////////////////////////////////////////////////////////////////////
    //void* operator new( size_t nSize );
    //void operator delete( void *ptr );

    void DeleteThis() override;

    //! Create new XML node.
    XmlNodeRef createNode(const char* tag) override;

    //! Get XML node tag.
    const char* getTag() const override
    { return m_tag; };
    void    setTag(const char* tag) override;

    //! Return true if given tag equal to node tag.
    bool isTag(const char* tag) const override;

    //! Get XML Node attributes.
    int getNumAttributes() const override
    { return m_pAttributes ? (int)m_pAttributes->size() : 0; };
    //! Return attribute key and value by attribute index.
    bool getAttributeByIndex(int index, const char** key, const char** value) override;

    //! Return attribute key and value by attribute index, string version.
    virtual bool getAttributeByIndex(int index, XmlString& key, XmlString& value);

    void copyAttributes(XmlNodeRef fromNode) override;

    //! Get XML Node attribute for specified key.
    const char* getAttr(const char* key) const override;

    //! Get XML Node attribute for specified key.
    // Returns true if the attribute existes, alse otherwise.
    bool                getAttr(const char* key, const char** value) const override;

    //! Check if attributes with specified key exist.
    bool haveAttr(const char* key) const override;

    //! Creates new xml node and add it to childs list.
    XmlNodeRef newChild(const char* tagName) override;

    //! Adds new child node.
    void addChild(const XmlNodeRef& node) override;
    //! Remove child node.
    void removeChild(const XmlNodeRef& node) override;

    //! Remove all child nodes.
    void removeAllChilds() override;

    //! Get number of child XML nodes.
    int getChildCount() const override { return m_pChilds ? (int)m_pChilds->size() : 0; }

    //! Get XML Node child nodes.
    XmlNodeRef getChild(int i) const override;

    //! Find node with specified tag.
    XmlNodeRef findChild(const char* tag) const override;
    void deleteChild(const char* tag);

    //! Get parent XML node.
    XmlNodeRef  getParent() const override { return m_parent; }
    void setParent(const XmlNodeRef& inRef) override;


    //! Returns content of this node.
    const char* getContent() const override
    { return m_content; };
    void setContent(const char* str) override;

    //! Set line number in xml.
    void setLine(int line) override { m_line = line; }

    //! Returns XML of this node and sub nodes.
    IXmlStringData* getXMLData(int nReserveMem = 0) const override;
    XmlString getXML(int level = 0) const override;
    bool saveToFile(const char* fileName) override;   // saves in one huge chunk
    bool saveToFile(const char* fileName, size_t chunkSizeBytes, AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle) override; // save in small memory chunks

    //! Set new XML Node attribute (or override attribute with same key).
    using IXmlNode::setAttr;
    void setAttr(const char* key, const char* value) override;
    void setAttr(const char* key, int value) override;
    void setAttr(const char* key, unsigned int value) override;
    void setAttr(const char* key, int64 value) override;
    void setAttr(const char* key, uint64 value, bool useHexFormat = true) override;
    void setAttr(const char* key, float value) override;
    void setAttr(const char* key, double value) override;
    void setAttr(const char* key, const Vec2& value) override;
    void setAttr(const char* key, const Ang3& value) override;
    void setAttr(const char* key, const Vec3& value) override;
    void setAttr(const char* key, const Vec4& value) override;
    void setAttr(const char* key, const Quat& value) override;

    //! Delete attrbute.
    void delAttr(const char* key) override;
    //! Remove all node attributes.
    void removeAllAttributes() override;

    //! Get attribute value of node.
    bool getAttr(const char* key, int& value) const override;
    bool getAttr(const char* key, unsigned int& value) const override;
    bool getAttr(const char* key, int64& value) const override;
    bool getAttr(const char* key, uint64& value, bool useHexFormat = true  /*ignored*/) const override;
    bool getAttr(const char* key, float& value) const override;
    bool getAttr(const char* key, double& value) const override;
    bool getAttr(const char* key, bool& value) const override;

    bool getAttr(const char* key, XmlString& value) const override
    {const char*    v(NULL); bool  boHasAttribute(getAttr(key, &v)); value = v; return boHasAttribute; }

    bool getAttr(const char* key, Vec2& value) const override;
    bool getAttr(const char* key, Ang3& value) const override;
    bool getAttr(const char* key, Vec3& value) const override;
    bool getAttr(const char* key, Vec4& value) const override;
    bool getAttr(const char* key, Quat& value) const override;
    bool getAttr(const char* key, ColorB& value) const override;

protected:

private:
    CXmlNode(const CXmlNode&);
    CXmlNode& operator = (const CXmlNode&);

private:
    void ReleaseChild(IXmlNode* pChild);
    void removeAllChildsImpl();

    void AddToXmlString(XmlString& xml, int level, AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle, size_t chunkSizeBytes = 0) const;
    char* AddToXmlStringUnsafe(char* xml, int level, char* endPtr, AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle, size_t chunkSizeBytes = 0) const;
    XmlString MakeValidXmlString(const XmlString& xml) const;
    bool IsValidXmlString(const char* str) const;
    XmlAttrConstIter GetAttrConstIterator(const char* key) const
    {
        assert(m_pAttributes);

        XmlAttribute tempAttr;
        tempAttr.key = key;

        XmlAttributes::const_iterator it = std::find(m_pAttributes->begin(), m_pAttributes->end(), tempAttr);
        return it;

        /*
        XmlAttributes::const_iterator it = std::lower_bound( m_attributes.begin(),m_attributes.end(),tempAttr );
        if (it != m_attributes.end() && _stricmp(it->key,key) == 0)
        return it;
        return m_attributes.end();
        */
    }
    XmlAttrIter GetAttrIterator(const char* key)
    {
        assert(m_pAttributes);

        XmlAttribute tempAttr;
        tempAttr.key = key;

        XmlAttributes::iterator it = std::find(m_pAttributes->begin(), m_pAttributes->end(), tempAttr);
        return it;

        //      XmlAttributes::iterator it = std::lower_bound( m_attributes.begin(),m_attributes.end(),tempAttr );
        //if (it != m_attributes.end() && _stricmp(it->key,key) == 0)
        //return it;
        //return m_attributes.end();
    }
    const char* GetValue(const char* key) const
    {
        if (m_pAttributes)
        {
            XmlAttrConstIter it = GetAttrConstIterator(key);
            if (it != m_pAttributes->end())
            {
                return it->value;
            }
        }
        return 0;
    }

protected:
    // String pool used by this node.
    IXmlStringPool* m_pStringPool;

    //! Tag of XML node.
    const char* m_tag;

private:

    //! Content of XML node.
    const char* m_content;
    //! Parent XML node.
    IXmlNode* m_parent;

    //typedef DynArray<CXmlNode*,XmlDynArrayAlloc>  XmlNodes;
    typedef std::vector<IXmlNode*>  XmlNodes;
    //XmlNodes m_childs;
    XmlNodes* m_pChilds;

    //! Xml node attributes.
    //XmlAttributes m_attributes;
    XmlAttributes* m_pAttributes;

    //! Line in XML file where this node firstly appeared (useful for debugging).
    int m_line;

    bool m_isProcessingInstruction;
    friend class XmlParserImp;
};

#ifdef CRY_COLLECT_XML_NODE_STATS
typedef std::set<CXmlNode*> TXmlNodeSet; // yes, slow, but really only for one-shot debugging
struct SXmlNodeStats
{
    SXmlNodeStats()
        : nAllocs(0)
        , nFrees(0) {}
    TXmlNodeSet nodeSet;
    uint32 nAllocs;
    uint32 nFrees;
};
extern SXmlNodeStats* g_pCXmlNode_Stats;
#endif


//////////////////////////////////////////////////////////////////////////
//
// Reusable XmlNode for XmlNode pool with shared xml string pool
//
//////////////////////////////////////////////////////////////////////////
class CXmlNodePool;

class CXmlNodeReuse
    : public CXmlNode
{
public:
    CXmlNodeReuse(const char* tag, CXmlNodePool* pPool);
    void Release() override;

protected:
    CXmlNodePool* m_pPool;
};

//////////////////////////////////////////////////////////////////////////
//
// Pool of reusable XML nodes with shared string pool
//
//////////////////////////////////////////////////////////////////////////
class CXmlNodePool
{
public:
    CXmlNodePool(unsigned int nBlockSize, bool bReuseStrings);
    virtual ~CXmlNodePool();

    XmlNodeRef GetXmlNode(const char* sNodeName);
    bool empty() const { return (m_nAllocated == 0); }

protected:
    virtual void OnRelease(int iRefCount, void* pThis);
    IXmlStringPool* GetStringPool() { return m_pStringPool; }

private:
    friend class CXmlNodeReuse;

    IXmlStringPool* m_pStringPool;
    unsigned int m_nAllocated;
    std::stack<CXmlNodeReuse*> m_pNodePool;
};
