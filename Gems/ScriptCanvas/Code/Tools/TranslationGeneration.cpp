/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TranslationGeneration.h"

#include <Source/Translation/TranslationBus.h>

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/string/regex.h>

#include <AzFramework/Gem/GemInfo.h>

#include <Libraries/Core/AzEventHandler.h>
#include <Libraries/Libraries.h>
#include <Libraries/Core/GetVariable.h>
#include <Libraries/Core/SetVariable.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>
#include <Source/Translation/TranslationSerializer.h>
#include "Data/DataRegistry.h"

namespace ScriptCanvasEditorTools
{
    namespace Helpers
    {
        //! Convenience function that writes a key/value string pair into a given JSON value
        void WriteString(rapidjson::Value& owner, const AZStd::string& key, const AZStd::string& value, rapidjson::Document& document);
    }

    TranslationGeneration::TranslationGeneration()
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ::ComponentApplicationBus::BroadcastResult(m_behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
    }

    void TranslationGeneration::TranslateBehaviorClasses()
    {
        for (const auto& behaviorClassPair : m_behaviorContext->m_classes)
        {
            TranslateBehaviorClass(behaviorClassPair.second);
        }
    }

    void TranslationGeneration::TranslateEBus(const AZ::BehaviorEBus* behaviorEBus)
    {
        if (ShouldSkip(behaviorEBus))
        {
            return;
        }

        TranslationFormat translationRoot;

        // Get the handlers
        if (!TranslateEBusHandler(behaviorEBus, translationRoot))
        {
            if (behaviorEBus->m_events.empty())
            {
                return;
            }

            Entry entry;

            // Generate the translation file
            entry.m_key = behaviorEBus->m_name;
            entry.m_details.m_category = Helpers::GetStringAttribute(behaviorEBus, AZ::Script::Attributes::Category);;
            entry.m_details.m_tooltip = behaviorEBus->m_toolTip;
            entry.m_details.m_name = behaviorEBus->m_name;
            entry.m_context = "EBusSender";

            AZStd::string prettyName = Helpers::GetStringAttribute(behaviorEBus, AZ::ScriptCanvasAttributes::PrettyName);
            if (!prettyName.empty())
            {
                entry.m_details.m_name = prettyName;
            }

            SplitCamelCase(entry.m_details.m_name);

            for (auto event : behaviorEBus->m_events)
            {
                const AZ::BehaviorEBusEventSender& ebusSender = event.second;

                AZ::BehaviorMethod* method = ebusSender.m_event;
                if (!method)
                {
                    method = ebusSender.m_broadcast;
                }

                if (!method)
                {
                    AZ_Warning("Script Canvas", false, "Failed to find method: %s", event.first.c_str());
                    continue;
                }

                Method eventEntry;
                const char* eventName = event.first.c_str();

                eventEntry.m_key = eventName;

                prettyName = Helpers::GetStringAttribute(behaviorEBus, AZ::ScriptCanvasAttributes::PrettyName);
                eventEntry.m_details.m_name = prettyName.empty() ? eventName : prettyName;
                eventEntry.m_details.m_tooltip = Helpers::ReadStringAttribute(event.second.m_attributes, AZ::Script::Attributes::ToolTip);

                SplitCamelCase(eventEntry.m_details.m_name);

                eventEntry.m_entry.m_name = "In";
                eventEntry.m_entry.m_tooltip = AZStd::string::format("When signaled, this will invoke %s", eventEntry.m_details.m_name.c_str());
                eventEntry.m_exit.m_name = "Out";
                eventEntry.m_exit.m_tooltip = AZStd::string::format("Signaled after %s is invoked", eventEntry.m_details.m_name.c_str());

                size_t start = method->HasBusId() ? 1 : 0;
                for (size_t i = start; i < method->GetNumArguments(); ++i)
                {
                    Argument argument;
                    auto argumentType = method->GetArgument(i)->m_typeId;

                    // Check the BC for metadata

                    Helpers::GetTypeNameAndDescription(argumentType, argument.m_details.m_name, argument.m_details.m_tooltip);

                    auto name = method->GetArgumentName(i);
                    if (name && !name->empty())
                    {
                        argument.m_details.m_name = *name;
                    }

                    auto tooltip = method->GetArgumentToolTip(i);
                    if (tooltip && !tooltip->empty())
                    {
                        argument.m_details.m_tooltip = *tooltip;
                    }

                    argument.m_typeId = argumentType.ToString<AZStd::string>();

                    SplitCamelCase(argument.m_details.m_name);

                    eventEntry.m_arguments.push_back(argument);
                }

                if (method->HasResult())
                {
                    Argument result;

                    auto resultType = method->GetResult()->m_typeId;

                    Helpers::GetTypeNameAndDescription(resultType, result.m_details.m_name, result.m_details.m_tooltip);

                    auto tooltip = method->GetArgumentToolTip(0);
                    if (tooltip && !tooltip->empty())
                    {
                        result.m_details.m_tooltip = *tooltip;
                    }

                    result.m_typeId = resultType.ToString<AZStd::string>();

                    SplitCamelCase(result.m_details.m_name);

                    eventEntry.m_results.push_back(result);
                }

                entry.m_methods.push_back(eventEntry);

            }

            translationRoot.m_entries.push_back(entry);

            SaveJSONData(AZStd::string::format("EBus/Senders/%s", behaviorEBus->m_name.c_str()), translationRoot);
        }
        else
        {
            SaveJSONData(AZStd::string::format("EBus/Handlers/%s", behaviorEBus->m_name.c_str()), translationRoot);
        }
    }

    AZ::Entity* TranslationGeneration::TranslateAZEvent(const AZ::BehaviorMethod& method)
    {
        // Make sure the method returns an AZ::Event by reference or pointer
        if (AZ::MethodReturnsAzEventByReferenceOrPointer(method))
        {
            // Read in AZ Event Description data to retrieve the event name and parameter names
            AZ::Attribute* azEventDescAttribute = AZ::FindAttribute(AZ::Script::Attributes::AzEventDescription, method.m_attributes);
            AZ::BehaviorAzEventDescription behaviorAzEventDesc;
            AZ::AttributeReader azEventDescAttributeReader(nullptr, azEventDescAttribute);
            azEventDescAttributeReader.Read<decltype(behaviorAzEventDesc)>(behaviorAzEventDesc);
            if (behaviorAzEventDesc.m_eventName.empty())
            {
                AZ_Error("NodeUtils", false, "Cannot create an AzEvent node with empty event name")
            }

            auto scriptCanvasEntity = aznew AZ::Entity{ AZStd::string::format("SC-EventNode(%s)", behaviorAzEventDesc.m_eventName.c_str()) };
            scriptCanvasEntity->Init();
            auto azEventHandler = scriptCanvasEntity->CreateComponent<ScriptCanvas::Nodes::Core::AzEventHandler>();

            azEventHandler->InitEventFromMethod(method);

            return scriptCanvasEntity;
        }

        return nullptr;
    }

