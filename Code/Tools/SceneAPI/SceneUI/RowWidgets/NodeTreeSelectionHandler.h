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
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <SceneAPI/SceneUI/RowWidgets/NodeTreeSelectionWidget.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>
#endif

class QWidget;

/*
=============================================================
= Handler Documentation                                     =
=============================================================

Handler Name: "NodeTreeSelection"

Available Attributes:
    FilterName - Name of the filter type used in the summary label.
    FilterType - Uuid for the type(s) to filter for. This attribute can be added multiple times. By default all
                    types will be considered but by adding one or more of filters only classes that match the uuid
                    of the given type or are derived of that type will be used for the selected and total count.
                    The object is an end-point it will also show in the selection graph, otherwise end-points are hidden.
    FilterVirtualType - Crc32 or name (string) for the type(s) to filter for. This attribute can be added multiple 
                    times. By default all types will be considered but by adding one or more of filters only objects
                    that match any of the virtual types will be used for the selected and total count.
                    The object is an end-point it will also show in the selection graph, otherwise end-points are hidden.
    NarrowSelection - If set to true only filter types will have a checkbox, otherwise all entries can be selected.
*/

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            class SCENE_UI_API NodeTreeSelectionHandler
                : public QObject
                , public AzToolsFramework::PropertyHandler<DataTypes::ISceneNodeSelectionList, NodeTreeSelectionWidget>
            {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR_DECL

                QWidget* CreateGUI(QWidget* parent) override;
                u32 GetHandlerName() const override;
                bool AutoDelete() const override;
                bool IsDefaultHandler() const override;
                
                void ConsumeAttribute(NodeTreeSelectionWidget* widget, u32 attrib,
                    AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
                void WriteGUIValuesIntoProperty(size_t index, NodeTreeSelectionWidget* GUI, property_t& instance,
                    AzToolsFramework::InstanceDataNode* node) override;
                bool ReadValuesIntoGUI(size_t index, NodeTreeSelectionWidget* GUI, const property_t& instance,
                    AzToolsFramework::InstanceDataNode* node) override;

                static void Register();
                static void Unregister();

            protected:
                virtual void ConsumeFilterNameAttribute(NodeTreeSelectionWidget* widget,
                    AzToolsFramework::PropertyAttributeReader* attrValue);
                virtual void ConsumeFilterTypeAttribute(NodeTreeSelectionWidget* widget,
                    AzToolsFramework::PropertyAttributeReader* attrValue);
                virtual void ConsumeFilterVirtualTypeAttribute(NodeTreeSelectionWidget* widget,
                    AzToolsFramework::PropertyAttributeReader* attrValue);
                virtual void ConsumeNarrowSelectionAttribute(NodeTreeSelectionWidget* widget,
                    AzToolsFramework::PropertyAttributeReader* attrValue);

            private:
                static NodeTreeSelectionHandler* s_instance;
            };
        } // UI
    } // SceneAPI
} // AZ
