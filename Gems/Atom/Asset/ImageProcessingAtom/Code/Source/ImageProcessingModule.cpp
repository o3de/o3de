/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Module.h>
#include "ImageProcessingSystemComponent.h"
#include "ImageBuilderComponent.h"
#include "Thumbnail/ImageThumbnailSystemComponent.h"

namespace ImageProcessingAtom
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
                    Thumbnails::ImageThumbnailSystemComponent::CreateDescriptor(),
                    ImageProcessingSystemComponent::CreateDescriptor(), // system component for editor
                    BuilderPluginComponent::CreateDescriptor(), // builder component for AP
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<ImageProcessingSystemComponent>(),
                azrtti_typeid<Thumbnails::ImageThumbnailSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), ImageProcessingAtom::ImageProcessingModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_ImageProcessingAtom, ImageProcessingAtom::ImageProcessingModule)
#endif
