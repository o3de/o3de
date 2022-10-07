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
#include <FormLineEditTagsWidget.h>
#endif

QT_FORWARD_DECLARE_CLASS(QCompleter)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QKeyEvent)

namespace AzQtComponents
{
    class StyledLineEdit;
}

namespace O3DE::ProjectManager
{
    class FormLineEditTagsWidget
        : public FormLineEditWidget 
    {
     
    public:

        FormLineEditTagsWidget(
            const QString& labelText,
            const QString& valueText,
            const QString& placeholderText,
            const QString& errorText,
            QWidget* parent = nullptr);
        explicit FormLineEditTagsWidget(const QString& labelText, const QString& valueText = "", QWidget* parent = nullptr);
        ~FormLineEditTagsWidget() = default;

        QStringList getTags()
        {
            return m_tags;
        }

    protected:
        QPushButton* m_dropdownButton = nullptr;
        QFrame* m_tagFrame = nullptr;
        

    private slots:
        void textChanged(const QString& text);
        void processTagDelete(int unused);

    private:
        void keyPressEvent(QKeyEvent* event) override;
        void setupCompletionTags();
        void refreshTagFrame();
        void forceSetText(const QString& text);
        void forceSubmitCurrentText();
        void addToTagList(const QString& text);

        QStringList m_completionTags;
        QStringList m_tags;
        QCompleter* m_completer;

    };
} // namespace O3DE::ProjectManager
