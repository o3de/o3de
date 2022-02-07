/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <QtWidgets/QBoxLayout>
AZ_POP_DISABLE_WARNING
#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QComboBox>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'QFileInfo::d_ptr': class 'QSharedDataPointer<QFileInfoPrivate>' needs to have dll-interface to be used by clients of class 'QFileInfo'
#include <QtWidgets/QFileDialog>
AZ_POP_DISABLE_WARNING
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QWidgetAction>
AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option") // 4144: conversion from 'int' to 'float', possible loss of data
                                                               // 4251: 'QInputEvent::modState': class 'QFlags<Qt::KeyboardModifier>' needs to have dll-interface to be used by clients of class 'QInputEvent'
#include <QMouseEvent>
AZ_POP_DISABLE_WARNING

#include <AzCore/IO/SystemFile.h>
#include <AzCore/XML/rapidxml.h>
#include <AzCore/XML/rapidxml_print.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/SearchWidget/SearchCriteriaWidget.hxx>

//  XML file format strings
#define ROOT_TAG        "SearchFilter"
#define OPERATOR_TAG    "Operator"
#define CRITERIA_TAG    "Criteria"

namespace AzToolsFramework
{
    //////////////////////////////////////////////////////////////////////////
    //  SearchCriteriaButton
    //////////////////////////////////////////////////////////////////////////
    SearchCriteriaButton::SearchCriteriaButton(QString tagText, QString labelText, QWidget* pParent)
        : QFrame(pParent)
        , m_tagText(tagText.toLower())
        , m_criteriaText(labelText)
        , m_filterEnabled(true)
        , m_mouseHover(false)
    {
        m_baseStyleSheet = styleSheet() + "padding: 0ex; border-radius: 2px;";
        setToolTip("Filter by " + m_tagText + " for \"" + m_criteriaText + "\"");
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMinimumSize(60, 24);

        QHBoxLayout* frameLayout = new QHBoxLayout(this);
        frameLayout->setMargin(0);
        frameLayout->setSpacing(4);
        frameLayout->setContentsMargins(4, 1, 4, 1);

        m_tagLabel = new QLabel(this);
        m_tagLabel->setStyleSheet(m_tagLabel->styleSheet() + tr("border: 0px; background-color: transparent;"));
        m_tagLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_tagLabel->setMinimumSize(24, 22);
        m_tagLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        QIcon closeIcon("Icons/animation/close.png");

        QPushButton* button = new QPushButton(this);
        button->setStyleSheet(button->styleSheet() + "border: 0px;");
        button->setFlat(true);
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->setFixedSize(QSize(16, 16));
        button->setProperty("iconButton", "true");
        button->setMouseTracking(true);
        button->setIcon(closeIcon);

        frameLayout->addWidget(m_tagLabel);
        frameLayout->addWidget(button);

        setLayout(frameLayout);
        setMouseTracking(true);

        SetFilterEnabled(true);

        connect(button, &QPushButton::clicked, this, &SearchCriteriaButton::OnCloseClicked);
    }

    void SearchCriteriaButton::OnCloseClicked()
    {
        Q_EMIT CloseRequested(this);
    }

    void SearchCriteriaButton::mouseReleaseEvent(QMouseEvent* event)
    {
        SetFilterEnabled(!m_filterEnabled);
        Q_EMIT RequestUpdate();
        event->accept();
    }


    void SearchCriteriaButton::enterEvent(QEvent* event)
    {
        (void)event;
        m_mouseHover = true;
        SetFilterEnabled(m_filterEnabled);
    }

    void SearchCriteriaButton::leaveEvent(QEvent* event)
    {
        (void)event;
        m_mouseHover = false;
        SetFilterEnabled(m_filterEnabled);
    }

    QString SearchCriteriaButton::Criteria()
    {
        if (m_tagText.isEmpty())
        {
            return m_criteriaText;
        }
        else
        {
            return m_tagText + ": " + m_criteriaText;
        }
    }

