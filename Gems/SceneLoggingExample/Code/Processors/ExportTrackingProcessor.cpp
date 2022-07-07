/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <Processors/ExportTrackingProcessor.h>
#include <Groups/LoggingGroup.h>

namespace SceneLoggingExample
{
    ExportTrackingProcessor::ExportTrackingProcessor()
    {
        // The scene conversion and exporting process uses the CallProcessorBus to move data and trigger additional work. 
        // The CallProcessorBus operates differently than typical EBuses because it doesn't have a specific set of functions 
        // that you can call. Instead, it works like a pseudo-remote procedure call, where the arguments for what would 
        // normally be a function are stored in a context. 
        //
        // The CallProcessorBus provides a single place to register and trigger the context calls. Based on the type 
        // of context, the appropriate functionality is executed. To make it easier to work with, a binding layer 
        // called CallProcessorBinder allows binding to a function that takes a context as an argument and performs 
        // all the routing. One of the benefits of this approach is that it provides several places to hook custom code 
        // into without having to update existing code. For example, you can use this approach to write additional information 
        // to a mesh file without having to change how the .cgf exporter works.
        //
        // The example below attaches the PrepareForExport function to the PreExportEventContext so that this context 
        // is sent to the CallProcessorBus at the start of every conversion and export process.
        BindToCall(&ExportTrackingProcessor::PrepareForExport);
        
        // By default, the CallProcessorBinder will only activate if the context exactly matches the argument of the 
        // bound function. That setup is often desired to avoid receiving many unrelated events. However, this example 
        // uses "Derived" and binds to the ICallContext so that all events are printed. Note that many events get fired 
        // multiple times due to multiple phases (pre, active, and post).
        BindToCall(&ExportTrackingProcessor::ContextCallback, AZ::SceneAPI::Events::CallProcessorBinder::TypeMatch::Derived);
    }

