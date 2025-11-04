/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPIExt/Rules/MetaDataRule.h>

#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/MotionEventCommands.h>
#include <EMotionFX/Source/TwoStringEventData.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            MetaDataRule::MetaDataRule(const AZStd::string& metaData)
                : IRule()
                , m_metaData(metaData)
            {
            }

            MetaDataRule::MetaDataRule(AZStd::vector<MCore::Command*> metaData)
                : IRule()
                , m_commands(metaData)
            {
            }

            MetaDataRule::~MetaDataRule()
            {
                for (MCore::Command* command : m_commands)
                {
                    delete command;
                }
                m_commands.clear();
            }

            template <>
            const AZStd::string& MetaDataRule::GetMetaData<const AZStd::string&>() const
            {
                return m_metaData;
            }

            template <>
            const AZStd::vector<MCore::Command*>& MetaDataRule::GetMetaData<const AZStd::vector<MCore::Command*>&>() const
            {
                return m_commands;
            }


            void MetaDataRule::SetMetaData(const AZStd::string& metaData)
            {
                m_metaData = metaData;
            }

            void MetaDataRule::SetMetaData(const AZStd::vector<MCore::Command*>& metaData)
            {
                m_commands = metaData;
            }

            bool MetaDataRuleConverter(AZ::SerializeContext & serializeContext, AZ::SerializeContext::DataElementNode & rootElementNode)
            {
                if (rootElementNode.GetVersion() >= 2)
                {
                    return false;
                }

                AZ::SerializeContext::DataElementNode* metaDataStringNode = rootElementNode.FindSubElement(AZ_CRC_CE("metaData"));
                AZStd::vector<AZStd::string> commands;
                if (metaDataStringNode)
                {
                    AZStd::string metaDataString;
                    metaDataStringNode->GetData(metaDataString);

                    // Replace all object id string occurrences (e.g.
                    // $(MOTIONID)) with a valid int, to be filled in when
                    // actually processing the deserialized commands
                    AzFramework::StringFunc::Replace(metaDataString, "$(MOTIONID)", "0");
                    AzFramework::StringFunc::Replace(metaDataString, "$(ACTORID)", "0");

                    // Tokenize by line delimiters.
                    AzFramework::StringFunc::Replace(metaDataString, "\r\n", "\n");
                    AzFramework::StringFunc::Replace(metaDataString, '\r', '\n');
                    AzFramework::StringFunc::Tokenize(metaDataString.c_str(), commands, '\n');
                }
                else
                {
                    return true;
                }

                const int commandsElementIndex = rootElementNode.AddElement<AZStd::vector<MCore::Command*>>(serializeContext, "commands");
                AZ::SerializeContext::DataElementNode& commandsElement = rootElementNode.GetSubElement(commandsElementIndex);

                for (const AZStd::string& commandString : commands)
                {
                    // get the command name
                    const size_t wordSeparatorIndex = commandString.find_first_of(MCore::CharacterConstants::wordSeparators);
                    const AZStd::string commandName = commandString.substr(0, wordSeparatorIndex);

                    MCore::Command* commandManagersCommand = CommandSystem::GetCommandManager()->FindCommand(commandName);
                    if (!commandManagersCommand || azrtti_typeid(commandManagersCommand) == azrtti_typeid<MCore::Command>())
                    {
                        continue;
                    }
                    AZStd::unique_ptr<MCore::Command> commandObject { commandManagersCommand->Create() };
                    const int commandElementIndex = commandsElement.AddElement(serializeContext, "element", azrtti_typeid(commandObject.get()));
                    AZ::SerializeContext::DataElementNode& commandElement = commandsElement.GetSubElement(commandElementIndex);

                    AZStd::string commandParameters;
                    if (wordSeparatorIndex != AZStd::string::npos)
                    {
                        commandParameters = commandString.substr(wordSeparatorIndex + 1);
                    }
                    AzFramework::StringFunc::TrimWhiteSpace(commandParameters, true, true);

                    MCore::CommandLine commandLine(commandParameters);

                    if (CommandSystem::CommandAdjustMotion* adjustMotionCommand = azrtti_cast<CommandSystem::CommandAdjustMotion*>(commandObject.get()))
                    {
                        adjustMotionCommand->SetMotionExtractionFlags(static_cast<EMotionFX::EMotionExtractionFlags>(commandLine.GetValueAsInt("motionExtractionFlags", 0)));
                        if (!commandElement.SetData(serializeContext, *adjustMotionCommand))
                        {
                            return false;
                        }
                    }
                    else if (CommandSystem::CommandClearMotionEvents* clearMotionEvents = azrtti_cast<CommandSystem::CommandClearMotionEvents*>(commandObject.get()))
                    {
                        if (!commandElement.SetData(serializeContext, *clearMotionEvents))
                        {
                            return false;
                        }
                    }
                    else if (CommandSystem::CommandCreateMotionEventTrack* createMotionEventTrackCommand = azrtti_cast<CommandSystem::CommandCreateMotionEventTrack*>(commandObject.get()))
                    {
                        createMotionEventTrackCommand->SetEventTrackName(commandLine.GetParameterValue(commandLine.FindParameterIndex("eventTrackName")));
                        if (!commandElement.SetData(serializeContext, *createMotionEventTrackCommand))
                        {
                            return false;
                        }
                    }
                    else if (CommandSystem::CommandAdjustMotionEventTrack* adjustMotionEventTrackCommand = azrtti_cast<CommandSystem::CommandAdjustMotionEventTrack*>(commandObject.get()))
                    {
                        adjustMotionEventTrackCommand->SetEventTrackName(commandLine.GetParameterValue(commandLine.FindParameterIndex("eventTrackName")));
                        adjustMotionEventTrackCommand->SetIsEnabled(commandLine.GetValueAsBool("enabled", true));
                        if (!commandElement.SetData(serializeContext, *adjustMotionEventTrackCommand))
                        {
                            return false;
                        }
                    }
                    else if (CommandSystem::CommandCreateMotionEvent* createMotionEventCommand = azrtti_cast<CommandSystem::CommandCreateMotionEvent*>(commandObject.get()))
                    {
                        createMotionEventCommand->SetEventTrackName(commandLine.GetParameterValue(commandLine.FindParameterIndex("eventTrackName")));
                        createMotionEventCommand->SetStartTime(commandLine.GetValueAsFloat("startTime", 0.0f));
                        createMotionEventCommand->SetEndTime(commandLine.GetValueAsFloat("endTime", 0.0f));

                        const size_t eventTypeIndex = commandLine.FindParameterIndex("eventType");
                        const size_t parametersIndex = commandLine.FindParameterIndex("parameters");
                        const size_t mirrorTypeIndex = commandLine.FindParameterIndex("mirrorType");
                        if (eventTypeIndex == InvalidIndex || parametersIndex == InvalidIndex || mirrorTypeIndex == InvalidIndex)
                        {
                            // Note: We have noticed some bad data issue in internal assets. The parameters could contain \r\n inside of the parameter string, which would result in the mirror type missing.
                            // Those are already been fixed in the command line object code, but we don't want to support the bad data in here by creating another loophole. Instead, we want the user to fix
                            // the broken .assetinfo.
                            AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Found corrupted data in the create motion event command. It can happen if the parameter has an end of line character that \
should be removed. Look for the metaData field of the MetaDataRule in this asset's .assetinfo file, and remove extraneous newlines from the parameter strings.");
                            return false;
                        }

                        const AZStd::string eventType = commandLine.GetParameterValue(eventTypeIndex);
                        const AZStd::string eventParameter = commandLine.GetParameterValue(parametersIndex);
                        const AZStd::string mirrorType = commandLine.GetParameterValue(mirrorTypeIndex);

                        createMotionEventCommand->SetEventDatas({{ EMotionFX::GetEventManager().FindOrCreateEventData<EMotionFX::TwoStringEventData>(eventType, eventParameter, mirrorType) }});
                        if (!commandElement.SetData(serializeContext, *createMotionEventCommand))
                        {
                            return false;
                        }
                    }
                }

                return true;
            }

            void MetaDataRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<MetaDataRule, AZ::SceneAPI::DataTypes::IManifestObject>()
                        ->Version(2, &MetaDataRuleConverter)
                        ->Field("commands", &MetaDataRule::m_commands)
                        // Actor still reads the metaData from the string, it
                        // has to persist
                        ->Field("metaData", &MetaDataRule::m_metaData)
                        ;

