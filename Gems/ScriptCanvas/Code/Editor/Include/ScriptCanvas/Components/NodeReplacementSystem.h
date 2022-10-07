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

#include <ScriptCanvas/Core/Node.h>
#include <Editor/Include/ScriptCanvas/Bus/NodeReplacementBus.h>

namespace ScriptCanvasEditor
{
    static constexpr const char NodeReplacementRootPath[] = "/Amazon/ScriptCanvas/NodeReplacement";
    static constexpr const char NodeReplacementOldNodeFieldName[] = "OldNode";
    static constexpr const char NodeReplacementNewNodeFieldName[] = "NewNode";
    static constexpr const char NodeReplacementUuidFieldName[] = "Uuid";
    static constexpr const char NodeReplacementClassFieldName[] = "Class";
    static constexpr const char NodeReplacementMethodFieldName[] = "Method";

    class NodeReplacementSystem final
        : public NodeReplacementRequestBus::Handler
    {
    public:
        NodeReplacementSystem() = default;
        virtual ~NodeReplacementSystem() = default;

        static NodeReplacementId GenerateReplacementId(const AZ::Uuid& id, const AZStd::string& className, const AZStd::string& methodName);
        static NodeReplacementId GenerateReplacementId(ScriptCanvas::Node* node);

        void LoadReplacementMetadata();
        void UnloadReplacementMetadata();

        // NodeReplacementRequests
        ScriptCanvas::NodeReplacementConfiguration GetNodeReplacementConfiguration(const NodeReplacementId& replacementId) const override;

    private:
        AZStd::unordered_map<NodeReplacementId, ScriptCanvas::NodeReplacementConfiguration> m_replacementMetadata;
    };
} // ScriptCanvasEditor
