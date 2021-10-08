/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <platform.h>
#include <Cry_Math.h>
#include <AzCore/IO/FileIO.h>

template <class T>
struct Color_tpl;
typedef Color_tpl<uint8> ColorB;

template <typename F>
struct Vec2_tpl;
typedef Vec2_tpl<f32>   Vec2;

template <typename F>
struct Vec3_tpl;
typedef Vec3_tpl<f32>   Vec3;

struct Vec4;

template <typename F>
struct Quat_tpl;
typedef Quat_tpl<f32>  Quat;

template <typename F>
struct Ang3_tpl;
typedef Ang3_tpl<f32>       Ang3;


#if defined(QT_VERSION)
#include <QColor>
#include <QString>
#elif defined(_AFX)
#include "Util/GuidUtil.h"
#endif

#include <AzCore/Math/Guid.h>
#include <AzCore/Math/Uuid.h>

class QColor;
class QString;

class IXMLBinarySerializer;
struct ISerialize;

/*
This is wrapper around expat library to provide DOM type of access for xml.
Do not use IXmlNode class directly instead always use XmlNodeRef wrapper that
takes care of memory management issues.

Usage Example:
-------------------------------------------------------
void testXml(bool bReuseStrings)
{
    XmlParser xml(bReuseStrings);
    XmlNodeRef root = xml.ParseFile("test.xml", true);

    if (root)
    {
        for (int i = 0; i < root->getChildCount(); ++i)
        {
            XmlNodeRef child = root->getChild(i);
            if (child->isTag("world"))
            {
                if (child->getAttr("name") == "blah")
                {
                    ....
                }
            }
        }
    }
}
*/

// Summary:
//   Special string wrapper for xml nodes.
class XmlString
    : public AZStd::string
{
public:
    XmlString() {};
    XmlString(const char* str)
        : AZStd::string(str) {};

    size_t GetAllocatedMemory() const
    {
        return sizeof(XmlString) + capacity() * sizeof(AZStd::string::value_type);
    }

    operator const char*() const
    {
        return c_str();
    }
};

// Summary:
//   XML string data.
struct IXmlStringData
{
    // <interfuscator:shuffle>
    virtual ~IXmlStringData(){}
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    virtual const char* GetString() = 0;
    virtual size_t      GetStringLength() = 0;
    // </interfuscator:shuffle>
};

class IXmlNode;

// Summary:
//   XmlNodeRef, wrapper class implementing reference counting for IXmlNode.
// See also:
//   IXmlNode
class XmlNodeRef
{
private:
    IXmlNode* p;
public:
    XmlNodeRef()
        : p(NULL) {}
    XmlNodeRef(IXmlNode* p_);
    XmlNodeRef(const XmlNodeRef& p_);

    ~XmlNodeRef();

    operator IXmlNode*() const {
        return p;
    }

    IXmlNode& operator*() const { return *p; }
    IXmlNode* operator->(void) const { return p; }

    XmlNodeRef&  operator=(IXmlNode* newp);
    XmlNodeRef&  operator=(const XmlNodeRef& newp);

    template<typename Sizer >
    void GetMemoryUsage(Sizer* pSizer) const
    {
        pSizer->AddObject(p);
    }

    //Support for range based for, and stl algorithms.
    class XmlNodeRefIterator begin();
    class XmlNodeRefIterator end();
};

// Summary:
//   IXmlNode class
// Notes:
//   Never use IXmlNode directly instead use reference counted XmlNodeRef.
// See also:
//   XmlNodeRef
class IXmlNode
{
protected:
    int m_nRefCount;

protected:
    // <interfuscator:shuffle>
    virtual void DeleteThis() = 0;
    virtual ~IXmlNode() {};
    // </interfuscator:shuffle>

public:

    // <interfuscator:shuffle>
    // Summary:
    //   Creates new XML node.
    virtual XmlNodeRef createNode(const char* tag) = 0;

    // Notes:
    // AddRef/Release need to be virtual to permit overloading from CXMLNodePool

