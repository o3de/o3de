/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Graph Model
#include <GraphModel/Model/Common.h>
#include <GraphModel/Model/Node.h>

namespace GraphModel
{
    //! Encapsulates an entire node graph as a node to be used in another graph.
    //! The graph that defines this node is called a Module Graph, which has
    //! GraphInputNodes and/or GraphOutputNodes to define inputs and outputs
    //! for the graph. These input/output nodes become input/output Slots in
    //! the ModuleNode.
    class ModuleNode : public Node
    {
    public:
        AZ_CLASS_ALLOCATOR(ModuleNode, AZ::SystemAllocator);
        AZ_RTTI(ModuleNode, "{C7D57EFE-462D-48A0-B46F-6E927D504BA5}", Node);
        static void Reflect(AZ::ReflectContext* context);

        ModuleNode() = default; // Needed by SerializeContext

        //! Constructor
        //! \param ownerGraph      The graph that owns this node
        //! \param sourceFileId    The unique id for the module node graph source file, which is the module graph that defines this ModuleNode.
        //! \param sourceFilePath  The path to the module node graph source file. This will be used for node naming and debug output.
        ModuleNode(GraphPtr ownerGraph, AZ::Uuid moduleGraphFileId, AZStd::string_view moduleGraphFileName);
        
        const char* GetTitle() const override;

        using Node::PostLoadSetup;
        void PostLoadSetup(GraphPtr ownerGraph, NodeId id) override;

    protected:

        //! Gets the module graph that defines this ModuleNode
        void LoadModuleGraph(ModuleGraphManagerPtr moduleGraphManager);

        //! Registers input and output SlotDescriptions based on the contents of the module graph
        void RegisterSlots() override;

        ConstGraphPtr m_moduleGraph;  //!< The module graph that defines the inputs, outputs, and behavior of this node
        AZStd::string m_nodeTitle;    //!< Node title indicates the name of the module file
        AZ::Uuid m_moduleGraphFileId; //!< Unique identifier of the source file that contains the module graph
    };

} 