    bool TranslationGeneration::TranslateBehaviorClass(const AZ::BehaviorClass* behaviorClass)
    {
        if (ShouldSkip(behaviorClass))
        {
            return false;
        }

        AZStd::string className = behaviorClass->m_name;
        AZStd::string prettyName = Helpers::GetStringAttribute(behaviorClass, AZ::ScriptCanvasAttributes::PrettyName);
        if (!prettyName.empty())
        {
            className = prettyName;
        }

        TranslationFormat translationRoot;

        Entry entry;
        entry.m_context = "BehaviorClass";
        entry.m_key = behaviorClass->m_name;

        EntryDetails& details = entry.m_details;
        details.m_name = className;
        details.m_category = Helpers::GetStringAttribute(behaviorClass, AZ::Script::Attributes::Category);
        details.m_tooltip = Helpers::GetStringAttribute(behaviorClass, AZ::Script::Attributes::ToolTip);

        SplitCamelCase(details.m_name);

        if (!behaviorClass->m_methods.empty())
        {
            for (const auto& methodPair : behaviorClass->m_methods)
            {
                const AZ::BehaviorMethod* behaviorMethod = methodPair.second;

                Method methodEntry;

                AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(methodPair.first);

                methodEntry.m_key = cleanName;
                methodEntry.m_context = className;

                methodEntry.m_details.m_category = "";
                methodEntry.m_details.m_tooltip = "";
                methodEntry.m_details.m_name = methodPair.second->m_name;

                AZStd::string prefix = className + "::";
                AZ::StringFunc::Replace(methodEntry.m_details.m_name, prefix.c_str(), "");
                SplitCamelCase(methodEntry.m_details.m_name);

                methodEntry.m_entry.m_name = "In";
                methodEntry.m_entry.m_tooltip = AZStd::string::format("When signaled, this will invoke %s", methodEntry.m_details.m_name.c_str());
                methodEntry.m_exit.m_name = "Out";
                methodEntry.m_exit.m_tooltip = AZStd::string::format("Signaled after %s is invoked", methodEntry.m_details.m_name.c_str());

                if (!Helpers::MethodHasAttribute(behaviorMethod, AZ::ScriptCanvasAttributes::FloatingFunction))
                {
                    methodEntry.m_details.m_category = details.m_category;
                }
                else if (Helpers::MethodHasAttribute(behaviorMethod, AZ::Script::Attributes::Category))
                {
                    methodEntry.m_details.m_category = Helpers::ReadStringAttribute(behaviorMethod->m_attributes, AZ::Script::Attributes::Category);
                }

                // Arguments (Input Slots)
                if (behaviorMethod->GetNumArguments() > 0)
                {
                    size_t startIndex = behaviorMethod->HasResult() ? 1 : 0;
                    for (size_t argIndex = startIndex; argIndex < behaviorMethod->GetNumArguments(); ++argIndex)
                    {
                        const AZ::BehaviorParameter* parameter = behaviorMethod->GetArgument(argIndex);

                        Argument argument;

                        AZStd::string argumentKey = parameter->m_typeId.ToString<AZStd::string>();
                        AZStd::string argumentName;
                        AZStd::string argumentDescription;

                        Helpers::GetTypeNameAndDescription(parameter->m_typeId, argumentName, argumentDescription);

                        const AZStd::string* argName = behaviorMethod->GetArgumentName(argIndex);
                        if (argName && !(*argName).empty())
                        {
                            argumentName = *argName;
                        }

                        const AZStd::string* argDesc = behaviorMethod->GetArgumentToolTip(argIndex);
                        if (argDesc && !(*argDesc).empty())
                        {
                            argumentDescription = *argDesc;
                        }

                        argument.m_typeId = argumentKey;
                        argument.m_details.m_name = argumentName;
                        argument.m_details.m_tooltip = argumentDescription;

                        SplitCamelCase(argument.m_details.m_name);

                        methodEntry.m_arguments.push_back(argument);
                    }
                }

                // Results (Output Slots)
                const AZ::BehaviorParameter* resultParameter = behaviorMethod->HasResult() ? behaviorMethod->GetResult() : nullptr;
                if (resultParameter)
                {
                    Argument result;

                    AZStd::string resultKey = resultParameter->m_typeId.ToString<AZStd::string>();
                    AZStd::string resultName = resultParameter->m_name;
                    AZStd::string resultDescription = "";

                    Helpers::GetTypeNameAndDescription(resultParameter->m_typeId, resultName, resultDescription);

                    result.m_typeId = resultKey;
                    result.m_details.m_name = resultParameter->m_name;
                    result.m_details.m_tooltip = resultDescription;

                    SplitCamelCase(result.m_details.m_name);

                    methodEntry.m_results.push_back(result);
                }

                entry.m_methods.push_back(methodEntry);
            }
        }

        // Behavior Class properties
        if (!behaviorClass->m_properties.empty())
        {
            for (const auto& propertyEntry : behaviorClass->m_properties)
            {
                AZ::BehaviorProperty* behaviorProperty = propertyEntry.second;
                if (behaviorProperty)
                {
                    TranslateBehaviorProperty(behaviorProperty, behaviorClass->m_name, "BehaviorClass", &entry);
                }
            }
        }

        translationRoot.m_entries.push_back(entry);

        AZStd::string sanitizedFilename = GraphCanvas::TranslationKey::Sanitize(className);
        AZStd::string fileName = AZStd::string::format("Classes/%s", sanitizedFilename.c_str());

        SaveJSONData(fileName, translationRoot);

        return true;
    }

