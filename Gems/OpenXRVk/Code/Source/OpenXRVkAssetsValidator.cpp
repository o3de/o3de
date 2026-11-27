/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/regex.h>

#include <OpenXRVk/OpenXRVkAssetsValidator.h>

namespace OpenXRVkAssetsValidator
{
    ///////////////////////////////////////////////////////////////////////////
    // Asset Validation Common Start
    // A regular Name is just a utf-8 string, it can contain upper case letters
    // and spaces in between. But it can not be empty, and can not contain leading to trailing spaces.
    static AZ::Outcome<void, AZStd::string> ValidateName(const AZStd::string& name)
    {
        if (name.empty())
        {
            return AZ::Failure("Name should not be empty.");
        }
        //Spaces at the beginning and end of the name are not allowed.
        AZStd::string tmpName(name);
        AZ::StringFunc::TrimWhiteSpace(tmpName, true, true);
        if (tmpName.empty())
        {
            return AZ::Failure("Name is just a bunch of spaces.");
        }
        if (tmpName.size() != name.size())
        {
            return AZ::Failure(
                AZStd::string::format("Trailing or leading spaces are not allowed in a Name [%s].",
                    name.c_str())
            );
        }
        return AZ::Success();
    }


    // Unlike an OpenXR Name, a Localized Name is just a utf-8 string, it can contain upper case letters
    // and spaces in between. But it can not be empty, and can not contain leading to trailing spaces.
    static AZ::Outcome<void, AZStd::string> ValidateOpenXRLocalizedName(const AZStd::string& name)
    {
        auto outcome = ValidateName(name);
        if (!outcome.IsSuccess())
        {
            return AZ::Failure(
                AZStd::string::format("Localized Name [%s] is invalid. Reason:\n%s", name.c_str(), outcome.GetError().c_str())
            );
        }
        return AZ::Success();
    }

    // Asset Validation Common End
    ///////////////////////////////////////////////////////////////////////////


    ///////////////////////////////////////////////////////////////////////////
    // OpenXRInteractionProfilesAsset Validation Start
    static AZ::Outcome<void, AZStd::string> ValidateActionTypeString(const AZStd::string& actionTypeStr)
    {
        using CPD = OpenXRVk::OpenXRInteractionComponentPathDescriptor;
        static const AZStd::unordered_set<AZStd::string> ValidActionTypes{
            {CPD::s_TypeBoolStr},
            {CPD::s_TypeFloatStr},
            {CPD::s_TypeVector2Str},
            {CPD::s_TypePoseStr},
            {CPD::s_TypeVibrationStr}
        };

        if (!ValidActionTypes.contains(actionTypeStr))
        {
            static AZStd::string ValidListStr;
            if (ValidListStr.empty())
            {
                ValidListStr += "[ ";
                for (const auto& validActionTypeStr : ValidActionTypes)
                {
                    if (!ValidListStr.empty())
                    {
                        ValidListStr += ", ";
                    }
                    ValidListStr += validActionTypeStr;
                }
                ValidListStr += " ]";
            }
            return AZ::Failure(
                AZStd::string::format("Action Type [%s] is invalid. It can only be one of %s",
                    actionTypeStr.c_str(), ValidListStr.c_str())
            );
        }

        return AZ::Success();
    }


    // An OpenXR path string only contain characters as described here
    // https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#well-formed-path-strings
    static AZ::Outcome<void, AZStd::string> ValidateOpenXRPath(const AZStd::string& path)
    {
        static AZStd::regex s_validCharactersRegEx(R"(^(/[a-z0-9\-_\.]+)+$)", AZStd::regex::ECMAScript);
        if (!AZStd::regex_match(path, s_validCharactersRegEx))
        {
            return AZ::Failure(
                AZStd::string::format("The path [%s] contains an invalid character, or is missing a leading '/' or contains a leading '/'", path.c_str())
            );
        }
        return AZ::Success();
    }