    // Summary:
    //   Reference counting.
    virtual void AddRef() { m_nRefCount++; };
    // Notes:
    //   When ref count reach zero XML node dies.
    virtual void Release()
    {
        if (--m_nRefCount <= 0)
        {
            DeleteThis();
        }
    };
    virtual int GetRefCount() const { return m_nRefCount; };

    // Summary:
    //   Gets XML node tag.
    virtual const char* getTag() const = 0;
    // Summary:
    //   Sets XML node tag.
    virtual void    setTag(const char* tag) = 0;

    // Summary:
    //   Returns true if a given tag equal to node tag.
    virtual bool isTag(const char* tag) const = 0;

    // Summary:
    //   Gets XML Node attributes.
    virtual int getNumAttributes() const = 0;
    // Summary:
    //   Returns attribute key and value by attribute index.
    virtual bool getAttributeByIndex(int index, const char** key, const char** value) = 0;

    // Summary:
    //   Copies attributes to this node from a given node.
    virtual void copyAttributes(XmlNodeRef fromNode) = 0;

    // Summary:
    //   Gets XML Node attribute for specified key.
    // Return Value:
    //   The value of the attribute if it exists, otherwise an empty string.
    virtual const char* getAttr(const char* key) const = 0;

    // Summary:
    //  Gets XML Node attribute for specified key.
    // Return Value:
    //  True if the attribute exists, false otherwise.
    virtual bool getAttr(const char* key, const char** value) const = 0;

    // Summary:
    //  Checks if attributes with specified key exist.
    virtual bool haveAttr(const char* key) const = 0;

    // Summary:
    //   Adds new child node.
    virtual void addChild(const XmlNodeRef& node) = 0;

    // Summary:
    //   Creates new xml node and add it to childs list.
    virtual XmlNodeRef newChild(const char* tagName) = 0;

    // Summary:
    //   Removes child node.
    virtual void removeChild(const XmlNodeRef& node) = 0;

    // Summary:
    //   Inserts child node.
    virtual void insertChild(int nIndex, const XmlNodeRef& node) = 0;

    // Summary:
    //   Replaces a specified child with the passed one
    //   Not supported by all node implementations
    virtual void replaceChild(int nIndex, const XmlNodeRef& fromNode) = 0;

    // Summary:
    //   Removes all child nodes.
    virtual void removeAllChilds() = 0;

    // Summary:
    //   Gets number of child XML nodes.
    virtual int getChildCount() const = 0;

    // Summary:
    //   Gets XML Node child nodes.
    virtual XmlNodeRef getChild(int i) const = 0;

    // Summary:
    //   Finds node with specified tag.
    virtual XmlNodeRef findChild(const char* tag) const = 0;

    // Summary:
    //   Gets parent XML node.
    virtual XmlNodeRef getParent() const = 0;

    // Summary:
    //   Sets parent XML node.
    virtual void setParent(const XmlNodeRef& inRef) = 0;

    // Summary:
    //   Returns content of this node.
    virtual const char* getContent() const = 0;
    // Summary:
    //   Sets content of this node.
    virtual void setContent(const char* str) = 0;

    // Summary:
    //   Deep clone of this and all child xml nodes.
    virtual XmlNodeRef clone() = 0;

    // Summary:
    //   Returns line number for XML tag.
    virtual int getLine() const = 0;
    // Summary:
    //   Set line number in xml.
    virtual void setLine(int line) = 0;

    // Summary:
    //   Returns XML of this node and sub nodes.
    // Notes:
    //   IXmlStringData pointer must be release when string is not needed anymore.
    // See also:
    //   IXmlStringData
    virtual IXmlStringData* getXMLData(int nReserveMem = 0) const = 0;
    // Summary:
    //   Returns XML of this node and sub nodes.
    virtual XmlString getXML(int level = 0) const = 0;
    virtual bool saveToFile(const char* fileName) = 0;

