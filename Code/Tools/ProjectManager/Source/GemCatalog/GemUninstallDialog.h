/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QDialog>

namespace O3DE::ProjectManager
{
    class GemUninstallDialog
        : public QDialog
    {
        Q_OBJECT
    public:
        explicit GemUninstallDialog(const QString& gemName, QWidget *parent = nullptr);
        ~GemUninstallDialog() = default;
    };
} // namespace O3DE::ProjectManager