    // Reflection is a basic requirement for components. For Exporting components, you can often keep the Reflect() function  
    // simple because the SceneAPI just needs to be able to find the component. For more details on the Reflect() function, 
    // see LoggingGroup.cpp.
    void ExportTrackingProcessor::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ExportTrackingProcessor, AZ::SceneAPI::SceneCore::ExportingComponent>()->Version(1);
        }
    }

    // This function is now bound to the CallProcessorBinder, so it will be called as soon as exporting starts. It is a good point 
    // at which to look at the available groups and see if there are groups that need to log the scene graph.
    AZ::SceneAPI::Events::ProcessingResult ExportTrackingProcessor::PrepareForExport(AZ::SceneAPI::Events::PreExportEventContext& context)
    {
        // Before doing any work, the manifest must be searched for instructions about what needs to be done. The instructions 
        // are in the form of groups and rules. In this example, we use this opportunity to log the scene graphs that are 
        // listed in every logging group.
        //
        // In this example, the manifest is cached for later use. This is typically not recommended because multiple builders can
        // be running at the same time, resulting in callbacks from multiple exports that are in flight. In general, you should
        // pass in any required information as a member of the context.
        m_manifest = &context.GetScene().GetManifest();
        
        // The manifest is a flat list of IManifestObjects and relies on AZ_RTTI to determine its content. Content can be retrieved
        // through an index-based approach or an iterator approach. The index-based approach tends to be easier to understand but
        // it also requires you to work with more code. The iterator has more complex syntax and can produce more complicated compile 
        // errors, but it has several utilities that make it more concise to work with and often makes code that better communicates 
        // intention. To provide examples of both cases, the index-based approach is used below, and the iterator approach is used in 
        // the ContextCallback function.
        size_t count = m_manifest->GetEntryCount();
        for (size_t i = 0; i < count; ++i)
        {
            AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IManifestObject> entry = m_manifest->GetValue(i);

            // The azrtti_cast is a run-time type-aware cast that will return a nullptr if the provided type
            // can't be cast to the target class. That principle is used here to filter for LoggingGroups only.
            const LoggingGroup* group = azrtti_cast<const LoggingGroup*>(entry.get());
            if (group)
            {
                if (group->DoesLogGraph())
                {
                    // For every group, write out the graph information, starting at the node the user selected.
                    LogGraph(context.GetScene().GetGraph(), group->GetGraphLogRoot());
                }
            }
        }

        return AZ::SceneAPI::Events::ProcessingResult::Success;
    }
    
    // In the constructor, this function was bound to accept any contexts that are derived from ICallContext, which is the base
    // for all CallProcessorBus events. This allows for monitoring of everything that happens during conversion and exporting.
    AZ::SceneAPI::Events::ProcessingResult ExportTrackingProcessor::ContextCallback([[maybe_unused]] AZ::SceneAPI::Events::ICallContext& context)
    {
        // PrepareForExport demonstrated getting data from the manifest using the index-based approach. The code below demonstrates the
        // iterator approach by getting a view (a begin- and end-iterator) and creating a filtered view on top of it.
        auto manifestValues = m_manifest->GetValueStorage();
        auto view = AZ::SceneAPI::Containers::MakeExactFilterView<LoggingGroup>(manifestValues);

        // Now that the filtered view of the manifest is constructed, the loop below will list only LoggingGroups. Groups typically
        // map one-to-one to an output file. This is not a hard requirement, but it is most often the case. In that case, it is typical 
        // for multiple groups to be individually exported to their own file. Most groups will also have rules (also called modifiers) 
        // that add fine-grained control to the conversion process. Usually this is in one particular area such as the world matrix or physics.
        for (const auto& it : view)
        {
            if (it.DoesLogProcessingEvents())
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "ExportEvent (%s): %s", it.GetName().c_str(), context.RTTI_GetTypeName());
            }
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
    uint8_t ExportTrackingProcessor::GetPriority() const
    {
        return EarliestProcessing;
    }

    // During the loading process, an in-memory representation of the scene is stored inside the SceneGraph. The SceneCore library
    // provides several interfaces that you can use as a basis for data that helps establish a common vocabulary for the various parts 
    // of the SceneAPI. The SceneData library provides an optional set of implementations of these interfaces for your convenience. Similar 
    // to the manifest, the SceneGraph can provide its data through an index-based or an iterator-based approach.
    void ExportTrackingProcessor::LogGraph(const AZ::SceneAPI::Containers::SceneGraph& graph, const AZStd::string& nodePath) const
    {
        namespace SceneViews = AZ::SceneAPI::Containers::Views;

        // Between runs, the source scene files (for example, .fbx) can change. Storing indices to nodes can lead to unexpected behavior,
        // so it is generally preferable to store the node path instead. This makes looking up nodes by name a common pattern. Rather than 
        // doing a linear search over the names, the SceneGraph has an optimized lookup of the node name.
        AZ::SceneAPI::Containers::SceneGraph::NodeIndex nodeIndex = graph.Find(nodePath);
        if (!nodeIndex.IsValid())
        {
            // Any SceneGraph is guaranteed to have at least a root node, even if it is otherwise empty. Note that not all loaders 
            // may choose to use this node. This can occasionally lead to an unexpected node at the top of the graph.
            nodeIndex = graph.GetRoot();
        }

        // The SceneGraph stores its data in separate containers, such as a content list and a name list. The relationship between nodes is 
        // stored in a similar flat list. This allows iterating over the content in both a hierarchical and a linear way. Because hierarchical
        // traversal is much more expensive than linear traversal, questions such as "list all entries of type X" are answered much more efficiently 
        // by using linear traversal.
        auto nameStorage = graph.GetNameStorage();
        auto contentStorage = graph.GetContentStorage();

        // As described previously, the name and content of the graph are stored separately. However, sometimes both are needed when traversing 
        // the graph. To combine the two in a single iterator, you can use the pair iterator in the following way.
        auto nameContentView = SceneViews::MakePairView(nameStorage, contentStorage);

        // The SceneGraph has several iterators that help with traversing the graph in a hierarchical way:
        //    - SceneGraphUpwardsIterator - Traverses from a given node to the root of the graph.
        //    - SceneGraphDownwardsIterator - Traverses over all children of a given node either breadth-first or depth-first.
        //    - SceneGraphChildIterator - Traverses over the direct children of a node only.
        // For this example, all nodes beneath the node that the user selected are listed so a downwards iterator is most appropriate.
        auto graphDownwardsView = SceneViews::MakeSceneGraphDownwardsView<SceneViews::BreadthFirst>(graph, nodeIndex, nameContentView.begin(), true);
        for (auto it = graphDownwardsView.begin(); it != graphDownwardsView.end(); ++it)
        {
            // While it's generally preferable to stick with either index- or iterator-based traversal, there may be times where switching between one
            // or the other becomes necessary. The SceneGraph provides utility functions to convert between the two approaches.
            [[maybe_unused]] AZ::SceneAPI::Containers::SceneGraph::NodeIndex itNodeIndex = graph.ConvertToNodeIndex(it.GetHierarchyIterator());

            // Nodes in the SceneGraph can be marked as endpoints. To the graph, this means that these nodes are not allowed to have children. 
            // While not a true one-to-one mapping, endpoints often act as attributes to a node. For example, a transform can be marked as an endpoint. 
            // This means that it applies its transform to the parent object like an attribute. If the transform is not marked as an endpoint, then it 
            // is the root transform for the group(s) that are its children.
            AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "'%s' '%s' contains data of type '%s'.", (graph.IsNodeEndPoint(itNodeIndex) ? "End point node" : "Node"),
                it->first.GetPath(),
                it->second ? it->second->RTTI_GetTypeName() : "No data");
        }
    }
} // namespace SceneLoggingExample
