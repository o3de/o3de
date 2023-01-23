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
#include <Editor/ComboBoxEditButtonPair.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#endif

namespace PhysX
{
    namespace Editor
    {
        /// Widget to connect the KinematicDescriptionDialog with the Kinematic setting for rigid bodies.
        class KinematicWidget
            : public QObject
            , public AzToolsFramework::PropertyHandler<bool, ComboBoxEditButtonPair>
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(KinematicWidget, AZ::SystemAllocator, 0);

            KinematicWidget() = default;

            AZ::u32 GetHandlerName() const override;
            QWidget* CreateGUI(QWidget* parent) override;
            void WriteGUIValuesIntoProperty(
                size_t index, widget_t* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
            bool ReadValuesIntoGUI(
                size_t index, widget_t* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

        private:
            void OnEditButtonClicked();

            QComboBox* m_widgetComboBox= nullptr;
        };
    } // namespace Editor
} // namespace PhysX
