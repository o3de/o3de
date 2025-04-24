/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Asset/AssetSystemBus.h>

namespace AZ
{
    namespace RPI
    {
        namespace AssetUtils
        {
            enum class TraceLevel
            {
                None,
                Warning,
                Error,
                Assert,
            };

            // Declarations...

            //! Finds the AssetId for a given product file path.
            ATOM_RPI_REFLECT_API Data::AssetId GetAssetIdForProductPath(const char* productPath, TraceLevel reporting = TraceLevel::Warning, Data::AssetType assetType = Data::s_invalidAssetType);

            //! Tries to compile the asset at the given product path.
            //! This will actively try to compile the asset every time it is called, it won't skip compilation just because the
            //! asset exists. This should only be used for assets that need to be at their most up-to-date version of themselves
            //! before getting loaded into the engine, as it can take seconds to minutes for this call to return. It is synchronously
            //! asking the Asset Processor to compile the asset, and then blocks until it gets a result. If the AP is busy, it can
            //! take a while to get a result even if the asset is already up-to-date.
            //! In release builds where the AP isn't connected this will immediately return with "Unknown".
            //! @param assetProductFilePath - the relative file path to the product asset (ex: default/models/sphere.azmodel)
            //! @param reporting - the reporting level to use for problems.
            //! @return true if the compilation is successful or unknown, false if an error was detected
            //! "Unknown" is considered a successful result because if there's no Asset Processor, there's no way to truly
            //! know the compile state of the asset. If the AP is connected, there *should* always be a result
            //! (Compiled, Failed, Missing, etc), but if this is called *before* the AP is connected, it's possible to get Unknown
            //! even when you think the AP is (or will be) connected.
            ATOM_RPI_REFLECT_API bool TryToCompileAsset(const AZStd::string& assetProductFilePath, TraceLevel reporting);

            //! Gets an Asset<AssetDataT> reference for a given product file path. This function does not cause the asset to load.
            //! @return a null asset if the asset could not be found.
            template<typename AssetDataT>
            Data::Asset<AssetDataT> GetAssetByProductPath(const char* productPath, TraceLevel reporting = TraceLevel::Warning);

            //! Loads an asset using a product file path, on the current thread.
            //! @return a null asset if the asset could not be found or loaded.
            template<typename AssetDataT>
            Data::Asset<AssetDataT> LoadAssetByProductPath(const char* productPath, TraceLevel reporting = TraceLevel::Warning);

            //! Loads an asset using an AssetId, on the current thread.
            //! @return a null asset if the asset could not be found or loaded.
            template<typename AssetDataT>
            Data::Asset<AssetDataT> LoadAssetById(Data::AssetId assetId, TraceLevel reporting = TraceLevel::Warning);

            //! Loads a critial asset using a file path (both source and product path should be same), on the current thread.
            //! If the asset wasn't compiled, wait until the asset is compiled.
            //! @return a null asset if the asset could not be compiled or loaded.
            template<typename AssetDataT>
            Data::Asset<AssetDataT> LoadCriticalAsset(const AZStd::string& assetFilePath, TraceLevel reporting = TraceLevel::Error);

            template<typename AssetDataT>
            bool LoadBlocking(AZ::Data::Asset<AssetDataT>& asset, TraceLevel reporting = TraceLevel::Warning);

            namespace AssetUtilsInternal
            {
                //! May call AZ_Warning, AZ_Error, or AZ_Assert depending on TraceLevel
                ATOM_RPI_REFLECT_API void ReportIssue(TraceLevel traceLevel, const char* message);
            }

            // Definitions...

            template<typename AssetDataT>
            Data::Asset<AssetDataT> GetAssetByProductPath(const char* productPath, TraceLevel reporting)
            {
                Data::AssetId assetId = GetAssetIdForProductPath(productPath, reporting);

                if(!assetId.IsValid())
                {
                    return {};
                }

                Data::Asset<AssetDataT> asset(AZ::Data::AssetLoadBehavior::PreLoad);
                if (!asset.Create(assetId))
                {
                    return {};
                }

                return asset;
            }

            template<typename AssetDataT>
            Data::Asset<AssetDataT> LoadAssetByProductPath(const char* productPath, TraceLevel reporting)
            {
                Data::AssetId assetId = GetAssetIdForProductPath(productPath, reporting);
                if (assetId.IsValid())
                {
                    Data::Asset<AssetDataT> asset = AZ::Data::AssetManager::Instance().GetAsset<AssetDataT>(
                        assetId, AZ::Data::AssetLoadBehavior::PreLoad);
                    asset.BlockUntilLoadComplete();

                    if (!asset.IsReady())
                    {
                        AssetUtilsInternal::ReportIssue(reporting, AZStd::string::format("Could not load '%s'", productPath).c_str());
                        return {};
                    }

                    return asset;
                }
                else
                {
                    return {};
                }
            }