    void TranslationGeneration::TranslateAZEvents()
    {
        GraphCanvas::TranslationKey translationKey;
        AZStd::vector<AZ::Entity*> nodes;

        // Methods
        for (const auto& behaviorMethod : m_behaviorContext->m_methods)
        {
            const auto& method = *behaviorMethod.second;
            AZ::Entity* node = TranslateAZEvent(method);
            if (node)
            {
                nodes.push_back(node);
            }
        }

        // Methods in classes
        for (auto behaviorClass : m_behaviorContext->m_classes)
        {
            for (auto behaviorMethod : behaviorClass.second->m_methods)
            {
                const auto& method = *behaviorMethod.second;
                AZ::Entity* node = TranslateAZEvent(method);
                if (node)
                {
                    nodes.push_back(node);
                }
            }
        }

        TranslationFormat translationRoot;

        for (auto& node : nodes)
        {
            ScriptCanvas::Nodes::Core::AzEventHandler* nodeComponent = node->FindComponent<ScriptCanvas::Nodes::Core::AzEventHandler>();
            nodeComponent->Init();
            nodeComponent->Configure();

            const ScriptCanvas::Nodes::Core::AzEventEntry& azEventEntry{ nodeComponent->GetEventEntry() };

            Entry entry;
            entry.m_key = azEventEntry.m_eventName;
            entry.m_context = "AZEventHandler";
            entry.m_details.m_name = azEventEntry.m_eventName;

            SplitCamelCase(entry.m_details.m_name);

            for (const ScriptCanvas::Slot& slot : nodeComponent->GetSlots())
            {
                Slot slotEntry;

                if (slot.IsVisible())
                {
                    slotEntry.m_key = slot.GetName();

                    if (slot.GetId() == azEventEntry.m_azEventInputSlotId)
                    {
                        slotEntry.m_details.m_name = azEventEntry.m_eventName;
                    }
                    else
                    {
                        slotEntry.m_details.m_name = slot.GetName();
                    }

                    entry.m_slots.push_back(slotEntry);
                }
            }

            translationRoot.m_entries.push_back(entry);

            // delete the node, don't need to keep it beyond this point
            delete node;


            AZStd::string filename = GraphCanvas::TranslationKey::Sanitize(entry.m_key);

            AZStd::string targetFile = AZStd::string::format("AZEvents/%s", filename.c_str());

            SaveJSONData(targetFile, translationRoot);

            translationRoot.m_entries.clear();
        }
    }

    void TranslationGeneration::TranslateNodes()
    {
        GraphCanvas::TranslationKey translationKey;
        AZStd::vector<AZ::TypeId> nodes;

        auto getNodeClasses = [this, &nodes](const AZ::SerializeContext::ClassData*, const AZ::Uuid& type)
        {
            bool foundBaseClass = false;
            auto baseClassVisitorFn = [&nodes, &type, &foundBaseClass](const AZ::SerializeContext::ClassData* reflectedBase, const AZ::TypeId& /*rttiBase*/)
            {
                if (!reflectedBase)
                {
                    foundBaseClass = false;
                    return false; // stop iterating
                }

                foundBaseClass = (reflectedBase->m_typeId == azrtti_typeid<ScriptCanvas::Node>());
                if (foundBaseClass)
                {
                    nodes.push_back(type);
                    return false; // we have a base, stop iterating
                }

                return true; // keep iterating
            };

            AZ::EntityUtils::EnumerateBaseRecursive(m_serializeContext, baseClassVisitorFn, type);

            return true;
        };

        m_serializeContext->EnumerateAll(getNodeClasses);

        for (auto& node : nodes)
        {
            TranslateNode(node);
        }
    }

    void TranslationGeneration::TranslateNode(const AZ::TypeId& nodeTypeId)
    {
        TranslationFormat translationRoot;

        if (const AZ::SerializeContext::ClassData* classData = m_serializeContext->FindClassData(nodeTypeId))
        {
            Entry entry;
            entry.m_key = classData->m_typeId.ToString<AZStd::string>();
            entry.m_context = "ScriptCanvas::Node";

            EntryDetails& details = entry.m_details;

            AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(classData->m_name);

            if (classData->m_editData)
            {
                details.m_name = classData->m_editData->m_name;
            }
            else
            {
                details.m_name = cleanName;
            }

            SplitCamelCase(details.m_name);

            // Tooltip attribute takes priority over the edit data description
            AZStd::string tooltip = Helpers::GetStringAttribute(classData, AZ::Script::Attributes::ToolTip);
            if (!tooltip.empty())
            {
                details.m_tooltip = tooltip;
            }
            else
            {
                details.m_tooltip = classData->m_editData ? classData->m_editData->m_description : "";
            }

            details.m_category = Helpers::GetStringAttribute(classData, AZ::Script::Attributes::Category);
            if (details.m_subtitle.empty())
            {
                details.m_subtitle = details.m_category;
            }

            if (details.m_category.empty())
            {
                details.m_category = Helpers::GetStringAttribute(classData, AZ::Script::Attributes::Category);
                if (details.m_category.empty() && classData->m_editData)
                {
                    details.m_category = Helpers::GetCategory(classData);

                    if (details.m_category.empty())
                    {
                        auto elementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                        const AZStd::string categoryAttribute = Helpers::ReadStringAttribute(elementData->m_attributes, AZ::Script::Attributes::Category);
                        if (!categoryAttribute.empty())
                        {
                            details.m_category = categoryAttribute;
                        }
                    }
                }
            }

            if (details.m_category.empty())
            {
                // Get the library's name as the category
                details.m_category = Helpers::GetLibraryCategory(*m_serializeContext, classData->m_name);
            }

            if (ScriptCanvas::Node* nodeComponent = reinterpret_cast<ScriptCanvas::Node*>(classData->m_factory->Create(classData->m_name)))
            {
                nodeComponent->Init();
                nodeComponent->Configure();

                int inputIndex = 0;
                int outputIndex = 0;

                const auto& allSlots = nodeComponent->GetAllSlots();
                for (const auto& slot : allSlots)
                {
                    Slot slotEntry;

                    if (slot->GetDescriptor().IsExecution())
                    {
                        if (slot->GetDescriptor().IsInput())
                        {
                            slotEntry.m_key = AZStd::string::format("Input_%s", slot->GetName().c_str());
                            inputIndex++;

                            slotEntry.m_details.m_name = slot->GetName();
                            slotEntry.m_details.m_tooltip = slot->GetToolTip();
                        }
                        else if (slot->GetDescriptor().IsOutput())
                        {
                            slotEntry.m_key = AZStd::string::format("Output_%s", slot->GetName().c_str());
                            outputIndex++;

                            slotEntry.m_details.m_name = slot->GetName();
                            slotEntry.m_details.m_tooltip = slot->GetToolTip();
                        }

                        entry.m_slots.push_back(slotEntry);
                    }
                    else
                    {
                        AZStd::string slotTypeKey = slot->GetDataType().IsValid() ? ScriptCanvas::Data::GetName(slot->GetDataType()) : "";
                        if (slotTypeKey.empty())
                        {
                            if (!slot->GetDataType().GetAZType().IsNull())
                            {
                                slotTypeKey = slot->GetDataType().GetAZType().ToString<AZStd::string>();
                            }
                        }

                        if (slotTypeKey.empty())
                        {
                            if (slot->GetDynamicDataType() == ScriptCanvas::DynamicDataType::Container)
                            {
                                slotTypeKey = "Container";
                            }
                            else if (slot->GetDynamicDataType() == ScriptCanvas::DynamicDataType::Value)
                            {
                                slotTypeKey = "Value";
                            }
                            else if (slot->GetDynamicDataType() == ScriptCanvas::DynamicDataType::Any)
                            {
                                slotTypeKey = "Any";
                            }
                        }

                        Argument& argument = slotEntry.m_data;

                        if (slot->GetDescriptor().IsInput())
                        {
                            slotEntry.m_key = AZStd::string::format("DataInput_%s", slot->GetName().c_str());
                            inputIndex++;

                            AZStd::string argumentKey = slotTypeKey;
                            AZStd::string argumentName = slot->GetName();
                            AZStd::string argumentDescription = slot->GetToolTip();

                            argument.m_typeId = argumentKey;
                            argument.m_details.m_name = argumentName;
                            argument.m_details.m_tooltip = argumentDescription;

                        }
                        else if (slot->GetDescriptor().IsOutput())
                        {
                            slotEntry.m_key = AZStd::string::format("DataOutput_%s", slot->GetName().c_str());
                            outputIndex++;

                            AZStd::string resultKey = slotTypeKey;
                            AZStd::string resultName = slot->GetName();
                            AZStd::string resultDescription = slot->GetToolTip();

                            argument.m_typeId = resultKey;
                            argument.m_details.m_name = resultName;
                            argument.m_details.m_tooltip = resultDescription;
                        }

                        entry.m_slots.push_back(slotEntry);
                    }
                }

                delete nodeComponent;
            }

            translationRoot.m_entries.push_back(entry);

            if (details.m_category.empty())
            {
                details.m_category = "Uncategorized";
            }

            AZStd::string prefix = GraphCanvas::TranslationKey::Sanitize(details.m_category);
            AZStd::string filename = GraphCanvas::TranslationKey::Sanitize(details.m_name);

            AZStd::string targetFile = AZStd::string::format("Nodes/%s_%s", prefix.c_str(), filename.c_str());

            SaveJSONData(targetFile, translationRoot);

            translationRoot.m_entries.clear();

        }
    }

