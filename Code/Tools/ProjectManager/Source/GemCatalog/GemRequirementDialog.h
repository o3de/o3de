/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(GemModel)

    class GemRequirementDialog
        : public QDialog
    {
        Q_OBJECT // AUTOMOC
    public:
        explicit GemRequirementDialog(GemModel* model, QWidget *parent = nullptr);
        ~GemRequirementDialog() = default;
    };
} // namespace O3DE::ProjectManager