    // Summary:
    //   Sets new XML Node attribute (or override attribute with same key).
    //##@{
    virtual void setAttr(const char* key, const char* value) = 0;
    virtual void setAttr(const char* key, int value) = 0;
    virtual void setAttr(const char* key, unsigned int value) = 0;
    virtual void setAttr(const char* key, int64 value) = 0;
    virtual void setAttr(const char* key, uint64 value, bool useHexFormat = true) = 0;
    virtual void setAttr(const char* key, float value) = 0;
    virtual void setAttr(const char* key, double value) = 0;
    virtual void setAttr(const char* key, const Vec2& value) = 0;
    virtual void setAttr(const char* key, const Ang3& value) = 0;
    virtual void setAttr(const char* key, const Vec3& value) = 0;
    virtual void setAttr(const char* key, const Vec4& value) = 0;
    virtual void setAttr(const char* key, const Quat& value) = 0;
#if defined(LINUX64) || defined(APPLE)
    // Compatibility functions, on Linux and Mac long int is the default int64_t
    ILINE void setAttr(const char* key, unsigned long int value, bool useHexFormat = true)
    {
        setAttr(key, (uint64)value, useHexFormat);
    }

    ILINE void setAttr(const char* key, long int value)
    {
        setAttr(key, (int64)value);
    }
#endif

    virtual void setAttr([[maybe_unused]] const char* key, [[maybe_unused]] const QColor& color)
    {
#if defined(QT_VERSION)
        setAttr(key, static_cast<unsigned long>(color.red() | (color.green() << 8) | (color.blue() << 16)));
#endif
    }
    //##@}

    // Summary:
    //   Inline Helpers.
    //##@{
    //##@}

    // Summary:
    //   Deletes attribute.
    virtual void delAttr(const char* key) = 0;
    // Summary:
    //   Removes all node attributes.
    virtual void removeAllAttributes() = 0;

    // Summary:
    //   Gets attribute value of node.
    //##@{
    virtual bool getAttr(const char* key, int& value) const = 0;
    virtual bool getAttr(const char* key, unsigned int& value) const = 0;
    virtual bool getAttr(const char* key, int64& value) const = 0;
    virtual bool getAttr(const char* key, uint64& value, bool useHexFormat = true) const = 0;
    virtual bool getAttr(const char* key, float& value) const = 0;
    virtual bool getAttr(const char* key, double& value) const = 0;
    virtual bool getAttr(const char* key, Vec2& value) const = 0;
    virtual bool getAttr(const char* key, Ang3& value) const = 0;
    virtual bool getAttr(const char* key, Vec3& value) const = 0;
    virtual bool getAttr(const char* key, Vec4& value) const = 0;
    virtual bool getAttr(const char* key, Quat& value) const = 0;
    virtual bool getAttr(const char* key, bool& value) const = 0;
    virtual bool getAttr(const char* key, XmlString& value) const = 0;
    virtual bool getAttr(const char* key, ColorB& value) const = 0;

#if defined(LINUX64) || defined(APPLE)
    // Compatibility functions, on Linux and Mac long int is the default int64_t
    ILINE bool getAttr(const char* key, unsigned long int& value, bool useHexFormat = true) const
    {
        return getAttr(key, (uint64&)value, useHexFormat);
    }

    ILINE bool getAttr(const char* key, long int& value) const
    {
        return getAttr(key, (int64&)value);
    }
#endif

    // Summary:
    //   Copies children to this node from a given node.
    //   Children are reference copied (shallow copy) and the children's parent is NOT set to this
    //   node, but left with its original parent (which is still the parent)
    virtual void shareChildren(const XmlNodeRef& fromNode) = 0;

    // Summary:
    //   Removes child node at known position.
    virtual void deleteChildAt(int nIndex) = 0;

    // Summary:
    //   Returns XML of this node and sub nodes into tmpBuffer without XML checks (much faster)
    virtual XmlString getXMLUnsafe(int level, [[maybe_unused]] char* tmpBuffer, [[maybe_unused]] uint32 sizeOfTmpBuffer) const { return getXML(level); }

    // Notes:
    //   Save in small memory chunks.
    virtual bool saveToFile(const char* fileName, size_t chunkSizeBytes, AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle) = 0;
    // </interfuscator:shuffle>

    //##@}

