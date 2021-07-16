/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Quaternion.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/MeshInfo.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NamedPropertyStringValue.h>

namespace EMotionFX
{
    class ActorInstance;
    class Node;
}

namespace EMStudio
{
    class NodeInfo final
    {
    public:
        AZ_RTTI(NodeInfo, "{AF8699EB-D11B-487B-84D4-089CA682DD27}")
        AZ_CLASS_ALLOCATOR_DECL

        NodeInfo() {}
        NodeInfo(EMotionFX::ActorInstance* actorInstance, EMotionFX::Node* node);
        ~NodeInfo() = default;

        static void Reflect(AZ::ReflectContext* context);

    private:
        bool HasMirror() const { return !m_mirrorNodeName.empty(); }
        bool HasChildNodes() const { return !m_childNodeNames.empty(); }
        bool HasAttributes() const { return !m_attributeTypes.empty(); }
        bool HasMeshes() const { return !m_meshByLod.empty(); }

        AZStd::string                m_name;
        AZ::Vector3                  m_position;
        AZ::Quaternion               m_rotation;
        AZ::Vector3                  m_scale;
        AZStd::string                m_parentName;
        AZStd::string                m_mirrorNodeName;
        AZStd::vector<AZStd::string> m_childNodeNames;
        AZStd::vector<AZStd::string> m_attributeTypes;
        AZStd::vector<MeshInfo>      m_meshByLod;
    };
} // namespace EMStudio
