/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QtWidgets/QFrame>

#include <AzCore/std/string/string.h>
#include <AzCore/std/functional.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzQtComponents/Components/FlowLayout.h>
#include <AzToolsFramework/UI/SearchWidget/SearchWidgetTypes.hxx>
#endif

class QBoxLayout;
class QLabel;
class QLineEdit;
class QMenu;
class QPushButton;
class QComboBox;

namespace AzToolsFramework
{
    //! This button is used by the SearchCriteriaWidget to display individual
    //! search terms. It contains a label and close button. Clicking the close
    //! button will notify the SearchCriteriaWidget to delete the button and
    //! remove the term from the search list.
    class SearchCriteriaButton
        : public QFrame
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(SearchCriteriaButton, AZ::SystemAllocator);

        SearchCriteriaButton(QString typeText, QString labelText, QWidget* pParent = nullptr);
        ~SearchCriteriaButton() {};


        QString Criteria();
        bool IsFilterEnabled() { return m_filterEnabled; }
        void SetFilterEnabled(bool enabled);

        //! Take a string from a SearchCriteriaButton and split it into its two parts.
        static void SplitTagAndText(const QString& fullText, QString& tagText, QString& criteriaText);

Q_SIGNALS:
        void CloseRequested(SearchCriteriaButton* criteria);
        void RequestUpdate();

    protected Q_SLOTS:
        void OnCloseClicked();

    protected:
        void mouseReleaseEvent(QMouseEvent* event) override;
        void enterEvent(QEvent* event) override;
        void leaveEvent(QEvent* event) override;

        QString m_tagText;
        QString m_criteriaText;
        bool m_filterEnabled;
        bool m_mouseHover;

        QString m_baseStyleSheet;

        QLabel* m_tagLabel;
    };

    //! This widget combines a line edit with a line of SearchCriteriaButtons to give you
    //! the ability to search multiple terms at once, and to remove those terms one at a
    //! time if need be. There is also an operator button to change between an 'and' or 'or'
    //! operation on the different search terms.
    //! It emits one signal (criteriaChanged) whenever a parameter of the search changes.
    //! This signal gives the list of terms and the current operator. The parent widget is
    //! responsible for using this information to filter/search its contents.
    class SearchCriteriaWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(SearchCriteriaWidget, AZ::SystemAllocator);

        SearchCriteriaWidget(QWidget* pParent = nullptr);
        ~SearchCriteriaWidget() {};

        void SelectTextEntryBox();
        void SetAcceptedTags(QStringList& acceptedTags, QString defaultTag = "name");
        void AddSearchCriteria(QString filter, QString defaultTag = "name");

        bool eventFilter(QObject* watched, QEvent* event) override;
        
        bool filterTextHasFocus();

Q_SIGNALS:
        void SearchCriteriaChanged(QStringList& criteriaList, FilterOperatorType filterOperator);

    protected Q_SLOTS:
        void RemoveCriteria(SearchCriteriaButton* criteria);
        void CollectAndEmitCriteria();

        void OnClearClicked();
        void OnSaveFilterClicked();
        void OnLoadFilterClicked();
        void OnFilterButtonToggled();
        void OnDefaultTagMenuClicked(QAction* action);

    protected:
        //! This is the internal implementation that can be used to skip emitting the signal if you plan
        //! on removing more than one criteria at once.
        void RemoveCriteriaWithEmit(SearchCriteriaButton* criteria, bool emitChanged = true);
        void OnEditingFinished();
        void OnToggleOperator(int index);

        void ForEachCriteria(AZStd::function< void(SearchCriteriaButton*) > callback);

        bool HasCriteria();

        void LoadFilter(const char* filename);
        void SaveFilter(const char* filename);

        QVBoxLayout* m_mainLayout;
        QHBoxLayout* m_filterLayout;
        FlowLayout* m_tagLayout;

        QLineEdit* m_filterText;
        QPushButton* m_defaultTagMenuButton;
        QMenu* m_availableTagMenu;
        QPushButton* m_saveButton;

        QPushButton* m_toggleAllFiltersButton;
        QComboBox* m_operatorComboBox;
        QPushButton* m_clearAllButton;

        bool m_suppressCriteriaChanged;

        FilterOperatorType     m_criteriaOperator;

        QStringList            m_acceptedTags;
        QString                m_defaultTag;
    };

    using FilterByCategoryMap = AZStd::unordered_map<AZStd::string, QRegExp>;
} // namespace AzToolsFramework