    // Summary:
    //   Inline Helpers.
    //##@{
#if !defined(LINUX64) && !defined(APPLE)
    bool getAttr(const char* key, long& value) const
    {
        int v;
        if (getAttr(key, v))
        {
            value = static_cast<long>(v);
            return true;
        }
        else
        {
            return false;
        }
    }
    bool getAttr(const char* key, unsigned long& value) const
    {
        int v;
        if (getAttr(key, v))
        {
            value = static_cast<unsigned long>(v);
            return true;
        }
        else
        {
            return false;
        }
    }
    void setAttr(const char* key, unsigned long value) { setAttr(key, (unsigned int)value); };
    void setAttr(const char* key, long value) { setAttr(key, (int)value); };
#endif
    bool getAttr(const char* key, unsigned short& value) const
    {
        int v;
        if (getAttr(key, v))
        {
            value = static_cast<unsigned short>(v);
            return true;
        }
        else
        {
            return false;
        }
    }
    bool getAttr(const char* key, unsigned char& value) const
    {
        int v;
        if (getAttr(key, v))
        {
            value = static_cast<unsigned char>(v);
            return true;
        }
        else
        {
            return false;
        }
    }
    bool getAttr(const char* key, short& value) const
    {
        int v;
        if (getAttr(key, v))
        {
            value = static_cast<short>(v);
            return true;
        }
        else
        {
            return false;
        }
    }
    bool getAttr(const char* key, char& value) const
    {
        int v;
        if (getAttr(key, v))
        {
            value = static_cast<char>(v);
            return true;
        }
        else
        {
            return false;
        }
    }
    //##@}

    // Summary:
    //   Gets QString attribute.
    bool getAttr([[maybe_unused]] const char* key, [[maybe_unused]] QString& value) const
    {
#if defined(QT_VERSION)
        if (!haveAttr(key))
        {
            return false;
        }
        value = getAttr(key);
        return true;
#else
        return false;
#endif
    }

    bool getAttr([[maybe_unused]] const char* key, [[maybe_unused]] QColor& color) const
    {
#if defined(QT_VERSION)
        if (!haveAttr(key))
        {
            return false;
        }
        int v;
        getAttr(key, v);
        color = QColor(v & 0xff, (v >> 8) & 0xff, (v >> 16) & 0xff);
        return true;
#else
        return false;
#endif
    }

#if defined(QT_VERSION)
    // Summary:
    //   Sets GUID attribute.
    void setAttr(const char* key, const GUID& value)
    {
        AZ::Uuid uuid;
        uuid = value;
        setAttr(key, uuid.ToString<AZStd::string>().c_str());
    };

    // Summary:
    //   Gets GUID from attribute.
    bool getAttr(const char* key, GUID& value) const
    {
        if (!haveAttr(key))
        {
            return false;
        }
        const char* guidStr = getAttr(key);
        value = AZ::Uuid(guidStr);
        if (value.Data1 == 0)
        {
            memset(&value, 0, sizeof(value));
            // If bad GUID, use old guid system.
            value.Data1 = atoi(guidStr);
        }
        return true;
    }
#elif defined(_AFX)
    // Summary:
    //   Sets GUID attribute.
    void setAttr(const char* key, REFGUID value)
    {
        const char* str = GuidUtil::ToString(value);
        setAttr(key, str);
    };

    // Summary:
    //   Gets GUID from attribute.
    bool getAttr(const char* key, GUID& value) const
    {
        if (!haveAttr(key))
        {
            return false;
        }
        const char* guidStr = getAttr(key);
        value = GuidUtil::FromString(guidStr);
        if (value.Data1 == 0)
        {
            memset(&value, 0, sizeof(value));
            // If bad GUID, use old guid system.
            value.Data1 = atoi(guidStr);
        }
        return true;
    }
#endif

    // Summary:
    //   Lets be friendly to him.
    friend class XmlNodeRef;
};

/*
// Summary:
//   Inline Implementation of XmlNodeRef
inline XmlNodeRef::XmlNodeRef(const char *tag, IXmlNode *node)
{
    if (node)
    {
        p = node->createNode(tag);
    }
    else
    {
        p = new XmlNode(tag);
    }
    p->AddRef();
}
*/

