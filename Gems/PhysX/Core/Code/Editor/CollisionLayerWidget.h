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
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyStringComboBoxCtrl.hxx>
#endif

namespace PhysX
{
    namespace Editor
    {
        class CollisionLayerWidget
            : public QObject
            , public AzToolsFramework::PropertyHandler<AzPhysics::CollisionLayer, AzToolsFramework::PropertyStringComboBoxCtrl>
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(CollisionLayerWidget, AZ::SystemAllocator);

            CollisionLayerWidget() = default;
            
            AZ::u32 GetHandlerName() const override;
            QWidget* CreateGUI(QWidget* parent) override;
            bool IsDefaultHandler() const override;

            void ConsumeAttribute(widget_t* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

            void WriteGUIValuesIntoProperty(size_t index, widget_t* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
            bool ReadValuesIntoGUI(size_t index, widget_t* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

        private:
            void OnEditButtonClicked();
            AzPhysics::CollisionLayer GetLayerFromName(const AZStd::string& layerName);
            AZStd::string GetNameFromLayer(const AzPhysics::CollisionLayer& layerIndex);
            AZStd::vector<AZStd::string> GetLayerNames();
        };
    }
    
} // namespace PhysX
