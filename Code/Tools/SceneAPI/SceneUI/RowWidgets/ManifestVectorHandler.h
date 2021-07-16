#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
