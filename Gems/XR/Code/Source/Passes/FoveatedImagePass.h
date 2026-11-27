/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RHI/XRRenderingInterface.h>

namespace XR
{
    static constexpr const char* const FoveatedImagePassClassName = "FoveatedImagePass";
    static constexpr const char* const FoveatedImagePassTemplateName = "FoveatedImagePassTemplate";
    static constexpr const char* const FoveatedImageSlotName = "FoveatedImageOutput";

    //! Custom data for the FoveatedImagePass Pass.
    struct FoveatedImagePassData
        : public AZ::RPI::PassData
    {
        AZ_RTTI(FoveatedImagePassData, "{DA97DB6D-6B41-4285-9266-8A7903887910}", AZ::RPI::PassData);
        AZ_CLASS_ALLOCATOR(FoveatedImagePassData, AZ::SystemAllocator);

        FoveatedImagePassData() = default;
        virtual ~FoveatedImagePassData() = default;

        //! Foveated level for the shading rate image
        AZ::RHI::XRFoveatedLevel m_foveatedLevel = AZ::RHI::XRFoveatedLevel::None;
    };

    //! This pass handles the initialization of the content of the shading rate image used
    //! for foveted rendering. This pass doesn't render or compute anything. It just creates the
    //! shading rate image (based on the pipeline output size and device capabilities) and fills it
    //! with the proper values depending on the foveated level. It also handles resizes of the
    //! pipeline output.
    class FoveatedImagePass : public AZ::RPI::Pass
    {
        using Base = AZ::RPI::Pass;
        AZ_RPI_PASS(FoveatedImagePass);
        
    public:
        AZ_RTTI(FoveatedImagePass, "{C4C33582-21E9-42CE-801B-BE3A1C468A72}", Base);
        AZ_CLASS_ALLOCATOR(FoveatedImagePass, AZ::SystemAllocator);
        virtual ~FoveatedImagePass() = default;
        
        /// Creates a FoveatedImagePass
        static AZ::RPI::Ptr<FoveatedImagePass> Create(const AZ::RPI::PassDescriptor& descriptor);
        
    private:
        FoveatedImagePass(const AZ::RPI::PassDescriptor& descriptor);
        
        // Pass behavior overrides...
        void ResetInternal() override;
        void BuildInternal() override;

        AZ::RPI::Ptr<AZ::RPI::PassAttachment> m_foveatedAttachment;
        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_foveatedImage;
        AZ::RHI::XRFoveatedLevel m_foveatedLevel = AZ::RHI::XRFoveatedLevel::None;
    };
} // namespace XR
