/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QLabel>
#include <QStringList>
#include <QWidget>
#endif

namespace O3DE::ProjectManager
{
    // Single tag
    class TagWidget
        : public QLabel
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit TagWidget(const QString& text, QWidget* parent = nullptr);
        ~TagWidget() = default;
    };

    // Widget containing multiple tags, automatically wrapping based on the size
    class TagContainerWidget
        : public QWidget
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit TagContainerWidget(QWidget* parent = nullptr);
        ~TagContainerWidget() = default;

        void Update(const QStringList& tags);
    };
} // namespace O3DE::ProjectManager
