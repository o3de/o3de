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

// Description : Cross platform DXGL helper types

#ifndef __GLCROSSPLATFORM__
#define __GLCROSSPLATFORM__

namespace NCryOpenGL
{
    void* CreateTLS();
    void SetTLSValue(void* pTLSHandle, void* pValue);
    void* GetTLSValue(void* pTLSHandle);
    void DestroyTLS(void* pTLSHandle);

    namespace NCrossPlatformImpl
    {
        struct SAutoLog
        {
            FILE* m_pFile;

            SAutoLog(const char* szFileName)
            {
                m_pFile = fopen("DXGL.log", "w");
            }

            ~SAutoLog()
            {
                if (m_pFile != NULL)
                {
                    fclose(m_pFile);
                }
            }
        };

        struct SAutoTLSSlot
        {
            void* m_pTLSHandle;

            SAutoTLSSlot()
            {
                m_pTLSHandle = CreateTLS();
            }

            ~SAutoTLSSlot()
            {
                DestroyTLS(m_pTLSHandle);
            }
        };

        inline uint32 CRC32Reflect(unsigned int ref, char ch)
        {
            unsigned int value = 0;

            // Swap bit 0 for bit 7
            // bit 1 for bit 6, etc.
            for (int i = 1; i < (ch + 1); i++)
            {
                if (ref & 1)
                {
                    value |= 1 << (ch - i);
                }
                ref >>= 1;
            }
            return value;
        }

        extern SAutoLog g_kLog;
        extern SAutoTLSSlot g_kCRCTable;
    }

#define PRINTF_PARAMS(a, b)

    void LogMessage(ELogSeverity eSeverity, const char* szFormat, ...) PRINTF_PARAMS(2, 3);
    inline void LogMessage(ELogSeverity eSeverity, const char* szFormat, ...)
    {
        if (NCrossPlatformImpl::g_kLog.m_pFile != NULL)
        {
            va_list kArgs;
            va_start(kArgs, szFormat);
            vfprintf(NCrossPlatformImpl::g_kLog.m_pFile, szFormat, kArgs);
            va_end(kArgs);
            fputc('\n', NCrossPlatformImpl::g_kLog.m_pFile);
            fflush(NCrossPlatformImpl::g_kLog.m_pFile);
        }
    }

    inline uint32 GetCRC32(const char* pData, size_t uSize, uint32 uCRC)
    {
        uint32* pCRCTable(static_cast<uint32*>(GetTLSValue(NCrossPlatformImpl::g_kCRCTable.m_pTLSHandle)));
        if (pCRCTable == NULL)
        {
            pCRCTable = new uint32[256];
            SetTLSValue(NCrossPlatformImpl::g_kCRCTable.m_pTLSHandle, pCRCTable);

            // This is the official polynomial used by CRC-32
            // in PKZip, WinZip and Ethernet.
            unsigned int ulPolynomial = 0x04c11db7;

            // 256 values representing ASCII character codes.
            for (int i = 0; i <= 0xFF; i++)
            {
                pCRCTable[i] = NCrossPlatformImpl::CRC32Reflect(i, 8) << 24;
                for (int j = 0; j < 8; j++)
                {
                    pCRCTable[i] = (pCRCTable[i] << 1) ^ (pCRCTable[i] & (1U << 31) ? ulPolynomial : 0);
                }
                pCRCTable[i] = NCrossPlatformImpl::CRC32Reflect(pCRCTable[i], 32);
            }
        }

        int len;
        unsigned char* buffer;

        // Get the length.
        len = uSize;

        // Save the text in the buffer.
        buffer = (unsigned char*)pData;
        // Perform the algorithm on each character in the string, using the lookup table values.

        while (len--)
        {
            uCRC = (uCRC >> 8) ^ pCRCTable[(uCRC & 0xFF) ^ *buffer++];
        }
        // Exclusive OR the result with the beginning value.
        return uCRC ^ 0xffffffff;
    }

    inline void* cryMemcpy(void* pDst, const void* pSrc, size_t uLength)
    {
        return memcpy(pDst, pSrc, uLength);
    }

    using std::string;

    struct STraceFile
    {
        STraceFile()
            : m_pFile(NULL)
        {
        }

        ~STraceFile()
        {
            if (m_pFile != NULL)
            {
                fclose(m_pFile);
            }
        }

        bool Open(const char* szFileName, bool bBinary)
        {
            if (m_pFile != NULL)
            {
                return false;
            }

            const char* szMode(bBinary ? "wb" : "w");
            m_pFile = fopen(szFileName, szMode);
            if (m_pFile != NULL)
            {
                return true;
            }

            return m_pFile != NULL;
        }

        void Write(const void* pvData, uint32 uSize)
        {
            fwrite(pvData, (size_t)uSize, 1, m_pFile);
        }

        void Printf(const char* szFormat, ...)
        {
            va_list kArgs;
            va_start(kArgs, szFormat);
            vfprintf(m_pFile, szFormat, kArgs);
            va_end(kArgs);
        }

        FILE* m_pFile;
    };
}

#endif //__GLCROSSPLATFORM__
