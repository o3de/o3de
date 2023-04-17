/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneData/Rules/UVsRule.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            static constexpr AZStd::string_view DefaultUVsGenerationMethodKeyIfNoRulePresent{
                "/O3DE/SceneAPI/UVsGenerateComponent/DefaultGenerationMethodIfNoRulePresent"
            };

            static constexpr AZStd::string_view DefaultUVsGenerationMethodKeyWhenAddingNewRules{
                "/O3DE/SceneAPI/UVsGenerateComponent/DefaultGenerationMethodWhenRuleIsPresent"
            };


            UVsRule::UVsRule()
                : DataTypes::IRule()
            {
                m_generationMethod = GetDefaultGenerationMethodWhenAddingNewRule();
            }

            AZ::SceneAPI::DataTypes::UVsGenerationMethod GetGenerationMethodFromRegistry(
                AZStd::string_view regKey, AZ::SceneAPI::DataTypes::UVsGenerationMethod defaultValue)
            {
                if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
                {
                    AZStd::string stringFromRegistry;
                    if (settingsRegistry->Get(stringFromRegistry, regKey))
                    {
                        const bool isCaseSensitive = false;
                        if (AZ::StringFunc::Equal(stringFromRegistry, "LeaveSceneDataAsIs", isCaseSensitive))
                        {
                            return AZ::SceneAPI::DataTypes::UVsGenerationMethod::LeaveSceneDataAsIs;
                        }
                        else if (AZ::StringFunc::Equal(stringFromRegistry, "SphericalProjection", isCaseSensitive))
                        {
                            return AZ::SceneAPI::DataTypes::UVsGenerationMethod::SphericalProjection;
                        }
                        else
                        {
                            AZ_Warning(
                                AZ::SceneAPI::Utilities::WarningWindow,
                                false,
                                "'%s' is not a valid default UV generation method. Check the value of " AZ_STRING_FORMAT " in your "
                                "settings registry, and change "
                                "it to 'LeaveSceneDataAsIs' or 'SphericalProjection'",
                                stringFromRegistry.c_str(),
                                AZ_STRING_ARG(regKey));
                        }
                    }
                }
                return defaultValue;
            }

            //! return the default method for when a new rule is explicitly created by script or user
            AZ::SceneAPI::DataTypes::UVsGenerationMethod UVsRule::GetDefaultGenerationMethodWhenAddingNewRule()
            {
                // When someone goes to the effort of actually adding a new rule, make the default actually do something
                return GetGenerationMethodFromRegistry(DefaultUVsGenerationMethodKeyWhenAddingNewRules,
                    AZ::SceneAPI::DataTypes::UVsGenerationMethod::SphericalProjection);
            }

            AZ::SceneAPI::DataTypes::UVsGenerationMethod UVsRule::GetDefaultGenerationMethodWithNoRule()
            {
                // when there is no rule on the mesh, do nothing by default
                return GetGenerationMethodFromRegistry(DefaultUVsGenerationMethodKeyIfNoRulePresent,
                    AZ::SceneAPI::DataTypes::UVsGenerationMethod::LeaveSceneDataAsIs);
            }

            AZ::SceneAPI::DataTypes::UVsGenerationMethod UVsRule::GetGenerationMethod() const
            {
                return m_generationMethod;
            }

            bool UVsRule::GetReplaceExisting() const
            {
                return m_replaceExisting;
            }

            void UVsRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<UVsRule, DataTypes::IRule>()
                    ->Version(1)
                    ->Field("generationMethod", &UVsRule::m_generationMethod)
                    ->Field("replaceExisting", &UVsRule::m_replaceExisting);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<UVsRule>("UVs", "Specify how UVs are imported or generated.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute("AutoExpand", true)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(
                            AZ::Edit::UIHandlers::ComboBox,
                            &AZ::SceneAPI::SceneData::UVsRule::m_generationMethod,
                            "Generation Method",
                            "Specify the UVs generation method when UVs are generated.")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::UVsGenerationMethod::LeaveSceneDataAsIs, "Do not generate UVs")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::UVsGenerationMethod::SphericalProjection, "Spherical Projection")
                        ->DataElement(0, &AZ::SceneAPI::SceneData::UVsRule::m_replaceExisting,
                            "Replace existing UVs",
                            "If true, will replace UVs in the source scene even if present in the incoming data.")
                    ;
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ
