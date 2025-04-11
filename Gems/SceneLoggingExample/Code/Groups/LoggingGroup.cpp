/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <Groups/LoggingGroup.h>

namespace SceneLoggingExample
{
    const char* LoggingGroup::s_disabledOption = "No logging";

    LoggingGroup::LoggingGroup()
        : m_logProcessingEvents(true)
    {
    }

    // The data in groups will be saved to the manifest file and will be reflected in the Scene Settings
    // window. For those systems to do their work, the LoggingGroup needs to tell a bit more about itself 
    // than the other classes in this example.
    void LoggingGroup::Reflect(AZ::ReflectContext* context)
    {
        // There are different kind of contexts, but for groups and rules, the only one that's
        // interesting is the SerializeContext. Check if the provided context is one.
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        // Next, specify the fields that need to be serialized to and from a manifest. This allows new
        // fields to be stored and loaded from the manifest (.assetinfo file). These are also needed
        // for the edit context below.
        serializeContext->Class<LoggingGroup, AZ::SceneAPI::DataTypes::IManifestObject>()->Version(1)
            ->Field("groupName", &LoggingGroup::m_groupName)
            ->Field("graphLogRoot", &LoggingGroup::m_graphLogRoot)
            ->Field("logProcessingEvents", &LoggingGroup::m_logProcessingEvents);

        // The EditContext allows you to add additional meta information to the previously registered fields. 
        // This meta information will be used in the Scene Settings, which uses the Reflected Property Editor.
        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            editContext->Class<LoggingGroup>("Logger", "Add additional logging to the SceneAPI.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute("AutoExpand", true)
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                ->DataElement(AZ::Edit::UIHandlers::Default, &LoggingGroup::m_groupName, 
                    "Name", "The name of the group will be used in the log")
                // The Reflected Property Editor can pick a default editor for many types. However, for the string
                // that will store the selected node, a more specialized editor is needed. NodeListSelection is
                // one such editor and it is SceneGraph-aware. It allows the selection of a specific node from the
                // graph and the selectable items can be filtered. You can find other available editors in the 
                // "RowWidgets"-folder of the SceneUI.
                ->DataElement(AZ_CRC_CE("NodeListSelection"), &LoggingGroup::m_graphLogRoot,
                    "Graph log root", "Select the node in the graph to list children of to the log, or disable logging.")
                    ->Attribute(AZ_CRC_CE("DisabledOption"), s_disabledOption)
                    // Nodes in the SceneGraph can be marked as endpoints. To the graph, this means that these nodes 
                    // are not allowed to have children. While not a true one-to-one mapping, endpoints often act as 
                    // attributes to a node. For example, a transform can be marked as an endpoint. This means that 
                    // it applies its transform to the parent object like an attribute. If the transform is not marked 
                    // as an endpoint, then it is the root transform for the group(s) that are its children.
                    ->Attribute(AZ_CRC_CE("ExcludeEndPoints"), true)
                ->DataElement(AZ::Edit::UIHandlers::Default, &LoggingGroup::m_logProcessingEvents,
                    "Log processing events", "Log processing events as they happen.");
        }
    }

    void LoggingGroup::SetName(const AZStd::string& name)
    {
        m_groupName = name;
    }

    void LoggingGroup::SetName(AZStd::string&& name)
    {
        m_groupName = AZStd::move(name);
    }

    const AZStd::string& LoggingGroup::GetName() const
    {
        return m_groupName;
    }

    // Groups need to provide a unique id that will be used to create the final sub id for the product
    // build using this group. While new groups created through the UI can remain fully random, it's
    // important that ids used for defaults are recreated the same way every time. It's recommended this
    // is done by using the source guid of the file and calling DataTypes::Utilities::CreateStableUuid.
    // If the id doesn't remain stable between updates this will cause the sub id to change which will in
    // turn cause the objects links to those products to break.
    //
    // As this example doesn't have a product, the id is not important so just always return the randomly
    // generated id.
    const AZ::Uuid& LoggingGroup::GetId() const
    {
        return m_id;
    }

    // Groups have the minimal amount of options to generate a working product in the cache and nothing more. 
    // A group might not be perfect or contain all the data the user would expect, but it will load in the 
    // engine and not crash. You can add additional settings to fine tune the exporting process in the form of 
    // rules (or "modifiers" in the Scene Settings UI). Rules usually group a subset of settings together, 
    // such as control of physics or level of detail. This approach keeps UI clutter to a minimum by only 
    // presenting options that are relevant for the user's file, while still providing access to all available 
    // settings. 
    // 
    // By using the "GetAvailableModifiers" in the ManifestMetaInfoHandler EBus, it's possible to filter out 
    // any options that are not relevant to the group. For example, if a group only allows for a single instance 
    // of a rule, it would no longer be added to this call if there is already one. Because the logging doesn't 
    // require any rules, empty defaults are provided.
    AZ::SceneAPI::Containers::RuleContainer& LoggingGroup::GetRuleContainer()
    {
        return m_ruleContainer;
    }

    const AZ::SceneAPI::Containers::RuleContainer& LoggingGroup::GetRuleContainerConst() const
    {
        return m_ruleContainer;
    }

    const AZStd::string& LoggingGroup::GetGraphLogRoot() const
    {
        return m_graphLogRoot;
    }

    bool LoggingGroup::DoesLogGraph() const
    {
        return m_graphLogRoot.compare(s_disabledOption) != 0;
    }

    bool LoggingGroup::DoesLogProcessingEvents() const
    {
        return m_logProcessingEvents;
    }

    void LoggingGroup::ShouldLogProcessingEvents(bool state)
    {
        m_logProcessingEvents = state;
    }
} // namespace SceneLoggingExample
