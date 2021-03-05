#pragma once

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

#if !defined(Q_MOC_RUN)
#include <AzCore/Math/Crc.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <SceneAPI/SceneUI/RowWidgets/HeaderWidget.h>

#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#endif

class QWidget;


/*
=============================================================
= Handler Documentation                                     =
=============================================================

Handler Name: "Header"
*/


namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            class HeaderHandler
                : public QObject, public AzToolsFramework::PropertyHandler<DataTypes::IManifestObject, HeaderWidget>
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR_DECL

                QWidget* CreateGUI(QWidget* parent) override;
                u32 GetHandlerName() const override;
                bool AutoDelete() const override;
                bool IsDefaultHandler() const override;

                void ConsumeAttribute(HeaderWidget* widget, u32 attrib,
                    AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
                void WriteGUIValuesIntoProperty(size_t index, HeaderWidget* GUI, property_t& instance,
                    AzToolsFramework::InstanceDataNode* node) override;
                bool ReadValuesIntoGUI(size_t index, HeaderWidget* GUI, const property_t& instance,
                    AzToolsFramework::InstanceDataNode* node) override;

                static void Register();
                static void Unregister();

            private:
                static HeaderHandler* s_instance;
            };
        } // UI
    } // SceneAPI
} // AZ