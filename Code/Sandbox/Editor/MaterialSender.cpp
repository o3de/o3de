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
#include "EditorDefs.h"

#include "MaterialSender.h"

bool CMaterialSender::SendMessage(int msg, const XmlNodeRef& node)
{
    bool bRet = false;

#if defined(AZ_PLATFORM_WINDOWS)
    if (!CheckWindows())
    {
        return false;
    }

    m_h.msg = msg;

    int nDataSize = sizeof(SMaterialMapFileHeader) + strlen(node->getXML().c_str()) + 1;

    //hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, nDataSize, "EditMatMappingObject");

    HANDLE mapFileHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "EditMatMappingObject");
    if (mapFileHandle)
    {
        void* pMes = MapViewOfFile(mapFileHandle, FILE_MAP_ALL_ACCESS, 0, 0, nDataSize);
        if (pMes)
        {
            memcpy(pMes, &m_h, sizeof(SMaterialMapFileHeader));
            azstrcpy(((char*)pMes) + sizeof(SMaterialMapFileHeader), nDataSize - sizeof(SMaterialMapFileHeader), node->getXML().c_str());
            UnmapViewOfFile(pMes);
            if (m_bIsMatEditor)
            {
                ::SendMessage(m_h.GetMaxHWND(), WM_MATEDITSEND, msg, 0);
            }
            else
            {
                ::SendMessage(m_h.GetEditorHWND(), WM_MATEDITSEND, msg, 0);
            }
            bRet = true;
        }
        CloseHandle(mapFileHandle);
    }
    else
    {
        CryLog("No File Map");
    }
#endif

    return bRet;
}
