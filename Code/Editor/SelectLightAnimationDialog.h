/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Used in a property item to select a light animation


#ifndef CRYINCLUDE_EDITOR_SELECTLIGHTANIMATIONDIALOG_H
#define CRYINCLUDE_EDITOR_SELECTLIGHTANIMATIONDIALOG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "GenericSelectItemDialog.h"
#endif

class CSelectLightAnimationDialog
    : public CGenericSelectItemDialog
{
    Q_OBJECT

public:
    CSelectLightAnimationDialog(QWidget* pParent = nullptr);

protected:
    void OnInitDialog() override;

    virtual void GetItems(std::vector<SItem>& outItems) override;
};

#endif // CRYINCLUDE_EDITOR_SELECTLIGHTANIMATIONDIALOG_H
