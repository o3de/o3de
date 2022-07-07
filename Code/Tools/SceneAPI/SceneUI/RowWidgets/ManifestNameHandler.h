#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <SceneAPI/SceneUI/RowWidgets/ManifestNameWidget.h>
#endif

class QWidget;

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            //Available Attributes:
            //    - "FilterType" - Uuid for the type(s) to filter for. If set, the name will only be unique for
            //                     classes of this type or derived classes.
            class ManifestNameHandler 
                : public QObject, public AzToolsFramework::PropertyHandler<AZStd::string, ManifestNameWidget>
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR_DECL

                QWidget* CreateGUI(QWidget* parent) override;
                u32 GetHandlerName() const override;
                bool AutoDelete() const override;
                
                void ConsumeAttribute(ManifestNameWidget* widget, u32 attrib,
                    AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
                void WriteGUIValuesIntoProperty(size_t index, ManifestNameWidget* GUI, property_t& instance,
                    AzToolsFramework::InstanceDataNode* node) override;
                bool ReadValuesIntoGUI(size_t index, ManifestNameWidget* GUI, const property_t& instance,
                    AzToolsFramework::InstanceDataNode* node) override;

                static void Register();
                static void Unregister();

            protected:
                virtual void ConsumeFilterTypeAttribute(ManifestNameWidget* widget,
                    AzToolsFramework::PropertyAttributeReader* attrValue);

            private:
                static ManifestNameHandler* s_instance;
            };
        } // SceneUI
    } // SceneAPI
} // AZ
