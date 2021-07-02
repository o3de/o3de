/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Resource.h>
#include <Atom/RHI/ShaderResourceGroupData.h>

namespace AZ
{
    namespace RHI
    {
         //! This class is a platform-independent base class for a shader resource group. It has a
         //! pointer to the resource group pool, if the user initialized the group onto a pool.
        class ShaderResourceGroup
            : public Resource
        {
            friend class ShaderResourceGroupPool;
        public:
            AZ_RTTI(ShaderResourceGroup, "{91B217A5-EFEC-46C5-82DA-B4C77931BC1A}", Resource);
            virtual ~ShaderResourceGroup() override = default;

            //! Defines the compilation modes for an SRG
            enum class CompileMode : uint8_t
            {
                Async,  // Queues SRG compilation for later. This is the most common case.
                Sync    // Compiles the SRG immediately. To be used carefully due to performance cost.
            };

            //! Compiles the SRG with the provided data.
            //! When using Async compile mode, it queues a request that the parent pool compile this group (compilation is deferred).
            //! When using Sync compile mode the SRG compilation will happen immediately.
            void Compile(const ShaderResourceGroupData& shaderResourceGroupData, CompileMode compileMode = CompileMode::Async);

            //! Returns the shader resource group pool that this group is registered on.
            ShaderResourceGroupPool* GetPool();
            const ShaderResourceGroupPool* GetPool() const;

            //! This implementation does not report any memory usage. Platforms may
            //! override to report more accurate usage metrics.
            void ReportMemoryUsage(MemoryStatisticsBuilder& builder) const override;

            //! Returns the data currently bound on the shader resource group.
            const ShaderResourceGroupData& GetData() const;

            //! Returns the binding slot specified by the layout associated to this shader resource group.
            uint32_t GetBindingSlot() const;

            //! Returns whether the group is currently queued for compilation.
            bool IsQueuedForCompile() const;

        protected:
            ShaderResourceGroup() = default;

        private:
            void SetData(const ShaderResourceGroupData& data);

            ShaderResourceGroupData m_data;

            // The binding slot cached from the layout.
            uint32_t m_bindingSlot = (uint32_t)-1;

            // Gates the Compile() function so that the SRG is only queued once.
            bool m_isQueuedForCompile = false;
        };
    }
}
