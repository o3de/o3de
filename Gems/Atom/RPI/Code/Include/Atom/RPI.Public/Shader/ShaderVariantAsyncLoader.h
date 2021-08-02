/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantTreeAsset.h>
#include <Atom/RPI.Reflect/Shader/IShaderVariantFinder.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        /**
         * A helper class used by ShaderSystem to manage asynchronous loading of ShaderVariantTreeAssets
         * and ShaderVariantAssets.
         * The notifications of assets being loaded & ready are dispatched via ShaderVariantFinderNotificationBus.
         */
        class ShaderVariantAsyncLoader final
            : public AZ::Interface<IShaderVariantFinder>::Registrar
            , public AZ::Data::AssetBus::MultiHandler
        {
        public:
            static constexpr char LogName[] = "ShaderVariantAsyncLoader";
            ~ShaderVariantAsyncLoader() { Shutdown(); }

            void Init();
            void Shutdown();

            struct TupleShaderAssetAndShaderVariantId
            {
                Data::Asset<ShaderAsset> m_shaderAsset;
                ShaderVariantId m_shaderVariantId;
                SupervariantIndex m_supervariantIndex;

                bool operator==(const TupleShaderAssetAndShaderVariantId& anotherTuple) const
                {
                    return (
                        (m_shaderAsset.GetId() == anotherTuple.m_shaderAsset.GetId()) &&
                        (m_shaderVariantId == anotherTuple.m_shaderVariantId) &&
                        (m_supervariantIndex == anotherTuple.m_supervariantIndex)
                    );
                }
            };

            ///////////////////////////////////////////////////////////////////
            // IShaderVariantFinder overrides
            bool QueueLoadShaderVariantAssetByVariantId(Data::Asset<ShaderAsset> shaderAsset, const ShaderVariantId& shaderVariantId, SupervariantIndex supervariantIndex) override;
            bool QueueLoadShaderVariantTreeAsset(const Data::AssetId& shaderAssetId) override;
            bool QueueLoadShaderVariantAsset(const Data::AssetId& shaderVariantTreeAssetId, ShaderVariantStableId variantStableId, SupervariantIndex supervariantIndex) override;

            Data::Asset<ShaderVariantAsset> GetShaderVariantAssetByVariantId(
                Data::Asset<ShaderAsset> shaderAsset, const ShaderVariantId& shaderVariantId, SupervariantIndex supervariantIndex) override;
            Data::Asset<ShaderVariantAsset> GetShaderVariantAssetByStableId(
                Data::Asset<ShaderAsset> shaderAsset, ShaderVariantStableId shaderVariantStableId,
                SupervariantIndex supervariantIndex) override;
            Data::Asset<ShaderVariantTreeAsset> GetShaderVariantTreeAsset(const Data::AssetId& shaderAssetId) override;
            Data::Asset<ShaderVariantAsset> GetShaderVariantAsset(
                const Data::AssetId& shaderVariantTreeAssetId, ShaderVariantStableId variantStableId,
                SupervariantIndex supervariantIndex) override;

            void Reset() override;
            ///////////////////////////////////////////////////////////////////

        private:

            ///////////////////////////////////////////////////////////////////////
            // AZ::Data::AssetBus::Handler overrides
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;
            void OnAssetError(Data::Asset<Data::AssetData> asset) override;
            ///////////////////////////////////////////////////////////////////////

            void OnShaderVariantTreeAssetReady(Data::Asset<ShaderVariantTreeAsset> shaderVariantTreeAsset);
            void OnShaderVariantAssetReady(Data::Asset<ShaderVariantAsset> shaderVariantAsset);
            void OnShaderVariantTreeAssetError(Data::Asset<ShaderVariantTreeAsset> shaderVariantTreeAsset);
            void OnShaderVariantAssetError(Data::Asset<ShaderVariantAsset> shaderVariantAsset);

            void ThreadServiceLoop();

            void QueueShaderVariantTreeForLoading(
                const TupleShaderAssetAndShaderVariantId& shaderAndVariantTuple,
                AZStd::unordered_set<Data::AssetId>& shaderVariantTreePendingRequests);

            //! This is a helper method called from the service thread.
            //! Returns true if a valid AssetId for the corresponding ShaderVariantTreeAsset is registered
            //! in the asset database AND a request to load such asset is properly queued.
            bool TryToLoadShaderVariantTreeAsset(const Data::AssetId& shaderAssetId);

            bool TryToLoadShaderVariantAsset(const Data::AssetId& shaderVariantAssetId);


            //! A thread that runs forever servicing shader variant and trees load requests.
            AZStd::thread m_serviceThread;
            AZStd::atomic_bool m_isServiceShutdown;
            AZStd::mutex m_mutex;
            AZStd::condition_variable m_workCondition;

            //! This is a list of AssetId of ShaderVariantAsset.
            AZStd::vector<TupleShaderAssetAndShaderVariantId> m_newShaderVariantPendingRequests;

            //! This is a list of AssetId of ShaderAsset (Do not confuse with the AssetId ShaderVariantTreeAsset).
            AZStd::vector<Data::AssetId> m_shaderVariantTreePendingRequests;

            //! This is a list of AssetId of ShaderVariantAsset.
            AZStd::vector<Data::AssetId> m_shaderVariantPendingRequests;

            struct ShaderVariantCollection
            {
                Data::AssetId m_shaderAssetId;
                Data::Asset<ShaderVariantTreeAsset> m_shaderVariantTree;
                //! The key is the AssetId of the ShaderVariantAsset
                AZStd::unordered_map<Data::AssetId, Data::Asset<ShaderVariantAsset>> m_shaderVariantsMap;
            };

            //! The key is the shader variant tree asset id.
            AZStd::unordered_map<Data::AssetId, ShaderVariantCollection> m_shaderVariantData;

            //! Key: AssetId of a ShaderAsset; Value: AssetId of a ShaderVariantTreeAsset.
            //! REMARK: To go the other way, you can use m_shaderVariantData.
            AZStd::unordered_map<Data::AssetId, Data::AssetId> m_shaderAssetIdToShaderVariantTreeAssetId;

        };



    } // namespace RPI
} // namespace AZ

namespace AZStd
{
    template<>
    struct hash<AZ::RPI::ShaderVariantId>
    {
        size_t operator()(const AZ::RPI::ShaderVariantId& variantId) const
        {
            return AZStd::hash_range(variantId.m_key.data(), variantId.m_key.data() + variantId.m_key.num_words());
        }
    };

    template<>
    struct hash<AZ::RPI::ShaderVariantAsyncLoader::TupleShaderAssetAndShaderVariantId>
    {
        size_t operator()(const AZ::RPI::ShaderVariantAsyncLoader::TupleShaderAssetAndShaderVariantId& tuple) const
        {
            static constexpr AZStd::hash<AZ::RPI::ShaderVariantId> hash_fn;
            size_t retVal = tuple.m_shaderAsset.GetId().m_guid.GetHash();
            AZStd::hash_combine(retVal, hash_fn(tuple.m_shaderVariantId));
            AZStd::hash_combine(retVal, tuple.m_supervariantIndex.GetIndex());
            return retVal;
        }
    };
} // namespace AZStd
