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
#include <AzCore/std/containers/vector.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <SceneAPI/SceneCore/DataTypes/IManifestObject.h>
#include <SceneAPI/SceneUI/RowWidgets/ManifestVectorWidget.h>
#endif


class QWidget;

namespace AZ
{
    class SerializeContext;

    namespace SceneAPI
    {
        namespace UI
        {
            class ManifestVectorHandler
            {
            public:
                static void Register();
                static void Unregister();
            };

            // This class only has two specializations, that are defined in the cpp file
            template<typename ManifestType>
            class IManifestVectorHandler
                : public QObject
                , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::shared_ptr<ManifestType>>, ManifestVectorWidget>
            {
                static_assert((AZStd::is_base_of<DataTypes::IManifestObject, ManifestType>::value), "Manifest type class must inherit from DataTypes::IManifestObject");
            public:
                AZ_CLASS_ALLOCATOR_DECL

                QWidget* CreateGUI(QWidget* parent) override;
                u32 GetHandlerName() const override;
                bool AutoDelete() const override;

                void ConsumeAttribute(ManifestVectorWidget* widget, u32 attrib,
                    AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
                void WriteGUIValuesIntoProperty(size_t index, ManifestVectorWidget* GUI, typename IManifestVectorHandler::property_t& instance,
                    AzToolsFramework::InstanceDataNode* node) override;
                bool ReadValuesIntoGUI(size_t index, ManifestVectorWidget* GUI, const typename IManifestVectorHandler::property_t& instance,
                    AzToolsFramework::InstanceDataNode* node) override;

                static void Register();
                static void Unregister();

            private:
                static SerializeContext* s_serializeContext;
                static IManifestVectorHandler* s_instance;
            };
        } // UI
    } // SceneAPI
} // AZ
