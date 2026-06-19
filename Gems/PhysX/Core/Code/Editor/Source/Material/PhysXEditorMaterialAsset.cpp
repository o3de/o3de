/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <Editor/Source/Material/PhysXEditorMaterialAsset.h>

namespace PhysX
{
    namespace Internal
    {
        // TODO: successfully converts older assets set in material slots during runtime, but does not convert when opened in Asset Editor
        bool MaterialAssetDataConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // We're not going to try to convert legacy assets right now
            // Just the latest materials before PhysX 5.6.1
            if (classElement.GetVersion() != 3)
            {
                return false;
            }

            const int configurationIdx = classElement.FindElement(AZ_CRC_CE("MaterialConfiguration"));
            if (configurationIdx >= 0)
            {
                auto& configurationElement = classElement.GetSubElement(configurationIdx);

                // Explicit control of Compliant Contact was removed in PhysX 5.6.1 and is instead enabled by negative restitution
                // This is the only change to material assets from version 2
                const int compliantContactModeIdx = configurationElement.FindElement(AZ_CRC_CE("CompliantContactMode"));
                const int restitutionIdx = configurationElement.FindElement(AZ_CRC_CE("Restitution"));
                if (compliantContactModeIdx >= 0 && restitutionIdx >= 0)
                {
                    auto& compliantContactModeElement = configurationElement.GetSubElement(compliantContactModeIdx);
                    auto& restitutionElement = configurationElement.GetSubElement(restitutionIdx);
                    if (compliantContactModeElement.GetVersion() < 2)
                    {
                        const int enabledIdx = compliantContactModeElement.FindElement(AZ_CRC_CE("Enabled"));
                        const int stiffnessIdx = compliantContactModeElement.FindElement(AZ_CRC_CE("Stiffness"));
                        if (enabledIdx >= 0 && stiffnessIdx >= 0)
                        {
                            auto& enabledElement = compliantContactModeElement.GetSubElement(enabledIdx);
                            auto& stiffnessElement = compliantContactModeElement.GetSubElement(stiffnessIdx);
                            bool enabled = false;
                            if (enabledElement.GetData(enabled)) // Set restitution to negative stiffness if it is enabled and remove switch
                            {
                                if (enabled)
                                {
                                    float stiffness = 0.0f;
                                    if (stiffnessElement.GetData(stiffness))
                                    {
                                        restitutionElement.SetData(context, -stiffness); //TODO: not triggered in editor
                                    }
                                }
                                compliantContactModeElement.RemoveElementByName(AZ_CRC_CE("Enabled"));
                            }
                        }
                    }
                }
            }

            return true;
        }
    }

    void EditorMaterialAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PhysX::EditorMaterialAsset, AZ::Data::AssetData>()
                ->Version(3, Internal::MaterialAssetDataConverter)
                ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                ->Field("MaterialConfiguration", &EditorMaterialAsset::m_materialConfiguration);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PhysX::EditorMaterialAsset>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorMaterialAsset::m_materialConfiguration, "PhysX Material",
                        "PhysX material properties")
                        ->Attribute(AZ::Edit::Attributes::ForceAutoExpand, true);
            }
        }
    }

    const MaterialConfiguration& EditorMaterialAsset::GetMaterialConfiguration() const
    {
        return m_materialConfiguration;
    }
} // namespace PhysX
