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

#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
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
        class BufferSystem final
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
