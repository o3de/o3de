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
#include "TouchBending_precompiled.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <Config/SceneProcessingConfigBus.h>

#include <Pipeline/TouchBendingSceneSystemComponent.h>


namespace TouchBending
{
    namespace Pipeline
    {
        TouchBendingSceneSystemComponent::TouchBendingSceneSystemComponent()
            : m_patternMatcher("_touchbend", AZ::SceneAPI::SceneCore::PatternMatcher::MatchApproach::PostFix),
              m_virtualType("TouchBend"),
              m_includeChildren(true)
        {

        }

        void TouchBendingSceneSystemComponent::Activate()
        {
            //Add our Virtual Type Softname.
            bool success = false;
            EBUS_EVENT_RESULT(success, AZ::SceneProcessingConfig::SceneProcessingConfigRequestBus, AddNodeSoftName,
                m_patternMatcher.GetPattern().c_str(), m_patternMatcher.GetMatchApproach(), m_virtualType.c_str(), m_includeChildren);
            if (!success)
            {
                AZ_Error("TouchBending::Pipeline", false, "Failed to add virtual type");
            }
        }

        void TouchBendingSceneSystemComponent::Deactivate()
        {
        }

        void TouchBendingSceneSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<TouchBendingSceneSystemComponent, AZ::SceneAPI::SceneCore::SceneSystemComponent>()->Version(1)
                    ->Field("pattern", &TouchBendingSceneSystemComponent::m_patternMatcher)
                    ->Field("virtualType", &TouchBendingSceneSystemComponent::m_virtualType)
                    ->Field("includeChildren", &TouchBendingSceneSystemComponent::m_includeChildren);

                if (AZ::EditContext* ec = serializeContext->GetEditContext())
                {
                    ec->Class<TouchBendingSceneSystemComponent>("TouchBending Scene Processing Config", "Use this component to fine tune the defaults for processing of scene files like Fbx with TouchBendable Meshes.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Assets")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TouchBendingSceneSystemComponent::m_patternMatcher,
                            "Soft naming convention", "Update the naming convention to suit your project.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TouchBendingSceneSystemComponent::m_virtualType,
                            "Virtual Type",
                            "The node(s) will be converted to this type after their pattern matches. Read Only.")
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TouchBendingSceneSystemComponent::m_includeChildren,
                            "Include child nodes",
                            "Whether or not the soft name only applies to the matching node or propagated to all its children as well.")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, false);
                }
            }
        }

        void TouchBendingSceneSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("SceneProcessingConfigService", 0x7b333b47));
        }

    } // Pipeline
} // TouchBending