/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Math/Vector4.h>

// NvCloth library includes
#include <NvCloth/Range.h>
#include <foundation/PxVec4.h>

namespace nv
{
    namespace cloth
    {
        class Factory;
        class Solver;
        class Fabric;
        class Cloth;
    }
}

namespace NvCloth
{
    //! Defines deleters for NvCloth types to destroy them appropriately,
    //! allowing to handle them with unique pointers.
    struct NvClothTypesDeleter
    {
        void operator()(nv::cloth::Factory* factory) const;
        void operator()(nv::cloth::Solver* solver) const;
        void operator()(nv::cloth::Fabric* fabric) const;
        void operator()(nv::cloth::Cloth* cloth) const;
    };

    using NvFactoryUniquePtr = AZStd::unique_ptr<nv::cloth::Factory, NvClothTypesDeleter>;
    using NvSolverUniquePtr = AZStd::unique_ptr<nv::cloth::Solver, NvClothTypesDeleter>;
    using NvFabricUniquePtr = AZStd::unique_ptr<nv::cloth::Fabric, NvClothTypesDeleter>;
    using NvClothUniquePtr = AZStd::unique_ptr<nv::cloth::Cloth, NvClothTypesDeleter>;

    //! Returns an AZ vector as a NvCloth Range, which points to vector's memory.
    template <typename T>
    inline nv::cloth::Range<const T> ToNvRange(const AZStd::vector<T>& azVector)
    {
        return nv::cloth::Range<const T>(
            azVector.data(),
            azVector.data() + azVector.size());
    }

    //! Returns an AZ vector of AZ::Vector4 elements as a NvCloth Range of physx::PxVec4 elements.
    //! The memory on the NvCloth Range points to the AZ vector's memory.
    //!
    //! It's safe to reinterpret AZ::Vector4 as physx::PxVec4 because they have the same memory layout
    //! and AZ::Vector4 has more memory alignment restrictions than physx::PxVec4.
    //! The opposite operation would NOT be safe.
    inline nv::cloth::Range<const physx::PxVec4> ToPxVec4NvRange(const AZStd::vector<AZ::Vector4>& azVector)
    {
        static_assert(sizeof(physx::PxVec4) == sizeof(AZ::Vector4), "Incompatible types");
        return nv::cloth::Range<const physx::PxVec4>(
            reinterpret_cast<const physx::PxVec4*>(azVector.data()),
            reinterpret_cast<const physx::PxVec4*>(azVector.data() + azVector.size()));
    }
} // namespace NvCloth
