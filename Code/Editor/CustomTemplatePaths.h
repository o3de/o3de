/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QDialog>


QT_BEGIN_NAMESPACE
namespace Ui { class CustomTemplatePaths; }
QT_END_NAMESPACE

class CustomTemplatePaths : public QDialog
{
    Q_OBJECT

public:
    explicit CustomTemplatePaths(QWidget* parent = nullptr);

    ~CustomTemplatePaths() override;

private:
    Ui::CustomTemplatePaths* ui;

    void LoadSettings() const;
    void SaveSettings() const;

    QString lastSelectedFolderPath;
};