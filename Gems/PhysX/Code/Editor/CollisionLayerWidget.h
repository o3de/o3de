/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <Editor/ComboBoxEditButtonPair.h>
#endif

namespace PhysX
{
    namespace Editor
    {
        class CollisionLayerWidget
            : public QObject
            , public AzToolsFramework::PropertyHandler<AzPhysics::CollisionLayer, ComboBoxEditButtonPair>
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(CollisionLayerWidget, AZ::SystemAllocator, 0);

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
