/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <Behaviors/LoggingGroupBehavior.h>
#include <Groups/LoggingGroup.h>

namespace SceneLoggingExample
{
    // Reflection is a basic requirement for components. For behaviors, you can often keep the Reflect() 
    // function simple because the SceneAPI just needs to be able to find the component. For more details 
    // on the Reflect() function, see LoggingGroup.cpp.
    void LoggingGroupBehavior::Reflect(AZ::ReflectContext* context)
    {
        // The data and UI elements used in the SceneAPI are not components, but they need to be reflected 
        // for serialization and the Scene Settings to work. This can done at any point in the gem, but the 
        // behavior that controls the data is a good place for this. Because the LoggingGroupBehavior controls 
        // the LoggingGroup, we will register it here.
        LoggingGroup::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<LoggingGroupBehavior, AZ::SceneAPI::SceneCore::BehaviorComponent>()->Version(1);
        }
    }

    // Later in this example, messages that deal with manifest changes and loading files will be used
    // to create the various ways that the behavior controls settings. Before any events can be sent 
    // to the behavior, it first needs to be connected to the EBuses that it monitors.
    void LoggingGroupBehavior::Activate()
    {
        AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusConnect();
        AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusConnect();
    }

    // Disconnect from the EBuses when this behavior is no longer active.
    void LoggingGroupBehavior::Deactivate()
    {
        AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusDisconnect();
        AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusDisconnect();
    }

    // This behavior will control the logging for the UI, so let's begin by registering the LoggingGroup with the UI under a new
    // "Logging" tab and ignore the position of the tab for now. This will add a new tab to the Scene Settings window. The tab
    // will have a single button to add a LoggingGroup. If additional groups are registered under the same tab name, the button
    // will be changed to a drop-down button and allow the registered groups to be added.
    //
    // The scene is passed as one of the arguments so that the manifest and/or the graph can be inspected to determine if a group
    // should be added. For example, if the graph doesn't contain any meshes, the mesh group can be left out. This helps prevent
    // users from adding groups that have no effect.
    void LoggingGroupBehavior::GetCategoryAssignments(CategoryRegistrationList& categories, [[maybe_unused]] const AZ::SceneAPI::Containers::Scene& scene)
    {
        categories.emplace_back("Logging", LoggingGroup::TYPEINFO_Uuid(), s_loggingPreferredTabOrder);
    }

    // When a scene is loaded for the first time (for example, from an .fbx file), there won't be a manifest (.assetinfo file).
    // If the scene was loaded previously, there might be a manifest that requires updates because it contains values that no  
    // longer match the graph. This EBus call gives a one-time opportunity right after loading has completed to update the manifest  
    // or to add data to a new one.
    //
    // In this example, let's add a LoggingGroup to a new manifest only. Don't forget to remove the manifest (.assetinfo file)
    // for your test scene file. Otherwise, the following code won't trigger.
    AZ::SceneAPI::Events::ProcessingResult LoggingGroupBehavior::UpdateManifest(AZ::SceneAPI::Containers::Scene& scene,
        ManifestAction action, [[maybe_unused]] RequestingApplication requester)
    {
        if (action == ManifestAction::ConstructDefault)
        {
            AZStd::shared_ptr<LoggingGroup> group = AZStd::make_shared<LoggingGroup>();

            // This might not be the only behavior that wants to make modifications to the new group. An example is a material
            // behavior that wants to add a material rule when a mesh group is created. By calling the EBus below, other behaviors
            // get a chance to change or add their own values. Listening to this EBus is also a good place to add any settings to 
            // the new group instead of doing it here. This is because this EBus is also called when tools such as the UI create 
            // a new group, which keeps initialization in one place.
            AZ::SceneAPI::Events::ManifestMetaInfoBus::Broadcast(
                &AZ::SceneAPI::Events::ManifestMetaInfoBus::Events::InitializeObject, scene, *group);
            if (scene.GetManifest().AddEntry(AZStd::move(group)))
            {
                // Let the SceneAPI know that a LoggingGroup has been successfully added.
                return AZ::SceneAPI::Events::ProcessingResult::Success;
            }
            else
            {
                // It wasn't possible to add the new logging group, so let the SceneAPI know that
                // a problem was encountered. Don't forget to also tell the user what is going on,
                // because this will cause the loading to fail.
                AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Unable to add a new logging group.");
                return AZ::SceneAPI::Events::ProcessingResult::Failure;
            }
        }
        // In any other situation, there's no plan to do anything so tell the SceneAPI to ignore this behavior.
        return AZ::SceneAPI::Events::ProcessingResult::Ignored;
    }

    // When a new manifest object is created, the caller can choose to allow other behaviors to change or add their own data, such 
    // as rules to a group. The EBus call in the above function shows a typical use case. Using InitializeObject() provides a more 
    // powerful alternative to default values. It allows domain logic to be spread to appropriate behaviors, but also allows general 
    // awareness of the manifest and graph to select default values that are more appropriate to the user.
    //
    // For this example, let's use the passed-in manifest to look for the last LoggingGroup in the manifest and use the log setting that 
    // is its opposite. When viewing this in the Scene Settings window, "Log processing events" will be off when adding a new logging group.
    // The one directly above it is on, and vice versa.
    void LoggingGroupBehavior::InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target)
    {
        // If the item being added isn't a LoggingGroup, ignore it.
        if (!target.RTTI_IsTypeOf(LoggingGroup::TYPEINFO_Uuid()))
        {
            return;
        }

        LoggingGroup* newGroup = azrtti_cast<LoggingGroup*>(&target);
        AZ_Assert(newGroup, "Manifest object has been identified as LoggingGroup, but failed to cast to it.");

        // First create a view that only contains instances that exactly match LoggingGroups. Use MakeDerivedFilterView() to do 
        // the same for any instances that implement a specific interface and/or base class. For more details on using iterators 
        // to get data from the manifest and graph, see ExportTrackingProcessor.cpp.
        auto values = scene.GetManifest().GetValueStorage();
        auto view = AZ::SceneAPI::Containers::MakeExactFilterView<LoggingGroup>(values);
        
        // Find the last LoggingGroup in the manifest.
        auto last = view.begin();
        while (AZStd::next(last) != view.end())
        {
            ++last;
        }
        // Only take the values if there's actually another LoggingGroup in the manifest.
        if (last != view.end())
        {
            newGroup->ShouldLogProcessingEvents(!last->DoesLogProcessingEvents());
        }

        // Let's also set a default name for this group. Groups often match one-to-one with the file that they output.
        // For example, a Mesh Group will produce a .cgf file with the same name. If the name is used as a file name, 
        // it is important to check whether it's a valid path name and isn't duplicating another name.
        const size_t size = AZStd::distance(view.begin(), view.end());
        newGroup->SetName(AZStd::string::format("Logger_%zu", size));
    }
} // namespace SceneLoggingExample
