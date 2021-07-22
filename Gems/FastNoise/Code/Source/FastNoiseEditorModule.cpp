/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <FastNoiseEditorModule.h>
#include <FastNoiseSystemComponent.h>
#include <EditorFastNoiseGradientComponent.h>

namespace FastNoiseGem
{
    FastNoiseEditorModule::FastNoiseEditorModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            EditorFastNoiseGradientComponent::CreateDescriptor()
        });
    }

    AZ::ComponentTypeList FastNoiseEditorModule::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList requiredComponents = FastNoiseModule::GetRequiredSystemComponents();

        return requiredComponents;
    }
}

AZ_DECLARE_MODULE_CLASS(Gem_FastNoiseEditor, FastNoiseGem::FastNoiseEditorModule)
