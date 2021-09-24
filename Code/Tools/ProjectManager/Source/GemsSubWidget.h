/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <TagWidget.h>
#endif

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QLabel)

namespace O3DE::ProjectManager
{
    // Title, description and tag widget container used for the depending and conflicting gems
    class GemsSubWidget
        : public QWidget
    {
    public:
        GemsSubWidget(QWidget* parent = nullptr);
        void Update(const QString& title, const QString& text, const QStringList& gemNames);

    private:
        QLabel* m_titleLabel = nullptr;
        QLabel* m_textLabel = nullptr;
        QVBoxLayout* m_layout = nullptr;
        TagContainerWidget* m_tagWidget = nullptr;
    };
} // namespace O3DE::ProjectManager
