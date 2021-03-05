/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

// AZ
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

// Graph Model
#include <GraphModel/Model/Common.h>

namespace AZ
{
    namespace IO
    {
        class FileIOStream;
    }
}

namespace GraphModel
{
    //! This is a manager that exists to support ModuleNode. A ModuleNode is a node that contains
    //! another node graph to be reused as a single node. If there are multiple ModuleNode instances 
    //! that all use the same graph, we should only need one copy of the referenced graph in memory.
    //! The collection of available modules graphs will be managed here.
    //! The graphs stored here are const/immutable, and used only for instancing ModuleNodes, which
    //! do not make any changes to the underlying module graph.
    class ModuleGraphManager
        : public AzToolsFramework::AssetSystemBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ModuleGraphManager, AZ::SystemAllocator, 0);
        AZ_RTTI(Graph, "{68476353-C672-4408-9B34-A409CC63858E}");

        explicit ModuleGraphManager(IGraphContextPtr graphContext, AZ::SerializeContext* serializeContext = nullptr);
        virtual ~ModuleGraphManager();

        //! Returns the Graph loaded from a module source file. If the file has already been loaded,
        //! it simply returns the Graph. If it has not been loaded yet, this function will first load
        //! the Graph from the source file.
        //! \param sourceFileId  Unique Id of the sourfe file that contains a module graph
        AZ::Outcome<ConstGraphPtr, AZStd::string> GetModuleGraph(AZ::Uuid sourceFileId);
        
    protected:
        //! Loads a module graph from the given stream
        virtual ConstGraphPtr LoadGraph(AZ::IO::FileIOStream& stream);

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::AssetSystemBus

        //! WHen a module graph source file is added or changed, this will cause the Graph to be reloaded
        void SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceUUID) override;

        //////////////////////////////////////////////////////////////////////////

    private:
        //! Loads a Graph from a module source file
        //! \param sourceFileId  Unique Id of the sourfe file that contains a module graph
        AZ::Outcome<ConstGraphPtr, AZStd::string> LoadGraph(AZ::Uuid sourceFileId);

        // We use a weak_ptr to allow the graphs to go out of scope and be deleted when not used
        using ModuleGraphMap = AZStd::unordered_map<AZ::Uuid /*Source File ID*/, AZStd::weak_ptr<const Graph>>;

        AZStd::weak_ptr<IGraphContext> m_graphContext; //!< interface to client system specific data and functionality. Uses a weak_ptr so the IGraphContext can hold this ModuleGraphManager.
        AZStd::string m_moduleFileExtension;
        AZ::SerializeContext* m_serializeContext;
        ModuleGraphMap m_graphs;
    };

} // namespace GraphModel
