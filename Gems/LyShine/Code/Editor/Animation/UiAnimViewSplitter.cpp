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

#include "UiCanvasEditor_precompiled.h"
#include "UiAnimViewSplitter.h"


// CUiAnimViewSplitter

IMPLEMENT_DYNAMIC(CUiAnimViewSplitter, CSplitterWnd)
CUiAnimViewSplitter::CUiAnimViewSplitter()
{
    m_cxSplitter = m_cySplitter = 3 + 1 + 1 - 1;
    m_cxBorderShare = m_cyBorderShare = 0;
    m_cxSplitterGap = m_cySplitterGap = 3 + 1 + 1 - 1;
    m_cxBorder = m_cyBorder = 0;
}

CUiAnimViewSplitter::~CUiAnimViewSplitter()
{
}


BEGIN_MESSAGE_MAP(CUiAnimViewSplitter, CSplitterWnd)
END_MESSAGE_MAP()



// CUiAnimViewSplitter message handlers

void CUiAnimViewSplitter::SetPane(int row, int col, CWnd* pWnd, SIZE sizeInit)
{
    assert(pWnd != NULL);

    // set the initial size for that pane
    m_pColInfo[col].nIdealSize = sizeInit.cx;
    m_pRowInfo[row].nIdealSize = sizeInit.cy;

    pWnd->ModifyStyle(0, WS_BORDER, WS_CHILD | WS_VISIBLE);
    pWnd->SetParent(this);

    CRect rect(CPoint(0, 0), sizeInit);
    pWnd->MoveWindow(0, 0, sizeInit.cx, sizeInit.cy, FALSE);
    pWnd->SetDlgCtrlID(IdFromRowCol(row, col));

    ASSERT((int)::GetDlgCtrlID(pWnd->m_hWnd) == IdFromRowCol(row, col));
}

void CUiAnimViewSplitter::OnDrawSplitter(CDC* pDC, ESplitType nType, const CRect& rectArg)
{
    // Let CSplitterWnd handle everything but the border-drawing
    //if((nType != splitBorder) || (pDC == NULL))
    {
        CSplitterWnd::OnDrawSplitter(pDC, nType, rectArg);
        return;
    }

    ASSERT_VALID(pDC);

    // Draw border
    pDC->Draw3dRect(rectArg, GetSysColor(COLOR_BTNSHADOW), GetSysColor(COLOR_BTNHIGHLIGHT));
}
