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

#include "CryXML_precompiled.h"

//#define _CRT_SECURE_NO_DEPRECATE 1
//#define _CRT_NONSTDC_NO_DEPRECATE
#include <stdlib.h>

#define XML_STATIC
#include <expat.h>
#include "xml.h"
#include "../IXMLSerializer.h"
#include "Util.h"
#include <assert.h>
#include <algorithm>
#include <stdio.h>

#include <Cry_Color.h>
#include <ISystem.h>
#include <AzCore/base.h>
#include <AzFramework/Archive/IArchive.h>

/////////////////////////////////////////////////////////////////////
// String pool implementation (from expat).
/////////////////////////////////////////////////////////////////////
class CSimpleStringPool
{
public:
    enum
    {
        STD_BLOCK_SIZE = 4096
    };
    struct BLOCK
    {
        BLOCK* next;
        int size;
        char s[1];
    };
    unsigned int m_blockSize;
    BLOCK* m_blocks;
    const char* m_end;
    char* m_ptr;
    char* m_start;
    int nUsedSpace;
    int nUsedBlocks;

    CSimpleStringPool()
    {
        m_blockSize = STD_BLOCK_SIZE;
        m_blocks = 0;
        m_start = 0;
        m_ptr = 0;
        m_end = 0;
        nUsedSpace = 0;
        nUsedBlocks = 0;
    }
    ~CSimpleStringPool()
    {
        BLOCK* p = m_blocks;
        while (p)
        {
            BLOCK* temp = p->next;
            //nFree++;
            CryModuleFree(p);
            p = temp;
        }
        m_blocks = 0;
        m_ptr = 0;
        m_start = 0;
        m_end = 0;
    }
    void SetBlockSize(unsigned int nBlockSize)
    {
        if (nBlockSize > 1024 * 1024)
        {
            nBlockSize = 1024 * 1024;
        }
        unsigned int size = 512;
        while (size < nBlockSize)
        {
            size *= 2;
        }

        m_blockSize = size;
    }
    char* Append(const char* ptr, int nStrLen)
    {
        char* ret = m_ptr;
        if (m_ptr && nStrLen + 1 < (m_end - m_ptr))
        {
            memcpy(m_ptr, ptr, nStrLen);
            m_ptr = m_ptr + nStrLen;
            *m_ptr++ = 0; // add null termination.
        }
        else
        {
            int nNewBlockSize = Util::getMax(nStrLen + 1, (int)m_blockSize);
            AllocBlock(nNewBlockSize);
            memcpy(m_ptr, ptr, nStrLen);
            m_ptr = m_ptr + nStrLen;
            *m_ptr++ = 0; // add null termination.
            ret = m_start;
        }
        nUsedSpace += nStrLen;
        return ret;
    }
    char* ReplaceString(const char* str1, const char* str2)
    {
        int nStrLen1 = strlen(str1);
        int nStrLen2 = strlen(str2);

        // undo ptr1 add.
        if (m_ptr != m_start)
        {
            m_ptr = m_ptr - nStrLen1 - 1;
        }

        assert(m_ptr == str1);

        int nStrLen = nStrLen1 + nStrLen2;

        char* ret = m_ptr;
        if (m_ptr && nStrLen + 1 < (m_end - m_ptr))
        {
            memcpy(m_ptr, str1, nStrLen1);
            memcpy(m_ptr + nStrLen1, str2, nStrLen2);
            m_ptr = m_ptr + nStrLen;
            *m_ptr++ = 0; // add null termination.
        }
        else
        {
            int nNewBlockSize = Util::getMax(nStrLen + 1, (int)m_blockSize);
            if (m_ptr == m_start)
            {
                ReallocBlock(nNewBlockSize * 2); // Reallocate current block.
                memcpy(m_ptr + nStrLen1, str2, nStrLen2);
            }
            else
            {
                AllocBlock(nNewBlockSize);
                memcpy(m_ptr, str1, nStrLen1);
                memcpy(m_ptr + nStrLen1, str2, nStrLen2);
            }

            m_ptr = m_ptr + nStrLen;
            *m_ptr++ = 0; // add null termination.
            ret = m_start;
        }
        nUsedSpace += nStrLen;
        return ret;
    }
private:
    void AllocBlock(int blockSize)
    {
        //nMallocs++;
        BLOCK* pBlock = (BLOCK*)CryModuleMalloc(offsetof(BLOCK, s) + blockSize * sizeof(char));
        if (!pBlock)
        {
            // no memory.
            //CryError( "Out of memory" );
            m_ptr = 0;
            m_start = 0;
            m_end = 0;
            return;
        }
        pBlock->size = blockSize;
        pBlock->next = m_blocks;
        m_blocks = pBlock;
        m_ptr = pBlock->s;
        m_start = pBlock->s;
        m_end = pBlock->s + blockSize;
        nUsedBlocks++;
    }
    void ReallocBlock(int blockSize)
    {
        BLOCK* pThisBlock = m_blocks;
        BLOCK* pPrevBlock = m_blocks->next;
        m_blocks = pPrevBlock;
        //nMallocs++;
        BLOCK* pBlock = (BLOCK*)CryModuleRealloc(pThisBlock, offsetof(BLOCK, s) + blockSize * sizeof(char));
        if (!pBlock)
        {
            // no memory.
            //CryError( "Out of memory" );
            m_ptr = 0;
            m_start = 0;
            m_end = 0;
            return;
        }
        pBlock->size = blockSize;
        pBlock->next = m_blocks;
        m_blocks = pBlock;
        m_ptr = pBlock->s;
        m_start = pBlock->s;
        m_end = pBlock->s + blockSize;
    }
};

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

