/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// AZ
#include <AzCore/std/string/string_view.h>

// Graph Model
#include <GraphModel/Model/Node.h>

namespace AZ
{
    class ReflectContext;
}

namespace GraphModel
{
    //! Common base class for GraphInput/OutputNodes
    class BaseInputOutputNode : public Node
    {
    public:
        AZ_RTTI(BaseInputOutputNode, "{C54F11AE-3151-44D7-B206-9206FA888963}", Node);

        static void Reflect(AZ::ReflectContext* context);

        BaseInputOutputNode() = default; // Needed by SerializeContext

        //! Constructor
        //! \param graph         The graph that owns this node
        //! \param dataType      The type of data represented by this node
        BaseInputOutputNode(GraphPtr graph, DataTypePtr dataType);

        const char* GetTitle() const override;
        GraphModel::DataTypePtr GetNodeDataType() const;
        AZStd::string GetName() const;
        AZStd::string GetDisplayName() const;
        AZStd::string GetDescription() const;

    protected:
        
        //! Registers metadata slots that are common for inputs and outputs, like name, displayName, and description.
        void RegisterCommonSlots(AZStd::string_view directionName);

        AZStd::string m_title;
        AZStd::shared_ptr<DataType> m_dataType;
    };


    //! Provides a node that serves as a data input into a node graph.
    class GraphInputNode : public BaseInputOutputNode
    {
    public:
        AZ_RTTI(GraphInputNode, "{4CDE10B9-14C1-4B5A-896C-C3E15EDAC665}", BaseInputOutputNode);

        static void Reflect(AZ::ReflectContext* context);

        GraphInputNode() = default; // Needed by SerializeContext

        //! Constructor
        //! \param graph         The graph that owns this node
        //! \param dataType      The type of data represented by this node
        GraphInputNode(GraphModel::GraphPtr graph, DataTypePtr dataType);

        using BaseInputOutputNode::PostLoadSetup;
        void PostLoadSetup(GraphPtr graph, NodeId id) override;

        //! Returns the value of the DefaultValue slot, which indicates the default value for this input. This
        //! is the value that will be used when this node's graph is used as a ModuleNode, but no data is 
        //! connected to this graph input.
        AZStd::any GetDefaultValue() const;

    protected:
        //! Registers SlotDescriptors for each of this node's slots
        void RegisterSlots() override;
    };


    //! Provides an node that serves as a data output from a node graph.
    class GraphOutputNode : public BaseInputOutputNode
    {
    public:
        AZ_RTTI(GraphOutputNode, "{5E5188E1-7F79-41D4-965F-248EECE7A735}", BaseInputOutputNode);

        static void Reflect(AZ::ReflectContext* context);

        GraphOutputNode() = default; // Needed by SerializeContext

        //! Constructor
        //! \param graph         The graph that owns this node
        //! \param dataType      The type of data represented by this node
        GraphOutputNode(GraphModel::GraphPtr graph, DataTypePtr dataType);

        using BaseInputOutputNode::PostLoadSetup;
        void PostLoadSetup(GraphPtr graph, NodeId id) override;

    protected:
        //! Registers SlotDescriptors for each of this node's slots
        void RegisterSlots() override;
    };


}
