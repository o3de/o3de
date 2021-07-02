/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <Editor/PropertyWidgets/LODTreeSelectionHandler.h>
#include <Editor/PropertyWidgets/PropertyWidgetAllocator.h>


namespace EMotionFX
{
    namespace Pipeline
    {
        namespace UI
        {
            AZ_CLASS_ALLOCATOR_IMPL(LODTreeSelectionHandler, PropertyWidgetAllocator, 0)

            QWidget* LODTreeSelectionHandler::CreateGUI(QWidget* parent)
            {
                LODTreeSelectionWidget* instance = aznew LODTreeSelectionWidget(parent);
                connect(instance, &LODTreeSelectionWidget::valueChanged, this,
                    [instance]()
                    {
                        EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, instance);
                    });
                return instance;
            }

            AZ::u32 LODTreeSelectionHandler::GetHandlerName() const
            {
                return AZ_CRC("LODTreeSelection", 0x25c27718);
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

                if (attrib == AZ_CRC("HideUncheckable", 0xa5bafb99))
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