class CXmlStringPool
    : public IXmlStringPool
{
public:
    char* AddString(const char* str) { return m_stringPool.Append(str, (int)strlen(str)); }
private:
    CSimpleStringPool m_stringPool;
};

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
    // Clear parent pointer from childs.
    for (XmlNodes::const_iterator it = m_childs.begin(); it != m_childs.end(); ++it)
    {
        IXmlNode* node = *it;
        ((CXmlNode*)node)->m_parent = 0;
    }
    m_pStringPool->Release();
}

CXmlNode::CXmlNode()
{
    m_tag = "";
    m_content = "";
    m_parent = 0;
    m_nRefCount = 0;
    m_pStringPool = 0; // must be changed later.
}

CXmlNode::CXmlNode(const char* tag)
{
    m_content = "";
    m_parent = 0;
    m_nRefCount = 0;
    m_pStringPool = new CXmlStringPool;
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
    m_content = str;
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

bool CXmlNode::getAttr(const char* key, const char** value) const
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
    XmlAttrConstIter it = GetAttrConstIterator(key);
    if (it != m_attributes.end())
    {
        return true;
    }
    return false;
}

void CXmlNode::delAttr(const char* key)
{
    XmlAttrIter it = GetAttrIterator(key);
    if (it != m_attributes.end())
    {
        m_attributes.erase(it);
    }
}

void CXmlNode::removeAllAttributes()
{
    m_attributes.clear();
}