    static AZ::Outcome<void, AZStd::string> ValidateComponentPathDescriptor(const OpenXRVk::OpenXRInteractionComponentPathDescriptor& componentPathDescriptor,
        AZStd::unordered_set<AZStd::string>& uniqueComponentPathNames, AZStd::unordered_set<AZStd::string>& uniqueComponentPathPaths)
    {
        {
            if (uniqueComponentPathNames.contains(componentPathDescriptor.m_name))
            {
                return AZ::Failure(
                    AZStd::string::format("A Component Path with name [%s] already exists.",
                        componentPathDescriptor.m_name.c_str())
                );
            }
            uniqueComponentPathNames.emplace(componentPathDescriptor.m_name);
            auto outcome = ValidateName(componentPathDescriptor.m_name);
            if (!outcome.IsSuccess())
            {
                return AZ::Failure(
                    AZStd::string::format("Component Name[%s] is invalid.Reason:\n%s", componentPathDescriptor.m_name.c_str(), outcome.GetError().c_str())
                );
            }
        }
        {
            if (uniqueComponentPathPaths.contains(componentPathDescriptor.m_path))
            {
                return AZ::Failure(
                    AZStd::string::format("A Component Path with path [%s] already exists.",
                        componentPathDescriptor.m_path.c_str())
                );
            }
            uniqueComponentPathPaths.emplace(componentPathDescriptor.m_path);
            auto outcome = ValidateOpenXRPath(componentPathDescriptor.m_path);
            if (!outcome.IsSuccess())
            {
                return AZ::Failure(
                    AZStd::string::format("Component Path path [%s] is invalid. Reason:\n%s", componentPathDescriptor.m_path.c_str(), outcome.GetError().c_str())
                );
            }
        }

        auto outcome = ValidateActionTypeString(componentPathDescriptor.m_actionTypeStr);
        if (!outcome.IsSuccess())
        {
            return AZ::Failure(
                AZStd::string::format("Component Path path [%s] has an invalid action type. Reason:\n%s", componentPathDescriptor.m_name.c_str(), outcome.GetError().c_str())
            );
        }

        return AZ::Success();
    }


    static AZ::Outcome<void, AZStd::string> ValidateUserPathDescriptor(const OpenXRVk::OpenXRInteractionUserPathDescriptor& userPathDescriptor,
        AZStd::unordered_set<AZStd::string>& uniqueUserPathNames, AZStd::unordered_set<AZStd::string>& uniqueUserPathPaths,
        AZStd::unordered_set<AZStd::string>& uniqueComponentPathNames, AZStd::unordered_set<AZStd::string>& uniqueComponentPathPaths)
    {
        {
            if (uniqueUserPathNames.contains(userPathDescriptor.m_name))
            {
                return AZ::Failure(
                    AZStd::string::format("An User Path with name [%s] already exists.",
                        userPathDescriptor.m_name.c_str())
                );
            }
            uniqueUserPathNames.emplace(userPathDescriptor.m_name);
            auto outcome = ValidateName(userPathDescriptor.m_name);
            if (!outcome.IsSuccess())
            {
                return AZ::Failure(
                    AZStd::string::format("User Path Name [%s] is invalid. Reason:\n%s", userPathDescriptor.m_name.c_str(), outcome.GetError().c_str())
                );
            }
        }
        {
            if (uniqueUserPathPaths.contains(userPathDescriptor.m_path))
            {
                return AZ::Failure(
                    AZStd::string::format("User Path with path [%s] already exists.",
                        userPathDescriptor.m_path.c_str())
                );
            }
            uniqueUserPathPaths.emplace(userPathDescriptor.m_path);
            auto outcome = ValidateOpenXRPath(userPathDescriptor.m_path);
            if (!outcome.IsSuccess())
            {
                return AZ::Failure(
                    AZStd::string::format("User Path path [%s] is invalid. Reason:\n%s", userPathDescriptor.m_path.c_str(), outcome.GetError().c_str())
                );
            }
        }
        for (const auto& componentPathDescriptor : userPathDescriptor.m_componentPathDescriptors)
        {
            auto outcome = ValidateComponentPathDescriptor(componentPathDescriptor,
                uniqueComponentPathNames, uniqueComponentPathPaths);
            if (!outcome.IsSuccess())
            {
                return AZ::Failure(
                    AZStd::string::format("Invalid Component Path [%s]. Reason:\n%s", componentPathDescriptor.m_name.c_str(), outcome.GetError().c_str())
                );
            }
        }

        return AZ::Success();
    }


