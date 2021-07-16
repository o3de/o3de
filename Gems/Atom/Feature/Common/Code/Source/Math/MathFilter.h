/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <AzCore/std/string/string.h>
#include <Math/MathFilterDescriptor.h>

namespace AZ
{
    namespace Render
    {
        //! Base class for several mathematical filters.
        class MathFilter
        {
        public:
            using BufferWithElementCounts = AZStd::pair<Data::Instance<RPI::Buffer>, AZStd::vector<uint32_t>>;

            MathFilter() = default;
            virtual ~MathFilter() = default;

            static BufferWithElementCounts FindOrCreateFilterBuffer(const MathFilterDescriptor& descriptor);

            virtual uint32_t GetElementCount() const = 0;
            virtual uint32_t GetElementSize() const = 0;
            virtual RHI::Format GetElementFormat() const = 0;
            virtual void StoreDataTo(void* dataPointer) = 0;

        private:
            static AZStd::string GenerateName(const MathFilterDescriptor& descriptor);
            static AZStd::vector<uint32_t> GetElementCounts(const AZStd::vector<AZStd::unique_ptr<MathFilter>>& filters);
        };
    } // namespace Render
} // namespace AZ
