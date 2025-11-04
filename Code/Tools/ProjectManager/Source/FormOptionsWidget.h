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
#endif

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QFrame)

namespace AzQtComponents
{
    class CheckBox;
}

namespace O3DE::ProjectManager
{
    class FormOptionsWidget : public QWidget
    {
        Q_OBJECT

    public:
        FormOptionsWidget(const QString& labelText,
                          const QStringList& options,
                          const QString& allOptionsText,
                          int optionItemSpacing = 24,
                          QWidget* parent = nullptr);

        void Clear();

        void Enable(const QString& option);

        void Enable(const QStringList& options);

        void Disable(const QString& option);

        void Disable(const QStringList& options);

        void EnableAll();

        QStringList GetOptions() const;

    private:
        int GetCheckedCount() const;
        QFrame* m_optionFrame = nullptr;
        QHash<QString, QCheckBox*> m_options;
        QCheckBox* m_allOptionsToggle = nullptr;
    };

} // namespace O3DE::ProjectManager