    void SearchCriteriaButton::SplitTagAndText(const QString& fullText, QString& tagText, QString& criteriaText)
    {
        //  Only the first colon is important, so use indexOf instead of split.
        int index = fullText.indexOf(tr(":"));
        if (index < 0)
        {
            tagText = QString();
            criteriaText = fullText.trimmed();
        }
        else
        {
            tagText = fullText.left(index).trimmed();
            criteriaText = fullText.mid(index + 1).trimmed();
        }
    }

    void SearchCriteriaButton::SetFilterEnabled(bool enabled)
    {
        m_filterEnabled = enabled;
        QString borderColor = "#808080";
        QString bgColor = "#404040";
        if (!m_filterEnabled)
        {
            borderColor = "#666666";
        }
        if (m_mouseHover)
        {
            bgColor = "#505050";
        }

        QString labelText;
        if (m_tagText.isEmpty())
        {
            if (m_filterEnabled)
            {
                labelText = m_criteriaText;
            }
            else
            {
                labelText = "<font color=#666666>" + m_criteriaText + "</font>";
            }
        }
        else
        {
            if (m_filterEnabled)
            {
                labelText = "<font color=#D9822E>" + m_tagText + ":</font> " + m_criteriaText;
            }
            else
            {
                labelText = "<font color=#666666>" + m_tagText + ": " + m_criteriaText + "</font>";
            }
        }

        m_tagLabel->setText(labelText);
        setStyleSheet(m_baseStyleSheet + "border: 1px solid " + borderColor + "; background-color: " + bgColor + ";");
    }

    //////////////////////////////////////////////////////////////////////////
    //  SearchCriteriaWidget
    //////////////////////////////////////////////////////////////////////////

