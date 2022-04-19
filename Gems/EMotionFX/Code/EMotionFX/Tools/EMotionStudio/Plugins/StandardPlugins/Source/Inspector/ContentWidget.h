/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

namespace EMStudio
{
    class ContentHeaderWidget;

    //! The content widget presents the object data to the user.
    //! It owns a header widget which will always be displayed above the actual data content widget.
    class ContentWidget
        : public QWidget
    {
        Q_OBJECT //AUTOMOC

    public:
        explicit ContentWidget(QWidget* parent = nullptr);
        ~ContentWidget() override = default;

        void Update(const QString& headerTitle, const QString& iconFilename, QWidget* widget);
        void Clear();

    private:
        ContentHeaderWidget* m_headerWidget = nullptr;
        QWidget* m_widget = nullptr;
        QVBoxLayout* m_layout = nullptr;
    };
} // namespace EMStudio
