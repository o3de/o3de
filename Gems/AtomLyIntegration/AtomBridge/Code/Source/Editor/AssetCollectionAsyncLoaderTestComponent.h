/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <Atom/Utils/AssetCollectionAsyncLoader.h>

namespace AZ
{
    namespace AtomBridge
    {
        /**
         * Interface for AZ::AtomBridge::AssetCollectionAsyncLoaderTestBus, which is an EBus that receives requests
         * to test AssetCollectionAsyncLoader API
         */
        class AssetCollectionAsyncLoaderTestInterface
            : public ComponentBus
        {
        public:
            AZ_RTTI(AssetCollectionAsyncLoaderTestInterface, "{2C000A68-3B9A-4462-B8CF-E2995FA2C208}");

            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            //////////////////////////////////////////////////////////////////////////

            /**
             * Destroys the instance of the class.
             */
            virtual ~AssetCollectionAsyncLoaderTestInterface() {}

            //////////////////////////////////////////////////////////////////////////
            // The API

            //! @pathToAssetListJson Path to a json file with a plain list of file paths.
            //!                      Each path is the path of an asset product.
            //!                      The AssetType will be deduced from the file extension.
            //! Returns true if the asset loading job starts successfully.
            virtual bool StartLoadingAssetsFromJsonFile(const AZStd::string& pathToAssetListJson) = 0;

            //! @assetList Similar as above but the list of assets is given directly.
            //! Returns true if the asset loading job starts successfully.
            virtual bool StartLoadingAssetsFromAssetList(const AZStd::vector<AZStd::string>& assetList) = 0;

            //! Cancels any pending job to that has been queued by this component.
            virtual void CancelLoadingAssets() = 0;

            //! Returns a list of the assets that have not been loaded yet from the Asset Processor Cache.
            virtual AZStd::vector<AZStd::string> GetPendingAssetsList() const = 0;

            //! Shortcut to GetPendingAssetsList().size()
            virtual uint32_t GetCountOfPendingAssets() const = 0;

            //! Returns true if the asset was loaded successfully.
            virtual bool ValidateAssetWasLoaded(const AZStd::string& assetPath) const = 0;
        };

        /**
         * The EBus for events  defined in AssetCollectionAsyncLoaderTestInterface class .
         */
        typedef AZ::EBus<AssetCollectionAsyncLoaderTestInterface> AssetCollectionAsyncLoaderTestBus;


        /*
        * This class is designed to be used under automation, but the user can add it to an entity
        * and manually specify a json file with a list of asset paths to load asynchronously. From an user point of view
        * it has no value, but for debugging it can be useful to try the AssetCollectionAsyncLoader API without having to write
        * a test suite for it.
        */
        class AssetCollectionAsyncLoaderTestComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public AssetCollectionAsyncLoaderTestBus::Handler
        {
        public:
            AZ_EDITOR_COMPONENT(AssetCollectionAsyncLoaderTestComponent, "{D0A558AD-F8CD-4DB8-80A4-40B4E1F947FA}"
                                , AzToolsFramework::Components::EditorComponentBase
                                , AssetCollectionAsyncLoaderTestInterface)

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // AssetCollectionAsyncLoaderTestBus overrides
            bool StartLoadingAssetsFromJsonFile(const AZStd::string& pathToAssetListJson) override;
            bool StartLoadingAssetsFromAssetList(const AZStd::vector<AZStd::string>& assetList) override;
            void CancelLoadingAssets() override;
            AZStd::vector<AZStd::string> GetPendingAssetsList() const override;
            uint32_t GetCountOfPendingAssets() const override;
            bool ValidateAssetWasLoaded(const AZStd::string& assetPath) const override;
            //////////////////////////////////////////////////////////////////////////

        protected:

            enum class State
            {
                Idle,
                LoadingAssets,
                FatalError,
            };

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
            {
                services.push_back(AZ_CRC("AssetCollectionAsyncLoaderTest", 0x66d04369));
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
            {
                services.push_back(AZ_CRC("AssetCollectionAsyncLoaderTest", 0x66d04369));
            }

            static void Reflect(AZ::ReflectContext* context);

            AZ::Crc32 OnStartCancelButtonClicked();
            AZStd::string GetStartCancelButtonText() const;

            //! Serialized member variables.
            //! A user editable path to a json file that contains the list of assets to load.
            AZStd::string m_pathToAssetListJson;

            //! Non-serialized member variables.
            State m_state = State::Idle;
            AZStd::unordered_set<AZStd::string> m_pendingAssets; // List of assets that have not been loaded yet.

            // This is the Object Under Test
            AZStd::unique_ptr<AZ::AssetCollectionAsyncLoader> m_assetCollectionAsyncLoader;
        };
    } // namespace AtomBridge
} // namespace AZ
