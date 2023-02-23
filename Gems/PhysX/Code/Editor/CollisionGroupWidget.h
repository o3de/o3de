/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyStringComboBoxCtrl.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEnumComboBoxCtrl.hxx>

#endif

namespace PhysX
{
    namespace Editor
    {
        class CollisionGroupWidget
            : public QObject
            , public AzToolsFramework::PropertyHandler<AzPhysics::CollisionGroups::Id, AzToolsFramework::PropertyStringComboBoxCtrl>
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(CollisionGroupWidget, AZ::SystemAllocator);

            CollisionGroupWidget();
            
            AZ::u32 GetHandlerName() const override;
            QWidget* CreateGUI(QWidget* parent) override;
            bool IsDefaultHandler() const override;

            void ConsumeAttribute(widget_t* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

            void WriteGUIValuesIntoProperty(size_t index, widget_t* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
            bool ReadValuesIntoGUI(size_t index, widget_t* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

        private:
            AzPhysics::CollisionGroups::Id GetGroupFromName(const AZStd::string& groupName);
            AZStd::string GetNameFromGroup(const AzPhysics::CollisionGroups::Id& groupId);
            AZStd::vector<AZStd::string> GetGroupNames();
            void OnEditButtonClicked();
        };

        class CollisionGroupEnumPropertyComboBoxHandler
            : public AzToolsFramework::GenericEnumPropertyComboBoxHandler<AzPhysics::CollisionGroup>
        {
        public:
            AZ_CLASS_ALLOCATOR(CollisionGroupEnumPropertyComboBoxHandler, AZ::SystemAllocator);
        };

    } // namespace Editor
} // namespace PhysX
