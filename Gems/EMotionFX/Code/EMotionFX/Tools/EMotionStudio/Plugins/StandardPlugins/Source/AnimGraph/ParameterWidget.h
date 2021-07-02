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
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include "../StandardPluginsConfig.h"
#include <QDialog>
#endif


// forward declarations
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QIcon)
QT_FORWARD_DECLARE_CLASS(QTreeWidget)
QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)
QT_FORWARD_DECLARE_CLASS(QLineEdit)

namespace AzQtComponents
{
    class FilteredSearchWidget;
}

namespace EMStudio
{
    class ParameterWidget
        : public QWidget
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(ParameterWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH)

    public:
        ParameterWidget(QWidget* parent, bool useSingleSelection);
        virtual ~ParameterWidget();

        void SetSelectionMode(bool useSingleSelection);
        void SetFilterTypes(const AZStd::vector<AZ::TypeId>& filterTypes);
        void Update(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& selectedParameters);
        void FireSelectionDoneSignal();
        MCORE_INLINE QTreeWidget* GetTreeWidget()                                                               { return mTreeWidget; }
        MCORE_INLINE AzQtComponents::FilteredSearchWidget* GetSearchWidget()                                    { return m_searchWidget; }

        // this calls UpdateSelection() and then returns the member array containing the selected items
        AZStd::vector<AZStd::string>& GetSelectedParameters()                                                   { UpdateSelection(); return mSelectedParameters; }

    signals:
        void OnSelectionDone(const AZStd::vector<AZStd::string>& selectedItems);
        void OnDoubleClicked(const AZStd::string& item);

    public slots:
        //void OnVisibilityChanged(bool isVisible);
        void Update();
        void UpdateSelection();
        void ItemDoubleClicked(QTreeWidgetItem* item, int column);
        void OnTextFilterChanged(const QString& text);

    private:
        void AddParameterToInterface(EMotionFX::AnimGraph* animGraph, const EMotionFX::Parameter* parameter, QTreeWidgetItem* groupParameterItem);

        EMotionFX::AnimGraph* mAnimGraph;
        QTreeWidget* mTreeWidget;
        AzQtComponents::FilteredSearchWidget* m_searchWidget;
        AZStd::string m_searchWidgetText;
        AZStd::vector<AZ::TypeId> m_filterTypes;
        AZStd::vector<AZStd::string> mSelectedParameters;
        AZStd::vector<AZStd::string> mOldSelectedParameters;
        bool mUseSingleSelection;
    };
} // namespace EMStudio