            template<typename AssetDataT>
            Data::Asset<AssetDataT> LoadAssetById(Data::AssetId assetId, TraceLevel reporting)
            {
                if (!assetId.IsValid())
                {
                    AssetUtilsInternal::ReportIssue(
                        reporting, AZStd::string::format("Could not load '%s'", assetId.ToString<AZStd::string>().c_str()).c_str());
                    return {};
                }

                Data::Asset<AssetDataT> asset =
                    AZ::Data::AssetManager::Instance().GetAsset<AssetDataT>(assetId, AZ::Data::AssetLoadBehavior::PreLoad);
                asset.BlockUntilLoadComplete();

                if (!asset.IsReady())
                {
                    AssetUtilsInternal::ReportIssue(
                        reporting, AZStd::string::format("Could not load '%s'", assetId.ToString<AZStd::string>().c_str()).c_str());
                    return {};
                }

                return asset;
            }

            template<typename AssetDataT>
            Data::Asset<AssetDataT> LoadCriticalAsset(const AZStd::string& assetFilePath, TraceLevel reporting)
            {
                TryToCompileAsset(assetFilePath, reporting);

                // Whether or not we were able to successfully compile the asset, we'll still try to load it.
                // A failed compile could mean that the asset relies on intermediate assets that haven't been created yet.
                return LoadAssetByProductPath<AssetDataT>(assetFilePath.c_str(), reporting);
            }

            template<typename AssetDataT>
            bool LoadBlocking(AZ::Data::Asset<AssetDataT>& asset, TraceLevel reporting)
            {
                if (asset.IsReady())
                {
                    return true;
                }

                if (!asset.GetId().IsValid())
                {
                    AssetUtilsInternal::ReportIssue(reporting, "Could not load null asset reference");
                    return false;
                }

                Data::Asset<AssetDataT> loadedAsset = AZ::Data::AssetManager::Instance().GetAsset<AssetDataT>(
                    asset.GetId(),
                    true, // queue load
                    nullptr, // filter
                    true // blocking
                    );

                if (!loadedAsset.IsReady())
                {
                    AssetUtilsInternal::ReportIssue(reporting, AZStd::string::format("Could not load '%s'", asset.template ToString<AZStd::string>().c_str()).c_str());
                    return false;
                }

                asset = loadedAsset;

                return true;
            }

            //! This class can be used to do basic asynchronous loading of assets without the need to implement
            //! multiple ebus functions to handle callbacks. It will invoke the provided callback function when the
            //! asset loads or errors. It will stop listening on destruction, so it should be held onto until the
            //! callback fires.
            //! This class will always invoke the callback during OnSystemTick() to prevent
            //! deadlocks related with StreamingImage assets. Here is a quick summary of the deadlock
            //! this class avoids:
             //** Main Thread                | ** Secondary Copy Queue Thread                                                
             //AssetBus::lock(mutex)         |                                                 
             //AssetBus::OnAssetReady        |                                                 
             //StreamingImage::FindOrCreate  |                                                 
             //AsyncUploadQueue::queueWork   |                                                 
             //Wait For Work Complete        |                                                 
             //                              |                      
             //                              | workQueue signaled                              
             //                              | Pop Work                                        
             //                              | StreaminImage::Destructor()                     
             //                              | AssetBus::Disconnect()                          
             //                              | AssetBus::lock(mutex) <- Deadlocked             
             //                                                                                                           
            AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            class ATOM_RPI_REFLECT_API AsyncAssetLoader :
                private Data::AssetBus::Handler,
                private SystemTickBus::Handler
            {
                AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING

            public:
                AZ_RTTI(AZ::RPI::AssetUtils::AsyncAssetLoader, "{E0FB5B08-B97D-40DF-8478-226249C0B654}");

                using AssetCallback = AZStd::function<void(Data::Asset<Data::AssetData>)>;

                AsyncAssetLoader() = default;
                ~AsyncAssetLoader();

                template<typename AssetDataT>
                static AZStd::shared_ptr<AsyncAssetLoader> Create(const AZStd::string& path, uint32_t subId, AssetCallback callback);

                template<typename AssetDataT>
                static AZStd::shared_ptr<AsyncAssetLoader> Create(Data::AssetId assetId, AssetCallback callback);

            private:
                explicit AsyncAssetLoader(AssetCallback callback);

                template<typename AssetDataT>
                void StartLoad(Data::AssetId& assetId);

                // Data::AssetBus::Handler overrides..
                void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
                void OnAssetError(Data::Asset<Data::AssetData> asset) override;

                // SystemTickBus::Handler overrides..
                void OnSystemTick() override;

                // This function should never be called directly under the scope
                // of any of the AssetBus::OnAssetXXXX() functions to avoid deadlocks
                // when working with StreaminImage assets.
                void HandleCallback(Data::Asset<Data::AssetData> asset);

                AssetCallback m_callback;
                Data::Asset<Data::AssetData> m_asset;
            };
        } // namespace AssetUtils
    } // namespace RPI
} // namespace AZ

#include <Atom/RPI.Reflect/Asset/AssetUtils.inl>
