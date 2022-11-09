/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Uuid.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

#include <Editor/Include/ScriptCanvas/Bus/NodeReplacementBus.h>

namespace ScriptCanvasEditor
{
    //! NodeReplacementSystem
    //! This class is centralized system to handle node replacement in Editor
    class NodeReplacementSystem final
        : public NodeReplacementRequestBus::Handler
    {
    public:
        NodeReplacementSystem();
        virtual ~NodeReplacementSystem();

        //! Generate node replacement id based on the given node metadata,
        //! including type, class name (optional) and method name (optional)
        static NodeReplacementId GenerateReplacementId(
            const AZ::Uuid& id, const AZStd::string& className = "", const AZStd::string& methodName = "");

        //! Generate node replacement id based on the given node object
        static NodeReplacementId GenerateReplacementId(ScriptCanvas::Node* node);

        //! Load replacement metadata
        void LoadReplacementMetadata();

        //! Unload replacement metadata
        void UnloadReplacementMetadata();

        // NodeReplacementRequests implements
        ScriptCanvas::NodeReplacementConfiguration GetNodeReplacementConfiguration(
            const NodeReplacementId& replacementId) const override;
        ScriptCanvas::NodeUpdateReport ReplaceNodeByReplacementConfiguration(
            const AZ::EntityId& graphId, ScriptCanvas::Node* oldNode, const ScriptCanvas::NodeReplacementConfiguration& config) override;

    private:
        ScriptCanvas::Node* GetOrCreateNodeFromReplacementConfiguration(
            const ScriptCanvas::NodeReplacementConfiguration& config);
        bool SanityCheckNodeReplacement(
            ScriptCanvas::Node* oldNode, ScriptCanvas::Node* newNode, ScriptCanvas::NodeUpdateReport& report);
        bool SanityCheckNodeReplacementWithCustomLogic(
            ScriptCanvas::Node* oldNode, ScriptCanvas::Node* newNode, ScriptCanvas::NodeUpdateReport& report);
        bool SanityCheckNodeReplacementWithSameTopology(
            ScriptCanvas::Node* oldNode, ScriptCanvas::Node* newNode, ScriptCanvas::NodeUpdateReport& report);

        //! Collection to store one to one mapping (from old node replacement id to new node replacement config)
        AZStd::unordered_map<NodeReplacementId, ScriptCanvas::NodeReplacementConfiguration> m_replacementMetadata;
    };
} // ScriptCanvasEditor
