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

#include <Atom/RHI/ShaderResourceGroupPool.h>
#include <AtomCore/Instance/InstanceData.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>


namespace AZ
{
    namespace RHI
    {
        class Device;
    }

    namespace RPI
    {
        /**
         * Wraps a RHI ShaderResourceGroupPool for use in the RPI to initialize ShaderResourceGroups.
         *
         * This class is a bit unusual in that InstanceData objects usually associate with an AssetData
         * class of corresponding type (for example, Material and MaterialAsset). In this case, 
         * the ShaderResourceGroupPool instance corresponds to a specific ShaderResourceGroupAsset,
         * not a ShaderResourceGroup[Pool]Asset. This is because there is a 1:1 relationship with
         * ShaderResourceGroupAsset's ShaderResourceGroupLayout and no additional configuration data 
         * is needed for the pool.
         *
         * User code should not need to access this pool directly; RPI::ShaderResourceGroup uses it internally.
         */
        class ShaderResourceGroupPool final
            : public Data::InstanceData
        {
            friend class ShaderSystem;
        public:
            AZ_INSTANCE_DATA(ShaderResourceGroupPool, "{D75561AF-C10A-41BC-BABF-63DBFA160388}");
            AZ_CLASS_ALLOCATOR(ShaderResourceGroupPool, AZ::SystemAllocator, 0);

            /**
             * Instantiates or returns an existing runtime pool for a given ShaderResourceGroup.
             * @param shaderResourceGroupPoolAsset The asset used to instantiate an instance of the streaming image pool.
             */
            static Data::Instance<ShaderResourceGroupPool> FindOrCreate(
                const Data::Asset<ShaderAsset>& shaderAsset, const SupervariantIndex& supervariantIndex, const AZ::Name& srgName);
            
            RHI::Ptr<RHI::ShaderResourceGroup> CreateRHIShaderResourceGroup();

            RHI::ShaderResourceGroupPool* GetRHIPool();
            const RHI::ShaderResourceGroupPool* GetRHIPool() const;

        private:
            ShaderResourceGroupPool() = default;

            // Standard RPI runtime instance initialization
            static Data::Instance<ShaderResourceGroupPool> CreateInternal(ShaderAsset& shaderAsset);
            bool IsInitialized() { return m_isInitialized; }
            RHI::ResultCode Init(ShaderAsset& shaderAsset, const SupervariantIndex& supervariantIndex, const AZ::Name& srgName);

            /// If true, Init() was called and was successful.
            bool m_isInitialized = false;
            RHI::Ptr<RHI::ShaderResourceGroupPool> m_pool;
        };
    }
}
