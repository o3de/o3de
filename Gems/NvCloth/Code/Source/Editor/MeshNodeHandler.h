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
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <Editor/ComboBoxEditButtonPair.h>
#include <QObject>
#endif

namespace NvCloth
{
    namespace Editor
    {
        /*
        =============================================================
        = Handler Documentation                                     =
        =============================================================

        Custom handler for the Cloth Component's Mesh Node property as a ComboBoxEditButtonPair widget.

        Handler Name: "MeshNodeSelector"

        Available Attributes:
            EntityId - Entity identifier used to query the mesh asset via MeshComponentRequestBus.
            StringList - List of mesh node names that contain cloth data.

        NOTE: EntityId must be the first attribute set so it's available when consuming StringList.
        */
        class MeshNodeHandler
            : public QObject
            , public AzToolsFramework::PropertyHandler<AZStd::string, ComboBoxEditButtonPair>
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(MeshNodeHandler, AZ::SystemAllocator, 0);

            MeshNodeHandler() = default;
            
            // AzToolsFramework::PropertyHandler overrides ...
            AZ::u32 GetHandlerName() const override;
            QWidget* CreateGUI(QWidget* parent) override;
            bool IsDefaultHandler() const override;
            void ConsumeAttribute(widget_t* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
            void WriteGUIValuesIntoProperty(size_t index, widget_t* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
            bool ReadValuesIntoGUI(size_t index, widget_t* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

        private:
            void OnEditButtonClicked(widget_t* GUI);

            AZ::Data::Asset<AZ::Data::AssetData> GetMeshAsset(const AZ::EntityId entityId) const;
        };
    } // namespace Editor
} // namespace NvCloth
