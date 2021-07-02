#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
#include <SceneAPI/SceneUI/SceneWidgets/SceneGraphWidget.h>
#endif

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class ISceneNodeSelectionList;
        }

        namespace UI
        {
            // QT space
            namespace Ui
            {
                class NodeTreeSelectionWidget;
            }

            class SCENE_UI_API NodeTreeSelectionWidget : public QWidget
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR_DECL

                explicit NodeTreeSelectionWidget(QWidget* parent);
                ~NodeTreeSelectionWidget() override;

                void SetList(const DataTypes::ISceneNodeSelectionList& list);
                void CopyListTo(DataTypes::ISceneNodeSelectionList& target);
                
                void SetFilterName(const AZStd::string& name);
                void SetFilterName(AZStd::string&& name);

                void AddFilterType(const Uuid& idProperty);
                void AddFilterVirtualType(Crc32 name);
                void UseNarrowSelection(bool enable);
                void UpdateSelectionLabel();

            signals:
                void valueChanged();

            protected:
                void SelectButtonClicked();
                void ListChangesAccepted();
                void ListChangesCanceled();
                virtual void ResetNewTreeWidget(const Containers::Scene& scene);
                size_t CalculateSelectedCount();
                size_t CalculateTotalCount();

                AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
                AZStd::set<Uuid> m_filterTypes;
                AZStd::set<Crc32> m_filterVirtualTypes;
                AZStd::string m_filterName;
                QScopedPointer<Ui::NodeTreeSelectionWidget> ui;
                AZStd::unique_ptr<SceneGraphWidget> m_treeWidget;
                AZStd::unique_ptr<DataTypes::ISceneNodeSelectionList> m_list;
                AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
                bool m_narrowSelection;
            };
        } // UI
    } // SceneAPI
} // AZ
