/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/ToolButtonWithWidget.h>

class QLineEdit;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API ToolButtonLineEdit
        : public ToolButtonWithWidget
    {
        Q_OBJECT

    public:
        explicit ToolButtonLineEdit(QWidget* parent = nullptr);
        void clear();
        QString text() const;
        void setText(const QString&);
        void setPlaceholderText(const QString&);
        QLineEdit* lineEdit() const;

    private:
        QLineEdit* const m_lineEdit;
    };
} // namespace AzQtComponents