    SearchCriteriaWidget::SearchCriteriaWidget(QWidget* pParent)
        : QWidget(pParent)
        , m_criteriaOperator(FilterOperatorType::Or)
        , m_suppressCriteriaChanged(false)
    {
        m_mainLayout = new QVBoxLayout(nullptr);
        m_mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        QHBoxLayout* secondaryLayout = new QHBoxLayout(nullptr);
        secondaryLayout->setSizeConstraint(QLayout::SetMinimumSize);
        secondaryLayout->setContentsMargins(0, 0, 0, 0);
        m_filterLayout = new QHBoxLayout(nullptr);
        m_tagLayout = new FlowLayout(nullptr);
        m_tagLayout->setAlignment(Qt::AlignLeft);

        QHBoxLayout* filterTextLayout = new QHBoxLayout(nullptr);
        filterTextLayout->setSizeConstraint(QLayout::SetMinimumSize);
        filterTextLayout->setContentsMargins(0, 0, 0, 0);
        filterTextLayout->setSpacing(0);

        m_filterText = new QLineEdit(this);
        m_filterText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_filterText->setFixedHeight(24);
        m_filterText->setPlaceholderText(tr("Add a search filter..."));

        m_defaultTagMenuButton = new QPushButton(this);
        m_defaultTagMenuButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_defaultTagMenuButton->setFixedSize(QSize(24, 24));
        m_defaultTagMenuButton->setFlat(true);
        m_defaultTagMenuButton->setContentsMargins(0, 0, 0, 0);

        m_defaultTagMenuButton->setToolTip(tr("Set the default filter tag (for when one isn't explicitly specified)."));

        m_availableTagMenu = new QMenu(tr("Filter by:"), this);
        m_availableTagMenu->installEventFilter(this);
        m_defaultTagMenuButton->setMenu(m_availableTagMenu);

        QPushButton* loadButton = new QPushButton(this);
        loadButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        loadButton->setFixedSize(QSize(24, 24));
        loadButton->setIcon(QIcon("UI/Icons/toolbar/libraryLoad.png"));
        loadButton->setToolTip(tr("Load a saved filter"));
        loadButton->setProperty("iconButton", "true");

        m_saveButton = new QPushButton(this);
        m_saveButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_saveButton->setFixedSize(QSize(24, 24));
        m_saveButton->setIcon(QIcon("UI/Icons/toolbar/librarySave.png"));
        m_saveButton->setToolTip(tr("Save the current filter"));
        m_saveButton->setEnabled(false);
        m_saveButton->setProperty("iconButton", "true");

        m_toggleAllFiltersButton = new QPushButton(this);
        m_toggleAllFiltersButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_toggleAllFiltersButton->setFixedSize(QSize(24, 24));
        m_toggleAllFiltersButton->setIcon(QIcon("Icons/animation/filter_16.png"));
        m_toggleAllFiltersButton->setVisible(false);
        m_toggleAllFiltersButton->setToolTip(tr("Toggle all filters on/off"));
        m_toggleAllFiltersButton->setProperty("iconButton", "true");
        m_toggleAllFiltersButton->setCheckable(true);
        m_toggleAllFiltersButton->setChecked(true);

        m_operatorComboBox = new QComboBox(this);
        m_operatorComboBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_operatorComboBox->setFixedSize(QSize(48, 24));
        m_operatorComboBox->insertItem((int)FilterOperatorType::And, tr("and"));
        m_operatorComboBox->insertItem((int)FilterOperatorType::Or, tr("or"));
        m_operatorComboBox->setCurrentIndex((int)FilterOperatorType::Or);
        m_operatorComboBox->setVisible(false);
        m_operatorComboBox->setToolTip(tr("Set the filter operator.\n\"And\" filters items that match all criteria.\n\"Or\" filters items that many any of the criteria."));

        m_clearAllButton = new QPushButton(this);
        m_clearAllButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_clearAllButton->setFixedSize(QSize(24, 24));
        m_clearAllButton->setIcon(QIcon("Icons/animation/close.png"));
        m_clearAllButton->setVisible(false);
        m_clearAllButton->setToolTip(tr("Clear all filters"));
        m_clearAllButton->setProperty("iconButton", "true");

        filterTextLayout->addWidget(m_filterText);
        filterTextLayout->addWidget(m_defaultTagMenuButton);

        m_filterLayout->setSpacing(3);
        m_filterLayout->addLayout(filterTextLayout);
        m_filterLayout->addWidget(loadButton);
        m_filterLayout->addWidget(m_saveButton);

        m_tagLayout->setSpacing(3);
        m_tagLayout->addWidget(m_toggleAllFiltersButton);
        m_tagLayout->addWidget(m_operatorComboBox);

        secondaryLayout->addLayout(m_tagLayout);
        secondaryLayout->addWidget(m_clearAllButton);
        secondaryLayout->setAlignment(m_clearAllButton, Qt::AlignRight);
        secondaryLayout->setStretchFactor(m_tagLayout, 1);
        secondaryLayout->setStretchFactor(m_clearAllButton, 0);

        m_mainLayout->setSpacing(3);
        m_mainLayout->addLayout(m_filterLayout);
        m_mainLayout->addLayout(secondaryLayout);

        setLayout(m_mainLayout);

        connect(m_clearAllButton, &QPushButton::clicked, this, &SearchCriteriaWidget::OnClearClicked);
        connect(m_operatorComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SearchCriteriaWidget::OnToggleOperator);
        connect(m_filterText, &QLineEdit::editingFinished, this, &SearchCriteriaWidget::OnEditingFinished);
        connect(loadButton, &QPushButton::clicked, this, &SearchCriteriaWidget::OnLoadFilterClicked);
        connect(m_saveButton, &QPushButton::clicked, this, &SearchCriteriaWidget::OnSaveFilterClicked);
        connect(m_availableTagMenu, &QMenu::triggered, this, &SearchCriteriaWidget::OnDefaultTagMenuClicked);
        connect(m_toggleAllFiltersButton, &QPushButton::toggled, this, &SearchCriteriaWidget::OnFilterButtonToggled);
    }

