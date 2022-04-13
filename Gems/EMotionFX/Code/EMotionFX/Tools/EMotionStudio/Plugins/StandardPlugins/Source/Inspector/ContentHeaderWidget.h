/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QHash>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QLabel)

namespace EMStudio
{
    //! Header widget used to identify the shown object in the content widget.
    //! The header widget will be shown in case one or multiple objects are selected.
    class ContentHeaderWidget
        : public QWidget
    {
        Q_OBJECT //AUTOMOC

    public:
        explicit ContentHeaderWidget(QWidget* parent = nullptr);
        ~ContentHeaderWidget() override = default;

        void Update(const QString& title, const QString& iconFilename);

    private:
        const QPixmap& FindOrCreateIcon(const QString& iconFilename);

        QHash<QString, QPixmap> m_cachedIcons;
        QLabel* m_iconLabel = nullptr;
        static constexpr int s_iconSize = 32;

        QLabel* m_titleLabel = nullptr;
    };
} // namespace EMStudio
