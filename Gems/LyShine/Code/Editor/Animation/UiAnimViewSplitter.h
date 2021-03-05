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
