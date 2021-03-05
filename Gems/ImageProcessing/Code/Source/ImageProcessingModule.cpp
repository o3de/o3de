/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "ImageProcessing_precompiled.h"
#include <AzCore/Module/Module.h>
#include "ImageProcessingSystemComponent.h"
#include "ImageBuilderComponent.h"
#include "AtlasBuilder/AtlasBuilderComponent.h"

namespace ImageProcessing
{
    class ImageProcessingModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(ImageProcessingModule, "{A5392495-DD0E-4719-948A-B98DBAE88197}", AZ::Module);

        ImageProcessingModule()
            : AZ::Module()
        {
            // Push results of the components' ::CreateDescriptor() into m_descriptors here. 
            m_descriptors.insert(m_descriptors.end(), {
                ImageProcessingSystemComponent::CreateDescriptor(), //system component for editor
                BuilderPluginComponent::CreateDescriptor(), //builder component for AP
                TextureAtlasBuilder::AtlasBuilderComponent::CreateDescriptor(), //builder component for texture atlas
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<ImageProcessingSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_ImageProcessing, ImageProcessing::ImageProcessingModule)