    bool SearchCriteriaWidget::eventFilter(QObject* watched, QEvent* event)
    {
        if (event->type() == QEvent::Show && watched == m_availableTagMenu)
        {
            m_availableTagMenu->move(m_filterText->mapToGlobal(QPoint(0, m_filterText->height())));
            m_availableTagMenu->setFixedWidth(m_filterText->width() + m_defaultTagMenuButton->width());
            return true;
        }
        return false;
    }

    bool SearchCriteriaWidget::filterTextHasFocus()
    {
        return m_filterText->hasFocus();
    }

    void SearchCriteriaWidget::SelectTextEntryBox()
    {
        m_filterText->setFocus();
        m_filterText->selectAll();
    }

    void SearchCriteriaWidget::SetAcceptedTags(QStringList& acceptedTags, QString defaultTag)
    {
        m_acceptedTags = acceptedTags;
        if (!defaultTag.isEmpty())
        {
            if (!m_acceptedTags.contains(defaultTag))
            {
                m_acceptedTags.push_front(defaultTag);
            }
            m_defaultTag = defaultTag;
        }
        else if (m_acceptedTags.size() > 0)
        {
            m_defaultTag = m_acceptedTags[0];
        }
        m_filterText->setPlaceholderText(tr("Add a search filter (default search is '") + m_defaultTag + tr("')..."));

        m_availableTagMenu->clear();
        for (QString& tag : m_acceptedTags)
        {
            QAction* action = m_availableTagMenu->addAction("Create filters by \"" + tag + "\"");
            action->setData(tag);
            action->setCheckable(true);
            action->setChecked(tag == defaultTag);
        }
    }

    void SearchCriteriaWidget::AddSearchCriteria(QString filter, QString defaultTag) 
    {
        if (!filter.isEmpty())
        {
            SearchCriteriaButton* newTag = aznew SearchCriteriaButton(defaultTag, filter, this);
            connect(newTag, &SearchCriteriaButton::CloseRequested, this, &SearchCriteriaWidget::RemoveCriteria);
            connect(newTag, &SearchCriteriaButton::RequestUpdate, this, &SearchCriteriaWidget::CollectAndEmitCriteria);
            m_tagLayout->addWidget(newTag);
            m_filterText->clear();

            bool showTagLayout = HasCriteria();
            m_toggleAllFiltersButton->setVisible(showTagLayout);
            m_operatorComboBox->setVisible(showTagLayout);
            m_clearAllButton->setVisible(showTagLayout);
            m_saveButton->setEnabled(showTagLayout);
            CollectAndEmitCriteria();
        }
    }

    void SearchCriteriaWidget::ForEachCriteria(AZStd::function< void(SearchCriteriaButton*) > callback)
    {
        for (int i = m_tagLayout->count() - 1; i >= 0; i--)
        {
            const char* className = m_tagLayout->itemAt(i)->widget()->metaObject()->className();
            if (!strncmp(className, "AzToolsFramework::SearchCriteriaButton", strlen(className)))
            {
                SearchCriteriaButton* button = static_cast<SearchCriteriaButton*>(m_tagLayout->itemAt(i)->widget());
                if (button)
                {
                    callback(button);
                }
            }
        }
    }

    void SearchCriteriaWidget::OnDefaultTagMenuClicked(QAction* action)
    {
        m_defaultTag = action->data().toString();
        m_filterText->setPlaceholderText(tr("Add a search filter (default search is '") + m_defaultTag + tr("')..."));

        SelectTextEntryBox();

        for (QAction* menuAction : m_availableTagMenu->actions())
        {
            menuAction->setChecked(menuAction == action);
        }
    }

    void SearchCriteriaWidget::OnClearClicked()
    {
        m_filterText->clear();
        auto criteriaCB = [this](SearchCriteriaButton* button)
            {
                RemoveCriteriaWithEmit(button, false);
            };

        ForEachCriteria(criteriaCB);
        CollectAndEmitCriteria();
    }

