/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <Math/MathFilter.h>
#include <Math/MathFilterDescriptor.h>

namespace AZ
{
    namespace Render
    {
        //! Gaussian filter used in shader code.
        class GaussianMathFilter final
            : public MathFilter
        {
        public:
            GaussianMathFilter() = delete;
            explicit GaussianMathFilter(const GaussianFilterDescriptor& descriptor);
            virtual ~GaussianMathFilter() = default;

            // MathFiler overrides...
            uint32_t GetElementCount() const override;
            uint32_t GetElementSize() const override;
            RHI::Format GetElementFormat() const override;
            void StoreDataTo(void* dataPointer) override;

            static const float ReliableSectionFactor;
            static const float StandardDeviationMax;

            static AZStd::string GenerateName(const GaussianFilterDescriptor& descriptor);

        private:
            AZStd::pair<float, uint32_t> GetSigmaAndTableSize() const;

            GaussianFilterDescriptor m_descriptor;
        };
    } // namespace Render
} // namespace AZ
