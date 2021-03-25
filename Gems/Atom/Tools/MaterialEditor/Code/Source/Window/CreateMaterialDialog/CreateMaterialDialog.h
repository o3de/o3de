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

#pragma once

#include <AzCore/std/containers/vector.h>

#include <Window/CreateMaterialDialog/ui_CreateMaterialDialog.h>

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
        ~CreateMaterialDialog() = default;

        QFileInfo m_materialFileInfo;
        QFileInfo m_materialTypeFileInfo;

    private:
        QScopedPointer<Ui::CreateMaterialDialog> m_ui;
        void InitMaterialTypeSelection();
        void InitMaterialFileSelection();
    };
} // namespace MaterialEditor