    void SearchCriteriaWidget::OnFilterButtonToggled()
    {
        if (!m_suppressCriteriaChanged)
        {
            auto criteriaCB = [this](SearchCriteriaButton* button)
                {
                    button->SetFilterEnabled(m_toggleAllFiltersButton->isChecked());
                };

            ForEachCriteria(criteriaCB);

            CollectAndEmitCriteria();
        }
    }

    void SearchCriteriaWidget::OnEditingFinished()
    {
        if (!m_filterText->text().isEmpty())
        {
            QString tag, text;
            SearchCriteriaButton::SplitTagAndText(m_filterText->text(), tag, text);
            if (tag.isEmpty())
            {
                tag = m_defaultTag;
            }

            if (m_acceptedTags.size() > 0)
            {
                bool isAcceptedTag = false;
                for (QString acceptedTag : m_acceptedTags)
                {
                    if (acceptedTag.startsWith(tag))
                    {
                        tag = acceptedTag;
                        isAcceptedTag = true;
                        break;
                    }
                }

                if (!isAcceptedTag)
                {
                    m_filterText->clear();
                    QString message = tr("This search field accepts the following search tags:");
                    for (QString acceptedTag : m_acceptedTags)
                    {
                        message += tr("\n") + acceptedTag;
                        if (acceptedTag == "name")
                        {
                            message += tr(" (default)");
                        }
                    }
                    QMessageBox::warning(this, tr("Invalid Search Filter Type"), message);
                    SelectTextEntryBox();
                    return;
                }
            }

            AddSearchCriteria(text, tag);
        }
    }

    void SearchCriteriaWidget::OnToggleOperator(int index)
    {
        m_criteriaOperator = (FilterOperatorType)index;

        if (!m_suppressCriteriaChanged)
        {
            CollectAndEmitCriteria();
        }
    }

    void SearchCriteriaWidget::CollectAndEmitCriteria()
    {
        QStringList criteriaList;
        bool anyFiltersEnabled = false;
        auto criteriaCB = [&](SearchCriteriaButton* button)
            {
                if (button->IsFilterEnabled())
                {
                    criteriaList.push_back(button->Criteria());
                    anyFiltersEnabled = true;
                }
            };

        ForEachCriteria(criteriaCB);
        m_suppressCriteriaChanged = true;
        m_toggleAllFiltersButton->setChecked(anyFiltersEnabled);
        m_suppressCriteriaChanged = false;

        Q_EMIT SearchCriteriaChanged(criteriaList, m_criteriaOperator);
    }

    bool SearchCriteriaWidget::HasCriteria()
    {
        bool hasCriteria = false;
        auto criteriaCB = [&](SearchCriteriaButton* button)
            {
                if (button)
                {
                    hasCriteria = true;
                }
            };

        ForEachCriteria(criteriaCB);
        return hasCriteria;
    }

    void SearchCriteriaWidget::RemoveCriteria(SearchCriteriaButton* criteria)
    {
        RemoveCriteriaWithEmit(criteria, true);
    }

    void SearchCriteriaWidget::RemoveCriteriaWithEmit(SearchCriteriaButton* criteria, bool emitChanged)
    {
        disconnect(criteria, &SearchCriteriaButton::CloseRequested, this, &SearchCriteriaWidget::RemoveCriteria);
        disconnect(criteria, &SearchCriteriaButton::RequestUpdate, this, &SearchCriteriaWidget::CollectAndEmitCriteria);
        m_tagLayout->removeWidget(criteria);
        delete criteria;

        bool showTagLayout = HasCriteria();
        m_toggleAllFiltersButton->setVisible(showTagLayout);
        m_operatorComboBox->setVisible(showTagLayout);
        m_clearAllButton->setVisible(showTagLayout);
        m_saveButton->setEnabled(showTagLayout);
        if (emitChanged)
        {
            CollectAndEmitCriteria();
        }
    }

