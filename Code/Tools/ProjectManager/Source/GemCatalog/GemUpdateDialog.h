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
    class GemUpdateDialog
        : public QDialog
    {
        Q_OBJECT // AUTOMOC
    public :
        explicit GemUpdateDialog(const QString& gemName, bool updateAvaliable = true, QWidget* parent = nullptr);
        ~GemUpdateDialog() = default;
    };
} // namespace O3DE::ProjectManager
