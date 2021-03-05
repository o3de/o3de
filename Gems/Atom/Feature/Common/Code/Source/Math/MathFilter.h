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
