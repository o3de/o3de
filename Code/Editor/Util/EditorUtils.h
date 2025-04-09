/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Utility classes used by Editor.


#ifndef CRYINCLUDE_EDITOR_UTIL_EDITORUTILS_H
#define CRYINCLUDE_EDITOR_UTIL_EDITORUTILS_H
#pragma once

#include <CryCommon/platform.h>
#include <IXml.h>
#include "Util/FileUtil.h"
#include <Cry_Color.h>
#include <CryCommon/ISystem.h>

#include <QColor>
#include <QDataStream>
#include <QGuiApplication>
#include <QSet>

#include <Include/SandboxAPI.h>
#include <AzCore/Debug/TraceMessageBus.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif


#ifdef LoadCursor
#undef LoadCursor
#endif

#define LINE_EPS (0.00001f)

template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];
#define arraysize(array) (sizeof(ArraySizeHelper(array)))

/// Some preprocessor utils
/// http://altdevblogaday.com/2011/07/12/abusing-the-c-preprocessor/
#define JOIN(x, y)       JOIN2(x, y)
#define JOIN2(x, y)  x##y

#define LIST_0(x)
#define LIST_1(x)       x##1
#define LIST_2(x)       LIST_1(x), x##2
#define LIST_3(x)       LIST_2(x), x##3
#define LIST_4(x)       LIST_3(x), x##4
#define LIST_5(x)       LIST_4(x), x##5
#define LIST_6(x)       LIST_5(x), x##6
#define LIST_7(x)       LIST_6(x), x##7
#define LIST_8(x)       LIST_7(x), x##8

#define LIST(cnt, x)     JOIN(LIST_, cnt)(x)

//! Checks heap for errors.
struct HeapCheck
{
    //! Runs consistency checks on the heap.
    static void Check(const char* file, int line);
};

#ifdef _DEBUG
#define HEAP_CHECK HeapCheck::Check(__FILE__, __LINE__);
#else
#define HEAP_CHECK
#endif

#define MAKE_SURE(x, action) { if (!(x)) { assert(0 && #x); action; } \
}

namespace EditorUtils
{
    // Class to create scoped variable value.
    template<typename TType>
    class TScopedVariableValue
    {
    public:
        //Relevant for containers, should not be used manually.
        TScopedVariableValue()
            : m_pVariable(nullptr)
        {
        }

        // Main constructor.
        TScopedVariableValue(TType& tVariable, const TType& tConstructValue, const TType& tDestructValue)
            : m_pVariable(&tVariable)
            , m_tConstructValue(tConstructValue)
            , m_tDestructValue(tDestructValue)
        {
            *m_pVariable = m_tConstructValue;
        }

        // Transfers ownership.
        TScopedVariableValue(TScopedVariableValue& tInput)
            : m_pVariable(tInput.m_pVariable)
            , m_tConstructValue(tInput.m_tConstructValue)
            , m_tDestructValue(tInput.m_tDestructValue)
        {
            // I'm not sure if anyone should use this but for now I'm adding one.
            tInput.m_pVariable = nullptr;
        }

        // Move constructor: needed to use CreateScopedVariable, and transfers ownership.
        TScopedVariableValue(TScopedVariableValue&& tInput)
        {
            m_pVariable = tInput.m_pVariable;
            m_tConstructValue = tInput.m_tConstructValue;
            m_tDestructValue = tInput.m_tDestructValue;
        }

        // Applies the scoping exit, if the variable is valid.
        virtual ~TScopedVariableValue()
        {
            if (m_pVariable)
            {
                *m_pVariable = m_tDestructValue;
            }
        }

        // Transfers ownership, if not self assignment.
        TScopedVariableValue& operator=(TScopedVariableValue& tInput)
        {
            // I'm not sure if this makes sense to exist... but for now I'm adding one.
            if (this != &tInput)
            {
                m_pVariable = tInput.m_pVariable;
                m_tConstructValue = tInput.m_tConstructValue;
                m_tDestructValue = tInput.m_tDestructValue;
                tInput.m_pVariable = nullptr;
            }

            return *this;
        }

    protected:
        TType* m_pVariable;
        TType m_tConstructValue;
        TType m_tDestructValue;
    };

    // Helper function to create scoped variable.
    // Ideal usage: auto tMyVariable=CreateScoped(tContainedVariable,tConstructValue,tDestructValue);
    template<typename TType>
    TScopedVariableValue<TType> CreateScopedVariableValue(TType& tVariable, const TType& tConstructValue, const TType& tDestructValue)
    {
        return TScopedVariableValue<TType>(tVariable, tConstructValue, tDestructValue);
    }

    class AzWarningAbsorber
        : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        SANDBOX_API AzWarningAbsorber(const char* window);
        SANDBOX_API ~AzWarningAbsorber();

        bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override;

        AZStd::string m_window;
    };

    namespace LevelFile
    {
        //! Retrieve old cry level file extension (With prepending '.')
        const char* GetOldCryFileExtension();
        //! Retrieve default level file extension (With prepending '.')
        const char* GetDefaultFileExtension();
    }
};

