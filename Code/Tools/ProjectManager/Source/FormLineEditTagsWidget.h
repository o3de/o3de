/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

//! @file
//! Provides a LineEdit field that is augmented to create a list of tags.
//! A new tag is created each time the user presses 'Space' or 'Return' on their keyboard when the LineEdit is not empty.
//! The list of tags are rendered as a second row just below the LineEdit field. If there are none, this row is hidden from view.

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
        Q_OBJECT
    public:

        //! The constructor first calls the parent constructor for FormLineEditWidget.
        //! Then it injects the following:
        //! 1. A Signal for m_lineEdit to process Spaces (used for detecting tag creation).
        //! 2. An auto-completer for m_lineEdit to suggest commonly used tags.
        //! 3. A dropdown button on the right side of m_lineEdit to open the auto-completer.
        //! 4. The tag frame to show all tags created by the user.
        FormLineEditTagsWidget(
            const QString& labelText,
            const QString& valueText,
            const QString& placeholderText,
            const QString& errorText,
            QWidget* parent = nullptr);
        explicit FormLineEditTagsWidget(const QString& labelText, const QString& valueText = "", QWidget* parent = nullptr);
        ~FormLineEditTagsWidget() = default;

        //! Returns the user created tags in the widget
        const QStringList getTags()
        {
            return m_tags;
        }

        void setTags(const QStringList& tagList)
        {
            m_tags = tagList;
            refreshTagFrame();
        }

        void clear();

    protected:
        //! The button that is placed on the right side of the line edit. Used to show the auto-completion menu.
        QPushButton* m_dropdownButton = nullptr;

        //! A container sub-widget which is used to house the tags that the user creates
        QFrame* m_tagFrame = nullptr; 
        

    private slots:
        //! Identifies tag that was clicked, deletes it from the tag list, and refreshes tag frame. 
        void processTagDelete(int unused);

    private:
        //! We use this to process the 'Return' key, and make sure the LineEdit creates a new tag.
        void keyPressEvent(QKeyEvent* event) override;

        //! This is used to initialize the auto-completer and pre-populate list of tags to suggest.
        void setupCompletionTags();

        //! Each time the user adds or deletes a tag, we need to reconstruct the tag frame for the current list of tags.
        //! If there are no tags, the tag frame is hidden.
        void refreshTagFrame();

        void forceSetText(const QString& text);
        void forceSubmitCurrentText();

        //! Performs validation before adding a new tag to the tag list.
        void addToTagList(const QString& text);

        QStringList m_completionTags; //!< List of tags for the auto-completer.
        QStringList m_tags; //!< list of user created tags.
        QCompleter* m_completer;
        const int tagSpacing = 8; //!< Spacing parameter in between each tag of the tag frame
    };
} // namespace O3DE::ProjectManager
