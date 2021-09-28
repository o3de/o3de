/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "CrySystem_precompiled.h"

#include <stdlib.h>

#define XML_STATIC // Alternative to defining this here would be setting it project-wide
#include <expat.h>
#include "xml.h"
#include <algorithm>
#include <stdio.h>
#include <AzFramework/Archive/IArchive.h>
#include <CryCommon/Cry_Color.h>
#include "XMLBinaryReader.h"

#define FLOAT_FMT   "%.8g"
#define DOUBLE_FMT  "%.17g"

#include "../SimpleStringPool.h"

#include "System.h"

#if AZ_TRAIT_OS_PLATFORM_APPLE || defined(AZ_PLATFORM_LINUX)
#include <clocale>
#include <locale>

class LocaleResetter
{
public:
    LocaleResetter()
    {
        m_oldLocale = std::setlocale(LC_NUMERIC, "C");
    }
    ~LocaleResetter()
    {
        std::setlocale(LC_NUMERIC, m_oldLocale);
    }

private:
    char* m_oldLocale;
};
#define SCOPED_LOCALE_RESETTER LocaleResetter l
#else
// noop on Windows
#define SCOPED_LOCALE_RESETTER
#endif

// Global counter for memory allocated in XML string pools.
size_t CSimpleStringPool::g_nTotalAllocInXmlStringPools = 0;

//////////////////////////////////////////////////////////////////////////
static int __cdecl ascii_stricmp(const char* dst, const char* src)
{
    int f, l;
    do
    {
        if (((f = (unsigned char)(*(dst++))) >= 'A') && (f <= 'Z'))
        {
            f -= 'A' - 'a';
        }
        if (((l = (unsigned char)(*(src++))) >= 'A') && (l <= 'Z'))
        {
            l -= 'A' - 'a';
        }
    }
    while (f && (f == l));
    return(f - l);
}

//////////////////////////////////////////////////////////////////////////
XmlStrCmpFunc g_pXmlStrCmp = &ascii_stricmp;

//////////////////////////////////////////////////////////////////////////
class CXmlStringData
    : public IXmlStringData
{
public:
    int m_nRefCount;
    XmlString m_string;

    CXmlStringData() { m_nRefCount = 0; }
    virtual void AddRef() { ++m_nRefCount; }
    virtual void Release()
    {
        if (--m_nRefCount <= 0)
        {
            delete this;
        }
    }

    virtual const char* GetString() { return m_string.c_str(); };
    virtual size_t GetStringLength() { return m_string.size(); };
};

//////////////////////////////////////////////////////////////////////////
class CXmlStringPool
    : public IXmlStringPool
{
public:
    explicit CXmlStringPool(bool bReuseStrings)
        : m_stringPool(bReuseStrings) {}

    const char* AddString(const char* str) { return m_stringPool.Append(str, (int)strlen(str)); }
    void Clear() { m_stringPool.Clear(); }
    void SetBlockSize(unsigned int nBlockSize) { m_stringPool.SetBlockSize(nBlockSize); }

private:
    CSimpleStringPool m_stringPool;
};

//xml_node_allocator XmlDynArrayAlloc::allocator;
//size_t XmlDynArrayAlloc::m_iAllocated = 0;
/**
******************************************************************************
* CXmlNode implementation.
******************************************************************************
*/

void CXmlNode::DeleteThis()
{
    delete this;
}

CXmlNode::~CXmlNode()
{
    m_nRefCount = 1;      // removeAllChildsImpl can make an XmlNodeRef to this node whilst it is deleting the children
    // doing this will cause the ref count to be increment and decremented and cause delete to be called again,
    // leading to a recursion crash. upping the ref count here once destruction has started avoids this problem
    removeAllChildsImpl();

    SAFE_DELETE(m_pAttributes);

    m_pStringPool->Release();
}

CXmlNode::CXmlNode()
    : m_pStringPool(NULL) // must be changed later.
    , m_tag("")
    , m_content("")
    , m_parent(NULL)
    , m_pChilds(NULL)
    , m_pAttributes(NULL)
    , m_line(0)
    , m_isProcessingInstruction(false)
{
    m_nRefCount = 0; //TODO: move initialization to IXmlNode constructor
}

CXmlNode::CXmlNode(const char* tag, bool bReuseStrings, bool bIsProcessingInstruction)
    : m_content("")
    , m_parent(NULL)
    , m_pChilds(NULL)
    , m_pAttributes(NULL)
    , m_line(0)
    , m_isProcessingInstruction(bIsProcessingInstruction)
{
    m_nRefCount = 0; //TODO: move initialization to IXmlNode constructor

    m_pStringPool = new CXmlStringPool(bReuseStrings);
    m_pStringPool->AddRef();
    m_tag = m_pStringPool->AddString(tag);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CXmlNode::createNode(const char* tag)
{
    CXmlNode* pNewNode = new CXmlNode;
    pNewNode->m_pStringPool = m_pStringPool;
    m_pStringPool->AddRef();
    pNewNode->m_tag = m_pStringPool->AddString(tag);
    return XmlNodeRef(pNewNode);
}

//////////////////////////////////////////////////////////////////////////
void CXmlNode::setTag(const char* tag)
{
    m_tag = m_pStringPool->AddString(tag);
}

//////////////////////////////////////////////////////////////////////////
void CXmlNode::setContent(const char* str)
{
    m_content = m_pStringPool->AddString(str);
}

//////////////////////////////////////////////////////////////////////////
bool CXmlNode::isTag(const char* tag) const
{
    return g_pXmlStrCmp(tag, m_tag) == 0;
}

const char* CXmlNode::getAttr(const char* key) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        return svalue;
    }
    return "";
}

bool    CXmlNode::getAttr(const char* key, const char** value) const
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

bool CXmlNode::haveAttr(const char* key) const
{
    if (m_pAttributes)
    {
        XmlAttrConstIter it = GetAttrConstIterator(key);
        if (it != m_pAttributes->end())
        {
            return true;
        }
    }
    return false;
}

