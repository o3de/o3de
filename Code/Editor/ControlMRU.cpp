/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"
#include "ControlMRU.h"

IMPLEMENT_XTP_CONTROL(CControlMRU, CXTPControlRecentFileList)

bool CControlMRU::DoesFileExist(CString& sFileName)
{
    return (_access(sFileName.GetBuffer(), 0) == 0);
}

void CControlMRU::OnCalcDynamicSize(DWORD dwMode)
{
    CRecentFileList* pRecentFileList = GetRecentFileList();

    if (!pRecentFileList)
    {
        return;
    }

    CString* pArrNames = pRecentFileList->m_arrNames;

    assert(pArrNames != NULL);
    if (!pArrNames)
    {
        return;
    }

    while (m_nIndex + 1 < m_pControls->GetCount())
    {
        CXTPControl* pControl = m_pControls->GetAt(m_nIndex + 1);
        assert(pControl);
        if (pControl->GetID() >= GetFirstMruID()
            && pControl->GetID() <= GetFirstMruID() + pRecentFileList->m_nSize)
        {
            m_pControls->Remove(pControl);
        }
        else
        {
            break;
        }
    }

    if (m_pParent->IsCustomizeMode())
    {
        m_dwHideFlags = 0;
        SetEnabled(TRUE);
        return;
    }

    if (pArrNames[0].IsEmpty())
    {
        SetCaption(CString(MAKEINTRESOURCE(IDS_NORECENTFILE_CAPTION)));
        SetDescription("No recently opened files");
        m_dwHideFlags = 0;
        SetEnabled(FALSE);

        return;
    }
    else
    {
        SetCaption(CString(MAKEINTRESOURCE(IDS_RECENTFILE_CAPTION)));
        SetDescription("Open this document");
    }

    m_dwHideFlags |= xtpHideGeneric;

    CString sCurDir = (Path::GetEditingGameDataFolder() + "\\").c_str();
    int nCurDir = sCurDir.GetLength();

    CString strName;
    CString strTemp;
    int iLastValidMRU = 0;

    for (int iMRU = 0; iMRU < pRecentFileList->m_nSize; iMRU++)
    {
        if (!pRecentFileList->GetDisplayName(strName, iMRU, sCurDir.GetBuffer(), nCurDir))
        {
            break;
        }

        if (DoesFileExist(pArrNames[iMRU]))
        {
            CString sCurEntryDir = pArrNames[iMRU].Left(nCurDir);

            if (sCurEntryDir.CompareNoCase(sCurDir) != 0)
            {
                //unavailable entry (wrong directory)
                continue;
            }
        }
        else
        {
            //invalid entry (not existing)
            continue;
        }

        int nId = iMRU + GetFirstMruID();

        CXTPControl* pControl = m_pControls->Add(xtpControlButton, nId, _T(""), m_nIndex + iLastValidMRU + 1, TRUE);
        assert(pControl);

        pControl->SetCaption(CXTPControlWindowList::ConstructCaption(strName, iLastValidMRU + 1));
        pControl->SetFlags(xtpFlagManualUpdate);
        pControl->SetBeginGroup(iLastValidMRU == 0 && m_nIndex != 0);
        pControl->SetParameter(pArrNames[iMRU]);

        CString sDescription = "Open file:  " + pArrNames[iMRU];
        pControl->SetDescription(sDescription);

        if ((GetFlags() & xtpFlagWrapRow) && iMRU == 0)
        {
            pControl->SetFlags(pControl->GetFlags() | xtpFlagWrapRow);
        }

        ++iLastValidMRU;
    }

    //if no entry was valid, treat as none would exist
    if (iLastValidMRU == 0)
    {
        SetCaption(CString(MAKEINTRESOURCE(IDS_NORECENTFILE_CAPTION)));
        SetDescription("No recently opened files");
        m_dwHideFlags = 0;
        SetEnabled(FALSE);
    }
}