    static AZ::Outcome<void, AZStd::string> ValidateInteractionProfileDescriptor(
        const OpenXRVk::OpenXRInteractionProfileDescriptor& interactionProfileDescriptor,
        AZStd::unordered_set<AZStd::string>& uniqueNames, AZStd::unordered_set<AZStd::string>& uniquePaths)
    {
        {
            if (uniqueNames.contains(interactionProfileDescriptor.m_name))
            {
                return AZ::Failure(
                    AZStd::string::format("Interaction Profile with name [%s] already exists.",
                        interactionProfileDescriptor.m_name.c_str())
                );
            }
            uniqueNames.emplace(interactionProfileDescriptor.m_name);
            auto outcome = ValidateName(interactionProfileDescriptor.m_name);
            if (!outcome.IsSuccess())
            {
                return AZ::Failure(
                    AZStd::string::format("Interaction Profile Unique Name [%s] is invalid. Reason:\n%s", interactionProfileDescriptor.m_name.c_str(), outcome.GetError().c_str())
                );
            }
        }

        {
            if (uniquePaths.contains(interactionProfileDescriptor.m_path))
            {
                return AZ::Failure(
                    AZStd::string::format("Interaction Profile with path [%s] already exists.",
                        interactionProfileDescriptor.m_path.c_str())
                );
            }
            uniquePaths.emplace(interactionProfileDescriptor.m_path);
            auto outcome = ValidateOpenXRPath(interactionProfileDescriptor.m_path);
            if (!outcome.IsSuccess())
            {
                return AZ::Failure(
                    AZStd::string::format("Interaction Profile Path [%s] is invalid. Reason:\n%s", interactionProfileDescriptor.m_path.c_str(), outcome.GetError().c_str())
                );
            }
        }

        AZStd::unordered_set<AZStd::string> uniqueUserPathNames;
        AZStd::unordered_set<AZStd::string> uniqueUserPathPaths;
        AZStd::unordered_set<AZStd::string> uniqueComponentPathNames;
        AZStd::unordered_set<AZStd::string> uniqueComponentPathPaths;
        for (const auto& userPathDescriptor : interactionProfileDescriptor.m_userPathDescriptors)
        {
            auto outcome = ValidateUserPathDescriptor(userPathDescriptor,
                uniqueUserPathNames, uniqueUserPathPaths, uniqueComponentPathNames, uniqueComponentPathPaths);
            if (!outcome.IsSuccess())
            {
                return AZ::Failure(
                    AZStd::string::format("Invalid User Path [%s]. Reason:\n%s", userPathDescriptor.m_name.c_str(), outcome.GetError().c_str())
                );
            }
        }

        for (const auto& componentPathDescriptor : interactionProfileDescriptor.m_commonComponentPathDescriptors)
        {
            auto outcome = ValidateComponentPathDescriptor(componentPathDescriptor,
                uniqueComponentPathNames, uniqueComponentPathPaths);
            if (!outcome.IsSuccess())
            {
                return AZ::Failure(
                    AZStd::string::format("Invalid Common Component Path [%s]. Reason:\n%s", componentPathDescriptor.m_name.c_str(), outcome.GetError().c_str())
                );
            }
        }

        return AZ::Success();
    }


    AZ::Outcome<void, AZStd::string> ValidateInteractionProfilesAsset(
        const OpenXRVk::OpenXRInteractionProfilesAsset& interactionProfilesAsset)
    {
        if (interactionProfilesAsset.m_interactionProfileDescriptors.empty())
        {
            return AZ::Failure("InteractionProfiles asset requires at least one Interaction Profile.");
        }

        AZStd::unordered_set<AZStd::string> uniqueNames;
        AZStd::unordered_set<AZStd::string> uniquePaths;
        uint32_t i = 0;
        for (const auto& interactionProfileDescriptor : interactionProfilesAsset.m_interactionProfileDescriptors)
        {
            auto outcome = ValidateInteractionProfileDescriptor(interactionProfileDescriptor,
                uniqueNames, uniquePaths);
            if (!outcome.IsSuccess())
            {
                return AZ::Failure(
                    AZStd::string::format("InteractionProfile[%u] is invalid. Reason:\n%s",
                        i, outcome.GetError().c_str())
                );
            }
            i++;
        }
        return AZ::Success();
    }
    // OpenXRInteractionProfilesAsset Validation End
    ///////////////////////////////////////////////////////////////////////////