    void TranslationGeneration::TranslateOnDemandReflectedTypes(TranslationFormat& translationRoot)
    {
        AZStd::vector<AZ::Uuid> onDemandReflectedTypes;

        for (auto& typePair : m_behaviorContext->m_typeToClassMap)
        {
            if (m_behaviorContext->IsOnDemandTypeReflected(typePair.first))
            {
                onDemandReflectedTypes.push_back(typePair.first);
            }

            // Check for methods that come from node generics
            if (typePair.second->HasAttribute(AZ::ScriptCanvasAttributes::Internal::ImplementedAsNodeGeneric))
            {
                onDemandReflectedTypes.push_back(typePair.first);
            }
        }

        // Now that I know all the on demand reflected, I'll dump it out
        for (auto& onDemandReflectedType : onDemandReflectedTypes)
        {
            AZ::BehaviorClass* behaviorClass = m_behaviorContext->m_typeToClassMap[onDemandReflectedType];
            if (behaviorClass)
            {
                Entry entry;

                EntryDetails& details = entry.m_details;
                details.m_name = behaviorClass->m_name;
                SplitCamelCase(details.m_name);

                // Get the pretty name
                AZStd::string prettyName;
                if (AZ::Attribute* prettyNameAttribute = AZ::FindAttribute(AZ::ScriptCanvasAttributes::PrettyName, behaviorClass->m_attributes))
                {
                    AZ::AttributeReader(nullptr, prettyNameAttribute).Read<AZStd::string>(prettyName, *m_behaviorContext);
                }

                entry.m_context = "OnDemandReflected";
                entry.m_key = behaviorClass->m_typeId.ToString<AZStd::string>().c_str();

                if (!prettyName.empty())
                {
                    details.m_name = prettyName;
                }

                details.m_category = Helpers::GetStringAttribute(behaviorClass, AZ::Script::Attributes::Category);
                details.m_tooltip = Helpers::GetStringAttribute(behaviorClass, AZ::Script::Attributes::ToolTip);

                for (auto& methodPair : behaviorClass->m_methods)
                {
                    AZ::BehaviorMethod* behaviorMethod = methodPair.second;
                    if (behaviorMethod)
                    {
                        Method methodEntry;

                        AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(methodPair.first);

                        methodEntry.m_key = cleanName;
                        methodEntry.m_context = entry.m_key;

                        methodEntry.m_details.m_tooltip = Helpers::GetStringAttribute(behaviorMethod, AZ::Script::Attributes::ToolTip);
                        methodEntry.m_details.m_name = methodPair.second->m_name;
                        SplitCamelCase(methodEntry.m_details.m_name);

                        // Strip the className from the methodName
                        AZStd::string qualifiedName = behaviorClass->m_name + "::";
                        AzFramework::StringFunc::Replace(methodEntry.m_details.m_name, qualifiedName.c_str(), "");

                        AZStd::string cleanMethodName = methodEntry.m_details.m_name;

                        methodEntry.m_entry.m_name = "In";
                        methodEntry.m_entry.m_tooltip = AZStd::string::format("When signaled, this will invoke %s", methodEntry.m_details.m_name.c_str());
                        methodEntry.m_exit.m_name = "Out";
                        methodEntry.m_exit.m_tooltip = AZStd::string::format("Signaled after %s is invoked", methodEntry.m_details.m_name.c_str());

                        // Arguments (Input Slots)
                        if (behaviorMethod->GetNumArguments() > 0)
                        {
                            for (size_t argIndex = 0; argIndex < behaviorMethod->GetNumArguments(); ++argIndex)
                            {
                                const AZ::BehaviorParameter* parameter = behaviorMethod->GetArgument(argIndex);

                                Argument argument;

                                AZStd::string argumentKey = parameter->m_typeId.ToString<AZStd::string>();
                                AZStd::string argumentName = parameter->m_name;
                                AZStd::string argumentDescription = "";

                                Helpers::GetTypeNameAndDescription(parameter->m_typeId, argumentName, argumentDescription);

                                argument.m_typeId = argumentKey;
                                argument.m_details.m_name = argumentName;
                                argument.m_details.m_category = "";
                                argument.m_details.m_tooltip = argumentDescription;

                                SplitCamelCase(argument.m_details.m_name);

                                methodEntry.m_arguments.push_back(argument);
                            }
                        }

                        const AZ::BehaviorParameter* resultParameter = behaviorMethod->HasResult() ? behaviorMethod->GetResult() : nullptr;
                        if (resultParameter)
                        {
                            Argument result;

                            AZStd::string resultKey = resultParameter->m_typeId.ToString<AZStd::string>();

                            AZStd::string resultName = resultParameter->m_name;
                            AZStd::string resultDescription = "";

                            Helpers::GetTypeNameAndDescription(resultParameter->m_typeId, resultName, resultDescription);

                            result.m_typeId = resultKey;
                            result.m_details.m_name = resultName;
                            result.m_details.m_tooltip = resultDescription;

                            SplitCamelCase(result.m_details.m_name);

                            methodEntry.m_results.push_back(result);
                        }

                        entry.m_methods.push_back(methodEntry);
                    }
                }

                translationRoot.m_entries.push_back(entry);
            }
        }

        SaveJSONData("Types/OnDemandReflectedTypes", translationRoot);
    }