void CXmlNode::setAttr(const char* key, const char* value)
{
    XmlAttrIter it = GetAttrIterator(key);
    if (it == m_attributes.end())
    {
        XmlAttribute tempAttr;
        tempAttr.key = m_pStringPool->AddString(key);
        tempAttr.value = m_pStringPool->AddString(value);
        m_attributes.push_back(tempAttr);
        // Sort attributes.
        //std::sort( m_attributes.begin(),m_attributes.end() );
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
    azsnprintf(str, sizeof(str), "%d", value);
    setAttr(key, str);
}

void CXmlNode::setAttr(const char* key, unsigned int value)
{
    char str[128];
    azsnprintf(str, sizeof(str), "%d", value);
    setAttr(key, str);
}

void CXmlNode::setAttr(const char* key, float value)
{
    char str[128];
    azsnprintf(str, sizeof(str), "%g", value);
    setAttr(key, str);
}

void CXmlNode::setAttr(const char* key, double value)
{
    char str[128];
    azsnprintf(str, sizeof(str), "%.17g", value);
    setAttr(key, str);
}

//////////////////////////////////////////////////////////////////////////
void CXmlNode::setAttr(const char* key, int64 value)
{
    char str[32];
    azsnprintf(str, sizeof(str), "%" PRId64, value);
    setAttr(key, str);
}

//////////////////////////////////////////////////////////////////////////
void CXmlNode::setAttr(const char* key, uint64 value, bool useHexFormat)
{
    char str[32];
    if (useHexFormat)
    {
        azsnprintf(str, sizeof(str), "%" PRIX64, value);
    }
    else
    {
        azsnprintf(str, sizeof(str), "%" PRIu64, value);
    }
    setAttr(key, str);
}

void CXmlNode::setAttr(const char* key, const Ang3& value)
{
    char str[128];
    azsnprintf(str, sizeof(str), "%g,%g,%g", value.x, value.y, value.z);
    setAttr(key, str);
}

void CXmlNode::setAttr(const char* key, const Vec2& value)
{
    char str[128];
    azsnprintf(str, sizeof(str), "%g,%g", value.x, value.y);
    setAttr(key, str);
}

void CXmlNode::setAttr(const char* key, const Vec2d& value)
{
    char str[128];
    azsnprintf(str, sizeof(str), "%.17g,%.17g", value.x, value.y);
    setAttr(key, str);
}

void CXmlNode::setAttr(const char* key, const Vec3& value)
{
    char str[128];
    azsnprintf(str, sizeof(str), "%g,%g,%g", value.x, value.y, value.z);
    setAttr(key, str);
}

void CXmlNode::setAttr(const char* key, const Vec3d& value)
{
    char str[128];
    azsnprintf(str, sizeof(str), "%.17g,%.17g,%.17g", value.x, value.y, value.z);
    setAttr(key, str);
}

void CXmlNode::setAttr(const char* key, const Vec4& value)
{
    char str[128];
    azsnprintf(str, sizeof(str), "%g,%g,%g,%g", value.x, value.y, value.z, value.w);
    setAttr(key, str);
}

void CXmlNode::setAttr(const char* key, const Quat& value)
{
    char str[128];
    azsnprintf(str, sizeof(str), "%g,%g,%g,%g", value.w, value.v.x, value.v.y, value.v.z);
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
        azsscanf(svalue, "%" PRId64, &value);
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
        if (useHexFormat)
        {
            azsscanf(svalue, "%" PRIX64, &value);
        }
        else
        {
            azsscanf(svalue, "%" PRIu64, &value);
        }
        return true;
    }
    return false;
}

bool CXmlNode::getAttr(const char* key, bool& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        value = atoi(svalue) != 0;
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
        float x, y, z;
        if (azsscanf(svalue, "%f,%f,%f", &x, &y, &z) == 3)
        {
            value(x, y, z);
            return true;
        }
    }
    return false;
}

bool CXmlNode::getAttr(const char* key, Vec2& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        float x, y;
        if (azsscanf(svalue, "%f,%f", &x, &y) == 2)
        {
            value = Vec2(x, y);
            return true;
        }
    }
    return false;
}

bool CXmlNode::getAttr(const char* key, Vec2d& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        double x, y;
        if (azsscanf(svalue, "%lf,%lf", &x, &y) == 2)
        {
            value = Vec2d(x, y);
            return true;
        }
    }
    return false;
}

bool CXmlNode::getAttr(const char* key, Vec3& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        float x, y, z;
        if (azsscanf(svalue, "%f,%f,%f", &x, &y, &z) == 3)
        {
            value(x, y, z);
            return true;
        }
    }
    return false;
}

