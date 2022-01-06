/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
    void GetItems(std::vector<SItem>& outItems) override;
};

#endif // CRYINCLUDE_EDITOR_SELECTSEQUENCEDIALOG_H