    void SearchCriteriaWidget::LoadFilter(const char* filename)
    {
        if (!AZ::IO::SystemFile::Exists(filename))
        {
            return;
        }

        uint64_t fileSize = AZ::IO::SystemFile::Length(filename);
        if (fileSize == 0)
        {
            return;
        }

        std::vector<char> buffer(fileSize + 1);
        buffer[fileSize] = 0;
        if (!AZ::IO::SystemFile::Read(filename, buffer.data()))
        {
            return;
        }

        AZ::rapidxml::xml_document<char>* xmlDoc = azcreate(AZ::rapidxml::xml_document<char>, (), AZ::SystemAllocator);
        if (xmlDoc->parse<0>(buffer.data()))
        {
            AZ::rapidxml::xml_node<char>* xmlRootNode = xmlDoc->first_node();
            if (!xmlRootNode || azstrnicmp(xmlRootNode->name(), ROOT_TAG, 12))
            {
                QMessageBox::warning(this, tr("Invalid Search Filter File"), tr("The filter you have tried to load is invalid.\nIt does not start with a <SearchFilter> tag."));
                return;
            }

            //  Remove any criteria widgets before loading the filter
            //  We do not need to emit SearchCriteriaChanged at this time.
            m_filterText->clear();
            auto criteriaCB = [this](SearchCriteriaButton* button)
                {
                    RemoveCriteriaWithEmit(button, false);
                };
            ForEachCriteria(criteriaCB);

            bool showErrorMessage = false;
            for (AZ::rapidxml::xml_node<char>* childNode = xmlRootNode->first_node(); childNode; childNode = childNode->next_sibling())
            {
                if (!azstricmp(childNode->name(), OPERATOR_TAG))
                {
                    if (!azstricmp(childNode->value(), "and"))
                    {
                        m_criteriaOperator = FilterOperatorType::And;
                    }
                    else
                    {
                        m_criteriaOperator = FilterOperatorType::Or;
                    }
                    m_suppressCriteriaChanged = true;
                    m_operatorComboBox->setCurrentIndex((int)m_criteriaOperator);
                    m_suppressCriteriaChanged = false;
                }
                else if (!azstricmp(childNode->name(), CRITERIA_TAG))
                {
                    QString criteria = childNode->value();
                    QString tag, text;
                    SearchCriteriaButton::SplitTagAndText(criteria, tag, text);
                    if (m_acceptedTags.size() > 0 && !m_acceptedTags.contains(tag))
                    {
                        showErrorMessage = true;
                    }
                    else
                    {
                        SearchCriteriaButton* newTag = aznew SearchCriteriaButton(tag, text, this);
                        connect(newTag, &SearchCriteriaButton::CloseRequested, this, &SearchCriteriaWidget::RemoveCriteria);
                        connect(newTag, &SearchCriteriaButton::RequestUpdate, this, &SearchCriteriaWidget::CollectAndEmitCriteria);
                        m_tagLayout->addWidget(newTag);
                    }
                }
            }

            if (showErrorMessage)
            {
                QMessageBox::warning(this, tr("Incompatible Search Criteria"), tr("The filter you have loaded contains tags\nthat are not supported by this window.\nThey have been ignored."));
            }

            azdestroy(xmlDoc, AZ::SystemAllocator, AZ::rapidxml::xml_document<char>);

            bool showTagLayout = HasCriteria();
            m_toggleAllFiltersButton->setVisible(showTagLayout);
            m_operatorComboBox->setVisible(showTagLayout);
            m_clearAllButton->setVisible(showTagLayout);
            m_saveButton->setEnabled(showTagLayout);
            CollectAndEmitCriteria();
        }
    }

