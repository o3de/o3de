/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>

#include <Source/Window/CreateMaterialDialog/ui_CreateMaterialDialog.h>

#include <QFileInfo>

namespace MaterialEditor
{
    //! Dialog sets up creation of a new material by prompting to select a source material type and target file
    class CreateMaterialDialog
        : public QDialog
    {
        Q_OBJECT
    public:
        CreateMaterialDialog(QWidget* parent = nullptr);
        CreateMaterialDialog(const QString& path, QWidget* parent = nullptr);
        ~CreateMaterialDialog() = default;

        QFileInfo m_materialFileInfo;
        QFileInfo m_materialTypeFileInfo;

    private:
        QScopedPointer<Ui::CreateMaterialDialog> m_ui;
        QString m_path;

        void InitMaterialTypeSelection();
        void InitMaterialFileSelection();
        void UpdateMaterialTypeSelection();
    };
} // namespace MaterialEditor
