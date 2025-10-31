/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyStringComboBoxCtrl.hxx>
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

        Custom handler for the Cloth Component's Mesh Node property as a PropertyStringComboBoxCtrl widget.

        Handler Name: "MeshNodeSelector"

        Available Attributes:
            EntityId - Entity identifier used to query the mesh asset via MeshComponentRequestBus.
            StringList - List of mesh node names that contain cloth data.

        NOTE: EntityId must be the first attribute set so it's available when consuming StringList.
        */
        class MeshNodeHandler
            : public QObject
            , public AzToolsFramework::PropertyHandler<AZStd::string, AzToolsFramework::PropertyStringComboBoxCtrl>
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(MeshNodeHandler, AZ::SystemAllocator);

            MeshNodeHandler() = default;
            
            // AzToolsFramework::PropertyHandler overrides ...
            AZ::u32 GetHandlerName() const override;
            QWidget* CreateGUI(QWidget* parent) override;
            bool IsDefaultHandler() const override;
            void ConsumeAttribute(widget_t* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
            void WriteGUIValuesIntoProperty(size_t index, widget_t* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
            bool ReadValuesIntoGUI(size_t index, widget_t* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

        private:
            void OnEditButtonClicked();
            AZ::Data::Asset<AZ::Data::AssetData> GetMeshAsset(const AZ::EntityId entityId) const;

            AZ::EntityId m_entityId;
        };
    } // namespace Editor
} // namespace NvCloth