    void SearchCriteriaWidget::SaveFilter(const char* filename)
    {
        AZ::rapidxml::xml_document<char>* xmlDoc = azcreate(AZ::rapidxml::xml_document<char>, (), AZ::SystemAllocator);
        AZ::rapidxml::xml_node<char>* xmlRootNode = xmlDoc->allocate_node(AZ::rapidxml::node_element, ROOT_TAG);

        AZ::rapidxml::xml_node<char>* operatorNode = xmlDoc->allocate_node(AZ::rapidxml::node_element, OPERATOR_TAG, m_criteriaOperator == FilterOperatorType::And ? "and" : "or");
        xmlRootNode->append_node(operatorNode);

        auto criteriaCB = [&](SearchCriteriaButton* button)
            {
                QString criteria = button->Criteria();
                char* value = xmlDoc->allocate_string(criteria.toStdString().c_str());
                AZ::rapidxml::xml_node<char>* criteriaNode = xmlDoc->allocate_node(AZ::rapidxml::node_element, CRITERIA_TAG, value);
                xmlRootNode->append_node(criteriaNode);
            };

        ForEachCriteria(criteriaCB);

        std::string xmlDocString;
        AZ::rapidxml::print(std::back_inserter(xmlDocString), *xmlRootNode, 0);

        AZ::IO::SystemFile outFile;
        outFile.Open(filename, AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);
        outFile.Write(xmlDocString.c_str(), xmlDocString.length());
        outFile.Close();

        azdestroy(xmlDoc, AZ::SystemAllocator, AZ::rapidxml::xml_document<char>);
    }

    void SearchCriteriaWidget::OnSaveFilterClicked()
    {
        char absoluteFilterPath[AZ_MAX_PATH_LEN] = { 0 };
        AZStd::string aliasedFilterPath = "@user@/filters";

        AZ::IO::FileIOBase::GetInstance()->ResolvePath(aliasedFilterPath.c_str(), absoluteFilterPath, AZ_MAX_PATH_LEN);

        if (!AZ::IO::FileIOBase::GetInstance()->Exists(absoluteFilterPath))
        {
            AZ::IO::FileIOBase::GetInstance()->CreatePath(absoluteFilterPath);
        }

        //  Qt's convenient getSaveFileName function won't add default extensions, so we'll make a save dialog from scratch.
        QFileDialog fileDialog;
        fileDialog.setWindowTitle(tr("Save Filter"));
        fileDialog.setWindowFilePath(absoluteFilterPath);
        fileDialog.setNameFilter(tr("Search Filter Files (*.sff)"));
        fileDialog.setAcceptMode(QFileDialog::AcceptSave);
        fileDialog.setFileMode(QFileDialog::AnyFile);
        fileDialog.setViewMode(QFileDialog::Detail);
        fileDialog.setOption(QFileDialog::DontUseNativeDialog, true);
        fileDialog.setDefaultSuffix("sff");

        if (fileDialog.exec())
        {
            QString filename = fileDialog.selectedFiles()[0];
            SaveFilter(filename.toStdString().c_str());
        }
    }

    void SearchCriteriaWidget::OnLoadFilterClicked()
    {
        char absoluteFilterPath[AZ_MAX_PATH_LEN] = { 0 };
        AZStd::string aliasedFilterPath = "@user@/filters";
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(aliasedFilterPath.c_str(), absoluteFilterPath, AZ_MAX_PATH_LEN);

        if (!AZ::IO::FileIOBase::GetInstance()->Exists(absoluteFilterPath))
        {
            AZ::IO::FileIOBase::GetInstance()->CreatePath(absoluteFilterPath);
        }

        QString filename = QFileDialog::getOpenFileName(this, "Load Filter", absoluteFilterPath, tr("Search Filter Files (*.sff)"), nullptr, QFileDialog::DontUseNativeDialog);
        if (!filename.isEmpty())
        {
            LoadFilter(filename.toStdString().c_str());
        }
    }

}   //  namespace AzToolsFramework

#include "UI/SearchWidget/moc_SearchCriteriaWidget.cpp"
