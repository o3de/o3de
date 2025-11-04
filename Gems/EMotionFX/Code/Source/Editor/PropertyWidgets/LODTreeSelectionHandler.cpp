/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/EBus.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <Editor/PropertyWidgets/LODTreeSelectionHandler.h>
#include <Editor/PropertyWidgets/PropertyWidgetAllocator.h>


namespace EMotionFX
{
    namespace Pipeline
    {
        namespace UI
        {
            AZ_CLASS_ALLOCATOR_IMPL(LODTreeSelectionHandler, PropertyWidgetAllocator)

            QWidget* LODTreeSelectionHandler::CreateGUI(QWidget* parent)
            {
                LODTreeSelectionWidget* instance = aznew LODTreeSelectionWidget(parent);
                connect(instance, &LODTreeSelectionWidget::valueChanged, this,
                    [instance]()
                    {
                        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                            &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, instance);
                    });
                return instance;
            }

            AZ::u32 LODTreeSelectionHandler::GetHandlerName() const
            {
                return AZ_CRC_CE("LODTreeSelection");
            }

            bool LODTreeSelectionHandler::IsDefaultHandler() const
            {
                return false;
            }

            void LODTreeSelectionHandler::ConsumeAttribute(SceneUI::NodeTreeSelectionWidget* widget, AZ::u32 attrib,
                AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
            {
                // Note: debugName could be nullptr in some cases. We want to avoid passing a nullptr to the ConsumeAttribute function.
                AZStd::string debugNameStr;
                if(debugName)
                {
                    debugNameStr = debugName;
                }
                SceneUI::NodeTreeSelectionHandler::ConsumeAttribute(widget, attrib, attrValue, debugNameStr.c_str());

                if (attrib == AZ_CRC_CE("HideUncheckable"))
                {
                    ConsumeHideUncheckableAttribute(widget, attrValue);
                }
            }

            void LODTreeSelectionHandler::ConsumeHideUncheckableAttribute(SceneUI::NodeTreeSelectionWidget* widget, AzToolsFramework::PropertyAttributeReader * attrValue)
            {
                // Upcast to use the LODWidget.
                LODTreeSelectionWidget* LODWidget = static_cast<LODTreeSelectionWidget*>(widget);
                AZ_Error("EMotionFX", LODWidget, "LODTreeSelectionHandler must handles a LODTreeSelectionWidget.");

                bool hide;
                if (attrValue->Read<bool>(hide))
                {
                    LODWidget->HideUncheckable(hide);
                }
                else
                {
                    AZ_Assert(false, "Failed to read boolean from 'HideUncheckable' attribute.");
                }
            }
        }
    }
}

#include <Source/Editor/PropertyWidgets/moc_LODTreeSelectionHandler.cpp>
