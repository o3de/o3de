/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "CrySystem_precompiled.h"
#include <IXml.h>
#include "xml.h"
#include "XmlUtils.h"
#include "ReadWriteXMLSink.h"

#include "../SimpleStringPool.h"
#include "SerializeXMLReader.h"
#include "SerializeXMLWriter.h"

#include "XMLBinaryWriter.h"
#include "XMLBinaryReader.h"

#include "XMLPatcher.h"
#include <md5.h>

//////////////////////////////////////////////////////////////////////////
CXmlNode_PoolAlloc* g_pCXmlNode_PoolAlloc = 0;
#ifdef CRY_COLLECT_XML_NODE_STATS
SXmlNodeStats* g_pCXmlNode_Stats = 0;
#endif

extern bool g_bEnableBinaryXmlLoading;

//////////////////////////////////////////////////////////////////////////
CXmlUtils::CXmlUtils(ISystem* pSystem)
{
    m_pSystem = pSystem;
    m_pSystem->GetISystemEventDispatcher()->RegisterListener(this);

    // create IReadWriteXMLSink object
    m_pReadWriteXMLSink = new CReadWriteXMLSink();
    g_pCXmlNode_PoolAlloc = new CXmlNode_PoolAlloc;
#ifdef CRY_COLLECT_XML_NODE_STATS
    g_pCXmlNode_Stats = new SXmlNodeStats();
#endif
    m_pStatsXmlNodePool = 0;
#ifndef _RELEASE
    m_statsThreadOwner = CryGetCurrentThreadId();
#endif
    m_pXMLPatcher = NULL;
}

//////////////////////////////////////////////////////////////////////////
CXmlUtils::~CXmlUtils()
{
    m_pSystem->GetISystemEventDispatcher()->RemoveListener(this);
    delete g_pCXmlNode_PoolAlloc;
#ifdef CRY_COLLECT_XML_NODE_STATS
    delete g_pCXmlNode_Stats;
#endif
    SAFE_DELETE(m_pStatsXmlNodePool);
    SAFE_DELETE(m_pXMLPatcher);
}