void CXmlNode::delAttr(const char* key)
{
    if (m_pAttributes)
    {
        XmlAttrIter it = GetAttrIterator(key);
        if (it != m_pAttributes->end())
        {
            m_pAttributes->erase(it);
        }
    }
}

void CXmlNode::removeAllAttributes()
{
    if (m_pAttributes)
    {
        m_pAttributes->clear();
        SAFE_DELETE(m_pAttributes);
    }
}

void CXmlNode::setAttr(const char* key, const char* value)
{
    if (!m_pAttributes)
    {
        m_pAttributes = new XmlAttributes;
    }
    assert(m_pAttributes);

    XmlAttrIter it = GetAttrIterator(key);
    if (it == m_pAttributes->end())
    {
        XmlAttribute tempAttr;
        tempAttr.key = m_pStringPool->AddString(key);
        tempAttr.value = m_pStringPool->AddString(value);
        m_pAttributes->push_back(tempAttr);
        // Sort attributes.
        //std::sort( m_pAttributes->begin(),m_pAttributes->end() );
    }
    else
    {
        // If already exist, override this member.
        it->value = m_pStringPool->AddString(value);
    }
}

void CXmlNode::setAttr(const char* key, int value)
{
    char str[128];
    azitoa(value, str, AZ_ARRAY_SIZE(str), 10);
    setAttr(key, str);
}

void CXmlNode::setAttr(const char* key, unsigned int value)
{
    char str[128];
    azui64toa(value, str, AZ_ARRAY_SIZE(str), 10);
    setAttr(key, str);
}

void CXmlNode::setAttr(const char* key, float value)
{
    char str[128];
    SCOPED_LOCALE_RESETTER;
    sprintf_s(str, FLOAT_FMT, value);
    setAttr(key, str);
}

void CXmlNode::setAttr(const char* key, double value)
{
    char str[128];
    SCOPED_LOCALE_RESETTER;
    sprintf_s(str, DOUBLE_FMT, value);
    setAttr(key, str);
}

//////////////////////////////////////////////////////////////////////////
void CXmlNode::setAttr(const char* key, int64 value)
{
    char str[32];
    sprintf_s(str, "%" PRId64, value);
    setAttr(key, str);
}

//////////////////////////////////////////////////////////////////////////
void CXmlNode::setAttr(const char* key, uint64 value, bool useHexFormat)
{
    char str[32];
    if (useHexFormat)
    {
        sprintf_s(str, "%" PRIX64, value);
    }
    else
    {
        sprintf_s(str, "%" PRIu64, value);
    }
    setAttr(key, str);
}

void CXmlNode::setAttr(const char* key, const Ang3& value)
{
    char str[128];
    SCOPED_LOCALE_RESETTER;
    sprintf_s(str, FLOAT_FMT "," FLOAT_FMT "," FLOAT_FMT, value.x, value.y, value.z);
    setAttr(key, str);
}
void CXmlNode::setAttr(const char* key, const Vec3& value)
{
    char str[128];
    SCOPED_LOCALE_RESETTER;
    sprintf_s(str, FLOAT_FMT "," FLOAT_FMT "," FLOAT_FMT, value.x, value.y, value.z);
    setAttr(key, str);
}
void CXmlNode::setAttr(const char* key, const Vec4& value)
{
    char str[128];
    SCOPED_LOCALE_RESETTER;
    sprintf_s(str, FLOAT_FMT "," FLOAT_FMT "," FLOAT_FMT "," FLOAT_FMT, value.x, value.y, value.z, value.w);
    setAttr(key, str);
}

void CXmlNode::setAttr(const char* key, const Vec2& value)
{
    char str[128];
    SCOPED_LOCALE_RESETTER;
    sprintf_s(str, FLOAT_FMT "," FLOAT_FMT, value.x, value.y);
    setAttr(key, str);
}

void CXmlNode::setAttr(const char* key, const Quat& value)
{
    char str[128];
    SCOPED_LOCALE_RESETTER;
    sprintf_s(str, FLOAT_FMT "," FLOAT_FMT "," FLOAT_FMT "," FLOAT_FMT, value.w, value.v.x, value.v.y, value.v.z);
    setAttr(key, str);
}

//////////////////////////////////////////////////////////////////////////
bool CXmlNode::getAttr(const char* key, int& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        value = atoi(svalue);
        return true;
    }
    return false;
}

bool CXmlNode::getAttr(const char* key, unsigned int& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        value = strtoul(svalue, NULL, 10);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CXmlNode::getAttr(const char* key, int64& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        value = strtoll(key, nullptr, 10);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CXmlNode::getAttr(const char* key, uint64& value, bool useHexFormat) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        value = strtoull(key, nullptr, useHexFormat ? 16 : 10);
        return true;
    }
    return false;
}

bool CXmlNode::getAttr(const char* key, bool& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        if (azstricmp(svalue, "true") == 0)
        {
            value = true;
        }
        else if (azstricmp(svalue, "false") == 0)
        {
            value = false;
        }
        else
        {
            value = atoi(svalue) != 0;
        }
        return true;
    }
    return false;
}

bool CXmlNode::getAttr(const char* key, float& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        value = (float)atof(svalue);
        return true;
    }
    return false;
}

bool CXmlNode::getAttr(const char* key, double& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        value = atof(svalue);
        return true;
    }
    return false;
}

