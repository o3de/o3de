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

#include <AzToolsFramework/Prefab/Spawnable/ComponentRequirementsValidator.h>

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/algorithm.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    void ComponentRequirementsValidator::SetPlatformTags(AZ::PlatformTagSet platformTags)
    {
        m_platformTags = AZStd::move(platformTags);
    }


    void ComponentRequirementsValidator::SetEntities(const AZStd::vector<AZ::Entity*>& entities)
    {
        m_immutableEntities.clear();
        m_immutableEntities.assign(entities.cbegin(), entities.cend());
    }

    ComponentRequirementsValidator::ValidationResult ComponentRequirementsValidator::Validate(
        const AZ::Component* component)
    {
        AZ::ComponentValidationResult result = component->ValidateComponentRequirements(m_immutableEntities, m_platformTags);
        if (!result.IsSuccess())
        {
            // Try to cast to GenericComponentWrapper, and if we can, get the internal template.
            const char* componentName = component->RTTI_GetTypeName();
            const auto* asEditorComponent = azrtti_cast<const Components::EditorComponentBase*>(component);
            const Components::GenericComponentWrapper* wrapper = azrtti_cast<const Components::GenericComponentWrapper*>(asEditorComponent);
            if (wrapper && wrapper->GetTemplate())
            {
                componentName = wrapper->GetTemplate()->RTTI_GetTypeName();
            }

            return AZ::Failure(AZStd::string::format(
                "Editor Component '%s' could not pass validation due to the error - %s",
                componentName,
                result.GetError().c_str())
            );
        }

        return AZ::Success();
    }

} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
