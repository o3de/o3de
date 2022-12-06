/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <AzQtComponents/Components/StyledLineEdit.h>


#include <FormLineEditTagsWidget.h>
#include <FormLineEditWidget.h>

#include <QAbstractItemView>
#include <QCheckBox>
#include <QCompleter>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpacerItem>
#include <QStyle>
#include <QValidator>
#include <QVBoxLayout>

namespace O3DE::ProjectManager
{

    void FormLineEditTagsWidget::setupCompletionTags()
    {
        QFile completionTagFile(":/ProjectManager/text/ProjectManagerCompletionTags.txt");
        completionTagFile.open(QFile::ReadOnly);
        while(!completionTagFile.atEnd())
        {
            m_completionTags << completionTagFile.readLine().trimmed();
        }

        m_completionTags.removeDuplicates();
        m_completionTags.sort();
    }

    FormLineEditTagsWidget::FormLineEditTagsWidget(
        const QString& labelText,
        const QString& valueText,
        const QString& placeholderText,
        const QString& errorText,
        QWidget* parent)
        :FormLineEditWidget(labelText, valueText, placeholderText, errorText, parent)        
    {
        
        setupCompletionTags();
        setMouseTracking(true);

        //add auto-completion for the line edit
        m_completer = new QCompleter(m_completionTags, this);
        m_completer->setObjectName("formCompleter");
        m_completer->setCaseSensitivity(Qt::CaseInsensitive);
        m_completer->setCompletionMode(QCompleter::PopupCompletion);
        m_completer->popup()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_completer->popup()->setFixedWidth(m_lineEdit->rect().width()*3);
        m_completer->popup()->setMouseTracking(true);
        m_completer->popup()->setObjectName("formCompleterPopup");

        /* Manually setting the stylesheet of this popup widget is not desired.
         * However, the current styling rules of the Project Manager make the background color blend in
         * with surroundings, making it difficult to see. Currently, attempts at rectifying this in the main
         * stylesheet proved very difficult, so a stop-gap measure of hard-coding the stylesheet was used.
         */
        QFile popupStyleSheetFile(":/ProjectManager/style/ProjectManagerCompleterPopup.qss");
        popupStyleSheetFile.open(QFile::ReadOnly);
        QString popupStyleSheet = QLatin1String(popupStyleSheetFile.readAll());

        m_completer->popup()->setStyleSheet(popupStyleSheet);

        m_lineEdit->setCompleter(m_completer);
        connect(m_completer, QOverload<const QString&>::of(&QCompleter::highlighted), this, &FormLineEditTagsWidget::forceSetText);
        connect(m_completer, QOverload<const QString&>::of(&QCompleter::activated), this, &FormLineEditTagsWidget::forceSetText);

        //add the drop down button for completion options
        m_dropdownButton = new QPushButton(QIcon(":/CarrotArrowDown.svg"), "", this);
        m_dropdownButton->setObjectName("dropDownButton");
        m_frameLayout->addWidget(m_dropdownButton);
        connect(m_dropdownButton, &QPushButton::clicked, this, [=]([[maybe_unused]]bool ignore){ m_lineEdit->completer()->complete(QRect()); });

        //section of form for showing tags
        m_tagFrame = new QFrame(this);
        m_tagFrame->setObjectName("formTagField");
        
        QHBoxLayout* tagsLayout = new QHBoxLayout();
        tagsLayout->setSpacing(tagSpacing);
        tagsLayout->addStretch();
        
        m_tagFrame->setLayout(tagsLayout);
        m_tagFrame->setVisible(false);
        tagsLayout->setSizeConstraint(QLayout::SetNoConstraint);

        m_mainLayout->addWidget(m_tagFrame);
    }

    FormLineEditTagsWidget::FormLineEditTagsWidget(const QString& labelText, const QString& valueText, QWidget* parent)
        : FormLineEditTagsWidget(labelText, valueText, "", "", parent)
    {
    }

    void FormLineEditTagsWidget::forceSetText(const QString& text)
    {
        m_lineEdit->setText(text);
        m_lineEdit->setFocus();
    }

    void FormLineEditTagsWidget::forceSubmitCurrentText()
    {
        addToTagList(m_lineEdit->text());
        m_lineEdit->clear();
        m_lineEdit->setText("");
        refreshTagFrame();
    }

    void FormLineEditTagsWidget::clear()
    {
        m_lineEdit->clear();
        m_tags.clear();
        refreshTagFrame();
    }

    void FormLineEditTagsWidget::refreshTagFrame()
    {
        // cleanup the tag frame widget and re-add the tag list
        qDeleteAll(m_tagFrame->children());
        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setSpacing(tagSpacing);

        for (auto tag : m_tags)
        {
            QCheckBox* tagCheckbox = new QCheckBox(tag, this);
            tagCheckbox->setLayoutDirection(Qt::RightToLeft);
            // connect the checked signal to a good slot
            connect(tagCheckbox, &QCheckBox::stateChanged, this, &FormLineEditTagsWidget::processTagDelete);
            layout->addWidget(tagCheckbox);
        }

        layout->addStretch();
        layout->setSizeConstraint(QLayout::SetNoConstraint);
        m_tagFrame->setLayout(layout);

        bool makeVisible = m_tags.count() > 0;
        m_tagFrame->setVisible(makeVisible);
        refreshStyle();
    }

    void FormLineEditTagsWidget::addToTagList(const QString& text)
    {
        if((QString::compare(text,"") > 0 || QString::compare(text,"") < 0) && !m_tags.contains(text))
        {
            m_tags << text;
        }
    }

    void FormLineEditTagsWidget::processTagDelete([[maybe_unused]] int unused)
    {
        //Only a checkbox's stateChanged signal causes this function to run
        QCheckBox* checkBox = qobject_cast<QCheckBox*>(sender());

        //remove the offending string
        m_tags.removeOne(checkBox->text());

        //now reconstruct the tag list
        refreshTagFrame();
    }


    void FormLineEditTagsWidget::keyPressEvent(QKeyEvent* event)
    {
        if (event->key() == Qt::Key_Return)
        {
            m_lineEdit->setFocus();
            forceSubmitCurrentText();
        }
    }
} // namespace O3DE::ProjectManager
