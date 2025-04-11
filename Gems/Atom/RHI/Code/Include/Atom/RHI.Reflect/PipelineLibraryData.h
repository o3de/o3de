/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>

namespace AZ::Serialize
{
    template<class T, bool U, bool A>
    struct InstanceFactory;
}
namespace AZ
{
    template<typename ValueType, typename>
    struct AnyTypeInfoConcept;

    class ReflectContext;
}

namespace AZ::RHI
{
    //! This class represents a blob of platform-specific PipelineLibrary data that can be serialized
    //! to and from disk, speeding up driver compilation time and memory consumption.
    //!
    //! Pipeline state data is expensive to compile and results in a lot of duplicated memory
    //! when pipeline states have little variance (for example, the same byte code but different
    //! render state). The pipeline library allows the platform to de-duplicate these identical
    //! components. Since this data is platform and driver specific, it gets serialized as an opaque
    //! blob.
    //!
    //! Another restriction enforced by certain platforms is that they won't actually copy the data,
    //! since it can be quite large. For example, if a pipeline library exists per thread, it is
    //! preferred to not copy the data N times.
    //!
    //! Therefore, this class is designed to be immutable after creation and support reference counting.
    //! This allows the platform to safely hold a reference and guarantees that the memory is not mutated
    //! externally.
    class PipelineLibraryData final
        : public AZStd::intrusive_base
    {
        friend class PipelineLibrary;
    public:
        AZ_CLASS_ALLOCATOR(PipelineLibraryData, SystemAllocator);
        AZ_TYPE_INFO(PipelineLibraryData, "{6A349BB4-4787-46B5-A70A-7BA90515391F}");

        static void Reflect(ReflectContext* reflectContext);

        static ConstPtr<PipelineLibraryData> Create(AZStd::vector<uint8_t>&& data);

        /// Returns the data payload which describes the platform-specific pipeline library data.
        AZStd::span<const uint8_t> GetData() const;

    private:
        PipelineLibraryData(AZStd::vector<uint8_t>&& data);
        PipelineLibraryData() = default;

        template <typename, typename>
        friend struct AnyTypeInfoConcept;
        template <typename, bool, bool>
        friend struct Serialize::InstanceFactory;

        AZStd::vector<uint8_t> m_data;
    };
}
