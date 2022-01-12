/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>
#include <SceneAPI/SceneCore/Utilities/HashHelper.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <cinttypes>
#include <AzCore/std/any.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace Events
        {
            struct ExportProduct;
            class ExportProductList;
        }
    }
}

namespace AZ::SceneAPI::Utilities
{
    constexpr int SceneGraphVersion = 1;

    struct DebugNode
    {
        AZ_TYPE_INFO(DebugNode, "{490B9D4C-1847-46EB-BEBC-49812E104626}");

        AZStd::string m_name;
        AZStd::string m_path;
        AZStd::string m_type;

        DebugNode() = default;

        DebugNode(AZStd::string name, AZStd::string path, AZStd::string type)
            : m_name(AZStd::move(name)),
              m_path(AZStd::move(path)),
              m_type(AZStd::move(type))
        {
        }

        static void Reflect(AZ::ReflectContext* context);

        using DataItem = AZStd::pair<AZStd::string, AZStd::any>;
        AZStd::vector<DataItem> m_data;
    };

    struct DebugSceneGraph
    {
        AZ_TYPE_INFO(DebugSceneGraph, "{375F6558-5709-409F-881E-8ED575D56C92}");

        int m_version = SceneGraphVersion;
        AZStd::string m_productName;
        AZStd::string m_sceneName;
        AZStd::vector<DebugNode> m_nodes;

        static void Reflect(AZ::ReflectContext* context);
    };

    class DebugOutput
    {
    public:
        DebugOutput(DebugNode node) : m_currentNode(AZStd::move(node)){}

        template<typename T>
        void Write(const char* name, const AZStd::vector<T>& data);

        template<typename T>
        void Write(const char* name, const AZStd::vector<AZStd::vector<T>>& data);

        template<typename T>
        void AddToNode(const char* name, const T& data)
        {
            if (!m_pauseNodeData)
            {
                m_currentNode.m_data.emplace_back(name, AZStd::make_any<AZStd::decay_t<T>>(data));
            }
        }
        
        SCENE_CORE_API void Write(const char* name, const char* data);
        SCENE_CORE_API void WriteArray(const char* name, const unsigned int* data, int size);
        SCENE_CORE_API void Write(const char* name, const AZStd::string& data);
        SCENE_CORE_API void Write(const char* name, double data);
        SCENE_CORE_API void Write(const char* name, uint64_t data);
        SCENE_CORE_API void Write(const char* name, int64_t data);
        SCENE_CORE_API void Write(const char* name, const DataTypes::MatrixType& data);
        SCENE_CORE_API void Write(const char* name, bool data);
        SCENE_CORE_API void Write(const char* name, Vector3 data);
        SCENE_CORE_API void Write(const char* name, AZStd::optional<bool> data);
        SCENE_CORE_API void Write(const char* name, AZStd::optional<float> data);
        SCENE_CORE_API void Write(const char* name, AZStd::optional<AZ::Vector3> data);

        SCENE_CORE_API const AZStd::string& GetOutput() const;
        SCENE_CORE_API DebugNode GetDebugNode() const;

        SCENE_CORE_API static void BuildDebugSceneGraph(const char* outputFolder, AZ::SceneAPI::Events::ExportProductList& productList, const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& scene, AZStd::string productName);

    protected:
        AZStd::string m_output;
        DebugSceneGraph m_graph;
        DebugNode m_currentNode;
        bool m_pauseNodeData = false; // If true, don't append any data to the DebugNode.  Useful when a Write function calls other Write functions
    };
}

#include "DebugOutput.inl"
