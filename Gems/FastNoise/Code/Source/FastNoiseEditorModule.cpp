/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "FastNoise_precompiled.h"
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
