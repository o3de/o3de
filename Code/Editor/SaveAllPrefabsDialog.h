/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QDialog>
#include <QScopedPointer>

namespace Ui
{
    class SaveAllPrefabsDialog;
}


    class SaveAllPrefabsDialog : public QDialog
    {
    public:
        SaveAllPrefabsDialog(QWidget* pParent = nullptr);
        ~SaveAllPrefabsDialog();

    private:
        Ui::SaveAllPrefabsDialog* ui;
    };

