/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <QSet>

namespace AzToolsFramework
{
    class IPropertyEditor;
    AZ_TYPE_INFO_SPECIALIZE(IPropertyEditor, "{78E75216-0559-43B9-93FE-33FD6EBF3B9C}");

    class IPropertyEditor
    {
        public:
            AZ_RTTI_NO_TYPE_INFO_DECL();
            virtual void SetValueComparisonFunction([[maybe_unused]] const InstanceDataHierarchy::ValueComparisonFunction& valueComparisonFunction){};

            using ReadOnlyQueryFunction = AZStd::function<bool(const InstanceDataNode*)>;
            virtual void SetReadOnlyQueryFunction([[maybe_unused]] const ReadOnlyQueryFunction& readOnlyQueryFunction){};

            using HiddenQueryFunction = AZStd::function<bool(const InstanceDataNode*)>;
            virtual void SetHiddenQueryFunction([[maybe_unused]] const HiddenQueryFunction& hiddenQueryFunction){};

            using IndicatorQueryFunction = AZStd::function<const char*(const InstanceDataNode*)>;
            virtual void SetIndicatorQueryFunction([[maybe_unused]] const IndicatorQueryFunction& indicatorQueryFunction){};

            virtual void SetFilterString([[maybe_unused]] AZStd::string str){};

            using InstanceDataHierarchyCallBack = AZStd::function<void(AzToolsFramework::InstanceDataHierarchy& /*hierarchy*/)>;
            virtual void EnumerateInstances([[maybe_unused]] InstanceDataHierarchyCallBack enumerationCallback) {};

            virtual void MoveNodeToIndex([[maybe_unused]] InstanceDataNode* node, [[maybe_unused]] int index) {};

            virtual void MoveNodeBefore([[maybe_unused]] InstanceDataNode* nodeToMove, [[maybe_unused]] InstanceDataNode* nodeToMoveBefore) {};

            virtual void MoveNodeAfter([[maybe_unused]] InstanceDataNode* nodeToMove, [[maybe_unused]] InstanceDataNode* nodeToMoveBefore) {};

            virtual void CancelQueuedRefresh() {};

            virtual void SetHideRootProperties([[maybe_unused]] bool hideRootProperties) {};

            virtual void SetAutoResizeLabels([[maybe_unused]] bool autoResizeLabels) {};

            virtual void InvalidateAll([[maybe_unused]] const char* filter = nullptr) {};

            virtual void QueueInvalidation([[maybe_unused]] PropertyModificationRefreshLevel level) {};

            virtual void PreventDataAccess([[maybe_unused]] bool shouldPrevent) {};

            virtual void ClearInstances() {};

            using DynamicEditDataProvider = AZStd::function<const AZ::Edit::ElementData*(const void* /*objectPtr*/, const AZ::SerializeContext::ClassData* /*classData*/)>;
            virtual void SetDynamicEditDataProvider([[maybe_unused]] DynamicEditDataProvider provider) {};

            virtual void SetSavedStateKey([[maybe_unused]] AZ::u32 key, [[maybe_unused]] AZStd::string propertyEditorName = {}) {}

            virtual bool HasFilteredOutNodes() const
            {
                return false;
            }

            virtual bool HasVisibleNodes() const
            {
                return false;
            }

            virtual PropertyRowWidget* GetWidgetFromNode([[maybe_unused]] InstanceDataNode* node) const
            {
                return nullptr;
            }

            virtual QSet<PropertyRowWidget*> GetTopLevelWidgets()
            {
                QSet<PropertyRowWidget*> toplevelWidgets;
                return toplevelWidgets;
            }

            virtual bool AddInstance(
                [[maybe_unused]] void* instance,
                [[maybe_unused]] const AZ::Uuid& classId,
                [[maybe_unused]] void* aggregateInstance = nullptr,
                [[maybe_unused]] void* compareInstance = nullptr)
            {
                return false;
            }

            // todo: Also bypassed a GetWidgets() method in the EntityPropertyEditor we should look into
    };

} // namespace AzToolsFramework
