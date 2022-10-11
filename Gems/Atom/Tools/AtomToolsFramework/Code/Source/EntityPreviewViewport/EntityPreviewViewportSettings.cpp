/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettings.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AtomToolsFramework
{
    void EntityPreviewViewportSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EntityPreviewViewportSettings>()
                ->Version(3)
                ->Field("enableGrid", &EntityPreviewViewportSettings::m_enableGrid)
                ->Field("enableAlternateSkybox", &EntityPreviewViewportSettings::m_enableAlternateSkybox)
                ->Field("enableShadowCatcher", &EntityPreviewViewportSettings::m_enableShadowCatcher)
                ->Field("fieldOfView", &EntityPreviewViewportSettings::m_fieldOfView)
                ->Field("displayMapperOperationType", &EntityPreviewViewportSettings::m_displayMapperOperationType)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EntityPreviewViewportSettings>(
                    "EntityPreviewViewportSettings", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EntityPreviewViewportSettings::m_enableGrid, "Enable Grid", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EntityPreviewViewportSettings::m_enableShadowCatcher, "Enable Shadow Catcher", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EntityPreviewViewportSettings::m_enableAlternateSkybox, "Enable Alternate Skybox", "")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &EntityPreviewViewportSettings::m_fieldOfView, "Field Of View", "")
                        ->Attribute(AZ::Edit::Attributes::Min, 60.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 120.0f)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EntityPreviewViewportSettings::m_displayMapperOperationType, "Display Mapper Type", "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<AZ::Render::DisplayMapperOperationType>())
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<EntityPreviewViewportSettings>("EntityPreviewViewportSettings")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "EntityPreview")
                ->Constructor()
                ->Constructor<const EntityPreviewViewportSettings&>()
                ->Property("enableGrid", BehaviorValueProperty(&EntityPreviewViewportSettings::m_enableGrid))
                ->Property("enableShadowCatcher", BehaviorValueProperty(&EntityPreviewViewportSettings::m_enableShadowCatcher))
                ->Property("enableAlternateSkybox", BehaviorValueProperty(&EntityPreviewViewportSettings::m_enableAlternateSkybox))
                ->Property("fieldOfView", BehaviorValueProperty(&EntityPreviewViewportSettings::m_fieldOfView))
                ->Property("displayMapperOperationType", BehaviorValueProperty(&EntityPreviewViewportSettings::m_displayMapperOperationType))
                ;
        }
    }
} // namespace AtomToolsFramework