    void TranslationGeneration::TranslateBehaviorGlobals()
    {
        for (const auto& [propertyName, behaviorProperty] : m_behaviorContext->m_properties)
        {
            TranslateBehaviorProperty(propertyName);
        }
    }

    void TranslationGeneration::TranslateBehaviorProperty(const AZStd::string& propertyName)
    {
        const auto behaviorPropertyEntry = m_behaviorContext->m_properties.find(propertyName);
        if (behaviorPropertyEntry == m_behaviorContext->m_properties.end())
        {
            return;
        }

        const AZ::BehaviorProperty* behaviorProperty = behaviorPropertyEntry->second;

        Entry entry;

        TranslateBehaviorProperty(behaviorProperty, propertyName, "Constant", &entry);

        TranslationFormat translationRoot;
        translationRoot.m_entries.push_back(entry);

        AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(behaviorProperty->m_name);
        AZStd::string fileName = AZStd::string::format("Properties/%s", cleanName.c_str());
        SaveJSONData(fileName, translationRoot);
    }

    void TranslationGeneration::TranslateDataTypes()
    {
        TranslationFormat translationRoot;

        auto dataRegistry = ScriptCanvas::GetDataRegistry();
        
        for (auto& typePair : dataRegistry->m_creatableTypes)
        {
            if (ScriptCanvas::Data::IsContainerType(typePair.first))
            {
                continue;
            }

            const AZStd::string typeIDStr = typePair.first.GetAZType().ToString<AZStd::string>();
            AZStd::string typeName = ScriptCanvas::Data::GetName(typePair.first);

            Entry entry;
            entry.m_key = typeIDStr;
            entry.m_context = "BehaviorType";
            entry.m_details.m_name = typeName;

            translationRoot.m_entries.emplace_back(entry);
        }

        SaveJSONData("Types/BehaviorTypes", translationRoot);
    }

    void TranslationGeneration::TranslateMethod(AZ::BehaviorMethod* behaviorMethod, Method& methodEntry)
    {
        // Arguments (Input Slots)
        if (behaviorMethod->GetNumArguments() > 0)
        {
            for (size_t argIndex = 0; argIndex < behaviorMethod->GetNumArguments(); ++argIndex)
            {
                const AZ::BehaviorParameter* parameter = behaviorMethod->GetArgument(argIndex);

                Argument argument;

                AZStd::string argumentKey = parameter->m_typeId.ToString<AZStd::string>();
                AZStd::string argumentName = parameter->m_name;
                AZStd::string argumentDescription;

                Helpers::GetTypeNameAndDescription(parameter->m_typeId, argumentName, argumentDescription);

                const AZStd::string* argName = behaviorMethod->GetArgumentName(argIndex);
                argument.m_typeId = argumentKey;
                argument.m_details.m_name = (argName && !argName->empty()) ? *argName : argumentName;
                argument.m_details.m_category = "";
                argument.m_details.m_tooltip = argumentDescription;

                SplitCamelCase(argument.m_details.m_name);

                methodEntry.m_arguments.push_back(argument);
            }
        }

        // Results (Output Slots)
        const AZ::BehaviorParameter* resultParameter = behaviorMethod->HasResult() ? behaviorMethod->GetResult() : nullptr;
        if (resultParameter)
        {
            Argument result;

            AZStd::string resultKey = resultParameter->m_typeId.ToString<AZStd::string>();
            AZStd::string resultName = resultParameter->m_name;
            AZStd::string resultDescription;

            Helpers::GetTypeNameAndDescription(resultParameter->m_typeId, resultName, resultDescription);

            const AZStd::string* resName = behaviorMethod->GetArgumentName(0);
            result.m_typeId = resultKey;
            result.m_details.m_name = (resName && !resName->empty()) ? *resName : resultName;
            result.m_details.m_tooltip = resultDescription;

            SplitCamelCase(result.m_details.m_name);

            methodEntry.m_results.push_back(result);
        }
    }

    void TranslationGeneration::TranslateBehaviorProperty(const AZ::BehaviorProperty* behaviorProperty, const AZStd::string& className, const AZStd::string& context, Entry* entry)
    {
        if (!behaviorProperty->m_getter && !behaviorProperty->m_setter)
        {
            return;
        }

        Entry localEntry;
        if (!entry)
        {
            entry = &localEntry;
        }
        else if (entry->m_key.empty())
        {
            entry->m_key = className;
            entry->m_context = context;

            entry->m_details.m_name = className;
            SplitCamelCase(entry->m_details.m_name);
        }

        if (behaviorProperty->m_getter)
        {
            AZStd::string cleanName = behaviorProperty->m_name;
            AZ::StringFunc::Replace(cleanName, "::Getter", "");

            Method method;

            AZStd::string methodName = "Get";
            methodName.append(cleanName);
            method.m_key = methodName;
            method.m_details.m_name = methodName;
            method.m_details.m_tooltip = behaviorProperty->m_getter->m_debugDescription ? behaviorProperty->m_getter->m_debugDescription : "";

            SplitCamelCase(method.m_details.m_name);

            TranslateMethod(behaviorProperty->m_getter, method);

            // We know this is a getter, so there will only be one parameter, we will use the method name as a best
            // guess for the argument name
            SplitCamelCase(cleanName);
            method.m_arguments[1].m_details.m_name = cleanName;

            entry->m_methods.push_back(method);

        }

        if (behaviorProperty->m_setter)
        {
            AZStd::string cleanName = behaviorProperty->m_name;
            AZ::StringFunc::Replace(cleanName, "::Setter", "");

            Method method;

            AZStd::string methodName = "Set";
            methodName.append(cleanName);

            method.m_key = methodName;
            method.m_details.m_name = methodName;
            method.m_details.m_tooltip = behaviorProperty->m_setter->m_debugDescription ? behaviorProperty->m_getter->m_debugDescription : "";

            SplitCamelCase(method.m_details.m_name);

            TranslateMethod(behaviorProperty->m_setter, method);

            // We know this is a setter, so there will only be one parameter, we will use the method name as a best
            // guess for the argument name
            SplitCamelCase(cleanName);
            method.m_arguments[1].m_details.m_name = cleanName;

            entry->m_methods.push_back(method);
        }

    }

