/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDebugComponent.h"

#include <AzCore/Serialization/Utils.h>
#include <AzCore/base.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>
#include <VegetationSystemComponent.h>

#include <ISystem.h>
#include <IConsole.h>

namespace Vegetation
{
    namespace EditorDebugComponentVersionUtility
    {
        bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            EditorVegetationComponentBaseVersionConverter<DebugComponent, DebugConfig>(context, classElement);

            if (classElement.GetVersion() < 2)
            {
                classElement.RemoveElementByName(AZ_CRC_CE("FilerTypeLevel"));
                classElement.RemoveElementByName(AZ_CRC_CE("SortType"));
            }
            return true;
        }
    }
     
    void EditorDebugComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<EditorDebugComponent, BaseClassType>()
                ->Version(2, &EditorDebugComponentVersionUtility::VersionConverter)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<EditorDebugComponent>(
                    s_componentName, s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, s_icon)
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, s_viewportIcon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, s_helpUrl)
                    ->Attribute(AZ::Edit::Attributes::Category, s_categoryName)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Level"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->UIElement(AZ::Edit::UIHandlers::Button, "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugComponent::OnDumpDataToFile)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Dump Performance Log")
                    ->UIElement(AZ::Edit::UIHandlers::Button, "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugComponent::OnClearReport)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Clear Performance Log")
                    ->UIElement(AZ::Edit::UIHandlers::Button, "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugComponent::OnRefreshAllAreas)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Refresh All Areas")
                    ->UIElement(AZ::Edit::UIHandlers::Button, "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugComponent::OnClearAllAreas)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Clear All Areas")
                    ;
            }
        }
    }

    void EditorDebugComponent::Activate()
    {
        BaseClassType::Activate();
    }

    void EditorDebugComponent::OnDumpDataToFile()
    {
        m_component.ExportCurrentReport();
    }

    void EditorDebugComponent::OnClearReport()
    {
        m_component.ClearPerformanceReport();
    }

    void EditorDebugComponent::OnRefreshAllAreas()
    {
        AreaSystemRequestBus::Broadcast(&AreaSystemRequestBus::Events::RefreshAllAreas);
    }

    void EditorDebugComponent::OnClearAllAreas()
    {
        AreaSystemRequestBus::Broadcast(&AreaSystemRequestBus::Events::ClearAllAreas);
        AreaSystemRequestBus::Broadcast(&AreaSystemRequestBus::Events::RefreshAllAreas);
    }
}