//////////////////////////////////////////////////////////////////////////
inline XmlNodeRef::XmlNodeRef(IXmlNode* p_)
    : p(p_)
{
    if (p)
    {
        p->AddRef();
    }
}

inline XmlNodeRef::XmlNodeRef(const XmlNodeRef& p_)
    : p(p_.p)
{
    if (p)
    {
        p->AddRef();
    }
}

inline XmlNodeRef::~XmlNodeRef()
{
    if (p)
    {
        p->Release();
    }
}

inline XmlNodeRef&  XmlNodeRef::operator=(IXmlNode* newp)
{
    if (newp)
    {
        newp->AddRef();
    }
    if (p)
    {
        p->Release();
    }
    p = newp;
    return *this;
}

inline XmlNodeRef&  XmlNodeRef::operator=(const XmlNodeRef& newp)
{
    if (newp.p)
    {
        newp.p->AddRef();
    }
    if (p)
    {
        p->Release();
    }
    p = newp.p;
    return *this;
}


//XmlNodeRef can be treated as a container. Iterating through it, iterates through its children
class XmlNodeRefIterator
{
public:
    XmlNodeRefIterator()
        : m_index(0)
    {
    }
    XmlNodeRefIterator(XmlNodeRef& parentNode, std::size_t index)
        : m_parentNode(parentNode)
        , m_index(index)
    {
        Update();
    }

    XmlNodeRefIterator& operator=(const XmlNodeRefIterator& other) = default;

    XmlNodeRefIterator& operator++()
    {
        ++m_index;
        Update();
        return *this;
    }
    XmlNodeRefIterator operator++(int)
    {
        XmlNodeRefIterator ret = *this;
        ++m_index;
        Update();
        return ret;
    }

    IXmlNode* operator*() const
    {
        return m_currentChildNode;
    }

    XmlNodeRefIterator& operator--()
    {
        --m_index;
        Update();
        return *this;
    }

    XmlNodeRefIterator operator--(int)
    {
        XmlNodeRefIterator ret = *this;
        --m_index;
        Update();
        return ret;
    }

    bool operator!=(const XmlNodeRefIterator& rhs) {
        return m_index != rhs.m_index;
    }

private:
    friend void swap(XmlNodeRefIterator& lhs, XmlNodeRefIterator& rhs);
    friend bool operator==(const XmlNodeRefIterator& lhs, const XmlNodeRefIterator& rhs);
    friend bool operator!=(const XmlNodeRefIterator& lhs, const XmlNodeRefIterator& rhs);

    void Update()
    {
        if (m_index < m_parentNode->getChildCount())
        {
            m_currentChildNode = m_parentNode->getChild(static_cast<int>(m_index));
        }
    }

    XmlNodeRef m_parentNode;
    XmlNodeRef m_currentChildNode;
    std::size_t m_index; //default to first child, if no children then this will equal size which is what we use for the end iterator
};

inline void swap(XmlNodeRefIterator& lhs, XmlNodeRefIterator& rhs)
{
    AZStd::swap(lhs.m_parentNode, rhs.m_parentNode);
    AZStd::swap(lhs.m_currentChildNode, rhs.m_currentChildNode);
    AZStd::swap(lhs.m_index, rhs.m_index);
}

inline bool operator==(const XmlNodeRefIterator& lhs, const XmlNodeRefIterator& rhs)
{
    return lhs.m_index == rhs.m_index;
}

inline bool operator!=(const XmlNodeRefIterator& lhs, const XmlNodeRefIterator& rhs)
{
    return lhs.m_index != rhs.m_index;
}

inline XmlNodeRefIterator XmlNodeRef::begin()
{
    return XmlNodeRefIterator(*this, 0);
}

inline XmlNodeRefIterator XmlNodeRef::end()
{
    return XmlNodeRefIterator(*this, (*this)->getChildCount());
}

