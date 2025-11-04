/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>

namespace AZ
{
    class Job;
    class JobContext;
    class ReflectContext;

    namespace RPI
    {
        class Buffer;

        //! Manages system-wide initialization and support for Buffer classes
        class ATOM_RPI_PUBLIC_API BufferSystem final
            : public BufferSystemInterface
        {
        public:
            static void Reflect(AZ::ReflectContext* context);
            static void GetAssetHandlers(AssetHandlerPtrList& assetHandlers);

            // BufferSystemInterface overrides...
            RHI::Ptr<RHI::BufferPool> GetCommonBufferPool(CommonBufferPoolType poolType) override;
            Data::Instance<Buffer> CreateBufferFromCommonPool(const CommonBufferDescriptor& descriptor) override;
            Data::Instance<Buffer> FindCommonBuffer(AZStd::string_view uniqueBufferName) override;

            void Init();
            void Shutdown();

        protected:
            bool CreateCommonBufferPool(CommonBufferPoolType poolType);

        private:
            RHI::Ptr<RHI::BufferPool> m_commonPools[static_cast<uint8_t>(CommonBufferPoolType::Count)];

            bool m_initialized = false;
        };
    } // namespace RPI
} // namespace AZ