#if defined(_DEBUG)
                    // Only enabled in debug builds, not in profile builds as this is currently only used for debugging
                    //      purposes and not to be presented to the user.
                    AZ::EditContext* editContext = serializeContext->GetEditContext();
                    if (editContext)
                    {
                        editContext->Class<MetaDataRule>("Meta data", "Additional information attached by EMotion FX.")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute("AutoExpand", true)
                                ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &MetaDataRule::m_commands, "", "EMotion FX data build as string.")
                                ->Attribute(AZ::Edit::Attributes::ReadOnly, true);
                    }
#endif // _DEBUG
                }
            }

            template <typename T>
            bool MetaDataRule::LoadMetaData(const AZ::SceneAPI::DataTypes::IGroup& group, T& outMetaData)
            {
                AZStd::shared_ptr<MetaDataRule> metaDataRule = group.GetRuleContainerConst().FindFirstByType<MetaDataRule>();
                if (!metaDataRule)
                {
                    outMetaData.clear();
                    return false;
                }

                outMetaData = metaDataRule->GetMetaData<const T&>();
                return true;
            }

            template bool MetaDataRule::LoadMetaData<AZStd::string>(const AZ::SceneAPI::DataTypes::IGroup& group, AZStd::string& outMetaData);
            template bool MetaDataRule::LoadMetaData<AZStd::vector<MCore::Command*>>(const AZ::SceneAPI::DataTypes::IGroup& group, AZStd::vector<MCore::Command*>& outMetaData);


            template <typename T>
            void MetaDataRule::SaveMetaData(AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IGroup& group, const T& metaData)
            {
                namespace SceneEvents = AZ::SceneAPI::Events;

                AZ::SceneAPI::Containers::RuleContainer& rules = group.GetRuleContainer();

                AZStd::shared_ptr<MetaDataRule> metaDataRule = rules.FindFirstByType<MetaDataRule>();
                if (!metaData.empty())
                {
                    // Update the meta data in case there is a meta data rule already.
                    if (metaDataRule)
                    {
                        metaDataRule->SetMetaData(metaData);
                        SceneEvents::ManifestMetaInfoBus::Broadcast(&SceneEvents::ManifestMetaInfoBus::Events::ObjectUpdated, scene, metaDataRule.get(), nullptr);
                    }
                    else
                    {
                        // No meta data rule exists yet, create one and add it.
                        metaDataRule = AZStd::make_shared<MetaDataRule>(metaData);
                        rules.AddRule(metaDataRule);
                        SceneEvents::ManifestMetaInfoBus::Broadcast(&SceneEvents::ManifestMetaInfoBus::Events::InitializeObject, scene, *metaDataRule);
                        SceneEvents::ManifestMetaInfoBus::Broadcast(&SceneEvents::ManifestMetaInfoBus::Events::ObjectUpdated, scene, &group, nullptr);
                    }
                }
                else
                {
                    if (metaDataRule)
                    {
                        // Rather than storing an empty string, remove the whole rule.
                        rules.RemoveRule(metaDataRule);
                        SceneEvents::ManifestMetaInfoBus::Broadcast(&SceneEvents::ManifestMetaInfoBus::Events::ObjectUpdated, scene, &group, nullptr);
                    }
                }
            }

            template void MetaDataRule::SaveMetaData<AZStd::string>(AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IGroup& group, const AZStd::string& metaData);
            template void MetaDataRule::SaveMetaData<AZStd::vector<MCore::Command*>>(AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IGroup& group, const AZStd::vector<MCore::Command*>& metaData);
        } // Rule
    } // Pipeline
} // EMotionFX
