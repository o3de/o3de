/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LogicLibrary.h"
#include "And.h"
#include "Not.h"
#include "Or.h"
#include "WeightedRandomSequencer.h"
#include "TargetedSequencer.h"

#include <ScriptCanvas/Libraries/ScriptCanvasNodeRegistry.h>

namespace ScriptCanvas::LogicLibrary
{
    void Reflect(AZ::ReflectContext* reflection)
    {
        Nodes::Logic::WeightedRandomSequencer::ReflectDataTypes(reflection);
    }

    void InitNodeRegistry(NodeRegistry* nodeRegistry)
    {
        using namespace ScriptCanvas::Nodes::Logic;
        nodeRegistry->m_nodes.push_back(AZ::AzTypeInfo<And>::Uuid());
        nodeRegistry->m_nodes.push_back(AZ::AzTypeInfo<Not>::Uuid());
        nodeRegistry->m_nodes.push_back(AZ::AzTypeInfo<Or>::Uuid());
    }

    AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors()
    {
        return AZStd::vector<AZ::ComponentDescriptor*>({
            ScriptCanvas::Nodes::Logic::And::CreateDescriptor(),
            ScriptCanvas::Nodes::Logic::Not::CreateDescriptor(),
            ScriptCanvas::Nodes::Logic::Or::CreateDescriptor(),
            });
    }
}