    bool TranslationGeneration::TranslateEBusHandler(const AZ::BehaviorEBus* behaviorEbus, TranslationFormat& translationRoot)
    {
        // Must be a valid ebus handler
        if (!behaviorEbus || !behaviorEbus->m_createHandler || !behaviorEbus->m_destroyHandler)
        {
            return false;
        }

        // Create the handler in order to get information out of it
        AZ::BehaviorEBusHandler* handler(nullptr);
        if (behaviorEbus->m_createHandler->InvokeResult(handler))
        {
            Entry entry;

            // Generate the translation file
            entry.m_key = behaviorEbus->m_name;
            entry.m_context = "EBusHandler";

            entry.m_details.m_name = behaviorEbus->m_name;
            entry.m_details.m_tooltip = behaviorEbus->m_toolTip;
            entry.m_details.m_category = "EBus Handlers";

            SplitCamelCase(entry.m_details.m_name);

            for (const AZ::BehaviorEBusHandler::BusForwarderEvent& event : handler->GetEvents())
            {
                Method methodEntry;

                AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(event.m_name);
                methodEntry.m_key = cleanName;
                methodEntry.m_details.m_category = "";
                methodEntry.m_details.m_tooltip = "";
                methodEntry.m_details.m_name = event.m_name;

                SplitCamelCase(methodEntry.m_details.m_name);

                // Arguments (Input Slots)
                if (!event.m_parameters.empty())
                {
                    for (size_t argIndex = AZ::eBehaviorBusForwarderEventIndices::ParameterFirst; argIndex < event.m_parameters.size(); ++argIndex)
                    {
                        const AZ::BehaviorParameter& parameter = event.m_parameters[argIndex];

                        Argument argument;

                        AZStd::string argumentKey = parameter.m_typeId.ToString<AZStd::string>();
                        AZStd::string argumentName = event.m_name;
                        AZStd::string argumentDescription = "";

                        if (!event.m_metadataParameters.empty() && event.m_metadataParameters.size() > argIndex)
                        {
                            argumentName = event.m_metadataParameters[argIndex].m_name;
                            argumentDescription = event.m_metadataParameters[argIndex].m_toolTip;
                        }

                        if (argumentName.empty())
                        {
                            Helpers::GetTypeNameAndDescription(parameter.m_typeId, argumentName, argumentDescription);
                        }

                        if (!event.m_metadataParameters.empty() && event.m_metadataParameters.size() > argIndex)
                        {
                            auto name = event.m_metadataParameters[argIndex].m_name;
                            auto tooltip = event.m_metadataParameters[argIndex].m_toolTip;

                            if (!name.empty())
                            {
                                argumentName = name;
                            }

                            if (!tooltip.empty())
                            {
                                argumentDescription = tooltip;
                            }
                        }

                        argument.m_typeId = argumentKey;
                        argument.m_details.m_name = argumentName;
                        argument.m_details.m_tooltip = argumentDescription;

                        SplitCamelCase(argument.m_details.m_name);

                        methodEntry.m_arguments.push_back(argument);
                    }
                }

                auto resultIndex = AZ::eBehaviorBusForwarderEventIndices::Result;
                const AZ::BehaviorParameter* resultParameter = event.HasResult() ? &event.m_parameters[resultIndex] : nullptr;
                if (resultParameter)
                {
                    Argument result;

                    AZStd::string resultKey = resultParameter->m_typeId.ToString<AZStd::string>();

                    AZStd::string resultName = event.m_name;
                    AZStd::string resultDescription = "";

                    if (!event.m_metadataParameters.empty() && event.m_metadataParameters.size() > resultIndex)
                    {
                        resultName = event.m_metadataParameters[resultIndex].m_name;
                        resultDescription = event.m_metadataParameters[resultIndex].m_toolTip;
                    }

                    if (resultName.empty())
                    {
                        Helpers::GetTypeNameAndDescription(resultParameter->m_typeId, resultName, resultDescription);
                    }

                    result.m_typeId = resultKey;
                    result.m_details.m_name = resultName;
                    result.m_details.m_tooltip = resultDescription;

                    SplitCamelCase(result.m_details.m_name);

                    methodEntry.m_results.push_back(result);
                }

                entry.m_methods.push_back(methodEntry);

            }

            behaviorEbus->m_destroyHandler->Invoke(handler); // Destroys the Created EbusHandler

            translationRoot.m_entries.push_back(entry);
        }

        if (!translationRoot.m_entries.empty())
        {
            return true;
        }

        return false;
    }