    ///////////////////////////////////////////////////////////////////////////
    // OpenXRActionSetsAsset Validation Start
    static AZ::Outcome<void, AZStd::string> ValidateActionPathDescriptor(const OpenXRVk::OpenXRActionPathDescriptor& actionPathDescriptor,
        const OpenXRVk::OpenXRInteractionProfilesAsset& interactionProfilesAsset,
        AZStd::unordered_set<AZStd::string>& uniqueActionPaths)
    {
        auto concatenatedActionPath = actionPathDescriptor.m_interactionProfileName
            + actionPathDescriptor.m_userPathName
            + actionPathDescriptor.m_componentPathName;
        if (uniqueActionPaths.contains(concatenatedActionPath))
        {
            return AZ::Failure(
                AZStd::string::format("An Action Path with profile[%s], userPath[%s], componentPath[%s] already exists.",
                    actionPathDescriptor.m_interactionProfileName.c_str(),
                    actionPathDescriptor.m_userPathName.c_str(),
                    actionPathDescriptor.m_componentPathName.c_str())
            );
        }
        uniqueActionPaths.emplace(AZStd::move(concatenatedActionPath));

        if (actionPathDescriptor.m_interactionProfileName.empty())
        {
            return AZ::Failure(
                AZStd::string::format("ActionPath Descriptor must have an InteractionProfile name.")
            );
        }
        const auto interactionProfileDescriptorPtr = interactionProfilesAsset.GetInteractionProfileDescriptor(actionPathDescriptor.m_interactionProfileName);
        if (!interactionProfileDescriptorPtr)
        {
            return AZ::Failure(
                AZStd::string::format("Unknown Interaction Profile Descriptor named [%s].",
                    actionPathDescriptor.m_interactionProfileName.c_str())
            );
        }

        if (actionPathDescriptor.m_userPathName.empty())
        {
            return AZ::Failure(
                AZStd::string::format("ActionPath Descriptor must have an UserPath name.")
            );
        }
        const auto userPathDescriptorPtr = interactionProfileDescriptorPtr->GetUserPathDescriptor(actionPathDescriptor.m_userPathName);
        if (!userPathDescriptorPtr)
        {
            return AZ::Failure(
                AZStd::string::format("Unknown UserPath descriptor named [%s].",
                    actionPathDescriptor.m_userPathName.c_str())
            );
        }

        if (actionPathDescriptor.m_componentPathName.empty())
        {
            return AZ::Failure(
                AZStd::string::format("ActionPath Descriptor must have a ComponentPath name.")
            );
        }
        const auto componentPathDescriptorPtr = interactionProfileDescriptorPtr->GetComponentPathDescriptor(*userPathDescriptorPtr, actionPathDescriptor.m_componentPathName);
        if (!componentPathDescriptorPtr)
        {
            return AZ::Failure(
                AZStd::string::format("Unknown ComponentPath descriptor named [%s].",
                    actionPathDescriptor.m_componentPathName.c_str())
            );
        }

        return AZ::Success();
    }


    static const AZStd::string& GetActionTypeStringFromActionPathDescriptor(
        const OpenXRVk::OpenXRInteractionProfilesAsset& interactionProfilesAsset,
        const OpenXRVk::OpenXRActionPathDescriptor& actionPathDescriptor
    )
    {
        return interactionProfilesAsset.GetActionPathTypeStr(
            actionPathDescriptor.m_interactionProfileName,
            actionPathDescriptor.m_userPathName,
            actionPathDescriptor.m_componentPathName
        );
    }


    static bool IsActionTypeBoolOrFloat(const AZStd::string& actionTypeStr)
    {
        return (
            (actionTypeStr == OpenXRVk::OpenXRInteractionComponentPathDescriptor::s_TypeBoolStr) ||
            (actionTypeStr == OpenXRVk::OpenXRInteractionComponentPathDescriptor::s_TypeFloatStr)
            );
    }


