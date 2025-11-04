/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OperatorsLibrary.h"

#include <Libraries/Operators/Math/OperatorLerpNodeableNode.h>
#include <Libraries/ScriptCanvasNodeRegistry.h>


namespace ScriptCanvas::OperatorsLibrary
{
    void Reflect(AZ::ReflectContext* reflection)
    {
        ScriptCanvas::Nodes::LerpBetweenNodeable<float>::Reflect(reflection);
        ScriptCanvas::Nodes::LerpBetweenNodeable<Data::Vector2Type>::Reflect(reflection);
        ScriptCanvas::Nodes::LerpBetweenNodeable<Data::Vector3Type>::Reflect(reflection);
        ScriptCanvas::Nodes::LerpBetweenNodeable<Data::Vector4Type>::Reflect(reflection);
        ScriptCanvas::Nodes::NodeableNodeOverloaded::Reflect(reflection);
    }

    void InitNodeRegistry(NodeRegistry* nodeRegistry)
    {
        nodeRegistry->m_nodes.push_back(AZ::AzTypeInfo<ScriptCanvas::Nodes::NodeableNodeOverloadedLerp>::Uuid());
    }

    AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors()
    {
        AZStd::vector<AZ::ComponentDescriptor*> descriptors =
        {
            ScriptCanvas::Nodes::NodeableNodeOverloadedLerp::CreateDescriptor(),
        };

        return descriptors;
    }
}
