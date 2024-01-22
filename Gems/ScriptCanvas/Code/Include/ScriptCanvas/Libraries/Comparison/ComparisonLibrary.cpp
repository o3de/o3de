/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ComparisonLibrary.h"
#include "EqualTo.h"
#include "NotEqualTo.h"
#include "Less.h"
#include "Greater.h"
#include "Less.h"
#include "LessEqual.h"
#include "GreaterEqual.h"
#include <ScriptCanvas/Libraries/ScriptCanvasNodeRegistry.h>
#include <AutoGen/ScriptCanvasAutoGenRegistry.h>

namespace ScriptCanvas::ComparisonLibrary
{
    void InitNodeRegistry(NodeRegistry* nodeRegistry)
    {
        using namespace ScriptCanvas::Nodes::Comparison;
        nodeRegistry->m_nodes.push_back(AZ::AzTypeInfo<EqualTo>::Uuid());
        nodeRegistry->m_nodes.push_back(AZ::AzTypeInfo<NotEqualTo>::Uuid());
        nodeRegistry->m_nodes.push_back(AZ::AzTypeInfo<Less>::Uuid());
        nodeRegistry->m_nodes.push_back(AZ::AzTypeInfo<Greater>::Uuid());
        nodeRegistry->m_nodes.push_back(AZ::AzTypeInfo<LessEqual>::Uuid());
        nodeRegistry->m_nodes.push_back(AZ::AzTypeInfo<GreaterEqual>::Uuid());
    }

    AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors()
    {
        return AZStd::vector<AZ::ComponentDescriptor*>({
            ScriptCanvas::Nodes::Comparison::EqualTo::CreateDescriptor(),
            ScriptCanvas::Nodes::Comparison::NotEqualTo::CreateDescriptor(),
            ScriptCanvas::Nodes::Comparison::Less::CreateDescriptor(),
            ScriptCanvas::Nodes::Comparison::Greater::CreateDescriptor(),
            ScriptCanvas::Nodes::Comparison::LessEqual::CreateDescriptor(),
            ScriptCanvas::Nodes::Comparison::GreaterEqual::CreateDescriptor(),
        });
    }
} 