//////////////////////////////////////////////////////////////////////////
// XML Helper functions.
//////////////////////////////////////////////////////////////////////////
namespace XmlHelpers
{
    SANDBOX_API inline XmlNodeRef CreateXmlNode(const char* sTag)
    {
        return GetISystem()->CreateXmlNode(sTag);
    }

    inline bool SaveXmlNode(IFileUtil* pFileUtil, XmlNodeRef node, const char* filename)
    {
        if (!pFileUtil->OverwriteFile(filename))
        {
            return false;
        }
        return node->saveToFile(filename);
    }

    SANDBOX_API inline XmlNodeRef LoadXmlFromFile(const char* fileName)
    {
        return GetISystem()->LoadXmlFromFile(fileName);
    }

    SANDBOX_API inline XmlNodeRef LoadXmlFromBuffer(const char* buffer, size_t size, bool suppressWarnings = false)
    {
        return GetISystem()->LoadXmlFromBuffer(buffer, size, false, suppressWarnings);
    }
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////

/*!
 * StdMap Wraps std::map to provide easier to use interface.
 */
template <class Key, class Value>
struct StdMap
{
private:
    typedef std::map<Key, Value> Map;
    Map m;

public:
    typedef typename Map::iterator Iterator;
    typedef typename Map::const_iterator ConstIterator;

    void    Insert(const Key& key, const Value& value) { m[key] = value; }
    int     GetCount() const { return m.size(); };
    bool    IsEmpty() const { return m.empty(); };
    void    Clear() { m.clear(); }
    int     Erase(const Key& key) { return m.erase(key); };
    Value& operator[](const Key& key) { return m[key]; };
    bool Find(const Key& key, Value& value) const
    {
        ConstIterator it = m.find(key);
        if (it == m.end())
        {
            return false;
        }
        value = it->second;
        return true;
    }
    Iterator Find(const Key& key) { return m.find(key); }
    ConstIterator Find(const Key& key) const { return m.find(key); }

    bool FindKeyByValue(const Value& value, Key& key) const
    {
        for (ConstIterator it = m.begin(); it != m.end(); ++it)
        {
            if (it->second == value)
            {
                key = it->first;
                return true;
            }
        }
        return false;
    }

    Iterator Begin() { return m.begin(); };
    Iterator End() { return m.end(); };
    ConstIterator Begin() const { return m.begin(); };
    ConstIterator End() const { return m.end(); };

    void    GetAsVector(std::vector<Value>& array) const
    {
        array.resize(m.size());
        int i = 0;
        for (ConstIterator it = m.begin(); it != m.end(); ++it)
        {
            array[i++] = it->second;
        }
    }
};

// This function will split a string containing separated strings, into a vector of strings
// better version of TokenizeString
inline void SplitString(const QString& rSrcStr, QStringList& rDestStrings, char aSeparator = ',')
{
    int crtPos = 0, lastPos = 0;

    while (true)
    {
        crtPos = rSrcStr.indexOf(aSeparator, lastPos);

        if (-1 == crtPos)
        {
            crtPos = rSrcStr.length();

            if (crtPos != lastPos)
            {
                rDestStrings.push_back(rSrcStr.mid(lastPos, crtPos - lastPos));
            }

            break;
        }
        else
        {
            if (crtPos != lastPos)
            {
                rDestStrings.push_back(rSrcStr.mid(lastPos, crtPos - lastPos));
            }
        }

        lastPos = crtPos + 1;
    }
}

inline bool CheckVirtualKey(Qt::MouseButton button)
{
    return (qApp->property("pressedMouseButtons").toInt() & button) != 0;
}
inline bool CheckVirtualKey(Qt::Key virtualKey)
{
    return qApp->property("pressedKeys").value<QSet<int>>().contains(virtualKey);
}

class QColor;
QColor ColorLinearToGamma(ColorF col);
ColorF ColorGammaToLinear(const QColor& col);

QColor ColorToQColor(uint32 color);

class QCursor;
class QPixmap;

template<typename T>
class QVector;

/*! Collection of Utility MFC functions.
*/
struct CMFCUtils
{
    static QCursor LoadCursor(unsigned int nIDResource, int hotX = -1, int hotY = -1);
};

#ifndef _AFX
class CArchive : public QDataStream
{
public:
    enum Mode
    {
        load,
        store
    };

    CArchive(QIODevice* device, Mode mode)
        : QDataStream(device)
        , m_mode(mode)
    {
        setByteOrder(LittleEndian);
    }

    bool IsLoading() const
    {
        return m_mode == load;
    }

    bool IsStoring() const
    {
        return m_mode == store;
    }

    uint Read(void* buffer, uint size)
    {
        QDataStream::readRawData(reinterpret_cast<char*>(buffer), size);
        return size;
    }

