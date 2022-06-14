/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>

#include <Atom/RHI/ImageView.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ::Render
{
    class BindlessImageArrayHandler
    {
        public:
            
            static constexpr uint16_t InvalidImageIndex = 0xFFFF;

            BindlessImageArrayHandler() = default;
            ~BindlessImageArrayHandler() = default;

            void Initialize(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& srg, const AZ::Name& propertyName);
            void Reset();
            bool IsInitialized() const;
            bool UpdateSrgIndices(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& srg, const AZ::Name& propertyName);

            uint16_t AppendBindlessImage(const RHI::ImageView* imageView);
            void UpdateBindlessImage(uint16_t index, const RHI::ImageView* imageView);
            void RemoveBindlessImage(uint16_t index);

            bool UpdateSrg(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& srg) const;
            
        private:

            AZStd::vector<const RHI::ImageView*> m_bindlessImageViews;
            AZStd::vector<uint16_t> m_bindlessImageViewFreeList;
            RHI::ShaderInputImageUnboundedArrayIndex m_texturesIndex;
            AZStd::shared_mutex m_updateMutex;
            bool m_isInitialized{ false };
    };
}

