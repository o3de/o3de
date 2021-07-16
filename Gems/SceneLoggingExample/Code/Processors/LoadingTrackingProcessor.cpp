/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <Processors/LoadingTrackingProcessor.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace SceneLoggingExample
{
    LoadingTrackingProcessor::LoadingTrackingProcessor()
    {
        // For details about the CallProcessorBus and CallProcessorBinder, see ExportTrackingProcessor.cpp. 
        BindToCall(&LoadingTrackingProcessor::ContextCallback, AZ::SceneAPI::Events::CallProcessorBinder::TypeMatch::Derived);
    }

    // Reflection is a basic requirement for components. For Loading components, you can often keep the Reflect() function  
    // simple because the SceneAPI just needs to be able to find the component. For more details on the Reflect() function, 
    // see LoggingGroup.cpp.
    void LoadingTrackingProcessor::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<LoadingTrackingProcessor, AZ::SceneAPI::SceneCore::LoadingComponent>()->Version(1);
        }
    }

    // Later in this example, we will listen to and log messages that relate to file loading. 
    // Before this can happen, we must connect to the bus that sends the messages.
    void LoadingTrackingProcessor::Activate()
    {
        AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusConnect();
        
        // Forward the call to the LoadingComponent so that the call bindings get activated.
        AZ::SceneAPI::SceneCore::LoadingComponent::Activate();
    }

    // Disconnect from the bus upon deactivation.
    void LoadingTrackingProcessor::Deactivate()
    {
        AZ::SceneAPI::SceneCore::LoadingComponent::Deactivate();

        // Forward the call to the LoadingComponent so that the call bindings get deactivated.
        AZ::SceneAPI::Events::CallProcessorBus::Handler::BusDisconnect();

        AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusDisconnect();
    }

    // Loading starts by announcing that loading will begin shortly. This provides an opportunity to prepare 
    // caches or to take any additional steps that are required before loading.
    AZ::SceneAPI::Events::ProcessingResult LoadingTrackingProcessor::PrepareForAssetLoading(AZ::SceneAPI::Containers::Scene& /*scene*/,
        RequestingApplication /*requester*/)
    {
        AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Preparing to load a scene.");
        // This function doesn't contribute anything to the loading, so let the SceneAPI know that it can ignore its contributions.
        return AZ::SceneAPI::Events::ProcessingResult::Ignored;
    }

    // After a call to PrepareForAssetLoading has been dispatched, the scene file (for example, .fbx) will be loaded. 
    // This is normally what scene builders will be looking for. If the file has an extension that a scene builder 
    // understands, it will start reading the source file, convert the data, and store it in the scene. This is also 
    // true for loading the manifest file, which happens in this same pass. 
    //
    // For this example, nothing is done because there's no data to read. We just echo the steps that are taken.
    AZ::SceneAPI::Events::LoadingResult LoadingTrackingProcessor::LoadAsset(AZ::SceneAPI::Containers::Scene& /*scene*/,
        [[maybe_unused]] const AZStd::string& path, const AZ::Uuid& /*guid*/, RequestingApplication /*requester*/)
    {
        AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Loading scene from '%s'.", path.c_str());
        return AZ::SceneAPI::Events::LoadingResult::Ignored;
    }

    // After the scene file and manifest are loaded, we finalize the loading by making two calls: first to FinalizeAssetLoading() 
    // and then to UpdateManifest(). 
    //
    // FinalizeAssetLoading() is the best time to close out any temporary buffers, clear cache, patch pointers, and any 
    // other final steps that are required to put the graph in a valid state and perform any necessary cleanup. 
    // We also disconnect from the Call Processor bus so that we won't receive export events later. It is possible to 
    // make updates to the manifest in FinalizeAssetLoading(), but UpdateManifest() is a better place to do this. 
    void LoadingTrackingProcessor::FinalizeAssetLoading(AZ::SceneAPI::Containers::Scene& /*scene*/, RequestingApplication /*requester*/)
    {
        AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Finished loading scene.");
    }

    // UpdateManifest() provides additional information about the state of the manifest, such as if a default manifest is being
    // built or an existing one is being updated. The SceneGraph is ready at this point, so this function can be used to create a
    // new manifest or make corrections to an existing one.
    AZ::SceneAPI::Events::ProcessingResult LoadingTrackingProcessor::UpdateManifest(AZ::SceneAPI::Containers::Scene& /*scene*/, ManifestAction action,
        RequestingApplication /*requester*/)
    {
        switch (action)
        {
        case ManifestAction::ConstructDefault:
            AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Constructing a new manifest.");
            break;
        case ManifestAction::Update:
            AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Updating the manifest.");
            break;
        default:
            AZ_TracePrintf(AZ::SceneAPI::Utilities::WarningWindow, "Unknown manifest update action.");
            break;
        }
        return AZ::SceneAPI::Events::ProcessingResult::Ignored;
    }

    // With the SceneAPI, the order in which an EBus calls its listeners is mostly random. This generally isn't a problem because most work
    // is done in isolation. If there is a dependency, we recommend that you break a call into multiple smaller calls, but this isn't always
    // an option. For example, perhaps there is no source code available for third-party extensions or you are trying to avoid making code
    // changes to the engine/editor. For those situations, the Call Processor allows you to specify a priority to make sure that a call is made
    // before or after all other listeners have done their work.
    //
    // In this example, we want the log messages to be printed before any other listeners do their work and potentially print their data. 
    // To accomplish this, we set the priority to the highest available number.
    uint8_t LoadingTrackingProcessor::GetPriority() const
    {
        return EarliestProcessing;
    }

    // In the constructor, this function was bound to accept any contexts that are derived from ICallContext, which is the base
    // for all CallProcessorBus events. This allows for monitoring of everything that happens during the loading process.
    AZ::SceneAPI::Events::ProcessingResult LoadingTrackingProcessor::ContextCallback([[maybe_unused]] AZ::SceneAPI::Events::ICallContext& context)
    {
        AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "LoadEvent: %s", context.RTTI_GetTypeName());
        return AZ::SceneAPI::Events::ProcessingResult::Ignored;
    }
} // namespace SceneLoggingExample
