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

#ifndef CRYINCLUDE_EDITOR_SELECTSEQUENCEDIALOG_H
#define CRYINCLUDE_EDITOR_SELECTSEQUENCEDIALOG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "GenericSelectItemDialog.h"
#endif

// CSelectSequence dialog

class CSelectSequenceDialog
    : public CGenericSelectItemDialog
{
    Q_OBJECT

public:
    CSelectSequenceDialog(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CSelectSequenceDialog() {}

protected:
    void OnInitDialog() override;

    // Derived Dialogs should override this
    virtual void GetItems(std::vector<SItem>& outItems);
};

#endif // CRYINCLUDE_EDITOR_SELECTSEQUENCEDIALOG_H
