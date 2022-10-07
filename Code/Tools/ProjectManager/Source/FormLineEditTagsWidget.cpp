/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <FormLineEditTagsWidget.h>
#include <AzQtComponents/Components/StyledLineEdit.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <QPushButton>
#include <QAbstractItemView>
#include <QSpacerItem>
#include <QCompleter>
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
        : QWidget(parent)
        
    {
        setObjectName("formLineEditWidget");

        setupCompletionTags();

        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setAlignment(Qt::AlignTop);
        {
            m_frame = new QFrame(this);
            m_frame->setObjectName("formFrame");

            // use a horizontal box layout so buttons can be added to the right of the field
            m_frameLayout = new QHBoxLayout();
            {
                QVBoxLayout* fieldLayout = new QVBoxLayout();

                QLabel* label = new QLabel(labelText, this);
                fieldLayout->addWidget(label);

                m_lineEdit = new AzQtComponents::StyledLineEdit(this);
                m_lineEdit->setFlavor(AzQtComponents::StyledLineEdit::Question);
                AzQtComponents::LineEdit::setErrorIconEnabled(m_lineEdit, false);
                m_lineEdit->setText(valueText);
                m_lineEdit->setPlaceholderText(placeholderText);

                connect(m_lineEdit, &AzQtComponents::StyledLineEdit::flavorChanged, this, &FormLineEditTagsWidget::flavorChanged);
                connect(m_lineEdit, &AzQtComponents::StyledLineEdit::onFocus, this, &FormLineEditTagsWidget::onFocus);
                connect(m_lineEdit, &AzQtComponents::StyledLineEdit::onFocusOut, this, &FormLineEditTagsWidget::onFocusOut);
                connect(m_lineEdit, &AzQtComponents::StyledLineEdit::textChanged, this, &FormLineEditTagsWidget::textChanged);

                m_lineEdit->setFrame(false);
                fieldLayout->addWidget(m_lineEdit);

                m_frameLayout->addLayout(fieldLayout);

                QWidget* emptyWidget = new QWidget(this);
                m_frameLayout->addWidget(emptyWidget);

                m_processingSpinnerMovie = new QMovie(":/in_progress.gif");
                m_processingSpinner = new QLabel(this);
                m_processingSpinner->setScaledContents(true);
                m_processingSpinner->setMaximumSize(32, 32);
                m_processingSpinner->setMovie(m_processingSpinnerMovie);
                m_frameLayout->addWidget(m_processingSpinner);

                m_validationErrorIcon = new QLabel(this);
                m_validationErrorIcon->setPixmap(QIcon(":/error.svg").pixmap(32, 32));
                m_frameLayout->addWidget(m_validationErrorIcon);

                m_validationSuccessIcon = new QLabel(this);
                m_validationSuccessIcon->setPixmap(QIcon(":/checkmark.svg").pixmap(32, 32));
                m_frameLayout->addWidget(m_validationSuccessIcon);

                QFrame* buttonFrame = new QFrame(this);
                buttonFrame->setObjectName("dropDownButtonFrame");
                QVBoxLayout* buttonLayout = new QVBoxLayout(this);
                m_dropdownButton = new QPushButton(QIcon(":/CarrotArrowDown.svg"), "", this);
                buttonLayout->addWidget(m_dropdownButton);
                buttonFrame->setLayout(buttonLayout);
                m_frameLayout->addWidget(buttonFrame);

                SetValidationState(ValidationState::NotValidating);

                m_completer = new QCompleter(m_completionTags, this);
                m_completer->setCaseSensitivity(Qt::CaseInsensitive);
                m_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
                m_completer->popup()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
                m_completer->popup()->setFixedWidth(m_lineEdit->rect().width()*3);
                m_lineEdit->setCompleter(m_completer);
                //it's possible this responds best to signals from line edit?
                connect(m_dropdownButton, &QPushButton::clicked, this, [=]([[maybe_unused]]bool ignore){ m_lineEdit->completer()->complete(QRect()); });
                connect(m_completer, QOverload<const QString&>::of(&QCompleter::highlighted), this, &FormLineEditTagsWidget::forceSetText);
                connect(m_completer, QOverload<const QString&>::of(&QCompleter::activated), this, &FormLineEditTagsWidget::forceSetText);
            }

            m_frame->setLayout(m_frameLayout);

            mainLayout->addWidget(m_frame);

            
            m_errorLabel = new QLabel(this);
            m_errorLabel->setObjectName("formErrorLabel");
            m_errorLabel->setText(errorText);
            m_errorLabel->setVisible(false);
            mainLayout->addWidget(m_errorLabel);

            m_tagFrame = new QFrame(this);
            m_tagFrame->setObjectName("formTagField");
            
            QHBoxLayout* tagsLayout = new QHBoxLayout();
            tagsLayout->setSpacing(8);
            tagsLayout->addStretch();
            
            m_tagFrame->setLayout(tagsLayout);
            m_tagFrame->setVisible(false);
            

            tagsLayout->setSizeConstraint(QLayout::SetNoConstraint);

            mainLayout->addWidget(m_tagFrame);

        }

        setLayout(mainLayout);
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

    void FormLineEditTagsWidget::setErrorLabelText(const QString& labelText)
    {
        m_errorLabel->setText(labelText);
    }

    void FormLineEditTagsWidget::setErrorLabelVisible(bool visible)
    {
        m_errorLabel->setVisible(visible);
        m_frame->setProperty("Valid", !visible);

        refreshStyle();
    }

    QLineEdit* FormLineEditTagsWidget::lineEdit() const
    {
        return m_lineEdit;
    }

    void FormLineEditTagsWidget::flavorChanged()
    {
        if (m_lineEdit->flavor() == AzQtComponents::StyledLineEdit::Flavor::Invalid)
        {
            m_frame->setProperty("Valid", false);
            m_errorLabel->setVisible(true);
        }
        else
        {
            m_frame->setProperty("Valid", true);
            m_errorLabel->setVisible(false);
        }
        refreshStyle();
    }

    void FormLineEditTagsWidget::onFocus()
    {
        m_frame->setProperty("Focus", true);
        refreshStyle();
    }

    void FormLineEditTagsWidget::onFocusOut()
    {
        m_frame->setProperty("Focus", false);
        refreshStyle();
    }

    void FormLineEditTagsWidget::refreshStyle()
    {
        // we must unpolish/polish every child after changing a property
        // or else they won't use the correct stylesheet selector
        for (auto child : findChildren<QWidget*>())
        {
            child->style()->unpolish(child);
            child->style()->polish(child);
        }
    }

    void FormLineEditTagsWidget::setText(const QString& text)
    {
        m_lineEdit->setText(text);
    }

    void FormLineEditTagsWidget::SetValidationState(ValidationState validationState)
    {
        switch (validationState)
        {
        case ValidationState::Validating:
            m_processingSpinnerMovie->start();
            m_processingSpinner->setVisible(true);
            m_validationErrorIcon->setVisible(false);
            m_validationSuccessIcon->setVisible(false);
            break;
        case ValidationState::ValidationSuccess:
            m_processingSpinnerMovie->stop();
            m_processingSpinner->setVisible(false);
            m_validationErrorIcon->setVisible(false);
            m_validationSuccessIcon->setVisible(true);
            break;
        case ValidationState::ValidationFailed:
            m_processingSpinnerMovie->stop();
            m_processingSpinner->setVisible(false);
            m_validationErrorIcon->setVisible(true);
            m_validationSuccessIcon->setVisible(false);
            break;
        case ValidationState::NotValidating:
        default:
            m_processingSpinnerMovie->stop();
            m_processingSpinner->setVisible(false);
            m_validationErrorIcon->setVisible(false);
            m_validationSuccessIcon->setVisible(false);
            break;
        }
    }

    void FormLineEditTagsWidget::mousePressEvent([[maybe_unused]] QMouseEvent* event)
    {
        m_lineEdit->setFocus();
    }

    void FormLineEditTagsWidget::keyPressEvent(QKeyEvent* event)
    {
        if (event->key() == Qt::Key_Return)
        {
            forceSubmitCurrentText();
        }
    }
} // namespace O3DE::ProjectManager