    uint Write(void* buffer, uint size)
    {
        // There is a bug in QT with writing files larger than 32MB. It separates
        // the write into 32MB blocks, but doesn't write the last block correctly.
        // To deal with this, we'll separate into blocks here so QT doesn't have to.

        // QT bug in qfileengine_win.cpp line 434. Block size is calculated once and always
        // used as the amount of data to write, but for the last block, unless there is exactly
        // block size left to write, the actual remaining amount needs to be written, not the
        // whole block size. This will cause WriteFile() to either write garbage to the file or
        // attempt to get into memory it doesn't have access to.

        const uint blockSize = 1024 * 1024 * 32; // This is the size QT uses for blocks.
        uint totalBytesLeftToWrite = size;
        uint totalBytesWritten = 0;

        while (totalBytesLeftToWrite > 0)
        {
            uint bytesToWrite = AZ::GetMin(blockSize, totalBytesLeftToWrite);
            uint bytesWritten = QDataStream::writeRawData(reinterpret_cast<char*>(buffer) + totalBytesWritten, bytesToWrite);

            totalBytesLeftToWrite -= bytesWritten;
            totalBytesWritten += bytesWritten;

            // If something goes wrong, stop.
            if (bytesWritten != bytesToWrite)
            {
                break;
            }

        }
        return totalBytesWritten;
    }

private:
    Mode m_mode;
};

inline quint64 readStringLength(CArchive& ar, int& charSize)
{
    // This is legacy MFC converted code. It used to use AfxReadStringLength() which has a complicated
    // decoding pattern.
    // The basic algorithm is that it reads in an 8 bit int, and if the length is less than 2^8,
    // then that's the length. Next it reads in a 16 bit int, and if the length is less than 2^16,
    // then that's the length. It does the same thing for 32 bit values and finally for 64 bit values.
    // The 16 bit length also indicates whether or not it's a UCS2 / wide-char Windows string, if it's
    // 0xfffe, but that comes after the first byte marker indicating there's a 16 bit length value.
    // So, if the first 3 bytes are: 0xFF, 0xFF, 0xFE, it's a 2 byte string being read in, and the real
    // length follows those 3 bytes (which may still be an 8, 16, or 32 bit length).

    // default to one byte strings
    charSize = 1;

    quint8 len8;
    ar >> len8;
    if (len8 < 0xff)
    {
        return len8;
    }

    quint16 len16;
    ar >> len16;
    if (len16 == 0xfffe)
    {
        charSize = 2;

        ar >> len8;
        if (len8 < 0xff)
        {
            return len8;
        }

        ar >> len16;
    }

    if (len16 < 0xffff)
    {
        return len16;
    }

    quint32 len32;
    ar >> len32;

    if (len32 < 0xffffffff)
    {
        return len32;
    }

    quint64 len64;
    ar >> len64;

    return len64;
}

inline CArchive& operator>>(CArchive& ar, QString& str)
{
    int charSize = 1;
    auto length = readStringLength(ar, charSize);
    QByteArray data = ar.device()->read(length * charSize);

    if (charSize == 1)
    {
        str = QString::fromUtf8(data);
    }
    else
    {
        char* raw = data.data();

        // check if it's short aligned; if it isn't, we need to copy to a temp buffer
        if ((reinterpret_cast<uintptr_t>(raw) & 1) != 0)
        {
            ushort* shortAlignedData = new ushort[length];
            memcpy(shortAlignedData, raw, length * 2);
            str = QString::fromUtf16(shortAlignedData, aznumeric_cast<int>(length));
            delete[] shortAlignedData;
        }
        else
        {
            str = QString::fromUtf16(reinterpret_cast<ushort*>(raw), aznumeric_cast<int>(length));
        }
    }

    return ar;
}

inline CArchive& operator<<(CArchive& ar, const QString& str)
{
    // This is written to mimic how MFC archiving worked, which was to
    // write markers to indicate the size of the length -
    // so a length that will fit into 8 bits takes 8 bits.
    // A length that requires more than 8 bits, puts an 8 bit marker (0xff)
    // to indicate that the length is greater, then 16 bits for the length.
    // If the length requires 32 bits, there's an 8 bit marker (0xff), a
    // 16 bit marker (0xffff) and then the 32 bit length.
    // Note that the legacy code could also encode to 16 bit Windows wide character
    // streams; that isn't necessary though, given that Qt supports Utf-8 out of the
    // box and is much less ambiguous on other platforms.

    QByteArray data = str.toUtf8();
    int length = data.length();

    if (length < 255)
    {
        ar << static_cast<quint8>(length);
    }
    else if (length < 0xfffe) // 0xfffe instead of 0xffff because 0xfffe indicated Windows wide character strings, which we aren't bothering with anymore
    {
        ar << static_cast<quint8>(0xff);
        ar << static_cast<quint16>(length);
    }
    else
    {
        ar << static_cast<quint8>(0xff);
        ar << static_cast<quint16>(0xffff);
        ar << static_cast<quint32>(length);
    }

    ar.device()->write(data);

    return ar;
}

#endif

#endif // CRYINCLUDE_EDITOR_UTIL_EDITORUTILS_H

