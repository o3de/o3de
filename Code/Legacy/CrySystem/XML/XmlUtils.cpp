/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "CrySystem_precompiled.h"
#include <IXml.h>
#include "xml.h"
#include "XmlUtils.h"

#include "../SimpleStringPool.h"
#include "SerializeXMLReader.h"
#include "SerializeXMLWriter.h"

#include "XMLBinaryWriter.h"
#include "XMLBinaryReader.h"

#include <md5.h>

//////////////////////////////////////////////////////////////////////////
#ifdef CRY_COLLECT_XML_NODE_STATS
SXmlNodeStats* g_pCXmlNode_Stats = 0;
#endif

//////////////////////////////////////////////////////////////////////////
CXmlUtils::CXmlUtils(ISystem* pSystem)
{
    m_pSystem = pSystem;

#ifdef CRY_COLLECT_XML_NODE_STATS
    g_pCXmlNode_Stats = new SXmlNodeStats();
#endif
}

//////////////////////////////////////////////////////////////////////////
CXmlUtils::~CXmlUtils()
{
#ifdef CRY_COLLECT_XML_NODE_STATS
    delete g_pCXmlNode_Stats;
#endif
}

//////////////////////////////////////////////////////////////////////////
IXmlParser* CXmlUtils::CreateXmlParser()
{
    const bool bReuseStrings = false; //TODO: do we ever want to reuse strings here?
    return new XmlParser(bReuseStrings);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CXmlUtils::LoadXmlFromFile(const char* sFilename, bool bReuseStrings)
{
    // XmlParser is supposed to log warnings and errors (if any),
    // so we don't need to call parser.getErrorString(),
    // CryLog() etc here.
    XmlParser parser(bReuseStrings);
    return parser.ParseFile(sFilename, true);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CXmlUtils::LoadXmlFromBuffer(const char* buffer, size_t size, bool bReuseStrings, bool bSuppressWarnings)
{
    XmlParser parser(bReuseStrings);
    XmlNodeRef node = parser.ParseBuffer(buffer, static_cast<int>(size), true, bSuppressWarnings);
    return node;
}


void GetMD5(const char* pSrcBuffer, int nSrcSize, char signatureMD5[16])
{
    MD5Context md5c;
    MD5Init(&md5c);
    MD5Update(&md5c, (unsigned char*)pSrcBuffer, nSrcSize);
    MD5Final((unsigned char*)signatureMD5, &md5c);
}

//////////////////////////////////////////////////////////////////////////
class CXmlSerializer
    : public IXmlSerializer
{
public:
    CXmlSerializer()
        : m_nRefCount(0)
        , m_pReaderImpl(nullptr)
        , m_pReaderSer(nullptr)
        , m_pWriterSer(nullptr)
        , m_pWriterImpl(nullptr)
    {
    }
    ~CXmlSerializer()
    {
        ClearAll();
    }
    void ClearAll()
    {
        SAFE_DELETE(m_pReaderSer);
        SAFE_DELETE(m_pReaderImpl);
        SAFE_DELETE(m_pWriterSer);
        SAFE_DELETE(m_pWriterImpl);
    }

    //////////////////////////////////////////////////////////////////////////
    void AddRef() override { ++m_nRefCount; }
    void Release() override
    {
        if (--m_nRefCount <= 0)
        {
            delete this;
        }
    }

    ISerialize* GetWriter(XmlNodeRef& node) override
    {
        ClearAll();
        m_pWriterImpl = new CSerializeXMLWriterImpl(node);
        m_pWriterSer = new CSimpleSerializeWithDefaults<CSerializeXMLWriterImpl>(*m_pWriterImpl);
        return m_pWriterSer;
    }
    ISerialize* GetReader(XmlNodeRef& node) override
    {
        ClearAll();
        m_pReaderImpl = new CSerializeXMLReaderImpl(node);
        m_pReaderSer = new CSimpleSerializeWithDefaults<CSerializeXMLReaderImpl>(*m_pReaderImpl);
        return m_pReaderSer;
    }

    //////////////////////////////////////////////////////////////////////////
private:
    int m_nRefCount;
    CSerializeXMLReaderImpl* m_pReaderImpl;
    CSimpleSerializeWithDefaults<CSerializeXMLReaderImpl>* m_pReaderSer;

    CSerializeXMLWriterImpl* m_pWriterImpl;
    CSimpleSerializeWithDefaults<CSerializeXMLWriterImpl>* m_pWriterSer;
};

//////////////////////////////////////////////////////////////////////////
IXmlSerializer* CXmlUtils::CreateXmlSerializer()
{
    return new CXmlSerializer;
}

//////////////////////////////////////////////////////////////////////////
class CXmlBinaryDataWriterFile
    : public XMLBinary::IDataWriter
{
public:
    CXmlBinaryDataWriterFile(const char* file)
    {
        m_fileHandle = gEnv->pCryPak->FOpen(file, "wb");
    }
    ~CXmlBinaryDataWriterFile()
    {
        if (m_fileHandle != AZ::IO::InvalidHandle)
        {
            gEnv->pCryPak->FClose(m_fileHandle);
        }
    };
    virtual bool IsOk()
    {
        return m_fileHandle != AZ::IO::InvalidHandle;
    }
    ;
    void Write(const void* pData, size_t size) override
    {
        if (m_fileHandle != AZ::IO::InvalidHandle)
        {
            gEnv->pCryPak->FWrite(pData, size, m_fileHandle);
        }
    }
private:
    AZ::IO::HandleType m_fileHandle;
};

//////////////////////////////////////////////////////////////////////////
class CXmlTableReader
    : public IXmlTableReader
{
public:
    CXmlTableReader();
    ~CXmlTableReader() override;

    void Release() override;

    bool Begin(XmlNodeRef rootNode) override;
    int  GetEstimatedRowCount() override;
    bool ReadRow(int& rowIndex) override;
    bool ReadCell(int& columnIndex, const char*& pContent, size_t& contentSize) override;

private:
    bool m_bExcel;

    XmlNodeRef m_tableNode;

    XmlNodeRef m_rowNode;

    int m_rowNodeIndex;
    int m_row;

    int m_columnNodeIndex;  // used if m_bExcel == true
    int m_column;

    size_t m_rowTextSize;   // used if m_bExcel == false
    size_t m_rowTextPos;    // used if m_bExcel == false
};


//////////////////////////////////////////////////////////////////////////
CXmlTableReader::CXmlTableReader()
{
}

//////////////////////////////////////////////////////////////////////////
CXmlTableReader::~CXmlTableReader()
{
}

//////////////////////////////////////////////////////////////////////////
void CXmlTableReader::Release()
{
    delete this;
}

//////////////////////////////////////////////////////////////////////////
bool CXmlTableReader::Begin(XmlNodeRef rootNode)
{
    m_tableNode = nullptr;

    if (!rootNode)
    {
        return false;
    }

    XmlNodeRef worksheetNode = rootNode->findChild("Worksheet");
    if (worksheetNode)
    {
        m_bExcel = true;
        m_tableNode = worksheetNode->findChild("Table");
    }
    else
    {
        m_bExcel = false;
        m_tableNode = rootNode->findChild("Table");
    }

    m_rowNode = nullptr;
    m_rowNodeIndex = -1;
    m_row = -1;

    return (m_tableNode != nullptr);
}

//////////////////////////////////////////////////////////////////////////
int CXmlTableReader::GetEstimatedRowCount()
{
    if (!m_tableNode)
    {
        return -1;
    }
    return m_tableNode->getChildCount();
}

//////////////////////////////////////////////////////////////////////////
bool CXmlTableReader::ReadRow(int& rowIndex)
{
    if (!m_tableNode)
    {
        return false;
    }

    m_columnNodeIndex = -1;
    m_column = -1;

    const int rowNodeCount = m_tableNode->getChildCount();

    if (m_bExcel)
    {
        for (;; )
        {
            if (++m_rowNodeIndex >= rowNodeCount)
            {
                m_rowNodeIndex = rowNodeCount;
                return false;
            }

            m_rowNode = m_tableNode->getChild(m_rowNodeIndex);
            if (!m_rowNode)
            {
                m_rowNodeIndex = rowNodeCount;
                return false;
            }

            if (!m_rowNode->isTag("Row"))
            {
                m_rowNode = nullptr;
                continue;
            }

            ++m_row;

            int index = 0;
            if (m_rowNode->getAttr("ss:Index", index))
            {
                --index;  // one-based -> zero-based
                if (index < m_row)
                {
                    m_rowNodeIndex = rowNodeCount;
                    m_rowNode = nullptr;
                    return false;
                }
                m_row = index;
            }
            rowIndex = m_row;
            return true;
        }
    }

    {
        m_rowTextSize = 0;
        m_rowTextPos = 0;

        if (++m_rowNodeIndex >= rowNodeCount)
        {
            m_rowNodeIndex = rowNodeCount;
            return false;
        }

        m_rowNode = m_tableNode->getChild(m_rowNodeIndex);
        if (!m_rowNode)
        {
            m_rowNodeIndex = rowNodeCount;
            return false;
        }

        const char* const pContent = m_rowNode->getContent();
        if (pContent)
        {
            m_rowTextSize = strlen(pContent);
        }

        m_row = m_rowNodeIndex;
        rowIndex = m_rowNodeIndex;
        return true;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CXmlTableReader::ReadCell(int& columnIndex, const char*& pContent, size_t& contentSize)
{
    pContent = nullptr;
    contentSize = 0;

    if (!m_tableNode)
    {
        return false;
    }

    if (!m_rowNode)
    {
        return false;
    }

    if (m_bExcel)
    {
        const int columnNodeCount = m_rowNode->getChildCount();

        for (;; )
        {
            if (++m_columnNodeIndex >= columnNodeCount)
            {
                m_columnNodeIndex = columnNodeCount;
                return false;
            }

            XmlNodeRef columnNode = m_rowNode->getChild(m_columnNodeIndex);
            if (!columnNode)
            {
                m_columnNodeIndex = columnNodeCount;
                return false;
            }

            if (!columnNode->isTag("Cell"))
            {
                continue;
            }

            ++m_column;

            int index = 0;
            if (columnNode->getAttr("ss:Index", index))
            {
                --index;  // one-based -> zero-based
                if (index < m_column)
                {
                    m_columnNodeIndex = columnNodeCount;
                    return false;
                }
                m_column = index;
            }
            columnIndex = m_column;

            XmlNodeRef dataNode = columnNode->findChild("Data");
            if (dataNode)
            {
                pContent = dataNode->getContent();
                if (pContent)
                {
                    contentSize = strlen(pContent);
                }
            }
            return true;
        }
    }

    {
        if (m_rowTextPos >= m_rowTextSize)
        {
            return false;
        }

        const char* const pRowContent = m_rowNode->getContent();
        if (!pRowContent)
        {
            m_rowTextPos = m_rowTextSize;
            return false;
        }
        pContent = &pRowContent[m_rowTextPos];

        columnIndex = ++m_column;

        for (;; )
        {
            char c = pRowContent[m_rowTextPos++];

            if ((c == '\n') || (c == '\0'))
            {
                return true;
            }

            if (c == '\r')
            {
                // ignore all '\r' chars
                for (;; )
                {
                    c = pRowContent[m_rowTextPos++];
                    if ((c == '\n') || (c == '\0'))
                    {
                        return true;
                    }
                    if (c != '\r')
                    {
                        // broken data. '\r' expected to be followed by '\n' or '\0'.
                        contentSize = 0;
                        m_rowTextPos = m_rowTextSize;
                        return false;
                    }
                }
            }

            ++contentSize;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
IXmlTableReader* CXmlUtils::CreateXmlTableReader()
{
    return new CXmlTableReader;
}