    void TranslationGeneration::SaveJSONData(const AZStd::string& filename, TranslationFormat& translationRoot)
    {
        rapidjson_ly::Document document;
        document.SetObject();
        rapidjson_ly::Value entries(rapidjson_ly::kArrayType);

        // Here I'll need to parse translationRoot myself and produce the JSON
        for (const auto& entrySource : translationRoot.m_entries)
        {
            rapidjson_ly::Value entry(rapidjson_ly::kObjectType);
            rapidjson_ly::Value value(rapidjson_ly::kStringType);

            value.SetString(entrySource.m_key.c_str(), document.GetAllocator());
            entry.AddMember(GraphCanvas::Schema::Field::key, value, document.GetAllocator());

            value.SetString(entrySource.m_context.c_str(), document.GetAllocator());
            entry.AddMember(GraphCanvas::Schema::Field::context, value, document.GetAllocator());

            value.SetString(entrySource.m_variant.c_str(), document.GetAllocator());
            entry.AddMember(GraphCanvas::Schema::Field::variant, value, document.GetAllocator());

            rapidjson_ly::Value details(rapidjson_ly::kObjectType);
            value.SetString(entrySource.m_details.m_name.c_str(), document.GetAllocator());
            details.AddMember("name", value, document.GetAllocator());

            Helpers::WriteString(details, "category", entrySource.m_details.m_category, document);
            Helpers::WriteString(details, "tooltip", entrySource.m_details.m_tooltip, document);
            Helpers::WriteString(details, "subtitle", entrySource.m_details.m_subtitle, document);

            entry.AddMember("details", details, document.GetAllocator());

            if (!entrySource.m_methods.empty())
            {
                rapidjson_ly::Value methods(rapidjson_ly::kArrayType);

                for (const auto& methodSource : entrySource.m_methods)
                {
                    rapidjson_ly::Value theMethod(rapidjson_ly::kObjectType);

                    value.SetString(methodSource.m_key.c_str(), document.GetAllocator());
                    theMethod.AddMember(GraphCanvas::Schema::Field::key, value, document.GetAllocator());

                    if (!methodSource.m_context.empty())
                    {
                        value.SetString(methodSource.m_context.c_str(), document.GetAllocator());
                        theMethod.AddMember(GraphCanvas::Schema::Field::context, value, document.GetAllocator());
                    }

                    if (!methodSource.m_entry.m_name.empty())
                    {
                        rapidjson_ly::Value entrySlot(rapidjson_ly::kObjectType);
                        value.SetString(methodSource.m_entry.m_name.c_str(), document.GetAllocator());
                        entrySlot.AddMember("name", value, document.GetAllocator());

                        Helpers::WriteString(entrySlot, "tooltip", methodSource.m_entry.m_tooltip, document);

                        theMethod.AddMember("entry", entrySlot, document.GetAllocator());
                    }

                    if (!methodSource.m_exit.m_name.empty())
                    {
                        rapidjson_ly::Value exitSlot(rapidjson_ly::kObjectType);
                        value.SetString(methodSource.m_exit.m_name.c_str(), document.GetAllocator());
                        exitSlot.AddMember("name", value, document.GetAllocator());

                        Helpers::WriteString(exitSlot, "tooltip", methodSource.m_exit.m_tooltip, document);

                        theMethod.AddMember("exit", exitSlot, document.GetAllocator());
                    }

                    rapidjson_ly::Value methodDetails(rapidjson_ly::kObjectType);

                    value.SetString(methodSource.m_details.m_name.c_str(), document.GetAllocator());
                    methodDetails.AddMember("name", value, document.GetAllocator());

                    Helpers::WriteString(methodDetails, "category", methodSource.m_details.m_category, document);
                    Helpers::WriteString(methodDetails, "tooltip", methodSource.m_details.m_tooltip, document);

                    theMethod.AddMember("details", methodDetails, document.GetAllocator());

                    if (!methodSource.m_arguments.empty())
                    {
                        rapidjson_ly::Value methodArguments(rapidjson_ly::kArrayType);

                        [[maybe_unused]] size_t index = 0;
                        for (const auto& argSource : methodSource.m_arguments)
                        {
                            rapidjson_ly::Value argument(rapidjson_ly::kObjectType);
                            rapidjson_ly::Value argumentDetails(rapidjson_ly::kObjectType);

                            value.SetString(argSource.m_typeId.c_str(), document.GetAllocator());
                            argument.AddMember("typeid", value, document.GetAllocator());

                            value.SetString(argSource.m_details.m_name.c_str(), document.GetAllocator());
                            argumentDetails.AddMember("name", value, document.GetAllocator());

                            Helpers::WriteString(argumentDetails, "category", argSource.m_details.m_category, document);
                            Helpers::WriteString(argumentDetails, "tooltip", argSource.m_details.m_tooltip, document);


                            argument.AddMember("details", argumentDetails, document.GetAllocator());

                            methodArguments.PushBack(argument, document.GetAllocator());

                        }

                        theMethod.AddMember("params", methodArguments, document.GetAllocator());

                    }

                    if (!methodSource.m_results.empty())
                    {
                        rapidjson_ly::Value methodArguments(rapidjson_ly::kArrayType);

                        for (const auto& argSource : methodSource.m_results)
                        {
                            rapidjson_ly::Value argument(rapidjson_ly::kObjectType);
                            rapidjson_ly::Value argumentDetails(rapidjson_ly::kObjectType);

                            value.SetString(argSource.m_typeId.c_str(), document.GetAllocator());
                            argument.AddMember("typeid", value, document.GetAllocator());

                            value.SetString(argSource.m_details.m_name.c_str(), document.GetAllocator());
                            argumentDetails.AddMember("name", value, document.GetAllocator());

                            Helpers::WriteString(argumentDetails, "category", argSource.m_details.m_category, document);
                            Helpers::WriteString(argumentDetails, "tooltip", argSource.m_details.m_tooltip, document);

                            argument.AddMember("details", argumentDetails, document.GetAllocator());

                            methodArguments.PushBack(argument, document.GetAllocator());
                        }


                        theMethod.AddMember("results", methodArguments, document.GetAllocator());

                    }

                    methods.PushBack(theMethod, document.GetAllocator());
                }

                entry.AddMember("methods", methods, document.GetAllocator());
            }

            if (!entrySource.m_slots.empty())
            {
                rapidjson_ly::Value slotsArray(rapidjson_ly::kArrayType);

                for (const auto& slotSource : entrySource.m_slots)
                {
                    rapidjson_ly::Value theSlot(rapidjson_ly::kObjectType);

                    value.SetString(slotSource.m_key.c_str(), document.GetAllocator());
                    theSlot.AddMember(GraphCanvas::Schema::Field::key, value, document.GetAllocator());

                    rapidjson_ly::Value sloDetails(rapidjson_ly::kObjectType);
                    if (!slotSource.m_details.m_name.empty())
                    {
                        Helpers::WriteString(sloDetails, "name", slotSource.m_details.m_name, document);
                        Helpers::WriteString(sloDetails, "tooltip", slotSource.m_details.m_tooltip, document);
                        theSlot.AddMember("details", sloDetails, document.GetAllocator());
                    }

                    if (!slotSource.m_data.m_details.m_name.empty())
                    {
                        rapidjson_ly::Value slotDataDetails(rapidjson_ly::kObjectType);
                        Helpers::WriteString(slotDataDetails, "name", slotSource.m_data.m_details.m_name, document);
                        theSlot.AddMember("details", slotDataDetails, document.GetAllocator());
                    }

                    slotsArray.PushBack(theSlot, document.GetAllocator());
                }

                entry.AddMember("slots", slotsArray, document.GetAllocator());
            }

            entries.PushBack(entry, document.GetAllocator());
        }

        document.AddMember("entries", entries, document.GetAllocator());

        AZ::IO::Path gemPath = Helpers::GetGemPath("ScriptCanvas.Editor");
        gemPath = gemPath / AZ::IO::Path("TranslationAssets");
        gemPath = gemPath / filename;
        gemPath.ReplaceExtension(".names");

        AZStd::string folderPath;

        AZ::StringFunc::Path::GetFolderPath(gemPath.c_str(), folderPath);

        if (!AZ::IO::FileIOBase::GetInstance()->Exists(folderPath.c_str()))
        {
            if (AZ::IO::FileIOBase::GetInstance()->CreatePath(folderPath.c_str()) != AZ::IO::ResultCode::Success)
            {
                AZ_Error("Translation", false, "Failed to create output folder");
                return;
            }
        }

        char resolvedBuffer[AZ_MAX_PATH_LEN] = { 0 };
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(gemPath.c_str(), resolvedBuffer, AZ_MAX_PATH_LEN);
        AZStd::string endPath = resolvedBuffer;
        AZ::StringFunc::Path::Normalize(endPath);

        AZ::IO::SystemFile outputFile;
        if (!outputFile.Open(endPath.c_str(),
            AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE |
            AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE_PATH |
            AZ::IO::SystemFile::OpenMode::SF_OPEN_WRITE_ONLY))
        {
            AZ_Error("Translation", false, "Failed to open file for writing: %s", filename.c_str());
            return;
        }

        rapidjson_ly::StringBuffer scratchBuffer;

        rapidjson_ly::PrettyWriter<rapidjson_ly::StringBuffer> writer(scratchBuffer);
        document.Accept(writer);

        outputFile.Write(scratchBuffer.GetString(), scratchBuffer.GetSize());
        outputFile.Close();

        scratchBuffer.Clear();

        AzQtComponents::ShowFileOnDesktop(endPath.c_str());

    }

