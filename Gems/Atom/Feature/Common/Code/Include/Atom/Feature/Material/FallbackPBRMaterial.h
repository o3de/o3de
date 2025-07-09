/*/
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/TransformService/TransformServiceFeatureProcessorInterface.h>
#include <Atom/RHI/ImageView.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <AzCore/Math/Color.h>

namespace AZ::Render
{
    // Fallback PBR: A generic PBR material for each visible mesh, with the parameters
    // guesstimated from the actual mesh material.
    namespace FallbackPBR
    {
        struct MaterialParameters
        {
            // color of the bounced light from this sub-mesh
            AZ::Color m_irradianceColor = AZ::Color(1.0f);

            // material data
            AZ::Color m_baseColor = AZ::Color(0.0f);
            float m_metallicFactor = 0.0f;
            float m_roughnessFactor = 0.0f;
            AZ::Color m_emissiveColor = AZ::Color(0.0f);

            // material textures
            RHI::Ptr<const RHI::ImageView> m_baseColorImageView;
            RHI::Ptr<const RHI::ImageView> m_normalImageView;
            RHI::Ptr<const RHI::ImageView> m_metallicImageView;
            RHI::Ptr<const RHI::ImageView> m_roughnessImageView;
            RHI::Ptr<const RHI::ImageView> m_emissiveImageView;
        };

        struct MaterialEntry : public AZStd::intrusive_base
        {
            TransformServiceFeatureProcessorInterface::ObjectId m_objectId;
            Data::Instance<RPI::Material> m_material;
            RPI::Material::ChangeId m_materialChangeId;
            MaterialParameters m_materialParameters;
        };
    } // namespace FallbackPBR

} // namespace AZ::Render
