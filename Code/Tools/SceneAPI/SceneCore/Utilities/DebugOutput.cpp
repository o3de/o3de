/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DebugOutput.h"
#include <AzCore/std/optional.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ::SceneAPI::Utilities
{
    void DebugNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);

        if (serialize)
        {
            serialize->Class<DebugNode>()
                ->Field("Name", &DebugNode::m_name)
                ->Field("Path", &DebugNode::m_path)
                ->Field("Type", &DebugNode::m_type)
                ->Field("Data", &DebugNode::m_data);
        }
    }

    void DebugSceneGraph::Reflect(AZ::ReflectContext* context)
    {
        DebugNode::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);

        if (serialize)
        {
            serialize->Class<DebugSceneGraph>()
            ->Field("Version", &DebugSceneGraph::m_version)
            ->Field("ProductName", &DebugSceneGraph::m_productName)
            ->Field("SceneName", &DebugSceneGraph::m_sceneName)
            ->Field("Nodes", &DebugSceneGraph::m_nodes);
        }
    }

    void DebugOutput::Write(const char* name, const char* data)
    {
        m_output += AZStd::string::format("\t%s: %s\n", name, data);
    }

    void DebugOutput::WriteArray(const char* name, const unsigned int* data, int size)
    {
        m_output += AZStd::string::format("\t%s: ", name);
        for (int index = 0; index < size; ++index)
        {
            m_output += AZStd::string::format("%d, ", data[index]);
        }
        m_output += AZStd::string::format("\n");
    }

    void DebugOutput::Write(const char* name, const AZStd::string& data)
    {
        Write(name, data.c_str());

        AddToNode(name, data);
    }

    void DebugOutput::Write(const char* name, double data)
    {
        m_output += AZStd::string::format("\t%s: %f\n", name, data);

        AddToNode(name, data);
    }

    void DebugOutput::Write(const char* name, uint64_t data)
    {
        m_output += AZStd::string::format("\t%s: %" PRIu64 "\n", name, data);

        AddToNode(name, data);
    }

    void DebugOutput::Write(const char* name, int64_t data)
    {
        m_output += AZStd::string::format("\t%s: %" PRId64 "\n", name, data);

        AddToNode(name, data);
    }

    void DebugOutput::Write(const char* name, const DataTypes::MatrixType& data)
    {
        AZ::Vector3 basisX{};
        AZ::Vector3 basisY{};
        AZ::Vector3 basisZ{};
        AZ::Vector3 translation{};
        data.GetBasisAndTranslation(&basisX, &basisY, &basisZ, &translation);

        m_pauseNodeData = true;
        m_output += AZStd::string::format("\t%s:\n", name);

        m_output += "\t";
        Write("BasisX", basisX);

        m_output += "\t";
        Write("BasisY", basisY);

        m_output += "\t";
        Write("BasisZ", basisZ);

        m_output += "\t";
        Write("Transl", translation);
        m_pauseNodeData = false;

        AddToNode(name, data);
    }

    void DebugOutput::Write(const char* name, bool data)
    {
        m_output += AZStd::string::format("\t%s: %s\n", name, data ? "true" : "false");

        AddToNode(name, data);
    }

    void DebugOutput::Write(const char* name, Vector3 data)
    {
        m_output += AZStd::string::format("\t%s: <% f, % f, % f>\n", name, data.GetX(), data.GetY(), data.GetZ());

        AddToNode(name, data);
    }

    void DebugOutput::Write(const char* name, AZStd::optional<bool> data)
    {
        if (data.has_value())
        {
            Write(name, data.value());
        }
        else
        {
            Write(name, "Not set");
        }
    }
    void DebugOutput::Write(const char* name, AZStd::optional<float> data)
    {
        if (data.has_value())
        {
            Write(name, data.value());
        }
        else
        {
            Write(name, "Not set");
        }
    }

    void DebugOutput::Write(const char* name, AZStd::optional<AZ::Vector3> data)
    {
        if (data.has_value())
        {
            Write(name, data.value());
        }
        else
        {
            Write(name, "Not set");
        }
    }

    const AZStd::string& DebugOutput::GetOutput() const
    {
        return m_output;
    }

    DebugNode DebugOutput::GetDebugNode() const
    {
        return m_currentNode;
    }

    void WriteAndLog(AZ::IO::SystemFile& dbgFile, const char* strToWrite)
    {
        AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "%s", strToWrite);
        dbgFile.Write(strToWrite, strlen(strToWrite));
        dbgFile.Write("\n", strlen("\n"));
    }

    void DebugOutput::BuildDebugSceneGraph(const char* outputFolder, AZ::SceneAPI::Events::ExportProductList& productList, const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& scene, AZStd::string productName)
    {
        AZStd::string debugSceneFile;

        AzFramework::StringFunc::Path::ConstructFull(outputFolder, productName.c_str(), debugSceneFile);
        AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "outputFolder %s, name %s.\n", outputFolder, productName.c_str());
        
        AZ::IO::SystemFile dbgFile;
        if (dbgFile.Open(debugSceneFile.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            WriteAndLog(dbgFile, AZStd::string::format("ProductName: %s", productName.c_str()).c_str());
            WriteAndLog(dbgFile, AZStd::string::format("debugSceneGraphVersion: %d", SceneGraphVersion).c_str());
            WriteAndLog(dbgFile, scene->GetName().c_str());

            const AZ::SceneAPI::Containers::SceneGraph& sceneGraph = scene->GetGraph();
            auto names = sceneGraph.GetNameStorage();
            auto content = sceneGraph.GetContentStorage();
            auto pairView = AZ::SceneAPI::Containers::Views::MakePairView(names, content);
            auto view = AZ::SceneAPI::Containers::Views::MakeSceneGraphDownwardsView<
                AZ::SceneAPI::Containers::Views::BreadthFirst>(
                    sceneGraph, sceneGraph.GetRoot(), pairView.cbegin(), true);

            DebugSceneGraph debugSceneGraph;
            debugSceneGraph.m_version = SceneGraphVersion;
            debugSceneGraph.m_productName = productName;
            debugSceneGraph.m_sceneName = scene->GetName().c_str();

            for (auto&& viewIt : view)
            {
                if (viewIt.second == nullptr)
                {
                    continue;
                }

                AZ::SceneAPI::DataTypes::IGraphObject* graphObject = const_cast<AZ::SceneAPI::DataTypes::IGraphObject*>(viewIt.second.get());
                
                WriteAndLog(dbgFile, AZStd::string::format("Node Name: %s", viewIt.first.GetName()).c_str());
                WriteAndLog(dbgFile, AZStd::string::format("Node Path: %s", viewIt.first.GetPath()).c_str());
                WriteAndLog(dbgFile, AZStd::string::format("Node Type: %s", graphObject->RTTI_GetTypeName()).c_str());
                
                AZ::SceneAPI::Utilities::DebugOutput debugOutput(
                    DebugNode(viewIt.first.GetName(), viewIt.first.GetPath(), graphObject->RTTI_GetTypeName()));
                
                viewIt.second->GetDebugOutput(debugOutput);

                if (!debugOutput.GetOutput().empty())
                {
                    WriteAndLog(dbgFile, debugOutput.GetOutput().c_str());
                }

                debugSceneGraph.m_nodes.push_back(debugOutput.GetDebugNode());
            }
            dbgFile.Close();

            Utils::SaveObjectToFile((debugSceneFile + ".xml").c_str(), DataStream::StreamType::ST_XML, &debugSceneGraph);

            static const AZ::Data::AssetType dbgSceneGraphAssetType("{07F289D1-4DC7-4C40-94B4-0A53BBCB9F0B}");
            productList.AddProduct(productName, AZ::Uuid::CreateName(productName.c_str()), dbgSceneGraphAssetType,
                AZStd::nullopt, AZStd::nullopt);

            static const AZ::Data::AssetType dbgSceneGraphXmlAssetType("{51F37614-0D77-4F36-9AC6-7ED70A0AC868}");
            productList.AddProduct(
                (productName + ".xml"), AZ::Uuid::CreateName((productName + ".xml").c_str()), dbgSceneGraphXmlAssetType,
                AZStd::nullopt, AZStd::nullopt);
        }
    }
}