    static bool AreCompatibleActionTypeStrings(const AZStd::string& lhs, const AZStd::string& rhs)
    {
        if (IsActionTypeBoolOrFloat(lhs) && IsActionTypeBoolOrFloat(rhs))
        {
            return true;
        }
        return (lhs == rhs);
    }


    // An OpenXR name string only contain characters which are allowed in a SINGLE LEVEL of a well-formed path string
    // https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#well-formed-path-strings
    static AZ::Outcome<void, AZStd::string> ValidateOpenXRName(const AZStd::string& name)
    {
        static AZStd::regex s_validCharactersRegEx(R"(^[a-z0-9\-_\.]+$)", AZStd::regex::ECMAScript);
        if (!AZStd::regex_match(name, s_validCharactersRegEx))
        {
            return AZ::Failure(
                AZStd::string::format("The name [%s] contains an invalid character", name.c_str())
            );
        }
        return AZ::Success();
    }


    static AZ::Outcome<void, AZStd::string> ValidateActionDescriptor(
        const OpenXRVk::OpenXRInteractionProfilesAsset& interactionProfilesAsset,
        const OpenXRVk::OpenXRActionDescriptor& actionDescriptor,
        AZStd::unordered_set<AZStd::string>& uniqueActionNames,
        AZStd::unordered_set<AZStd::string>& uniqueActionLocalizedNames)
    {
        {
            if (uniqueActionNames.contains(actionDescriptor.m_name))
            {
                return AZ::Failure(
                    AZStd::string::format("An Action with name [%s] already exists.",
                        actionDescriptor.m_name.c_str())
                );
            }
            uniqueActionNames.emplace(actionDescriptor.m_name);
            auto outcome = ValidateOpenXRName(actionDescriptor.m_name);
            if (!outcome.IsSuccess())
            {
                return AZ::Failure(
                    AZStd::string::format("Failed to validate Action Descriptor named=[%s].\nReason:\n%s",
                        actionDescriptor.m_name.c_str(), outcome.GetError().c_str())
                );
            }
        }

        // Only validate if not empty. If empty, the asset builder will force this to be a copy of
        // actionDescriptor.m_name.
        if (!actionDescriptor.m_localizedName.empty())
        {
            if (uniqueActionLocalizedNames.contains(actionDescriptor.m_localizedName))
            {
                return AZ::Failure(
                    AZStd::string::format("An Action with localized name [%s] already exists.",
                        actionDescriptor.m_localizedName.c_str())
                );
            }
            uniqueActionLocalizedNames.emplace(actionDescriptor.m_localizedName);
            auto outcome = ValidateOpenXRLocalizedName(actionDescriptor.m_localizedName);
            if (!outcome.IsSuccess())
            {
                return AZ::Failure(
                    AZStd::string::format("Failed to validate localized name of Action Descriptor named=[%s]\nReason:\n%s",
                        actionDescriptor.m_name.c_str(), outcome.GetError().c_str())
                );
            }
        }


        if (actionDescriptor.m_actionPathDescriptors.empty())
        {
            return AZ::Failure(
                AZStd::string::format("At least one ActionPath Descriptor is required by Action Descriptor named=[%s].\n",
                    actionDescriptor.m_name.c_str())
            );
        }

        AZStd::unordered_set<AZStd::string> uniqueActionPaths;

        // It is very important that all action path descriptors have compatible data types.
        const AZStd::string& firstActionTypeStr = GetActionTypeStringFromActionPathDescriptor(
            interactionProfilesAsset, actionDescriptor.m_actionPathDescriptors[0]);
        uint32_t actionPathIndex = 0;
        for (const auto& actionPathDescriptor : actionDescriptor.m_actionPathDescriptors)
        {
            auto outcome = ValidateActionPathDescriptor(actionPathDescriptor, interactionProfilesAsset, uniqueActionPaths);
            if (!outcome.IsSuccess())
            {
                return AZ::Failure(
                    AZStd::string::format("Failed to validate Action Path Descriptor for Action Descriptor named=[%s].\nReason:\n%s",
                        actionDescriptor.m_name.c_str(), outcome.GetError().c_str())
                );
            }
            const AZStd::string& actionTypeStr = GetActionTypeStringFromActionPathDescriptor(
                interactionProfilesAsset, actionPathDescriptor);
            if (!AreCompatibleActionTypeStrings(firstActionTypeStr, actionTypeStr))
            {
                return AZ::Failure(
                    AZStd::string::format("ActionType=[%s] of ActionPath Descriptor[%u] is NOT compatible with the ActionType=[%s] ActionPath Descriptor[0]",
                        actionTypeStr.c_str(), actionPathIndex, firstActionTypeStr.c_str())
                );
            }
            actionPathIndex++;
        }

        return AZ::Success();
    }


