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
    class FormOptionsWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        FormOptionsWidget(const QStringList& options, const QString& allOptionsText, QWidget* parent = nullptr);

        void clear();

        void enable(const QString& option);

        void enable(const QStringList& options);

        void disable(const QString& option);

        void disable(const QStringList& options);

        void enableAll();

        QStringList getOptions() const;

    private:
        QFrame* m_optionFrame = nullptr;
        QHash<QString, QCheckBox*> m_options;
        QCheckBox* m_allOptionsToggle = nullptr;

        //! Represents active options in m_options. Should never exceed the count of keys in m_options
        int m_currentlyActive = 0;
    }

} // namespace O3DE::ProjectManager

