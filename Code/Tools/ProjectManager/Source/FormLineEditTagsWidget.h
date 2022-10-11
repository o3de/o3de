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
#include <FormLineEditWidget.h>
#endif

class QCompleter;
class QPushButton;
class QLineEdit;
class QLabel;
class QFrame;
class QKeyEvent;

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

        const QStringList getTags()
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
        const int tagSpacing = 8;
    };
} // namespace O3DE::ProjectManager