bool CXmlNode::getAttr(const char* key, Ang3& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        SCOPED_LOCALE_RESETTER;
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
bool CXmlNode::getAttr(const char* key, Vec3& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        SCOPED_LOCALE_RESETTER;
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
bool CXmlNode::getAttr(const char* key, Vec4& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        SCOPED_LOCALE_RESETTER;
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
bool CXmlNode::getAttr(const char* key, Vec2& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        SCOPED_LOCALE_RESETTER;
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
bool CXmlNode::getAttr(const char* key, Quat& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        SCOPED_LOCALE_RESETTER;
        float w, x, y, z;
        if (azsscanf(svalue, "%f,%f,%f,%f", &w, &x, &y, &z) == 4)
        {
            if (fabs(w) > VEC_EPSILON || fabs(x) > VEC_EPSILON || fabs(y) > VEC_EPSILON || fabs(z) > VEC_EPSILON)
            {
                //[AlexMcC|02.03.10] directly assign to members to avoid triggering the assert in Quat() with data from bad assets
                value.w = w;
                value.v = Vec3(x, y, z);
                return value.IsValid();
            }
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CXmlNode::getAttr(const char* key, ColorB& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
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


XmlNodeRef CXmlNode::findChild(const char* tag) const
{
    if (m_pChilds)
    {
        XmlNodes& childs = *m_pChilds;
        for (int i = 0, num = static_cast<int>(childs.size()); i < num; ++i)
        {
            if (childs[i]->isTag(tag))
            {
                return childs[i];
            }
        }
    }
    return 0;
}

void CXmlNode::removeChild(const XmlNodeRef& node)
{
    if (m_pChilds)
    {
        XmlNodes::iterator it = std::find(m_pChilds->begin(), m_pChilds->end(), (IXmlNode*)node);
        if (it != m_pChilds->end())
        {
            ReleaseChild(*it);
            m_pChilds->erase(it);
        }
    }
}

void CXmlNode::removeAllChilds()
{
    removeAllChildsImpl();
}

//////////////////////////////////////////////////////////////////////////
void CXmlNode::deleteChild(const char* tag)
{
    if (m_pChilds)
    {
        XmlNodes& childs = *m_pChilds;
        for (int i = 0, num = static_cast<int>(childs.size()); i < num; ++i)
        {
            if (childs[i]->isTag(tag))
            {
                ReleaseChild(childs[i]);
                childs.erase(childs.begin() + i);
                return;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CXmlNode::deleteChildAt(int nIndex)
{
    if (m_pChilds)
    {
        XmlNodes& childs = *m_pChilds;
        if (nIndex >= 0 && nIndex < (int)childs.size())
        {
            ReleaseChild(childs[nIndex]);
            childs.erase(childs.begin() + nIndex);
        }
    }
}

//! Adds new child node.
void CXmlNode::addChild(const XmlNodeRef& node)
{
    if (!m_pChilds)
    {
        m_pChilds = new XmlNodes;
    }

    assert(node != 0);
    IXmlNode* pNode = ((IXmlNode*)node);
    pNode->AddRef();
    m_pChilds->push_back(pNode);
    pNode->setParent(this);
};

void CXmlNode::shareChildren(const XmlNodeRef& inFromMe)
{
    int     numChildren = inFromMe->getChildCount();

    removeAllChilds();

    if (numChildren > 0)
    {
        XmlNodeRef      child;

        m_pChilds = new XmlNodes;
        m_pChilds->reserve(numChildren);
        for (int i = 0; i < numChildren; i++)
        {
            child = inFromMe->getChild(i);

            child->AddRef();
            // not overwriting parent assignment of child, we share the node but do not exclusively own it
            m_pChilds->push_back(child);
        }
    }
}

void CXmlNode::setParent(const XmlNodeRef& inNewParent)
{
    // note, parent ptrs are not ref counted
    m_parent = inNewParent;
}

void CXmlNode::insertChild(int inIndex, const XmlNodeRef& inNewChild)
{
    assert(inIndex >= 0 && inIndex <= getChildCount());
    assert(inNewChild != 0);
    if (inIndex >= 0 && inIndex <= getChildCount() && inNewChild)
    {
        if (getChildCount() == 0)
        {
            addChild(inNewChild);
        }
        else
        {
            IXmlNode* pNode = ((IXmlNode*)inNewChild);
            pNode->AddRef();
            m_pChilds->insert(m_pChilds->begin() + inIndex, pNode);
            pNode->setParent(this);
        }
    }
}

void CXmlNode::replaceChild(int inIndex, const XmlNodeRef& inNewChild)
{
    assert(inIndex >= 0 && inIndex < getChildCount());
    assert(inNewChild != 0);
    if (inIndex >= 0 && inIndex < getChildCount() && inNewChild)
    {
        IXmlNode* wasChild = (*m_pChilds)[inIndex];

        if (wasChild->getParent() == this)
        {
            wasChild->setParent(XmlNodeRef());          // child is orphaned, will be freed by Release() below if this parent is last holding a reference to it
        }
        wasChild->Release();
        inNewChild->AddRef();
        (*m_pChilds)[inIndex] = inNewChild;
        inNewChild->setParent(this);
    }
}

XmlNodeRef CXmlNode::newChild(const char* tagName)
{
    XmlNodeRef node = createNode(tagName);
    addChild(node);
    return node;
}

//! Get XML Node child nodes.
XmlNodeRef CXmlNode::getChild(int i) const
{
    assert(m_pChilds);
    XmlNodes& childs = *m_pChilds;
    assert(i >= 0 && i < (int)childs.size());
    return childs[i];
}

//////////////////////////////////////////////////////////////////////////
void CXmlNode::copyAttributes(XmlNodeRef fromNode)
{
    IXmlNode* inode = fromNode;
    CXmlNode* n = (CXmlNode*)inode;
    assert(n);
    PREFAST_ASSUME(n);

    if (n != this)
    {
        if (n->m_pAttributes)
        {
            if (!m_pAttributes)
            {
                m_pAttributes = new XmlAttributes;
            }

            if (n->m_pStringPool == m_pStringPool)
            {
                *m_pAttributes = *(n->m_pAttributes);
            }
            else
            {
                XmlAttributes& lhs = *m_pAttributes;
                const XmlAttributes& rhs = *(n->m_pAttributes);
                lhs.resize(rhs.size());
                for (size_t i = 0; i < rhs.size(); ++i)
                {
                    lhs[i].key = m_pStringPool->AddString(rhs[i].key);
                    lhs[i].value = m_pStringPool->AddString(rhs[i].value);
                }
            }
        }
        else
        {
            SAFE_DELETE(m_pAttributes);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CXmlNode::getAttributeByIndex(int index, const char** key, const char** value)
{
    if (m_pAttributes)
    {
        XmlAttributes::iterator it = m_pAttributes->begin();
        if (it != m_pAttributes->end())
        {
            std::advance(it, index);
            if (it != m_pAttributes->end())
            {
                *key = it->key;
                *value = it->value;
                return true;
            }
        }
    }
    return false;
}
//////////////////////////////////////////////////////////////////////////
bool CXmlNode::getAttributeByIndex(int index, XmlString& key, XmlString& value)
{
    if (m_pAttributes)
    {
        XmlAttributes::iterator it = m_pAttributes->begin();
        if (it != m_pAttributes->end())
        {
            std::advance(it, index);
            if (it != m_pAttributes->end())
            {
                key = it->key;
                value = it->value;
                return true;
            }
        }
    }
    return false;
}
//////////////////////////////////////////////////////////////////////////
XmlNodeRef CXmlNode::clone()
{
    CXmlNode* node = new CXmlNode;
    XmlNodeRef  result(node);
    node->m_pStringPool = m_pStringPool;
    m_pStringPool->AddRef();
    node->m_tag = m_tag;
    node->m_content = m_content;
    // Clone attributes.
    CXmlNode* n = (CXmlNode*)(IXmlNode*)node;
    n->copyAttributes(this);
    // Clone sub nodes.

    if (m_pChilds)
    {
        const XmlNodes& childs = *m_pChilds;

        node->m_pChilds = new XmlNodes;
        node->m_pChilds->reserve(childs.size());
        for (int i = 0, num = static_cast<int>(childs.size()); i < num; ++i)
        {
            node->addChild(childs[i]->clone());
        }
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////
static void AddTabsToString(XmlString& xml, int level)
{
    static const char* tabs[] = {
        "",
        " ",
        "  ",
        "   ",
        "    ",
        "     ",
        "      ",
        "       ",
        "        ",
        "         ",
        "          ",
        "           ",
    };
    // Add tabs.
    if (level < sizeof(tabs) / sizeof(tabs[0]))
    {
        xml += tabs[level];
    }
    else
    {
        for (int i = 0; i < level; i++)
        {
            xml += "  ";
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CXmlNode::IsValidXmlString(const char* str) const
{
    int len = static_cast<int>(strlen(str));

    {
        // Prevents invalid characters not from standard ASCII set to propagate to xml.
        // A bit of hack for efficiency, fixes input string in place.
        char* s = const_cast<char*>(str);
        for (int i = 0; i < len; i++)
        {
            if ((unsigned char)s[i] > 0x7F)
            {
                s[i] = ' ';
            }
        }
    }

    if (strcspn(str, "\"\'&><") == len)
    {
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
XmlString CXmlNode::MakeValidXmlString(const XmlString& instr) const
{
    XmlString str = instr;

    // check if str contains any invalid characters
    AZ::StringFunc::Replace(str, "&", "&amp;");
    AZ::StringFunc::Replace(str, "\"", "&quot;");
    AZ::StringFunc::Replace(str, "\'", "&apos;");
    AZ::StringFunc::Replace(str, "<", "&lt;");
    AZ::StringFunc::Replace(str, ">", "&gt;");
    AZ::StringFunc::Replace(str, "...", "&gt;");
    AZ::StringFunc::Replace(str, "\n", "&#10;");

    return str;
}

void CXmlNode::ReleaseChild(IXmlNode* pChild)
{
    if (pChild)
    {
        if (pChild->getParent() == this)      // if check to handle shared children which are supported by the CXmlNode impl
        {
            pChild->setParent(NULL);
        }
        pChild->Release();
    }
}

void CXmlNode::removeAllChildsImpl()
{
    if (m_pChilds)
    {
        for (XmlNodes::iterator iter = m_pChilds->begin(), endIter = m_pChilds->end(); iter != endIter; ++iter)
        {
            ReleaseChild(*iter);
        }
        SAFE_DELETE(m_pChilds);
    }
}

//////////////////////////////////////////////////////////////////////////
void CXmlNode::AddToXmlString(XmlString& xml, int level, AZ::IO::HandleType fileHandle, size_t chunkSize) const
{
    if (fileHandle != AZ::IO::InvalidHandle && chunkSize > 0)
    {
        auto fileIoBase = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIoBase != nullptr, "FileIOBase is expected to be initialized for CXmlNode");
        size_t len = xml.length();
        if (len >= chunkSize)
        {
            fileIoBase->Write(fileHandle, xml.c_str(), len);
            xml.assign (""); // should not free memory and does not!
        }
    }

    AddTabsToString(xml, level);

    const bool bHasChildren = (m_pChilds && !m_pChilds->empty());

    // Begin Tag
    if (!m_pAttributes || m_pAttributes->empty())
    {
        xml += "<";
        if (m_isProcessingInstruction)
        {
            xml += "?";
        }
        xml += m_tag;
        if (*m_content == 0 && !bHasChildren)
        {
            if (m_isProcessingInstruction)
            {
                xml += "?>\n";
            }
            else
            {
            // Compact tag form.
            xml += " />\n";
            }
            return;
        }
        xml += ">";
    }
    else
    {
        xml += "<";
        if (m_isProcessingInstruction)
        {
            xml += "?";
        }
        xml += m_tag;
        xml += " ";

        // Put attributes.
        for (XmlAttributes::const_iterator it = m_pAttributes->begin(); it != m_pAttributes->end(); )
        {
            xml += it->key;
            xml += "=\"";
            if (IsValidXmlString(it->value))
            {
                xml += it->value;
            }
            else
            {
                xml += MakeValidXmlString(it->value);
            }
            ++it;
            if (it != m_pAttributes->end())
            {
                xml += "\" ";
            }
            else
            {
                xml += "\"";
            }
        }
        if (*m_content == 0 && !bHasChildren)
        {
            if (m_isProcessingInstruction)
            {
                xml += "?>\n";
            }
            else
            {
            // Compact tag form.
            xml += "/>\n";
            }
            return;
        }
        xml += ">";
    }

    // Put node content.
    if (IsValidXmlString(m_content))
    {
        xml += m_content;
    }
    else
    {
        xml += MakeValidXmlString(m_content);
    }

    if (!bHasChildren)
    {
        xml += "</";
        xml += m_tag;
        xml += ">\n";
        return;
    }

    xml += "\n";

    // Add sub nodes.
    for (XmlNodes::iterator it = m_pChilds->begin(), itEnd = m_pChilds->end(); it != itEnd; ++it)
    {
        IXmlNode* node = *it;
        ((CXmlNode*)node)->AddToXmlString(xml, level + 1, fileHandle, chunkSize);
    }

    // Add tabs.
    AddTabsToString(xml, level);
    xml += "</";
    xml += m_tag;
    xml += ">\n";
}

#if !defined(APPLE) && !defined(LINUX) && !defined(AZ_LEGACY_CRYSYSTEM_TRAIT_HASSTPCPY)
ILINE static char* stpcpy(char* dst, const char* src)
{
    while (src[0])
    {
        dst[0] = src[0];
        dst++;
        src++;
    }
    return dst;
}
#endif

char* CXmlNode::AddToXmlStringUnsafe(char* xml, int level, char* endPtr, AZ::IO::HandleType fileHandle, size_t chunkSize) const
{
    const bool bHasChildren = (m_pChilds && !m_pChilds->empty());

    for (int i = 0; i < level; i++)
    {
        *(xml++) = ' ';
        *(xml++) = ' ';
    }

    // Begin Tag
    if (!m_pAttributes || m_pAttributes->empty())
    {
        *(xml++) = '<';
        xml = stpcpy(xml, m_tag);
        if (*m_content == 0 && !bHasChildren)
        {
            *(xml++) = '/';
            *(xml++) = '>';
            *(xml++) = '\n';
            return xml;
        }
        *(xml++) = '>';
    }
    else
    {
        *(xml++) = '<';
        xml = stpcpy(xml, m_tag);
        *(xml++) = ' ';

        // Put attributes.
        for (XmlAttributes::const_iterator it = m_pAttributes->begin(); it != m_pAttributes->end(); )
        {
            xml = stpcpy(xml, it->key);
            *(xml++) = '=';
            *(xml++) = '\"';
#ifndef _RELEASE
            if (it->value[strcspn(it->value, "\"\'&><")])
            {
                __debugbreak();
            }
#endif
            xml = stpcpy(xml, it->value);
            ++it;
            *(xml++) = '\"';
            if (it != m_pAttributes->end())
            {
                *(xml++) = ' ';
            }
        }
        if (*m_content == 0 && !bHasChildren)
        {
            // Compact tag form.
            *(xml++) = '/';
            *(xml++) = '>';
            *(xml++) = '\n';
            return xml;
        }
        *(xml++) = '>';
    }

#ifndef _RELEASE
    if (m_content[strcspn(m_content, "\"\'&><")])
    {
        __debugbreak();
    }
#endif
    xml = stpcpy(xml, m_content);

    if (!bHasChildren)
    {
        *(xml++) = '<';
        *(xml++) = '/';
        xml = stpcpy(xml, m_tag);
        *(xml++) = '>';
        *(xml++) = '\n';
        return xml;
    }

    *(xml++) = '\n';

    // Add sub nodes.
    for (XmlNodes::iterator it = m_pChilds->begin(), itEnd = m_pChilds->end(); it != itEnd; ++it)
    {
        IXmlNode* node = *it;
        xml = ((CXmlNode*)node)->AddToXmlStringUnsafe(xml, level + 1, endPtr, fileHandle, chunkSize);
    }

    for (int i = 0; i < level; i++)
    {
        *(xml++) = ' ';
        *(xml++) = ' ';
    }
    *(xml++) = '<';
    *(xml++) = '/';
    xml = stpcpy(xml, m_tag);
    *(xml++) = '>';
    *(xml++) = '\n';

    assert(xml < endPtr);

    return xml;
}

//////////////////////////////////////////////////////////////////////////
IXmlStringData* CXmlNode::getXMLData(int nReserveMem) const
{
    CXmlStringData* pStrData = new CXmlStringData;
    pStrData->m_string.reserve(nReserveMem);
    AddToXmlString(pStrData->m_string, 0);
    return pStrData;
}

//////////////////////////////////////////////////////////////////////////
XmlString CXmlNode::getXML(int level) const
{
    XmlString xml;
    xml = "";
    xml.reserve(1024);

    AddToXmlString(xml, level);
    return xml;
}

XmlString CXmlNode::getXMLUnsafe(int level, char* tmpBuffer, uint32 sizeOfTmpBuffer) const
{
    char* endPtr = tmpBuffer + sizeOfTmpBuffer - 1;
    char* endOfBuffer = AddToXmlStringUnsafe(tmpBuffer, level, endPtr);
    endOfBuffer[0] = '\0';
    XmlString ret(tmpBuffer);
    return ret;
}


// TODO: those 2 saving functions are a bit messy. should probably make a separate one for the use of PlatformAPI
bool CXmlNode::saveToFile(const char* fileName)
{
    if (!fileName)
    {
        return false;
    }

    {
        AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(fileName, "wt");
        if (fileHandle != AZ::IO::InvalidHandle)
        {
#ifdef WIN32
            XmlString xml = getXML();   // this would not work in consoles because the size limits in strings
            const char* sxml = (const char*)xml;
            gEnv->pCryPak->FWrite(sxml, xml.length(), fileHandle);
            gEnv->pCryPak->FClose(fileHandle);
            return true;
#else
            constexpr size_t chunkSizeBytes = (15 * 1024);
            bool ret = saveToFile(fileName, chunkSizeBytes, fileHandle);
            gEnv->pCryPak->FClose(fileHandle);
            return ret;
#endif
        }
        return false;
    }
}

bool CXmlNode::saveToFile([[maybe_unused]] const char* fileName, size_t chunkSize, AZ::IO::HandleType fileHandle)
{
#ifdef WIN32
    CrySetFileAttributes(fileName, 0x00000080);  // FILE_ATTRIBUTE_NORMAL
#endif //WIN32

    if (chunkSize < 256 * 1024)   // make at least 256k
    {
        chunkSize = 256 * 1024;
    }


    XmlString xml;
    xml.assign ("");
    xml.reserve(chunkSize * 2); // we reserve double memory, as writing in chunks is not really writing in fixed blocks but a bit fuzzy
    auto fileIoBase = AZ::IO::FileIOBase::GetInstance();
    AZ_Assert(fileIoBase != nullptr, "FileIOBase is expected to be initialized for CXmlNode");
    if (fileHandle == AZ::IO::InvalidHandle)
    {
        return false;
    }
    AddToXmlString(xml, 0, fileHandle, chunkSize);
    size_t len = xml.length();
    if (len > 0)
    {
        fileIoBase->Write(fileHandle, xml.c_str(), len);
    }
    xml.clear(); // xml.resize(0) would not reclaim memory
    return true;
}


/**
******************************************************************************
* XmlParserImp class.
******************************************************************************
*/
class XmlParserImp
    : public IXmlStringPool
{
public:
    explicit XmlParserImp(bool bReuseStrings);
    ~XmlParserImp();

    void ParseBegin(bool bCleanPools);
    XmlNodeRef ParseFile(const char* filename, XmlString& errorString, bool bCleanPools);
    XmlNodeRef ParseBuffer(const char* buffer, size_t bufLen, XmlString& errorString, bool bCleanPools, bool bSuppressWarnings = false);
    void ParseEnd();

    // Add new string to pool.
    const char* AddString(const char* str) { return m_stringPool.Append(str, (int)strlen(str)); }
protected:
    void    onStartElement(const char* tagName, const char** atts);
    void    onEndElement(const char* tagName);
    void    onRawData(const char* data);

    static void startElement(void* userData, const char* name, const char** atts)
    {
        ((XmlParserImp*)userData)->onStartElement(name, atts);
    }
    static void endElement(void* userData, const char* name)
    {
        ((XmlParserImp*)userData)->onEndElement(name);
    }
    static void characterData(void* userData, const char* s, int len)
    {
        char str[32700];
        if (len > sizeof(str) - 1)
        {
            assert(0);
            len = sizeof(str) - 1;
        }
        // Note that XML buffer userData has no terminating '\0'.
        memcpy(str, s, len);
        str[len] = 0;
        ((XmlParserImp*)userData)->onRawData(str);
    }

    void CleanStack();

    struct SStackEntity
    {
        XmlNodeRef node;
        std::vector<IXmlNode*> childs; //TODO: is it worth lazily initializing this, like CXmlNode::m_pChilds?
    };

    // First node will become root node.
    std::vector<SStackEntity> m_nodeStack;
    int m_nNodeStackTop;

    XmlNodeRef m_root;

    XML_Parser m_parser;
    CSimpleStringPool m_stringPool;
};

//////////////////////////////////////////////////////////////////////////
void XmlParserImp::CleanStack()
{
    m_nNodeStackTop = 0;
    for (int i = 0, num = static_cast<int>(m_nodeStack.size()); i < num; i++)
    {
        m_nodeStack[i].node = 0;
        m_nodeStack[i].childs.resize(0);
    }
}

/**
******************************************************************************
* XmlParserImp
******************************************************************************
*/
void    XmlParserImp::onStartElement(const char* tagName, const char** atts)
{
    CXmlNode* pCNode = new CXmlNode;
    pCNode->m_pStringPool = this;
    pCNode->m_pStringPool->AddRef();
    pCNode->m_tag = AddString(tagName);

    XmlNodeRef node = pCNode;

    m_nNodeStackTop++;
    if (m_nNodeStackTop >= (int)m_nodeStack.size())
    {
        m_nodeStack.resize(m_nodeStack.size() * 2);
    }

    m_nodeStack[m_nNodeStackTop].node = pCNode;
    m_nodeStack[m_nNodeStackTop - 1].childs.push_back(pCNode);

    if (!m_root)
    {
        m_root = node;
    }
    else
    {
        pCNode->m_parent = (IXmlNode*)m_nodeStack[m_nNodeStackTop - 1].node;
        node->AddRef(); // Childs need to be add refed.
    }

    node->setLine(XML_GetCurrentLineNumber((XML_Parser)m_parser));

    // Call start element callback.
    int i = 0;
    int numAttrs = 0;
    while (atts[i] != 0)
    {
        numAttrs++;
        i += 2;
    }
    if (numAttrs > 0)
    {
        i = 0;
        if (!pCNode->m_pAttributes)
        {
            pCNode->m_pAttributes = new XmlAttributes;
        }

        XmlAttributes& nodeAtts = *(pCNode->m_pAttributes);
        nodeAtts.resize(numAttrs);
        int nAttr = 0;
        while (atts[i] != 0)
        {
            nodeAtts[nAttr].key = AddString(atts[i]);
            nodeAtts[nAttr].value = AddString(atts[i + 1]);
            nAttr++;
            i += 2;
        }
        // Sort attributes.
        //std::sort( pCNode->m_attributes.begin(),pCNode->m_attributes.end() );
    }
}

void    XmlParserImp::onEndElement([[maybe_unused]] const char* tagName)
{
    assert(m_nNodeStackTop > 0);

    if (m_nNodeStackTop > 0)
    {
        // Copy current childs to the parent.
        SStackEntity& entry = m_nodeStack[m_nNodeStackTop];
        CXmlNode* currNode = static_cast<CXmlNode*>(static_cast<IXmlNode*>(entry.node));
        if (!entry.childs.empty())
        {
            if (!currNode->m_pChilds)
            {
                currNode->m_pChilds = new CXmlNode::XmlNodes;
            }
            *currNode->m_pChilds = entry.childs;
        }
        entry.childs.resize(0);
        entry.node = NULL;
    }
    m_nNodeStackTop--;
}

void    XmlParserImp::onRawData(const char* data)
{
    assert(m_nNodeStackTop >= 0);
    if (m_nNodeStackTop >= 0)
    {
        size_t len = strlen(data);
        if (len > 0)
        {
            if (strspn(data, "\r\n\t ") != len)
            {
                CXmlNode* node = (CXmlNode*)(IXmlNode*)m_nodeStack[m_nNodeStackTop].node;
                if (*node->m_content != '\0')
                {
                    node->m_content = m_stringPool.ReplaceString(node->m_content, data);
                }
                else
                {
                    node->m_content = AddString(data);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
XmlParserImp::XmlParserImp(bool bReuseStrings)
    : m_nNodeStackTop(0)
    , m_parser(NULL)
    , m_stringPool(bReuseStrings)
{
    m_nodeStack.resize(32);
    CleanStack();
}

XmlParserImp::~XmlParserImp()
{
    ParseEnd();
}

namespace
{
    void* custom_xml_malloc(size_t nSize)
    {
        return CryModuleMalloc(nSize);
    }
    void* custom_xml_realloc(void* p, size_t nSize)
    {
        return CryModuleRealloc(p, nSize);
    }
    void custom_xml_free(void* p)
    {
        CryModuleFree(p);
    }
}

//////////////////////////////////////////////////////////////////////////
void XmlParserImp::ParseBegin(bool bCleanPools)
{
    m_root = 0;
    CleanStack();

    if (bCleanPools)
    {
        m_stringPool.Clear();
    }

    XML_Memory_Handling_Suite memHandler;
    memHandler.malloc_fcn = custom_xml_malloc;
    memHandler.realloc_fcn = custom_xml_realloc;
    memHandler.free_fcn = custom_xml_free;

    m_parser = XML_ParserCreate_MM(NULL, &memHandler, NULL);

    XML_SetUserData(m_parser, this);
    XML_SetElementHandler(m_parser, startElement, endElement);
    XML_SetCharacterDataHandler(m_parser, characterData);
    XML_SetEncoding(m_parser, "utf-8");
}

//////////////////////////////////////////////////////////////////////////
void XmlParserImp::ParseEnd()
{
    if (m_parser)
    {
        XML_ParserFree(m_parser);
    }
    m_parser = 0;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef XmlParserImp::ParseBuffer(const char* buffer, size_t bufLen, XmlString& errorString, bool bCleanPools, bool bSuppressWarnings)
{
    static const char* const errorPrefix = "XML parser: ";

    XmlNodeRef root = 0;

    // Let's try to parse the buffer as binary XML
    {
        XMLBinary::XMLBinaryReader reader;
        XMLBinary::XMLBinaryReader::EResult result;
        root = reader.LoadFromBuffer(XMLBinary::XMLBinaryReader::eBufferMemoryHandling_MakeCopy, buffer, bufLen, result);
        if (root)
        {
            return root;
        }
        if (result != XMLBinary::XMLBinaryReader::eResult_NotBinXml)
        {
            const char* const str = reader.GetErrorDescription();
            errorString = str;
            if (!bSuppressWarnings)
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "%s%s (data size: %u)", errorPrefix, str, static_cast<unsigned>(bufLen));
            }
            return 0;
        }
    }

    // This is not binary XML, so let's use text XML parser.
    {
        ParseBegin(bCleanPools);
        m_stringPool.SetBlockSize(static_cast<unsigned>(bufLen) / 16);

        if (XML_Parse(m_parser, buffer, static_cast<int>(bufLen), 1))
        {
            root = m_root;
        }
        else
        {
            char str[1024];
            sprintf_s(str, "%s%s at line %d", errorPrefix, XML_ErrorString(XML_GetErrorCode(m_parser)), (int)XML_GetCurrentLineNumber(m_parser));
            errorString = str;
            if (!bSuppressWarnings)
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "%s", str);
            }
        }

        m_root = 0;
        ParseEnd();
    }

    return root;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef XmlParserImp::ParseFile(const char* filename, XmlString& errorString, bool bCleanPools)
{
    if (!filename)
    {
        return 0;
    }

    static const char* const errorPrefix = "XML reader: ";

    XmlNodeRef root = 0;

    char* pFileContents = 0;
    size_t fileSize = 0;

    char str[1024];

    AZStd::fixed_string<256> adjustedFilename;
    AZStd::fixed_string<256> pakPath;
    if (fileSize <= 0)
    {
        CCryFile xmlFile;

        if (!xmlFile.Open(filename, "rb"))
        {
            sprintf_s(str, "%sCan't open file (%s)", errorPrefix, filename);
            errorString = str;
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "%s", str);
            return 0;
        }

        fileSize = xmlFile.GetLength();
        if (fileSize <= 0)
        {
            sprintf_s(str, "%sFile is empty (%s)", errorPrefix, filename);
            errorString = str;
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "%s", str);
            return 0;
        }

        pFileContents = new char[fileSize];
        if (!pFileContents)
        {
            sprintf_s(str, "%sCan't allocate %u bytes of memory (%s)", errorPrefix, static_cast<unsigned>(fileSize), filename);
            errorString = str;
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "%s", str);
            return 0;
        }

        if (xmlFile.ReadRaw(pFileContents, fileSize) != fileSize)
        {
            delete [] pFileContents;
            sprintf_s(str, "%sCan't read file (%s)", errorPrefix, filename);
            errorString = str;
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "%s", str);
            return 0;
        }

        AZ::IO::FixedMaxPath resolvedPath(AZ::IO::PosixPathSeparator);
        auto fileIoBase = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIoBase != nullptr, "FileIOBase is expected to be initialized for CXmlNode");
        if (fileIoBase->ResolvePath(resolvedPath, xmlFile.GetFilename()))
        {
            adjustedFilename = resolvedPath.MakePreferred().Native();
        }
        if (fileIoBase->ResolvePath(resolvedPath, xmlFile.GetPakPath()))
        {
            pakPath = resolvedPath.MakePreferred().Native();
        }
    }

    XMLBinary::XMLBinaryReader reader;
    XMLBinary::XMLBinaryReader::EResult result;
    root = reader.LoadFromBuffer(XMLBinary::XMLBinaryReader::eBufferMemoryHandling_TakeOwnership, pFileContents, fileSize, result);
    if (root)
    {
        return root;
    }
    if (result != XMLBinary::XMLBinaryReader::eResult_NotBinXml)
    {
        delete [] pFileContents;
        sprintf_s(str, "%s%s (%s)", errorPrefix, reader.GetErrorDescription(), filename);
        errorString = str;
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "%s", str);
        return 0;
    }
    else
    {
        // not binary XML - refuse to load if in scripts dir and not in bin xml to help reduce hacking
        // wish we could compile the text xml parser out, but too much work to get everything moved over
        constexpr AZStd::fixed_string<32> strScripts{"Scripts/"};
        // exclude files and PAKs from Mods folder
        constexpr AZStd::fixed_string<8> modsStr{"Mods/"};
        if (_strnicmp(filename, strScripts.c_str(), strScripts.length()) == 0 &&
            _strnicmp(adjustedFilename.c_str(), modsStr.c_str(), modsStr.length()) != 0 &&
            _strnicmp(pakPath.c_str(), modsStr.c_str(), modsStr.length()) != 0)
        {
#ifdef _RELEASE
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Non binary XML found in scripts dir (%s)", filename);
#endif
        }
    }

    {
        ParseBegin(bCleanPools);
        m_stringPool.SetBlockSize(static_cast<unsigned>(fileSize / 16));

        if (XML_Parse(m_parser, pFileContents, static_cast<int>(fileSize), 1))
        {
            root = m_root;
        }
        else
        {
            sprintf_s(str, "%s%s at line %d (%s)", errorPrefix, XML_ErrorString(XML_GetErrorCode(m_parser)), (int)XML_GetCurrentLineNumber(m_parser), filename);
            errorString = str;
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "%s", str);
        }

        m_root = 0;
        ParseEnd();
    }

    delete [] pFileContents;

    return root;
}

XmlParser::XmlParser(bool bReuseStrings)
{
    m_nRefCount = 0;
    m_pImpl = new XmlParserImp(bReuseStrings);
    m_pImpl->AddRef();
}

XmlParser::~XmlParser()
{
    m_pImpl->Release();
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef XmlParser::ParseBuffer(const char* buffer, int nBufLen, bool bCleanPools, bool bSuppressWarnings)
{
    m_errorString = "";
    return m_pImpl->ParseBuffer(buffer, nBufLen, m_errorString, bCleanPools, bSuppressWarnings);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef XmlParser::ParseFile(const char* filename, bool bCleanPools)
{
    m_errorString = "";
    return m_pImpl->ParseFile(filename, m_errorString, bCleanPools);
}

//////////////////////////////////////////////////////////////////////////
//
// Implements special reusable XmlNode for XmlNode pool
//
//////////////////////////////////////////////////////////////////////////
CXmlNodeReuse::CXmlNodeReuse(const char* tag, CXmlNodePool* pPool)
    : m_pPool(pPool)
{
    SAFE_RELEASE(m_pStringPool);
    m_pStringPool = m_pPool->GetStringPool();
    m_pStringPool->AddRef();
    m_tag = m_pStringPool->AddString(tag);
}

void CXmlNodeReuse::Release()
{
    m_pPool->OnRelease(m_nRefCount, this);
    CXmlNode::Release();
}

//////////////////////////////////////////////////////////////////////////
//
// Pool of reusable XML nodes with shared string pool
//
//////////////////////////////////////////////////////////////////////////
CXmlNodePool::CXmlNodePool(unsigned int nBlockSize, bool bReuseStrings)
{
    m_pStringPool = new CXmlStringPool(bReuseStrings);
    assert(m_pStringPool != 0);

    // in order to avoid memory fragmentation
    // allocates 1Mb buffer for shared string pool
    static_cast<CXmlStringPool*>(m_pStringPool)->SetBlockSize(nBlockSize);
    m_pStringPool->AddRef();
    m_nAllocated = 0;
}

CXmlNodePool::~CXmlNodePool()
{
    while (!m_pNodePool.empty())
    {
        CXmlNodeReuse* pNode = m_pNodePool.top();
        m_pNodePool.pop();
        pNode->Release();
    }
    m_pStringPool->Release();
}

XmlNodeRef CXmlNodePool::GetXmlNode(const char* sNodeName)
{
    CXmlNodeReuse* pNode = 0;

    // NOTE: at the moment xml node pool is dedicated for statistics nodes only

    // first at all check if we have already free node
    if (!m_pNodePool.empty())
    {
        pNode = m_pNodePool.top();
        m_pNodePool.pop();

        // init it to new node name
        pNode->setTag(sNodeName);

        m_nAllocated++;
        //if (0 == m_nAllocated % 1000)
        //CryLog("[CXmlNodePool]: already reused nodes [%d]", m_nAllocated);
    }
    else
    {
        // there is no free nodes so create new one
        // later it will be reused as soon as no external references left
        pNode = new CXmlNodeReuse(sNodeName, this);
        assert(pNode != 0);

        // increase ref counter for reusing node later
        pNode->AddRef();

        m_nAllocated++;
        //if (0 == m_nAllocated % 1000)
        //CryLog("[CXmlNodePool]: already allocated nodes [%d]", m_nAllocated);
    }
    return pNode;
}

void CXmlNodePool::OnRelease(int iRefCount, void* pThis)
{
    // each reusable node call OnRelease before parent release
    // since we keep reference on xml node so when ref count equals
    // to 2 means that it is last external object releases reference
    // to reusable node and it can be save for reuse later
    if (2 == iRefCount)
    {
        CXmlNodeReuse* pNode = static_cast<CXmlNodeReuse*>(pThis);

        pNode->removeAllChilds();
        pNode->removeAllAttributes();

        m_pNodePool.push(pNode);

        // decrease totally allocated by xml node pool counter
        // when counter equals to zero it means that all external
        // objects do not have references to allocated reusable node
        // at that point it is safe to clear shared string pool
        m_nAllocated--;

        if (0 == m_nAllocated)
        {
            //CryLog("[CXmlNodePool]: clear shared string pool");
            static_cast<CXmlStringPool*>(m_pStringPool)->Clear();
        }
    }
}

#undef SCOPED_LOCALE_RESETTER


