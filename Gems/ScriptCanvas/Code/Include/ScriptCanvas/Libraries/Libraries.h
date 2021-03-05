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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    class ReflectContext;
    class ComponentDescriptor;
}

namespace ScriptCanvas
{
    struct NodeRegistry final
    {
        AZ_TYPE_INFO(NodeRegistry, "{C1613BD5-3104-44E4-98FE-A917A90B2014}");

        using NodeList = AZStd::vector<AZStd::pair<AZ::Uuid, AZStd::string>>;

        AZStd::unordered_map<AZ::Uuid, NodeList> m_nodeMap;
    };

    // Call once to initialize Node Registry
    void InitNodeRegistry();
    // Call to reset Node Registry
    void ResetNodeRegistry();
    AZ::EnvironmentVariable<NodeRegistry> GetNodeRegistry();
    static const char* s_nodeRegistryName = "ScriptCanvasNodeRegistry";

    namespace Library
    {
        struct LibraryDefinition
        {
            AZ_RTTI(LibraryDefinition, "{C7A74062-1577-4925-897F-BB7600D2016D}");

            virtual ~LibraryDefinition() = default;

            static const NodeRegistry::NodeList GetNodes(const AZ::Uuid& libraryType)
            { 
                NodeRegistry& registry = (*GetNodeRegistry());
                const auto& libraryIterator = registry.m_nodeMap.find(libraryType);
                if (libraryIterator != registry.m_nodeMap.end())
                {
                    const NodeRegistry::NodeList& nodeTypes = libraryIterator->second;
                    return nodeTypes;
                }

                return NodeRegistry::NodeList();
            }

            static bool HasNode(const AZ::Uuid& libraryId, const AZ::Uuid& nodeId)
            {
                NodeRegistry::NodeList nodes = GetNodes(libraryId);
                for (const auto& node : nodes)
                {
                    if (node.first == nodeId)
                    {
                        return true;
                    }
                }
                return false;
            }
        };

        template<typename NodeGroup, typename NodeType>
        void AddNodeToRegistry(NodeRegistry& nodeRegistry)
        {
            auto& nodes = nodeRegistry.m_nodeMap[AZ::AzTypeInfo<NodeGroup>::Uuid()];
            nodes.push_back({ AZ::AzTypeInfo<NodeType>::Uuid(), AZ::AzTypeInfo<NodeType>::Name() });
        }

        struct Core : public LibraryDefinition
        {
            AZ_RTTI(Core, "{D8F5B691-CC40-43C8-9B11-2228DEFE0FDC}", LibraryDefinition);

            static void Reflect(AZ::ReflectContext*);
            static void InitNodeRegistry(NodeRegistry& nodeRegistry);
            static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();

            ~Core() override = default;

        };

        struct Math : public LibraryDefinition
        {
            AZ_RTTI(Math, "{76898795-2B30-4645-B6D4-67568ECC889F}", LibraryDefinition);

            static void Reflect(AZ::ReflectContext*);
            static void InitNodeRegistry(NodeRegistry& nodeRegistry);
            static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();

            ~Math() override = default;

        };

        struct Logic : public LibraryDefinition
        {
            AZ_RTTI(Logic, "{5E0A7A06-F05A-472B-9135-6DCC4F260E02}", LibraryDefinition);

            static void Reflect(AZ::ReflectContext*);
            static void InitNodeRegistry(NodeRegistry& nodeRegistry);
            static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();

            ~Logic() override = default;

        };

        struct Entity : public LibraryDefinition
        {
            AZ_RTTI(Entity, "{C9789111-6A18-4C6E-81A7-87D057D59D2C}", LibraryDefinition);

            static void Reflect(AZ::ReflectContext*);
            static void InitNodeRegistry(NodeRegistry& nodeRegistry);
            static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();

            ~Entity() override = default;

        };

        struct Time : public LibraryDefinition
        {
            AZ_RTTI(Time, "{13B4FC85-CAC5-4DBF-B973-73EBE5CB18DC}", LibraryDefinition);

            static void Reflect(AZ::ReflectContext*);
            static void InitNodeRegistry(NodeRegistry& nodeRegistry);
            static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();

            ~Time() override = default;

        };

        struct String : public LibraryDefinition
        {
            AZ_RTTI(String, "{5B700838-21A2-4579-9303-F4A4822AFEF4}", LibraryDefinition);

            static void Reflect(AZ::ReflectContext*);
            static void InitNodeRegistry(NodeRegistry& nodeRegistry);
            static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();

            ~String() override = default;
        };

        struct UnitTesting : public LibraryDefinition
        {
            AZ_RTTI(UnitTesting, "{50EB4F9F-CB8E-4946-A389-23216083FC4B}", LibraryDefinition);

            static void Reflect(AZ::ReflectContext*);
            static void InitNodeRegistry(NodeRegistry& nodeRegistry);
            static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();

            ~UnitTesting() override = default;

        };

        struct Operators : public LibraryDefinition
        {
            AZ_RTTI(Operators, "{ED0D5642-4E89-4265-B687-251236961AF4}", LibraryDefinition);

            static void Reflect(AZ::ReflectContext*);
            static void InitNodeRegistry(NodeRegistry& nodeRegistry);
            static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();

            ~Operators() override = default;

        };

    }

    void ReflectLibraries(AZ::ReflectContext*);

    AZStd::vector<AZ::ComponentDescriptor*> GetLibraryDescriptors();
}