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
            bool AutoDelete() const;

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
