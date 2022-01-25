/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzFramework/Asset/AssetRegistry.h>

#include <LmbrCentral/Bundling/BundlingSystemComponentBus.h>

#include <AzFramework/Archive/ArchiveBus.h>

namespace AzFramework
{
    class AssetBundleManifest;
}

namespace LmbrCentral
{
    struct OpenBundleInfo
    {
        OpenBundleInfo() = default;

        AZStd::shared_ptr<AzFramework::AssetBundleManifest> m_manifest;
        AZStd::shared_ptr<AzFramework::AssetRegistry> m_catalog;
    };
    /**
     * System component for managing bundles
     */
    class BundlingSystemComponent
        : public AZ::Component
        , public BundlingSystemRequestBus::Handler
        , public AZ::IO::ArchiveNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(BundlingSystemComponent, "{0FB7153D-EE80-4B1C-9584-134270401AAF}");
        static void Reflect(AZ::ReflectContext* context);

        BundlingSystemComponent() = default;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("BundlingService", 0xc9a43659));
        }

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // BundlingSystemRequestBus
        void LoadBundles(const char* baseFolder, const char* fileExtension) override;
        void UnloadBundles() override;

        void BundleOpened(const char* bundleName, AZStd::shared_ptr<AzFramework::AssetBundleManifest> bundleManifest, const char* nextBundle, AZStd::shared_ptr<AzFramework::AssetRegistry> bundleCatalog) override;
        void BundleClosed(const char* bundleName) override;


        AZStd::vector<AZStd::string> GetBundleList(const char* bundlePath, const char* bundleExtension) const;

        //! Bundles which are split across archives (Usually due to size constraints) have the dependent bundles listed in the manifest
        //! of the main bundle.  This method manages opening the dependent bundles.
        void OpenDependentBundles(const char* bundleName, AZStd::shared_ptr<AzFramework::AssetBundleManifest> bundleManifest);
        //! Bundles which are split across archives (Usually due to size constraints) have the dependent bundles listed in the manifest
        //! of the main bundle.  This method manages closing the dependent bundles.
        void CloseDependentBundles(const char* bundleName, AZStd::shared_ptr<AzFramework::AssetBundleManifest> bundleManifest);

        size_t GetOpenedBundleCount() const override;
    private:
        // Maintain a list of all opened bundles as reported through the BundleOpened ebus
        AZStd::unordered_map<AZStd::string, AZStd::unique_ptr<OpenBundleInfo>> m_openedBundles;
        //  Used to maintain the order of the opened bundles to properly apply catalog info
        AZStd::vector<AZStd::string> m_openedBundleList;

        // Maintain a list of bundles opened through our "LoadBundles" command.
        // We'll unmount only this list rather than all opened bundles when calling UnloadBundles
        AZStd::vector<AZStd::string> m_bundleModeBundles;
        mutable AZStd::mutex m_openedBundleMutex;
        AZStd::mutex m_bundleModeMutex;
    };
}
