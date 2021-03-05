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

// AZ
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Component/ComponentApplicationBus.h>

// Graph Model
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/IGraphContext.h>
#include <GraphModel/Model/Module/ModuleGraphManager.h>

namespace GraphModel
{
    ModuleGraphManager::ModuleGraphManager(IGraphContextPtr graphContext, AZ::SerializeContext* serializeContext)
        : m_graphContext(graphContext)
        , m_moduleFileExtension(graphContext->GetModuleFileExtension())
        , m_serializeContext(serializeContext)
    {
        if (m_serializeContext == nullptr)
        {
            // use the default app serialize context
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            if (!m_serializeContext)
            {
                AZ_Error(graphContext->GetSystemName(), false, "ModuleGraphManager: No serialize context provided! We will not be able to load module files.");
            }
        }

        AzToolsFramework::AssetSystemBus::Handler::BusConnect();
    }

    ModuleGraphManager::~ModuleGraphManager()
    {
        AzToolsFramework::AssetSystemBus::Handler::BusDisconnect();
    }

    void ModuleGraphManager::SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceUUID)
    {
        AZStd::string extension;
        if (AzFramework::StringFunc::Path::GetExtension(relativePath.data(), extension) && extension == m_moduleFileExtension)
        {
            // Force the manager to reload the graph next time GetModuleGraph() is called.
            m_graphs.erase(sourceUUID);
        }
    }

    ConstGraphPtr ModuleGraphManager::LoadGraph(AZ::IO::FileIOStream& stream)
    {
        GraphPtr graph = AZStd::make_shared<Graph>();
        bool loadSuccess = AZ::Utils::LoadObjectFromStreamInPlace(stream, *graph, m_serializeContext);
        if (loadSuccess)
        {
            graph->PostLoadSetup(m_graphContext.lock());
            return graph;
        }
        else
        {
            return nullptr;
        }
    }

    AZ::Outcome<ConstGraphPtr, AZStd::string> ModuleGraphManager::LoadGraph(AZ::Uuid sourceFileId)
    {
        bool gotSourceInfo = false;
        AZ::Data::AssetInfo assetInfo;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(gotSourceInfo, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourceUUID, sourceFileId, assetInfo, watchFolder);
        if (!gotSourceInfo)
        {
            return AZ::Failure<AZStd::string>("Could not get source file info for [" + sourceFileId.ToString<AZStd::string>() + "]");
        }

        AZStd::string extension;
        if (!AzFramework::StringFunc::Path::GetExtension(assetInfo.m_relativePath.data(), extension) || extension != m_moduleFileExtension)
        {
            return AZ::Failure<AZStd::string>("Incorrect extension [" + assetInfo.m_relativePath + "]. Must be [" + m_moduleFileExtension + "]");
        }

        AZStd::string fullAssetPath;
        AzFramework::StringFunc::Path::Join(watchFolder.data(), assetInfo.m_relativePath.data(), fullAssetPath);
        AZ::IO::FileIOStream stream;
        stream.Open(fullAssetPath.data(), AZ::IO::OpenMode::ModeRead);

        if (!stream.IsOpen())
        {
            return AZ::Failure<AZStd::string>("Could not open [" + fullAssetPath + "]");
        }

        ConstGraphPtr graph = LoadGraph(stream);

        if (!graph)
        {
            return AZ::Failure<AZStd::string>("Could not load [" + fullAssetPath + "]");
        }

        return AZ::Success(graph);
    }

    AZ::Outcome<ConstGraphPtr, AZStd::string> ModuleGraphManager::GetModuleGraph(AZ::Uuid sourceFileId)
    {
        auto iter = m_graphs.find(sourceFileId);

        // If the soure file has never been loaded, go ahead and load it now
        if (iter == m_graphs.end())
        {
            AZ::Outcome<ConstGraphPtr, AZStd::string> graphOutcome = LoadGraph(sourceFileId);
            if (!graphOutcome.IsSuccess())
            {
                return graphOutcome;
            }

            m_graphs[sourceFileId] = graphOutcome.GetValue();

            return graphOutcome;
        }
        else
        {
            // If the Graph has been loaded and is still in memory, we can just return it
            ConstGraphPtr graph = iter->second.lock();
            if (graph)
            {
                return AZ::Success(graph);
            }
            // The Graph has been released at some point and needs to be loaded again
            else
            {
                AZ::Outcome<ConstGraphPtr, AZStd::string> graphOutcome = LoadGraph(sourceFileId);
                if (!graphOutcome.IsSuccess())
                {
                    m_graphs.erase(iter);
                    return graphOutcome;
                }

                m_graphs[sourceFileId] = graphOutcome.GetValue();

                return graphOutcome;
            }
        }
    }

} // namespace GraphModel
