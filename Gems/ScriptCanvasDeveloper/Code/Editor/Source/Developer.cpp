/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvasDeveloperEditor/Developer.h>
#include <ScriptCanvasDeveloperEditor/Mock.h>
#include <ScriptCanvasDeveloperEditor/WrapperMock.h>
#include <ScriptCanvas/Libraries/Libraries.h>

namespace ScriptCanvas::Developer
{
    void InitNodeRegistry()
    {
        NodeRegistry* registry = NodeRegistry::GetInstance();
        registry->m_nodes.push_back(AZ::AzTypeInfo<ScriptCanvas::Developer::Nodes::Mock>::Uuid());
        registry->m_nodes.push_back(AZ::AzTypeInfo<ScriptCanvas::Developer::Nodes::WrapperMock>::Uuid());
    }

    AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors()
    {
        return AZStd::vector<AZ::ComponentDescriptor*>({
            ScriptCanvas::Developer::Nodes::Mock::CreateDescriptor(),
            ScriptCanvas::Developer::Nodes::WrapperMock::CreateDescriptor()
        });
    }
}
