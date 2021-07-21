/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QComboBox>
#include <Blast/BlastMaterial.h>
#endif


namespace Blast
{
    namespace Editor
    {
        class MaterialIdWidget
            : public QObject
            , public AzToolsFramework::PropertyHandler<Blast::BlastMaterialId, QComboBox>
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(MaterialIdWidget, AZ::SystemAllocator, 0);

            MaterialIdWidget() = default;

            AZ::u32 GetHandlerName() const override;
            QWidget* CreateGUI(QWidget* parent) override;
            bool IsDefaultHandler() const override;

            void ConsumeAttribute(
                widget_t* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue,
                const char* debugName) override;

            void WriteGUIValuesIntoProperty(
                size_t index, widget_t* gui, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
            bool ReadValuesIntoGUI(
                size_t index, widget_t* gui, const property_t& instance,
                AzToolsFramework::InstanceDataNode* node) override;

        private:
            Blast::BlastMaterialId GetIdForIndex(size_t index);
            int GetIndexForId(Blast::BlastMaterialId id);

            AZ::Data::AssetId m_materialLibraryId;
            AZStd::vector<Blast::BlastMaterialId> m_libraryIds;
        };
    } // namespace Editor
} // namespace Blast