//////////////////////////////////////////////////////////////////////////
IXmlParser* CXmlUtils::CreateXmlParser()
{
    const bool bReuseStrings = false; //TODO: do we ever want to reuse strings here?
    return new XmlParser(bReuseStrings);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CXmlUtils::LoadXmlFromFile(const char* sFilename, bool bReuseStrings, bool bEnablePatching)
{
    XmlParser parser(bReuseStrings);
    XmlNodeRef node = parser.ParseFile(sFilename, true);

    // XmlParser is supposed to log warnings and errors (if any),
    // so we don't need to call parser.getErrorString(),
    // CryLog() etc here.

    if (node && bEnablePatching && m_pXMLPatcher)
    {
        node = m_pXMLPatcher->ApplyXMLDataPatch(node, sFilename);
    }

    return node;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CXmlUtils::LoadXmlFromBuffer(const char* buffer, size_t size, bool bReuseStrings, bool bSuppressWarnings)
{
    XmlParser parser(bReuseStrings);
    XmlNodeRef node = parser.ParseBuffer(buffer, size, true, bSuppressWarnings);
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
const char* CXmlUtils::HashXml(XmlNodeRef node)
{
    static char signature[16 * 2 + 1];
    static char temp[16];
    static const char* hex = "0123456789abcdef";
    XmlString str = node->getXML();
    GetMD5(str.data(), str.length(), temp);
    for (int i = 0; i < 16; i++)
    {
        signature[2 * i + 0] = hex[((uint8)temp[i]) >> 4];
        signature[2 * i + 1] = hex[((uint8)temp[i]) & 0xf];
    }
    signature[16 * 2] = 0;
    return signature;
}

//////////////////////////////////////////////////////////////////////////
IReadWriteXMLSink* CXmlUtils::GetIReadWriteXMLSink()
{
    return m_pReadWriteXMLSink;
}

//////////////////////////////////////////////////////////////////////////
class CXmlSerializer
    : public IXmlSerializer
{
public:
    CXmlSerializer()
        : m_nRefCount(0)
        , m_pReaderImpl(NULL)
        , m_pReaderSer(NULL)
        , m_pWriterSer(NULL)
        , m_pWriterImpl(NULL)
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
    virtual void AddRef() { ++m_nRefCount; }
    virtual void Release()
    {
        if (--m_nRefCount <= 0)
        {
            delete this;
        }
    }

    virtual ISerialize* GetWriter(XmlNodeRef& node)
    {
        ClearAll();
        m_pWriterImpl = new CSerializeXMLWriterImpl(node);
        m_pWriterSer = new CSimpleSerializeWithDefaults<CSerializeXMLWriterImpl>(*m_pWriterImpl);
        return m_pWriterSer;
    }
    virtual ISerialize* GetReader(XmlNodeRef& node)
    {
        ClearAll();
        m_pReaderImpl = new CSerializeXMLReaderImpl(node);
        m_pReaderSer = new CSimpleSerializeWithDefaults<CSerializeXMLReaderImpl>(*m_pReaderImpl);
        return m_pReaderSer;
    }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->Add(*this);
        pSizer->AddObject(m_pReaderImpl);
        pSizer->AddObject(m_pWriterImpl);
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
void CXmlUtils::GetMemoryUsage(ICrySizer* pSizer)
{
    {
        SIZER_COMPONENT_NAME(pSizer, "Nodes");
        g_pCXmlNode_PoolAlloc->GetMemoryUsage(pSizer);
    }

#ifdef CRY_COLLECT_XML_NODE_STATS
    // yes, slow
    std::vector<const CXmlNode*> rootNodes;
    {
        TXmlNodeSet::const_iterator iter = g_pCXmlNode_Stats->nodeSet.begin();
        TXmlNodeSet::const_iterator iterEnd = g_pCXmlNode_Stats->nodeSet.end();
        while (iter != iterEnd)
        {
            const CXmlNode* pNode = *iter;
            if (pNode->getParent() == 0)
            {
                rootNodes.push_back(pNode);
            }
            ++iter;
        }
    }

    // use the following to log to console
#if 0
    CryLogAlways("NumXMLRootNodes=%d NumXMLNodes=%d TotalAllocs=%d TotalFrees=%d",
        rootNodes.size(), g_pCXmlNode_Stats->nodeSet.size(),
        g_pCXmlNode_Stats->nAllocs, g_pCXmlNode_Stats->nFrees);
#endif


    // use the following to debug the nodes in the system
#if 0
    {
        std::vector<const CXmlNode*>::const_iterator iter = rootNodes.begin();
        std::vector<const CXmlNode*>::const_iterator iterEnd = rootNodes.end();
        while (iter != iterEnd)
        {
            const CXmlNode* pNode = *iter;
            CryLogAlways("Node 0x%p Tag='%s'", pNode, pNode->getTag());
            ++iter;
        }
    }
#endif

    // only for debugging, add it as pseudo numbers to the CrySizer.
    // shift it by 10, so we get the actual number
    {
        SIZER_COMPONENT_NAME(pSizer, "#NumTotalNodes");
        pSizer->Add("#NumTotalNodes", g_pCXmlNode_Stats->nodeSet.size() << 10);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "#NumRootNodes");
        pSizer->Add("#NumRootNodes", rootNodes.size() << 10);
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
void CXmlUtils::OnSystemEvent(ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
    case ESYSTEM_EVENT_LEVEL_LOAD_END:
        g_pCXmlNode_PoolAlloc->FreeMemoryIfEmpty();
        break;
    }
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
    virtual void Write(const void* pData, size_t size)
    {
        if (m_fileHandle != AZ::IO::InvalidHandle)
        {
            gEnv->pCryPak->FWrite(pData, size, 1, m_fileHandle);
        }
    }
private:
    AZ::IO::HandleType m_fileHandle;
};

//////////////////////////////////////////////////////////////////////////
bool CXmlUtils::SaveBinaryXmlFile(const char* filename, XmlNodeRef root)
{
    CXmlBinaryDataWriterFile fileSink(filename);
    if (!fileSink.IsOk())
    {
        return false;
    }
    XMLBinary::CXMLBinaryWriter writer;
    string error;
    return writer.WriteNode(&fileSink, root, false, 0, error);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CXmlUtils::LoadBinaryXmlFile(const char* filename, bool bEnablePatching)
{
    XMLBinary::XMLBinaryReader reader;
    XMLBinary::XMLBinaryReader::EResult result;
    XmlNodeRef root = reader.LoadFromFile(filename, result);

    if (result == XMLBinary::XMLBinaryReader::eResult_Success && bEnablePatching == true && m_pXMLPatcher != NULL)
    {
        root = m_pXMLPatcher->ApplyXMLDataPatch(root, filename);
    }

    return root;
}

//////////////////////////////////////////////////////////////////////////
bool CXmlUtils::EnableBinaryXmlLoading(bool bEnable)
{
    bool bPrev = g_bEnableBinaryXmlLoading;
    g_bEnableBinaryXmlLoading = bEnable;
    return bPrev;
}

//////////////////////////////////////////////////////////////////////////
class CXmlTableReader
    : public IXmlTableReader
{
public:
    CXmlTableReader();
    virtual ~CXmlTableReader();

    virtual void Release();

    virtual bool Begin(XmlNodeRef rootNode);
    virtual int  GetEstimatedRowCount();
    virtual bool ReadRow(int& rowIndex);
    virtual bool ReadCell(int& columnIndex, const char*& pContent, size_t& contentSize);
    float GetCurrentRowHeight() override;

private:
    bool m_bExcel;

    XmlNodeRef m_tableNode;

    XmlNodeRef m_rowNode;

    float m_currentRowHeight;
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
    m_tableNode = 0;

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

    m_rowNode = 0;
    m_rowNodeIndex = -1;
    m_row = -1;

    return (m_tableNode != 0);
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
    m_currentRowHeight = 0.0f;
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
                m_rowNode = 0;
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
                    m_rowNode = 0;
                    return false;
                }
                m_row = index;
            }
            float height;
            if (m_rowNode->getAttr("ss:Height", height))
            {
                m_currentRowHeight = height;
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
    pContent = 0;
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

float CXmlTableReader::GetCurrentRowHeight()
{
    return m_currentRowHeight;
}
//////////////////////////////////////////////////////////////////////////
IXmlTableReader* CXmlUtils::CreateXmlTableReader()
{
    return new CXmlTableReader;
}

//////////////////////////////////////////////////////////////////////////
// Init xml stats nodes pool
void CXmlUtils::InitStatsXmlNodePool(uint32 nPoolSize)
{
    CHECK_STATS_THREAD_OWNERSHIP();
    if (0 == m_pStatsXmlNodePool)
    {
        // create special xml node pools for game statistics

        const bool bReuseStrings = true;    // TODO parameterise?
        m_pStatsXmlNodePool = new CXmlNodePool(nPoolSize, bReuseStrings);
        assert(m_pStatsXmlNodePool);
    }
    else
    {
        CryLog("[CXmlNodePool]: Xml stats nodes pool already initialized");
    }
}

//////////////////////////////////////////////////////////////////////////
// Creates new xml node for statistics.
XmlNodeRef CXmlUtils::CreateStatsXmlNode(const char* sNodeName)
{
    CHECK_STATS_THREAD_OWNERSHIP();
    if (0 == m_pStatsXmlNodePool)
    {
        CryLog("[CXmlNodePool]: Xml stats nodes pool isn't initialized. Perform default initialization.");
        InitStatsXmlNodePool();
    }
    return m_pStatsXmlNodePool->GetXmlNode(sNodeName);
}

void CXmlUtils::SetStatsOwnerThread([[maybe_unused]] threadID threadId)
{
#ifndef _RELEASE
    m_statsThreadOwner = threadId;
#endif
}

void CXmlUtils::FlushStatsXmlNodePool()
{
    CHECK_STATS_THREAD_OWNERSHIP();
    if (m_pStatsXmlNodePool)
    {
        if (m_pStatsXmlNodePool->empty())
        {
            SAFE_DELETE(m_pStatsXmlNodePool);
        }
    }
}

void CXmlUtils::SetXMLPatcher(XmlNodeRef* pPatcher)
{
    SAFE_DELETE(m_pXMLPatcher);

    if (pPatcher != NULL)
    {
        m_pXMLPatcher = new CXMLPatcher(*pPatcher);
    }
}


