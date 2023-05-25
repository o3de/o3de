/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>

#include <MCore/Source/Command.h>

namespace AZ
{
    class ReflectContext;
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }

        namespace DataTypes
        {
            class IGroup;
        }
    }
}

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            class MetaDataRule
                : public AZ::SceneAPI::DataTypes::IRule
            {
            public:
                AZ_RTTI(MetaDataRule, "{8D759063-7D2E-4543-8EB3-AB510A5886CF}", AZ::SceneAPI::DataTypes::IRule);
                AZ_CLASS_ALLOCATOR(MetaDataRule, AZ::SystemAllocator)

                MetaDataRule() = default;
                MetaDataRule(const AZStd::string& metaData);
                MetaDataRule(const AZStd::vector<MCore::Command*> metaData);
                ~MetaDataRule() override;

                /**
                 * Get the string containing the list of commands representing the changes the user did on the source asset.
                 * @result A string called meta data containing a list of commands.
                 */
                template <typename T>
                const T& GetMetaData() const;

                /**
                 * Set the meta data string which contains a list of commands representing the changes the user did on the source asset.
                 * This string can be constructed using CommandSystem::GenerateActorMetaData().
                 * @param metaData The meta data string containing a list of commands to be applied on the source asset.
                 */
                void SetMetaData(const AZStd::string& metaData);
                void SetMetaData(const AZStd::vector<MCore::Command*>& metaData);

                /**
                 * Get the meta data string from the group. Search the rule container of the given group for a meta data rule and read out the meta data string.
                 * @param group The group to search for the meta data.
                 * @param[out] outMetaDataString Holds the meta data string after the operation has been completed. String will be empty in case something failed or there is no meta data.
                 */
                template <typename T>
                static bool LoadMetaData(const AZ::SceneAPI::DataTypes::IGroup& group, T& outMetaDataString);

                /**
                 * Set the meta data string to the given group. Search the rule container of the given group for a meta data rule, create one in case there is none yet
                 * and set the given meta data string to the rule.
                 * @param group The group to set the meta data for.
                 * @param metaDataString The meta data string to set. In case the string is empty, any existing meta data rule will be removed.
                 */
                template <typename T>
                static void SaveMetaData(AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IGroup& group, const T& metaData);

                template<typename T, typename MetaDataType>
                static bool SaveMetaDataToFile(const AZStd::string& sourceAssetFilename, const AZStd::string& groupName, const MetaDataType& metaDataString);

                template<typename T, typename MetaDataType>
                static bool SaveMetaDataToFile(const AZStd::string& sourceAssetFilename, const AZStd::string& groupName, const MetaDataType& metaDataString, AZStd::string& outResult);

                static void Reflect(AZ::ReflectContext* context);

            protected:
                AZStd::string m_metaData;
                AZStd::vector<MCore::Command*> m_commands;
            };

        } // Rule
    } // Pipeline
} // EMotionFX
#include <SceneAPIExt/Rules/MetaDataRule.inl>
