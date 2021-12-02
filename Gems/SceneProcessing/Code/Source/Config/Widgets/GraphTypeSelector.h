/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QPushButton>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <Config/SettingsObjects/FileSoftNameSetting.h>
#endif

class QWidget;

namespace AZ
{
    namespace SceneProcessingConfig
    {
        class GraphTypeSelector 
            : public QObject
            , public AzToolsFramework::PropertyHandler<FileSoftNameSetting::GraphTypeContainer, QPushButton>
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR_DECL

            QWidget* CreateGUI(QWidget* parent) override;
            u32 GetHandlerName() const override;
            bool AutoDelete() const override;

            bool IsDefaultHandler() const override;

            void ConsumeAttribute(QPushButton* widget, u32 attrib,
                AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
            void WriteGUIValuesIntoProperty(size_t index, QPushButton* GUI, property_t& instance,
                AzToolsFramework::InstanceDataNode* node) override;
            bool ReadValuesIntoGUI(size_t index, QPushButton* GUI, const property_t& instance,
                AzToolsFramework::InstanceDataNode* node) override;

            static void Register();
            static void Unregister();

        private:
            bool eventFilter(QObject* object, QEvent* event) override;

            static GraphTypeSelector* s_instance;
        };
    } // namespace SceneProcessingConfig
} // namespace AZ
