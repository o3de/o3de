/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <FormLineEditTagsWidget.h>
#include <AzQtComponents/Components/StyledLineEdit.h>
#include <FormLineEditWidget.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <QPushButton>
#include <QAbstractItemView>
#include <QSpacerItem>
#include <QCompleter>
#include <QFile>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QMovie>
#include <QFrame>
#include <QValidator>
#include <QStyle>
#include <QKeyEvent>

namespace O3DE::ProjectManager
{

    void FormLineEditTagsWidget::setupCompletionTags()
    {
        m_completionTags << "Audio" << "Animation" << "Physics" << "GameDevelopment" << "IK" << "Blender" << "Rendering" << "Terrain";
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
        //This logic will enable us to convert text into tag objects
        connect(m_lineEdit, &AzQtComponents::StyledLineEdit::textChanged, this, &FormLineEditTagsWidget::textChanged);


        //add auto-completion for the line edit
        m_completer = new QCompleter(m_completionTags, this);
        m_completer->setObjectName("formCompleter");
        m_completer->setCaseSensitivity(Qt::CaseInsensitive);
        m_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
        m_completer->popup()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_completer->popup()->setFixedWidth(m_lineEdit->rect().width()*3);
        m_completer->popup()->setMouseTracking(true);
        m_completer->popup()->setObjectName("formCompleterPopup");


        /* NOTE(tkothadev): Manually setting the stylesheet of this popup widget is not desired.
         * However, the current styling rules of the Project Manager make the background color blend in
         * with surroundings, making it difficult to see. Currently, attempts at rectifying this in the main
         * stylesheet proved very difficult, so a stop-gap measure of hard-coding the stylesheet was used.
         * I have discussed this with AMZN-alexpete.
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
        tagsLayout->setSpacing(8);
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

    void FormLineEditTagsWidget::refreshTagFrame()
    {
        // cleanup the tag frame widget and re-add the tag list
        qDeleteAll(m_tagFrame->children());
        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setSpacing(8);

        for (auto t : m_tags)
        {
            QCheckBox* tc = new QCheckBox(t, this);
            tc->setLayoutDirection(Qt::RightToLeft);
            // connect the checked signal to a good slot
            connect(tc, &QCheckBox::stateChanged, this, &FormLineEditTagsWidget::processTagDelete);
            layout->addWidget(tc);
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

    void FormLineEditTagsWidget::textChanged(const QString& text)
    {
        if(text.contains(" "))
        {
            //do tag adding
            QString interim = m_lineEdit->text();
            m_lineEdit->clear();
            for(auto str : interim.split(" "))
            {
                addToTagList(str);
            }
            refreshTagFrame();
        }
    } 

    void FormLineEditTagsWidget::processTagDelete([[maybe_unused]] int unused)
    {
        auto checkBoxes = m_tagFrame->findChildren<QCheckBox* >();
        QString string;
        bool found = false;
        for(auto c : checkBoxes)
        {
            if(c->isChecked())
            {
                string = c->text();
                found = true;
                break;
            }
        }

        if(!found)
        {
            return;
        }
        //remove the offending string
        m_tags.removeOne(string);

        //now reconstruct the tag list
        refreshTagFrame();
    }


    void FormLineEditTagsWidget::keyPressEvent(QKeyEvent* event)
    {
        if (event->key() == Qt::Key_Return)
        {
            forceSubmitCurrentText();
        }
    }
} // namespace O3DE::ProjectManager
