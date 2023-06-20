/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/Memory_fwd.h>
#include <AzCore/Module/Module.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <AzCore/RTTI/TypeInfoSimple.h>

namespace Compression
{
    class DecompressionRegistrarInterface;

    class CompressionModuleInterface
        : public AZ::Module
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(CompressionModuleInterface);
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_CLASS_ALLOCATOR_DECL;

        CompressionModuleInterface();
        ~CompressionModuleInterface() override;

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;

    private:
        // DecompressionRegistrar interface used to register Decompression interfaces
        // Available in ALL applications to allow decompression to occur
        AZStd::unique_ptr<DecompressionRegistrarInterface> m_decompressionRegistrarInterface;
    };
}// namespace Compression