//////////////////////////////////////////////////////////////////////////
struct IXmlSerializer
{
    // <interfuscator:shuffle>
    virtual ~IXmlSerializer(){}
    virtual void AddRef() = 0;
    virtual void Release() = 0;

    virtual ISerialize* GetWriter(XmlNodeRef& node) = 0;
    virtual ISerialize* GetReader(XmlNodeRef& node) = 0;
    // </interfuscator:shuffle>
};

//////////////////////////////////////////////////////////////////////////
// Summary:
//   XML Parser interface.
struct IXmlParser
{
    virtual ~IXmlParser(){}
    virtual void AddRef() = 0;
    virtual void Release() = 0;

    // Summary:
    //   Parses xml file.
    virtual XmlNodeRef ParseFile(const char* filename, bool bCleanPools) = 0;

    // Summary:
    //   Parses xml from memory buffer.
    virtual XmlNodeRef ParseBuffer(const char* buffer, int nBufLen, bool bCleanPools, bool bSuppressWarnings = false) = 0;
};

//////////////////////////////////////////////////////////////////////////
// Summary:
//   XML Table Reader interface.
//
//   Can be used to read tables exported from Excel in .xml format. Supports
//   reading CryEngine's version of those Excel .xml tables (produced by RC).
//
// Usage:
//   p->Begin(rootNode);
//   while (p->ReadRow(...))
//   {
//     while (p->ReadCell(...))
//     {
//       ...
//     }
//   }
struct IXmlTableReader
{
    // <interfuscator:shuffle>
    virtual ~IXmlTableReader(){}

    virtual void Release() = 0;

    // Returns false if XML tree is not in supported table format.
    virtual bool Begin(XmlNodeRef rootNode) = 0;

    // Returns estimated number of rows (estimated number of ReadRow() calls returning true).
    // Returned number is equal *or greater* than real number, because it's impossible to
    // know real number in advance in case of Excel XML.
    virtual int GetEstimatedRowCount() = 0;

    // Prepares next row for reading by ReadCell().
    // Returns true and sets rowIndex if the row was prepared successfully.
    // Note: empty rows are skipped sometimes, so use returned rowIndex if you need
    // to know absolute row index.
    // Returns false if no rows left.
    virtual bool ReadRow(int& rowIndex) = 0;

    // Reads next cell in the current row.
    // Returns true and sets columnIndex, pContent, contenSize if the cell was read successfully.
    // Note: empty cells are skipped sometimes, so use returned cellIndex if you need
    // to know absolute cell index (i.e. column).
    // Returns false if no cells left in the row.
    virtual bool ReadCell(int& columnIndex, const char*& pContent, size_t& contentSize) = 0;
    // </interfuscator:shuffle>
};

//////////////////////////////////////////////////////////////////////////
// Summary:
//   IXmlUtils structure.
struct IXmlUtils
{
    // <interfuscator:shuffle>
    virtual ~IXmlUtils(){}

    // Summary:
    //   Loads xml file, returns 0 if load failed.
    virtual XmlNodeRef LoadXmlFromFile(const char* sFilename, bool bReuseStrings = false) = 0;
    // Summary:
    //   Loads xml from memory buffer, returns 0 if load failed.
    virtual XmlNodeRef LoadXmlFromBuffer(const char* buffer, size_t size, bool bReuseStrings = false, bool bSuppressWarnings = false) = 0;

    // Summary:
    //   Creates XML Writer for ISerialize interface.
    // See also:
    //   IXmlSerializer
    virtual IXmlSerializer* CreateXmlSerializer() = 0;

    // Summary:
    //   Creates XML Parser.
    // Notes:
    //   WARNING!!!
    //   IXmlParser does not normally support recursive XML loading, all nodes loaded by this parser are invalidated on loading new file.
    //   This is a specialized interface for fast loading of many XMLs,
    //   After use it must be released with call to Release method.
    virtual IXmlParser* CreateXmlParser() = 0;

    // Summary:
    //   Creates XML Table reader.
    // Notes:
    //   After use it must be released with call to Release method.
    virtual IXmlTableReader* CreateXmlTableReader() = 0;
};