bool CXmlNode::getAttr(const char* key, Vec4& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        float x, y, z, w;
        if (azsscanf(svalue, "%f,%f,%f,%f", &x, &y, &z, &w) == 3)
        {
            value(x, y, z, w);
            return true;
        }
    }
    return false;
}

bool CXmlNode::getAttr(const char* key, Vec3d& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
        double x, y, z;
        if (azsscanf(svalue, "%lf,%lf,%lf", &x, &y, &z) == 3)
        {
            value = Vec3d(x, y, z);
            return true;
        }
    }
    return false;
}

bool CXmlNode::getAttr(const char* key, Quat& value) const
{
    const char* svalue = GetValue(key);
    if (svalue)
    {
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
                value = ColorB(r, g, b, a);
                return true;
            }
        }
    }
    return false;
}


XmlNodeRef CXmlNode::findChild(const char* tag) const
{
    for (XmlNodes::const_iterator it = m_childs.begin(); it != m_childs.end(); ++it)
    {
        if ((*it)->isTag(tag))
        {
            return *it;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CXmlNode::deleteChild(const char* tag)
{
    for (XmlNodes::iterator it = m_childs.begin(); it != m_childs.end(); ++it)
    {
        if ((*it)->isTag(tag))
        {
            m_childs.erase(it);
            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CXmlNode::deleteChildAt(int nIndex)
{
    if (nIndex >= 0 && nIndex < (int)m_childs.size())
    {
        m_childs.erase(m_childs.begin() + nIndex);
    }
}

//! Adds new child node.
void CXmlNode::addChild(const XmlNodeRef& node)
{
    assert(node != 0);
    m_childs.push_back(node);
    IXmlNode* n = node;
    ((CXmlNode*)n)->m_parent = this;
};

void CXmlNode::setParent(const XmlNodeRef& inNewParent)
{
    // note, parent ptrs are not ref counted
    IXmlNode* n = inNewParent;
    m_parent = (CXmlNode*)n;
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
            m_childs.insert(m_childs.begin() + inIndex, pNode);
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
        IXmlNode* wasChild = m_childs[inIndex];

        if (wasChild->getParent() == this)
        {
            wasChild->setParent(XmlNodeRef());          // child is orphaned, will be freed by Release() below if this parent is last holding a reference to it
        }
        wasChild->Release();
        inNewChild->AddRef();
        m_childs[inIndex] = inNewChild;
        inNewChild->setParent(this);
    }
}

XmlNodeRef CXmlNode::newChild(const char* tagName)
{
    XmlNodeRef node = createNode(tagName);
    addChild(node);
    return node;
}

void CXmlNode::removeChild(const XmlNodeRef& node)
{
    XmlNodes::iterator it = std::find(m_childs.begin(), m_childs.end(), (IXmlNode*)node);
    if (it != m_childs.end())
    {
        m_childs.erase(it);
    }
}

void CXmlNode::removeAllChilds()
{
    m_childs.clear();
}

//! Get XML Node child nodes.
XmlNodeRef CXmlNode::getChild(int i) const
{
    assert(i >= 0 && i < (int)m_childs.size());
    return m_childs[i];
}

//////////////////////////////////////////////////////////////////////////
void CXmlNode::copyAttributes(XmlNodeRef fromNode)
{
    IXmlNode* inode = fromNode;
    CXmlNode* n = (CXmlNode*)inode;
    if (n->m_pStringPool == m_pStringPool)
    {
        m_attributes = n->m_attributes;
    }
    else
    {
        m_attributes.resize(n->m_attributes.size());
        for (int i = 0; i < (int)n->m_attributes.size(); i++)
        {
            m_attributes[i].key = m_pStringPool->AddString(n->m_attributes[i].key);
            m_attributes[i].value = m_pStringPool->AddString(n->m_attributes[i].value);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CXmlNode::getAttributeByIndex(int index, const char** key, const char** value)
{
    XmlAttributes::iterator it = m_attributes.begin();
    if (it != m_attributes.end())
    {
        std::advance(it, index);
        if (it != m_attributes.end())
        {
            *key = it->key;
            *value = it->value;
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CXmlNode::clone()
{
    CXmlNode* node = new CXmlNode;
    node->m_pStringPool = m_pStringPool;
    m_pStringPool->AddRef();
    node->m_tag = m_tag;
    node->m_content = m_content;
    // Clone attributes.
    CXmlNode* n = (CXmlNode*)(IXmlNode*)node;
    n->copyAttributes(this);
    // Clone sub nodes.
    for (XmlNodes::const_iterator it = m_childs.begin(); it != m_childs.end(); ++it)
    {
        XmlNodeRef child = (*it)->clone();
        node->addChild(child);
    }

    return node;
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
    if (strcspn(str, "\"\'&><") == strlen(str))
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
    str.replace("&", "&amp;");
    str.replace("\"", "&quot;");
    str.replace("\'", "&apos;");
    str.replace("<", "&lt;");
    str.replace(">", "&gt;");

    return str;
}

//////////////////////////////////////////////////////////////////////////
void CXmlNode::AddToXmlString(XmlString& xml, int level) const
{
    AddTabsToString(xml, level);

    // Begin Tag
    if (m_attributes.empty())
    {
        xml += "<";
        xml += m_tag;
        if (*m_content == 0 && m_childs.empty())
        {
            // Compact tag form.
            xml += " />\n";
            return;
        }
        xml += ">";
    }
    else
    {
        xml += "<";
        xml += m_tag;
        xml += " ";

        // Put attributes.
        for (XmlAttributes::const_iterator it = m_attributes.begin(); it != m_attributes.end(); )
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
            it++;
            if (it != m_attributes.end())
            {
                xml += "\" ";
            }
            else
            {
                xml += "\"";
            }
        }
        if (*m_content == 0 && m_childs.empty())
        {
            // Compact tag form.
            xml += "/>\n";
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

    if (m_childs.empty())
    {
        xml += "</";
        xml += m_tag;
        xml += ">\n";
        return;
    }

    xml += "\n";

    // Add sub nodes.
    for (XmlNodes::const_iterator it = m_childs.begin(); it != m_childs.end(); ++it)
    {
        IXmlNode* node = *it;
        ((CXmlNode*)node)->AddToXmlString(xml, level + 1);
    }

    // Add tabs.
    AddTabsToString(xml, level);
    xml += "</";
    xml += m_tag;
    xml += ">\n";
}

IXmlStringData* CXmlNode::getXMLData(int nReserveMem) const
{
    CXmlStringData* pStrData = new CXmlStringData;
    pStrData->m_string.reserve(nReserveMem);
    AddToXmlString(pStrData->m_string, 0);
    return pStrData;
}

XmlString CXmlNode::getXML(int level) const
{
    static XmlString xml;
    xml = "";
    xml.reserve(6000000);

    AddToXmlString(xml, level);
    return xml;
}

bool CXmlNode::saveToFile(const char* fileName)
{
    XmlString xml = getXML();
    FILE* file = nullptr;
    azfopen(&file, fileName, "wt");
    if (file)
    {
        const char* sxml = (const char*)xml;
        fprintf(file, "%s", sxml);
        fclose(file);
        return true;
    }
    return false;
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
    explicit XmlParserImp(bool bRemoveNonessentialSpacesFromContent);
    ~XmlParserImp();
    void beginParse();
    bool parse(const char* buffer, int bufLen);
    XmlNodeRef endParse(XmlString& errorString);

    // Add new string to pool.
    char* AddString(const char* str) { return m_stringPool.Append(str, (int)strlen(str)); }
    //char* AddString( const char *str ) { return (char*)str; }

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
        char str[500000];
        if (len > sizeof(str) - 1)
        {
            assert(0);
            len = sizeof(str) - 1;
        }
        memcpy(str, s, len);
        str[len] = 0;
        ((XmlParserImp*)userData)->onRawData(str);
    }

    // First node will become root node.
    std::vector<XmlNodeRef> nodeStack;
    XmlNodeRef m_root;

    XML_Parser m_parser;
    CSimpleStringPool m_stringPool;
    bool m_bRemoveNonessentialSpacesFromContent;
};

/**
******************************************************************************
* XmlParserImp
******************************************************************************
*/
void    XmlParserImp::onStartElement(const char* tagName, const char** atts)
{
    XmlNodeRef parent;
    CXmlNode* pCNode = new CXmlNode;
    pCNode->m_pStringPool = this;
    pCNode->m_pStringPool->AddRef();
    pCNode->m_tag = AddString(tagName);

    XmlNodeRef node = pCNode;

    if (!nodeStack.empty())
    {
        parent = nodeStack.back();
    }
    else
    {
        m_root = node;
    }
    nodeStack.push_back(node);

    if (parent)
    {
        parent->addChild(node);
    }

    uint64 line = XML_GetCurrentLineNumber((XML_Parser)m_parser);
    node->setLine(line > INT_MAX ? INT_MAX : (int)line);

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
        pCNode->m_attributes.resize(numAttrs);
        int nAttr = 0;
        while (atts[i] != 0)
        {
            pCNode->m_attributes[nAttr].key = AddString(atts[i]);
            pCNode->m_attributes[nAttr].value = AddString(atts[i + 1]);
            nAttr++;
            i += 2;
        }
        // Sort attributes.
        //std::sort( pCNode->m_attributes.begin(),pCNode->m_attributes.end() );
    }
}

void    XmlParserImp::onEndElement([[maybe_unused]] const char* tagName)
{
    assert(!nodeStack.empty());
    if (!nodeStack.empty())
    {
        nodeStack.pop_back();
    }
}

void    XmlParserImp::onRawData(const char* const data)
{
    if (data && data[0])
    {
        CXmlNode* const node = (CXmlNode*)(IXmlNode*)nodeStack.back();

        if (!m_bRemoveNonessentialSpacesFromContent)
        {
            // Implementation note: Skipping spaces in beginning (even although
            // m_bRemoveNonessentialSpacesFromContent is false) allows us
            // to avoid having lot of "space only" content nodes
            if (node->m_content.empty())
            {
                const size_t len = strlen(data);
                const size_t spaceCount = strspn(data, "\r\n\t ");

                if (spaceCount < len)
                {
                    node->m_content += &data[spaceCount];
                }
            }
            else
            {
                node->m_content += data;
            }
        }
        else
        {
            const size_t len = strlen(data);
            const size_t spaceCount = strspn(data, "\r\n\t ");

            if ((spaceCount > 0) && (!node->m_content.empty()))
            {
                node->m_content += " ";
            }

            if (spaceCount < len)
            {
                node->m_content += &data[spaceCount];
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
static void* custom_xml_malloc(size_t nSize)
{
    return CryModuleMalloc(nSize);
}
static void* custom_xml_realloc(void* p, size_t nSize)
{
    return CryModuleRealloc(p, nSize);
}
static void custom_xml_free(void* p)
{
    CryModuleFree(p);
}

namespace CryXML_Internal
{
    XML_Memory_Handling_Suite memHandler;
    XML_Memory_Handling_Suite* GetMemoryHandler()
    {
        memHandler.malloc_fcn = custom_xml_malloc; // CryModuleMalloc;
        memHandler.realloc_fcn = custom_xml_realloc; // CryModuleRealloc;
        memHandler.free_fcn = custom_xml_free;  // CryModuleFree;
        return &memHandler;
    }
}

XmlParserImp::XmlParserImp(bool bRemoveNonessentialSpacesFromContent)
{
    m_bRemoveNonessentialSpacesFromContent = bRemoveNonessentialSpacesFromContent;

    m_root = 0;
    nodeStack.reserve(100);
        
    m_parser = XML_ParserCreate_MM(NULL, CryXML_Internal::GetMemoryHandler(), NULL);

    XML_SetUserData(m_parser, this);
    XML_SetElementHandler(m_parser, startElement, endElement);
    XML_SetCharacterDataHandler(m_parser, characterData);
    XML_SetEncoding(m_parser, "utf-8");
}

XmlParserImp::~XmlParserImp()
{
    XML_ParserFree(m_parser);
}

void XmlParserImp::beginParse()
{
    m_root = 0;

    m_stringPool.SetBlockSize(1 << 20);
}

bool XmlParserImp::parse(const char* buffer, int bufLen)
{
    if (!XML_Parse(m_parser, buffer, (int)bufLen, 0))
    {
        m_root = 0;
        return false;
    }
    return true;
}

XmlNodeRef XmlParserImp::endParse(XmlString& errorString)
{
    errorString = "";

    if (!XML_Parse(m_parser, "", 0, 1))
    {
        m_root = 0;
    }

    if (!m_root)
    {
        const char* const errorText = XML_ErrorString(XML_GetErrorCode(m_parser));
        if (errorText)
        {
            errorString += "XML Error: ";
            errorString += errorText;
            // The following code is disabled by 'if (false)' because XML_GetCurrentLineNumber()
            // XML_GetCurrentColumnNumber() return incorrect numbers.
            // The issue (wrong numbers) might be fixed if/when we upgrade to a newer version
            // of the Expat XML library (on 2014/02/26 CryEngine still uses expat version 1.95.2
            // from 2001/07/27, although the latest expat version is 2.1.0 from 2012/03/24).
            if (false)
            {
                char s[64];
                azsprintf(s, " at line %d, column %d", (int)XML_GetCurrentLineNumber(m_parser), (int)XML_GetCurrentColumnNumber(m_parser));
                errorString += s;
            }
        }
    }

    XmlNodeRef root = m_root;
    m_root = 0;
    return root;
}

XmlParser::XmlParser(bool bRemoveNonessentialSpacesFromContent)
{
    m_pImpl = new XmlParserImp(bRemoveNonessentialSpacesFromContent);
    m_pImpl->AddRef();
}

XmlParser::~XmlParser()
{
    m_pImpl->Release();
}

//! Parse xml file.
XmlNodeRef XmlParser::parse(const char* fileName)
{
    m_errorString = "";

    std::vector<char> buf;
    auto pPak = GetISystem()->GetIPak();
    AZ::IO::HandleType file = pPak->FOpen(fileName, "rb");
    if (file)
    {
        pPak->FSeek(file, 0, SEEK_END);
        int fileSize = pPak->FTell(file);
        pPak->FSeek(file, 0, SEEK_SET);
        buf.resize(fileSize);
        pPak->FRead(&(buf[0]), fileSize, file);
        pPak->FClose(file);
        m_pImpl->parse(&buf[0], buf.size());
        return m_pImpl->endParse(m_errorString);
    }
    else
    {
        return XmlNodeRef();
    }
}

//! Parse xml from memory buffer.
XmlNodeRef XmlParser::parseBuffer(const char* buffer)
{
    m_errorString = "";
    m_pImpl->beginParse();
    m_pImpl->parse(buffer, strlen(buffer));
    return m_pImpl->endParse(m_errorString);
}

XmlNodeRef XmlParser::parseSource(const IXmlBufferSource* source)
{
    m_errorString = "";
    char buffer[40000];
    enum
    {
        bufferSize = sizeof(buffer) / sizeof(buffer[0])
    };
    m_pImpl->beginParse();
    int bytesRead = source->Read(buffer, bufferSize);
    while (bytesRead)
    {
        if (!m_pImpl->parse(buffer, bytesRead))
        {
            break;
        }
        bytesRead = source->Read(buffer, bufferSize);
    }
    return m_pImpl->endParse(m_errorString);
}
