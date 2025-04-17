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

                bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;

                static void Register();
                static void Unregister();

            private:
                static HeaderHandler* s_instance;
            };
        } // UI
    } // SceneAPI
} // AZ
