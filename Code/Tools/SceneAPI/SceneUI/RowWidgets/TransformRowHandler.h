/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Math/Transform.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <SceneAPI/SceneUI/RowWidgets/TransformRowWidget.h>
#endif

class QWidget;

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            class TranformRowHandler 
                : public QObject
                , public AzToolsFramework::PropertyHandler<AZ::Transform, TransformRowWidget>
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR_DECL

                QWidget* CreateGUI(QWidget* parent) override;
                u32 GetHandlerName() const override;
                bool AutoDelete() const override;

                bool IsDefaultHandler() const override;
                
                void ConsumeAttribute(TransformRowWidget* widget, u32 attrib,
                    AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
                void WriteGUIValuesIntoProperty(size_t index, TransformRowWidget* GUI, property_t& instance,
                    AzToolsFramework::InstanceDataNode* node) override;
                bool ReadValuesIntoGUI(size_t index, TransformRowWidget* GUI, const property_t& instance,
                    AzToolsFramework::InstanceDataNode* node) override;

                static void Register();
                static void Unregister();

            protected:
                virtual void ConsumeFilterTypeAttribute(TransformRowWidget* widget,
                    AzToolsFramework::PropertyAttributeReader* attrValue);

            private:
                static TranformRowHandler* s_instance;
            };
        } // namespace SceneUI
    } // namespace SceneAPI
} // namespace AZ
