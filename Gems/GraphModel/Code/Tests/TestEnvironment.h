/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// AZ ...
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzTest/AzTest.h>

// GraphModel ...
#include <GraphModel/Model/DataType.h>
#include <GraphModel/Model/IGraphContext.h>
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/Node.h>
#include <GraphModel/Model/Slot.h>

// Mock GraphCanvas buses ...
#include <Tests/MockGraphCanvas.h>

namespace GraphModelIntegrationTest
{
    static const GraphCanvas::EditorId NODE_GRAPH_TEST_EDITOR_ID = AZ_CRC("GraphModelIntegrationTestEditor", 0x56953df8);
    static const char* TEST_STRING_INPUT_ID = "inputString";
    static const char* TEST_STRING_OUTPUT_ID = "outputString";
    static const char* TEST_EVENT_INPUT_ID = "inputEvent";
    static const char* TEST_EVENT_OUTPUT_ID = "outputEvent";

    enum TestDataTypeEnum : GraphModel::DataType::Enum
    {
        TestDataTypeEnum_String,
        TestDataTypeEnum_EntityId,
        TestDataTypeEnum_Count
    };

    class TestGraphContext
        : public GraphModel::IGraphContext
        , public AZStd::enable_shared_from_this<TestGraphContext>
    {
    public:
        TestGraphContext();
        virtual ~TestGraphContext() = default;

        const char* GetSystemName() const override;
        const char* GetModuleFileExtension() const override;
        const GraphModel::DataTypeList& GetAllDataTypes() const override;
        GraphModel::DataTypePtr GetDataType(AZ::Uuid typeId) const override;
        GraphModel::DataTypePtr GetDataType(GraphModel::DataType::Enum typeEnum) const override;

        template<typename T>
        GraphModel::DataTypePtr GetDataType() const
        {
            return IGraphContext::GetDataType<T>();
        }

        GraphModel::ModuleGraphManagerPtr GetModuleGraphManager() const override;

    private:
        GraphModel::DataTypeList m_dataTypes;
    };

    class TestNode
        : public GraphModel::Node
    {
    public:
        AZ_RTTI(TestNode, "{C51A8CE2-229A-4807-9173-96CF730C6C2B}", Node);

        using TestNodePtr = AZStd::shared_ptr<TestNode>;

        static void Reflect(AZ::ReflectContext* context);

        TestNode() = default;
        explicit TestNode(GraphModel::GraphPtr graph, AZStd::shared_ptr<TestGraphContext> graphContext);

        const char* GetTitle() const override;

    protected:
        void RegisterSlots() override;

        AZStd::shared_ptr<TestGraphContext> m_graphContext = nullptr;
    };

    class ExtendableSlotsNode
        : public GraphModel::Node
    {
    public:
        AZ_RTTI(ExtendableSlotsNode, "{5670CFB9-EE42-456D-B1AE-CACC55EC0967}", Node);

        using ExtendableSlotsNodePtr = AZStd::shared_ptr<ExtendableSlotsNode>;

        static void Reflect(AZ::ReflectContext* context);

        ExtendableSlotsNode() = default;
        explicit ExtendableSlotsNode(GraphModel::GraphPtr graph, AZStd::shared_ptr<TestGraphContext> graphContext);

        const char* GetTitle() const override;

    protected:
        void RegisterSlots() override;

        AZStd::shared_ptr<TestGraphContext> m_graphContext = nullptr;
    };

    class BadNode
        : public GraphModel::Node
    {
    public:
        AZ_RTTI(BadNode, "{8ECC580C-1AFC-4D66-863D-72AC9971CEC8}", Node);

        using BadNodePtr = AZStd::shared_ptr<BadNode>;

        static void Reflect(AZ::ReflectContext* context);

        BadNode() = default;
        explicit BadNode(GraphModel::GraphPtr graph, AZStd::shared_ptr<TestGraphContext> graphContext);

        const char* GetTitle() const override;

    protected:
        void RegisterSlots() override;

        AZStd::shared_ptr<TestGraphContext> m_graphContext = nullptr;
    };

    class GraphModelTestEnvironment
        : public AZ::Test::ITestEnvironment
    {
    protected:
        void SetupEnvironment() override;
        void TeardownEnvironment() override;

        AZ::ComponentApplication* m_application;
        AZ::Entity* m_systemEntity;
    };
}
