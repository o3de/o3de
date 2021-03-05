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

#ifndef CRYINCLUDE_EDITOR_MATERIALSENDER_H
#define CRYINCLUDE_EDITOR_MATERIALSENDER_H
#pragma once

#ifndef WM_MATEDITSEND
// Value copied from Code/Tools/MaxCryExport/CryShader/MaterialSender.h
# define WM_MATEDITSEND (WM_USER + 315)
#endif

enum EMaterialSenderMessage
{
    eMSM_Create = 1,
    eMSM_GetSelectedMaterial = 2,
    eMSM_Init = 3,
};

#if defined(AZ_PLATFORM_WINDOWS)
struct SMaterialMapFileHeader
{
    // max
    void SetMaxHWND(HWND hWnd)
    {
        hwndMax = (int64)hWnd;
    }
    HWND GetMaxHWND() const
    {
        return (HWND)hwndMax;
    }
    // editor
    void SetEditorHWND(HWND hWnd)
    {
        hwndMatEdit = (int64)hWnd;
    }
    HWND GetEditorHWND() const
    {
        return (HWND)hwndMatEdit;
    }
    int64 msg;// 64bits for both 32 and 64
    int64 Reserved;// 64bits for both 32 and 64
protected:
    uint64 hwndMax;// HWND for 32 and 64 is different
    uint64 hwndMatEdit;// HWND for 32 and 64 is different
};
#endif // AZ_PLATFORM_WINDOWS


class CMaterialSender
{
public:

    CMaterialSender(bool bIsMatEditor)
        : m_bIsMatEditor(bIsMatEditor)
    {
#if defined(AZ_PLATFORM_WINDOWS)
        m_h.SetEditorHWND(0);
        m_h.SetMaxHWND(0);
        m_h.msg = 0;
        hMapFile = 0;
#endif
    }

    ~CMaterialSender()
    {
#if defined(AZ_PLATFORM_WINDOWS)
        if (hMapFile)
        {
            CloseHandle(hMapFile);
        }
        hMapFile = 0;
#endif
    }

    bool GetMessage()
    {
        LoadMapFile();
        return true;
    }

    bool CheckWindows()
    {
#if defined(AZ_PLATFORM_WINDOWS)
        if (!m_h.GetMaxHWND() || !m_h.GetEditorHWND() || !::IsWindow(m_h.GetMaxHWND()) || !::IsWindow(m_h.GetEditorHWND()))
        {
            LoadMapFile();
        }
        if (!m_h.GetMaxHWND() || !m_h.GetEditorHWND() || !::IsWindow(m_h.GetMaxHWND()) || !::IsWindow(m_h.GetEditorHWND()))
        {
            return false;
        }
#endif
        return true;
    }

    bool Create()
    {
#if defined(AZ_PLATFORM_WINDOWS)
        hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 1024 * 1024, "EditMatMappingObject");
        if (hMapFile)
        {
            return true;
        }
        CryLog("Can't create File Map");

        return false;
#else
        return true;
#endif
    }

    bool SendMessage(int msg, const XmlNodeRef& node);

    void SetupWindows(QWidget* hwndMax, QWidget* hwndMatEdit)
    {
#if defined(AZ_PLATFORM_WINDOWS)
        m_h.SetMaxHWND(reinterpret_cast<HWND>(hwndMax->winId()));
        m_h.SetEditorHWND(reinterpret_cast<HWND>(hwndMatEdit->winId()));
#endif
    }

private:

    bool LoadMapFile()
    {
#if defined(AZ_PLATFORM_WINDOWS)
        bool bRet = false;
        const HANDLE mapFileHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "EditMatMappingObject");
        if (mapFileHandle)
        {
            void* const pMes = MapViewOfFile(mapFileHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);

            if (pMes)
            {
                memcpy(&m_h, pMes, sizeof(SMaterialMapFileHeader));
                const char* const pXml = ((const char*)pMes) + sizeof(SMaterialMapFileHeader);
                m_node = XmlHelpers::LoadXmlFromBuffer(pXml, strlen(pXml));
                UnmapViewOfFile(pMes);
                bRet = true;
            }

            CloseHandle(mapFileHandle);
        }

        return bRet;
#else
        return false;
#endif
    }

public:
#if defined(AZ_PLATFORM_WINDOWS)
    SMaterialMapFileHeader m_h;
#endif
    XmlNodeRef m_node;
private:
    bool m_bIsMatEditor;
#if defined(AZ_PLATFORM_WINDOWS)
    HANDLE hMapFile;
#endif
};


#endif // CRYINCLUDE_EDITOR_MATERIALSENDER_H
