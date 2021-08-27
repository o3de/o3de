/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once



// CUiAnimViewSplitter

class CUiAnimViewSplitter
    : public CSplitterWnd
{
    DECLARE_DYNAMIC(CUiAnimViewSplitter)

    virtual CWnd * GetActivePane(int* pRow = NULL, int* pCol = NULL)
    {
        return GetFocus();
    }

    void SetPane(int row, int col, CWnd* pWnd, SIZE sizeInit);
    // Ovveride this for flat look.
    void OnDrawSplitter(CDC* pDC, ESplitType nType, const CRect& rectArg);

public:
    CUiAnimViewSplitter();
    virtual ~CUiAnimViewSplitter();

protected:
    DECLARE_MESSAGE_MAP()
};
