/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DiffuseProbeGridModule.h>
#include <Components/DiffuseProbeGridSystemComponent.h>
#include <Components/DiffuseProbeGridComponent.h>
#include <Components/DiffuseGlobalIlluminationComponent.h>

#ifdef DIFFUSEPROBEGRID_EDITOR
#include <EditorComponents/EditorDiffuseProbeGridComponent.h>
#include <EditorComponents/EditorDiffuseGlobalIlluminationComponent.h>
#endif

namespace AZ
{
    namespace Render
    {
        DiffuseProbeGridModule::DiffuseProbeGridModule()
        {
            m_descriptors.insert(m_descriptors.end(),
                {
                    DiffuseProbeGridSystemComponent::CreateDescriptor(),
                    DiffuseProbeGridComponent::CreateDescriptor(),
                    DiffuseGlobalIlluminationComponent::CreateDescriptor(),

                    #ifdef DIFFUSEPROBEGRID_EDITOR
                        EditorDiffuseProbeGridComponent::CreateDescriptor(),
                        EditorDiffuseGlobalIlluminationComponent::CreateDescriptor(),
                    #endif
                });
        }

        AZ::ComponentTypeList DiffuseProbeGridModule::GetRequiredSystemComponents() const
        {
            return AZ::ComponentTypeList{ azrtti_typeid<DiffuseProbeGridSystemComponent>() };
        }
    }
}
