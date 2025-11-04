/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneCore/Containers/SceneManifest.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ::SceneAPI::DataTypes
{
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(IManifestObject, "IManifestObject", "{3B839407-1884-4FF4-ABEA-CA9D347E83F7}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(IManifestObject)
    // Implement IManifestObject reflection in a cpp file
    void IManifestObject::Reflect(AZ::ReflectContext* context)
    {
        if(AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<IManifestObject>()
                ->Version(0);
        }
    }
}

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {


            const char ErrorWindowName[] = "SceneManifest";

            AZ_CLASS_ALLOCATOR_IMPL(SceneManifest, AZ::SystemAllocator)
            SceneManifest::~SceneManifest()
            {
            }

            void SceneManifest::Clear()
            {
                m_storageLookup.clear();
                m_values.clear();
            }

            bool SceneManifest::AddEntry(AZStd::shared_ptr<DataTypes::IManifestObject>&& value)
            {
                auto itValue = m_storageLookup.find(value.get());
                if (itValue != m_storageLookup.end())
                {
                    AZ_TracePrintf(Utilities::WarningWindow, "Manifest Object has already been registered with the manifest.");
                    return false;
                }

                Index index = aznumeric_caster(m_values.size());
                m_storageLookup[value.get()] = index;
                m_values.push_back(AZStd::move(value));

                AZ_Assert(m_values.size() == m_storageLookup.size(),
                    "SceneManifest values and storage-lookup tables have gone out of lockstep (%i vs %i)",
                    m_values.size(), m_storageLookup.size());
                return true;
            }

            bool SceneManifest::RemoveEntry(const DataTypes::IManifestObject* const value)
            {
                auto storageLookupIt = m_storageLookup.find(value);
                if (storageLookupIt == m_storageLookup.end())
                {
                    AZ_Assert(false, "Value not registered in SceneManifest.");
                    return false;
                }

                size_t index = storageLookupIt->second;

                m_values.erase(m_values.begin() + index);
                m_storageLookup.erase(storageLookupIt);

                for (auto& entry : m_storageLookup)
                {
                    if (entry.second > index)
                    {
                        entry.second--;
                    }
                }

                return true;
            }

            SceneManifest::Index SceneManifest::FindIndex(const DataTypes::IManifestObject* const value) const
            {
                auto it = m_storageLookup.find(value);
                return it != m_storageLookup.end() ? (*it).second : s_invalidIndex;
            }

            bool SceneManifest::LoadFromFile(const AZStd::string& absoluteFilePath, SerializeContext* context)
            {
                if (absoluteFilePath.empty())
                {
                    AZ_Error(ErrorWindowName, false, "Unable to load Scene Manifest: no file path was provided.");
                    return false;
                }

                // Utils::ReadFile fails if the file doesn't exist.
                // Check if it exists first, it's not an error if there is no scene manifest.
                if (!AZ::IO::SystemFile::Exists(absoluteFilePath.c_str()))
                {
                    return false;
                }

                auto readFileOutcome = Utils::ReadFile(absoluteFilePath, MaxSceneManifestFileSizeInBytes);
                if (!readFileOutcome.IsSuccess())
                {
                    AZ_Error(ErrorWindowName, false, readFileOutcome.GetError().c_str());
                    return false;
                }

                AZStd::string fileContents(readFileOutcome.TakeValue());

                // Attempt to read the file as JSON
                auto loadJsonOutcome = LoadFromString(fileContents, context);
                if (loadJsonOutcome.IsSuccess())
                {
                    return true;
                }

                // If JSON parsing failed, try to deserialize with XML
                auto loadXmlOutcome = LoadFromString(fileContents, context, nullptr, true);

                AZStd::string fileName;
                AzFramework::StringFunc::Path::GetFileName(absoluteFilePath.c_str(), fileName);

                if (loadXmlOutcome.IsSuccess())
                {
                    AZ_TracePrintf(ErrorWindowName, "Scene Manifest ( %s ) is using the deprecated XML file format. It will be upgraded to JSON the next time it is modified.\n", fileName.c_str());
                    return true;
                }

                // If both failed, throw an error
                AZ_Error(ErrorWindowName, false,
                    "Unable to deserialize ( %s ) using JSON or XML. \nJSON reported error: %s\nXML reported error: %s",
                    fileName.c_str(), loadJsonOutcome.GetError().c_str(), loadXmlOutcome.GetError().c_str());
                return false;
            }

            bool SceneManifest::SaveToFile(const AZStd::string& absoluteFilePath, SerializeContext* context)
            {
                AZ_TraceContext(ErrorWindowName, absoluteFilePath);

                if (absoluteFilePath.empty())
                {
                    AZ_Error(ErrorWindowName, false, "Unable to save Scene Manifest: no file path was provided.");
                    return false;
                }

                AZStd::string errorMsg = AZStd::string::format("Unable to save Scene Manifest to ( %s ):\n", absoluteFilePath.c_str());

                AZ::Outcome<rapidjson::Document, AZStd::string> saveToJsonOutcome = SaveToJsonDocument(context);
                if (!saveToJsonOutcome.IsSuccess())
                {
                    AZ_Error(ErrorWindowName, false, "%s%s", errorMsg.c_str(), saveToJsonOutcome.GetError().c_str());
                    return false;
                }

                auto saveToFileOutcome = AZ::JsonSerializationUtils::WriteJsonFile(saveToJsonOutcome.GetValue(), absoluteFilePath);
                if (!saveToFileOutcome.IsSuccess())
                {
                    AZ_Error(ErrorWindowName, false, "%s%s", errorMsg.c_str(), saveToFileOutcome.GetError().c_str());
                    return false;
                }

                return true;
            }

            AZStd::shared_ptr<const DataTypes::IManifestObject> SceneManifest::SceneManifestConstDataConverter(
                const AZStd::shared_ptr<DataTypes::IManifestObject>& value)
            {
                return AZStd::shared_ptr<const DataTypes::IManifestObject>(value);
            }

            void SceneManifest::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<SceneManifest>()
                        ->Version(1, &SceneManifest::VersionConverter)
                        ->Field("values", &SceneManifest::m_values);
                }

                BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
                if (behaviorContext)
                {
                    behaviorContext->Class<SceneManifest>()
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene")
                        ->Method("ImportFromJson", [](SceneManifest& self, AZStd::string_view jsonBuffer) -> bool
                        {
                            auto outcome = self.LoadFromString(jsonBuffer);
                            if (outcome.IsSuccess())
                            {
                                return true;
                            }
                            AZ_Warning(ErrorWindowName, false, "LoadFromString outcome failure (%s)", outcome.GetError().c_str());
                            return true;
                        })
                        ->Method("ExportToJson", [](SceneManifest& self) -> AZStd::string
                        {
                            auto outcome = self.SaveToJsonDocument();
                            if (outcome.IsSuccess())
                            {
                                // write the manifest to a UTF-8 string buffer and move return the string
                                rapidjson::StringBuffer sb;
                                rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<>> writer(sb);
                                rapidjson::Document& document = outcome.GetValue();
                                document.Accept(writer);
                                return AZStd::move(AZStd::string(sb.GetString()));
                            }
                            AZ_Warning(ErrorWindowName, false, "SaveToJsonDocument outcome failure (%s)", outcome.GetError().c_str());
                            return {};
                        });
                }
            }

            bool SceneManifest::VersionConverter(SerializeContext& context, SerializeContext::DataElementNode& node)
            {
                if (node.GetVersion() != 0)
                {
                    AZ_TracePrintf(ErrorWindowName, "Unable to upgrade SceneManifest from version %i.", node.GetVersion());
                    return false;
                }

                // Copy out the original values.
                AZStd::vector<SerializeContext::DataElementNode> values;
                values.reserve(node.GetNumSubElements());
                for (int i = 0; i < node.GetNumSubElements(); ++i)
                {
                    // The old format stored AZStd::pair<AZStd::string, AZStd::shared_ptr<IManifestObjets>>. All this
                    //      data is still used, but needs to be move to the new location. The shared ptr needs to be
                    //      moved into the new container, while the name needs to be moved to the group name.

                    SerializeContext::DataElementNode& pairNode = node.GetSubElement(i);
                    // This is the original content of the shared ptr. Using the shared pointer directly caused
                    //      registration issues so it's extracting the data the shared ptr was storing instead.
                    SerializeContext::DataElementNode& elementNode = pairNode.GetSubElement(1).GetSubElement(0);

                    SerializeContext::DataElementNode& nameNode = pairNode.GetSubElement(0);
                    AZStd::string name;
                    if (nameNode.GetData(name))
                    {
                        elementNode.AddElementWithData<AZStd::string>(context, "name", name);
                    }
                    // It's better not to set a default name here as the default behaviors will take care of that
                    //      will have more information to work with.

                    values.push_back(elementNode);
                }

                // Delete old values
                for (int i = 0; i < node.GetNumSubElements(); ++i)
                {
                    node.RemoveElement(i);
                }

                // Put stored values back
                int vectorIndex = node.AddElement<ValueStorage>(context, "values");
                SerializeContext::DataElementNode& vectorNode = node.GetSubElement(vectorIndex);
                for (SerializeContext::DataElementNode& value : values)
                {
                    value.SetName("element");

                    // Put in a blank shared ptr to be filled with a value stored from "values".
                    int valueIndex = vectorNode.AddElement<ValueStorageType>(context, "element");
                    SerializeContext::DataElementNode& pointerNode = vectorNode.GetSubElement(valueIndex);

                    // Type doesn't matter as it will be overwritten by the stored value.
                    pointerNode.AddElement<int>(context, "element");
                    pointerNode.GetSubElement(0) = value;
                }

                AZ_TracePrintf(Utilities::WarningWindow,
                    "The SceneManifest has been updated from version %i. It's recommended to save the updated file.", node.GetVersion());
                return true;
            }

            AZ::Outcome<void, AZStd::string> SceneManifest::LoadFromString(const AZStd::string& fileContents, SerializeContext* context, JsonRegistrationContext* registrationContext, bool loadXml)
            {
                Clear();

                AZStd::string failureMessage;
                if (loadXml)
                {
                    // Attempt to read the stream as XML (old format)

                    // Gems can be removed, causing the setting for manifest objects in the the Gem to not be registered. Instead of failing
                    //      to load the entire manifest, just ignore those values.
                    ObjectStream::FilterDescriptor loadFilter(&AZ::Data::AssetFilterNoAssetLoading, ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);

                    if (Utils::LoadObjectFromBufferInPlace<SceneManifest>(fileContents.data(), fileContents.size(), *this, context, loadFilter))
                    {
                        Init();
                        return AZ::Success();
                    }

                    failureMessage = "Unable to load Scene Manifest as XML";
                }
                else
                {
                    // Attempt to read the stream as JSON
                    auto readJsonOutcome = AZ::JsonSerializationUtils::ReadJsonString(fileContents);
                    AZStd::string errorMsg;
                    if (!readJsonOutcome.IsSuccess())
                    {
                        return AZ::Failure(readJsonOutcome.TakeError());
                    }
                    rapidjson::Document document = readJsonOutcome.TakeValue();

                    AZ::JsonDeserializerSettings settings;
                    settings.m_serializeContext = context;
                    settings.m_registrationContext = registrationContext;

                    AZ::JsonSerializationResult::ResultCode jsonResult = AZ::JsonSerialization::Load(*this, document, settings);
                    if (jsonResult.GetProcessing() != AZ::JsonSerializationResult::Processing::Halted)
                    {
                        Init();
                        return AZ::Success();
                    }

                    failureMessage = jsonResult.ToString("");
                }

                return AZ::Failure(failureMessage);
            }

            AZ::Outcome<rapidjson::Document, AZStd::string> SceneManifest::SaveToJsonDocument(SerializeContext* context, JsonRegistrationContext* registrationContext)
            {
                AZ::JsonSerializerSettings settings;
                settings.m_serializeContext = context;
                settings.m_registrationContext = registrationContext;
                rapidjson::Document jsonDocument;
                auto jsonResult = JsonSerialization::Store(jsonDocument, jsonDocument.GetAllocator(), *this, settings);
                if (jsonResult.GetProcessing() == AZ::JsonSerializationResult::Processing::Halted)
                {
                    return AZ::Failure(AZStd::string::format("JSON serialization failed: %s", jsonResult.ToString("").c_str()));
                }

                return AZ::Success(AZStd::move(jsonDocument));
            }

            void SceneManifest::Init()
            {
                auto end = AZStd::remove_if(m_values.begin(), m_values.end(),
                    [](const ValueStorageType& entry) -> bool
                    {
                        return !entry;
                    });
                m_values.erase(end, m_values.end());

                for (size_t i = 0; i < m_values.size(); ++i)
                {
                    Index index = aznumeric_caster(i);
                    m_storageLookup[m_values[i].get()] = index;
                }
            }

        } // Containers
    } // SceneAPI
} // AZ