    AZ::Outcome<void, AZStd::string> ValidateActionSetsAsset(const OpenXRVk::OpenXRActionSetsAsset& actionSetsAsset,
        const OpenXRVk::OpenXRInteractionProfilesAsset& interactionProfilesAsset)
    {
        if (actionSetsAsset.m_actionSetDescriptors.empty())
        {
            return AZ::Failure("At least one ActionSet must be listed in an ActionSets asset");
        }

        AZStd::unordered_set<AZStd::string> uniqueActionSetNames;
        AZStd::unordered_set<AZStd::string> uniqueActionSetLocalizedNames;
        for (const auto& actionSetDescriptor : actionSetsAsset.m_actionSetDescriptors)
        {
            {
                if (uniqueActionSetNames.contains(actionSetDescriptor.m_name))
                {
                    return AZ::Failure(
                        AZStd::string::format("An ActionSet named=[%s] already exists.",
                            actionSetDescriptor.m_name.c_str())
                    );
                }
                uniqueActionSetNames.emplace(actionSetDescriptor.m_name);
                auto outcome = ValidateOpenXRName(actionSetDescriptor.m_name);
                if (!outcome.IsSuccess())
                {
                    return AZ::Failure(
                        AZStd::string::format("Failed to validate ActionSet Descriptor name=[%s]. Reason:\n%s",
                            actionSetDescriptor.m_name.c_str(), outcome.GetError().c_str())
                    );
                }
            }

            // Only validate if not empty. If empty, the asset builder will force this to be a copy of
            // actionSetDescriptor.m_name.
            if (!actionSetDescriptor.m_localizedName.empty())
            {
                if (uniqueActionSetLocalizedNames.contains(actionSetDescriptor.m_localizedName))
                {
                    return AZ::Failure(
                        AZStd::string::format("An ActionSet with localized named=[%s] already exists.",
                            actionSetDescriptor.m_localizedName.c_str())
                    );
                }
                uniqueActionSetLocalizedNames.emplace(actionSetDescriptor.m_localizedName);
                auto outcome = ValidateOpenXRLocalizedName(actionSetDescriptor.m_localizedName);
                if (!outcome.IsSuccess())
                {
                    return AZ::Failure(
                        AZStd::string::format("Failed to validate ActionSet Descriptor name=[%s]. Reason:\n%s",
                            actionSetDescriptor.m_name.c_str(), outcome.GetError().c_str())
                    );
                }
            }

            if (actionSetDescriptor.m_actionDescriptors.empty())
            {
                return AZ::Failure(
                    AZStd::string::format("ActionSet [%s] must contain at least one ActionDescriptor.",
                        actionSetDescriptor.m_name.c_str())
                );
            }

            AZStd::unordered_set<AZStd::string> uniqueActionNames;
            AZStd::unordered_set<AZStd::string> uniqueActionLocalizedNames;
            for (const auto& actionDescriptor : actionSetDescriptor.m_actionDescriptors)
            {
                auto outcome = ValidateActionDescriptor(interactionProfilesAsset, actionDescriptor,
                    uniqueActionNames, uniqueActionLocalizedNames);
                if (!outcome.IsSuccess())
                {
                    return AZ::Failure(
                        AZStd::string::format("Failed to validate ActionSet Descriptor name=[%s]. Reason:\n%s",
                            actionSetDescriptor.m_name.c_str(), outcome.GetError().c_str())
                    );
                }
            }
        }

        return AZ::Success();
    }
    // OpenXRActionSetsAsset Validation End
    ///////////////////////////////////////////////////////////////////////////


} // namespace OpenXRVkAssetsValidator
