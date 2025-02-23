/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
//AZTF-SHARED

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include "AzToolsFramework/UI/DocumentPropertyEditor/ui_KeyQueryDPE.h"
#include <QDialog>

namespace AzToolsFramework
{
    class AZTF_API KeyQueryDPE
        : public QDialog
        , Ui::KeyQueryDPE
    {
    public:
        KeyQueryDPE(AZ::DocumentPropertyEditor::DocumentAdapterPtr keyQueryAdapter, QWidget* parentWidget = nullptr);
    };
} // namespace AzToolsFramework
