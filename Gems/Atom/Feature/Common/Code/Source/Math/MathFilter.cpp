/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <AtomCore/Instance/Instance.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Math/GaussianMathFilter.h>
#include <Math/MathFilter.h>

namespace AZ
{
    namespace Render
    {
        MathFilter::BufferWithElementCounts MathFilter::FindOrCreateFilterBuffer(const MathFilterDescriptor& descriptor)
        {
            AZStd::vector<AZStd::unique_ptr<MathFilter>> filters;
            switch (descriptor.m_kind)
            {
            case MathFilterKind::Gaussian:
            {
                // Create Gaussian filters for given set of standard deviations.
                filters.reserve(descriptor.m_gaussians.size());
                for (const GaussianFilterDescriptor& gaussian : descriptor.m_gaussians)
                {
                    filters.push_back(AZStd::make_unique<GaussianMathFilter>(gaussian));
                }
                break;
            }
            case MathFilterKind::None:
            default:
                break;
            }
            AZ_Assert(!filters.empty(), "Cannot create the math filter.");

            // If there already exists the filter parameter buffer,
            // return it along with parameter counts of each filters.
            AZStd::string uniqueName = GenerateName(descriptor);
            if (auto buffer = RPI::BufferSystemInterface::Get()->FindCommonBuffer(uniqueName))
            {
                return AZStd::make_pair(buffer, GetElementCounts(filters));
            }

            // Calculate  total element count of the buffer.
            uint32_t totalElementCount = 0;
            for (const AZStd::unique_ptr<MathFilter>& filter : filters)
            {
                totalElementCount += filter->GetElementCount();
            }

            // Fill the contents of the buffer by each filter.
            const uint32_t elementSize = filters.front()->GetElementSize();
            AZStd::vector<uint8_t> data(totalElementCount * elementSize);
            uint32_t offset = 0;
            for (const AZStd::unique_ptr<MathFilter>& filter : filters)
            {
                filter->StoreDataTo(data.data() + offset * elementSize);
                offset += filter->GetElementCount();
            }

            // Create the filter parameter buffer.
            RPI::CommonBufferDescriptor desc;
            desc.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
            desc.m_bufferName = uniqueName;
            desc.m_elementSize = elementSize;
            desc.m_elementFormat = filters.front()->GetElementFormat();
            desc.m_byteCount = totalElementCount * elementSize;
            desc.m_bufferData = data.data();
            desc.m_isUniqueName = true;

            auto buffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);

            // Return the buffer along with parameter counts of each filters.
            return make_pair(buffer, GetElementCounts(filters));
        }

        AZStd::string MathFilter::GenerateName(const MathFilterDescriptor& descriptor)
        {
            switch (descriptor.m_kind)
            {
            case MathFilterKind::None:
                AZ_Assert(false, "Math filter kind is not offered.");
                break;
            case MathFilterKind::Gaussian:
            {
                AZStd::string nameString("GaussianFilterBuffer");
                for (const GaussianFilterDescriptor& gauss : descriptor.m_gaussians)
                {
                    nameString += GaussianMathFilter::GenerateName(gauss);
                }
                return nameString;
            }
            default:
                AZ_Assert(false, "Illegal math filter kind");
                break;
            }
            return AZStd::string();
        }

        AZStd::vector<uint32_t> MathFilter::GetElementCounts(const AZStd::vector<AZStd::unique_ptr<MathFilter>>& filters)
        {
            AZStd::vector<uint32_t> offsetSizes;
            offsetSizes.reserve(filters.size());
            for (const AZStd::unique_ptr<MathFilter>& filter : filters)
            {
                offsetSizes.push_back(filter->GetElementCount());
            }
            return offsetSizes;
        }

    } // namespace Render
} // namespace AZ
