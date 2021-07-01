/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include <AzCore/std/containers/vector.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include "EMStudioConfig.h"
#include <QDialog>
#endif


// forward declarations
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QTreeWidget)
QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)
QT_FORWARD_DECLARE_CLASS(QLineEdit)

namespace AzQtComponents
{
    class FilteredSearchWidget;
}

namespace EMStudio
{
    struct EMSTUDIO_API MotionSetSelectionItem
    {
        AZStd::string           mMotionId;
        EMotionFX::MotionSet*   mMotionSet;

        MotionSetSelectionItem(const AZStd::string& motionId, EMotionFX::MotionSet* motionSet)
            : mMotionId(motionId)
            , mMotionSet(motionSet)
        {
        }
    };


    class EMSTUDIO_API MotionSetHierarchyWidget
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MotionSetHierarchyWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        MotionSetHierarchyWidget(QWidget* parent, bool useSingleSelection, CommandSystem::SelectionList* selectionList = nullptr);
        virtual ~MotionSetHierarchyWidget();

        void SetSelectionMode(bool useSingleSelection);
        void Update(EMotionFX::MotionSet* motionSet, CommandSystem::SelectionList* selectionList = nullptr);
        void FireSelectionDoneSignal();

        MCORE_INLINE QTreeWidget* GetTreeWidget()                               { return mHierarchy; }
        MCORE_INLINE AzQtComponents::FilteredSearchWidget* GetSearchWidget()    { return m_searchWidget; }

        void Select(const AZStd::vector<MotionSetSelectionItem>& selectedItems);

        // this calls UpdateSelection() and then returns the member array containing the selected items
        AZStd::vector<MotionSetSelectionItem>& GetSelectedItems();

        void SelectItemsWithText(QString text);

        AZStd::vector<AZStd::string> GetSelectedMotionIds(EMotionFX::MotionSet* motionSet);

    signals:
        void SelectionChanged(AZStd::vector<MotionSetSelectionItem> selectedNodes);

    public slots:
        //void OnVisibilityChanged(bool isVisible);
        void Update();
        void UpdateSelection();
        void ItemDoubleClicked(QTreeWidgetItem* item, int column);
        void OnTextFilterChanged(const QString& text);

    private:
        void RecursiveAddMotionSet(QTreeWidgetItem* parent, EMotionFX::MotionSet* motionSet, CommandSystem::SelectionList* selectionList);
        void AddMotionSetWithParents(EMotionFX::MotionSet* motionSet);

        EMotionFX::MotionSet*                   mMotionSet;
        QTreeWidget*                            mHierarchy;
        AzQtComponents::FilteredSearchWidget*   m_searchWidget;
        AZStd::string                           m_searchWidgetText;
        AZStd::vector<MotionSetSelectionItem>   mSelected;
        CommandSystem::SelectionList*           mCurrentSelectionList;
        bool                                    mUseSingleSelection;
    };
} // namespace EMStudio