    void TranslationGeneration::SplitCamelCase(AZStd::string& text)
    {
        AZStd::regex splitRegex(R"(/[a-z]+|[0-9]+|(?:[A-Z][a-z]+)|(?:[A-Z]+(?=(?:[A-Z][a-z])|[^AZa-z]|[$\d\n]))/g)");
        text = AZStd::regex_replace(text, splitRegex, " $&");
        text = AZ::StringFunc::LStrip(text);
        AZ::StringFunc::Replace(text, "  ", " ");
    }

    namespace Helpers
    {
        AZStd::string ReadStringAttribute(const AZ::AttributeArray& attributes, const AZ::Crc32& attribute)
        {
            AZStd::string attributeValue = "";
            if (auto attributeItem = azrtti_cast<AZ::AttributeData<AZStd::string>*>(AZ::FindAttribute(attribute, attributes)))
            {
                attributeValue = attributeItem->Get(nullptr);
                return attributeValue;
            }

            if (auto attributeItem = azrtti_cast<AZ::AttributeData<const char*>*>(AZ::FindAttribute(attribute, attributes)))
            {
                attributeValue = attributeItem->Get(nullptr);
                return attributeValue;
            }

            return {};
        }

        bool MethodHasAttribute(const AZ::BehaviorMethod* method, AZ::Crc32 attribute)
        {
            return AZ::FindAttribute(attribute, method->m_attributes) != nullptr; // warning C4800: 'AZ::Attribute *': forcing value to bool 'true' or 'false' (performance warning)
        }

        void GetTypeNameAndDescription(AZ::TypeId typeId, AZStd::string& outName, AZStd::string& outDescription)
        {
            AZ::SerializeContext* serializeContext{};
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(serializeContext, "Serialize Context is required");

            if (const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(typeId))
            {
                if (classData->m_editData)
                {
                    outName = classData->m_editData->m_name ? classData->m_editData->m_name : classData->m_name;
                    outDescription = classData->m_editData->m_description ? classData->m_editData->m_description : "";
                }
                else
                {
                    outName = classData->m_name;
                }
            }
        }

        AZStd::string GetGemPath(const AZStd::string& gemName)
        {
            if (auto settingsRegistry = AZ::Interface<AZ::SettingsRegistryInterface>::Get(); settingsRegistry != nullptr)
            {
                AZ::IO::Path gemSourceAssetDirectories;
                AZStd::vector<AzFramework::GemInfo> gemInfos;
                if (AzFramework::GetGemsInfo(gemInfos, *settingsRegistry))
                {
                    auto FindGemByName = [gemName](const AzFramework::GemInfo& gemInfo)
                    {
                        return gemInfo.m_gemName == gemName;
                    };

                    // Gather unique list of Gem Paths from the Settings Registry
                    auto foundIt = AZStd::find_if(gemInfos.begin(), gemInfos.end(), FindGemByName);
                    if (foundIt != gemInfos.end())
                    {
                        const AzFramework::GemInfo& gemInfo = *foundIt;
                        for (const AZ::IO::Path& absoluteSourcePath : gemInfo.m_absoluteSourcePaths)
                        {
                            gemSourceAssetDirectories = (absoluteSourcePath / gemInfo.GetGemAssetFolder());
                        }

                        return gemSourceAssetDirectories.c_str();
                    }
                }
            }
            return "";
        }

        AZStd::string GetCategory(const AZ::SerializeContext::ClassData* classData)
        {
            AZStd::string categoryPath;

            if (classData->m_editData)
            {
                auto editorElementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                if (editorElementData)
                {
                    if (auto categoryAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::Category))
                    {
                        if (auto categoryAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(categoryAttribute))
                        {
                            categoryPath = categoryAttributeData->Get(nullptr);
                        }
                    }
                }
            }

            return categoryPath;
        }

        AZStd::string GetLibraryCategory(const AZ::SerializeContext& serializeContext, const AZStd::string& nodeName)
        {
            AZStd::string category;

            // Get all the types.
            auto EnumerateLibraryDefintionNodes = [&nodeName, &category, &serializeContext](
                const AZ::SerializeContext::ClassData* classData, const AZ::Uuid&) -> bool
            {
                AZStd::string categoryPath = classData->m_editData ? classData->m_editData->m_name : classData->m_name;

                if (classData->m_editData)
                {
                    auto editorElementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                    if (editorElementData)
                    {
                        if (auto categoryAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::Category))
                        {
                            if (auto categoryAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(categoryAttribute))
                            {
                                categoryPath = categoryAttributeData->Get(nullptr);
                            }
                        }
                    }
                }

                // Children
                for (auto& node : ScriptCanvas::Library::LibraryDefinition::GetNodes(classData->m_typeId))
                {
                    // Pass in the associated class data so we can do more intensive lookups?
                    const AZ::SerializeContext::ClassData* nodeClassData = serializeContext.FindClassData(node.first);

                    if (nodeClassData == nullptr)
                    {
                        continue;
                    }

                    // Skip over some of our more dynamic nodes that we want to populate using different means
                    else if (nodeClassData->m_azRtti && nodeClassData->m_azRtti->IsTypeOf<ScriptCanvas::Nodes::Core::GetVariableNode>())
                    {
                        continue;
                    }
                    else if (nodeClassData->m_azRtti && nodeClassData->m_azRtti->IsTypeOf<ScriptCanvas::Nodes::Core::SetVariableNode>())
                    {
                        continue;
                    }
                    else
                    {
                        if (node.second == nodeName)
                        {
                            category = categoryPath;
                            return false;
                        }
                    }
                }

                return true;
            };

            const AZ::TypeId& libraryDefTypeId = azrtti_typeid<ScriptCanvas::Library::LibraryDefinition>();
            serializeContext.EnumerateDerived(EnumerateLibraryDefintionNodes, libraryDefTypeId, libraryDefTypeId);

            return category;
        }

        void WriteString(rapidjson_ly::Value& owner, const AZStd::string& key, const AZStd::string& value, rapidjson_ly::Document& document)
        {
            if (key.empty() || value.empty())
            {
                return;
            }

            rapidjson_ly::Value item(rapidjson_ly::kStringType);
            item.SetString(value.c_str(), document.GetAllocator());

            rapidjson_ly::Value keyVal(rapidjson_ly::kStringType);
            keyVal.SetString(key.c_str(), document.GetAllocator());

            owner.AddMember(keyVal, item, document.GetAllocator());
        }

    }
}
