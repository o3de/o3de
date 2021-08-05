/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <InAppPurchases/InAppPurchasesInterface.h>

namespace InAppPurchases
{
    class InAppPurchasesResponse
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void ProductInfoRetrieved(const AZStd::vector<AZStd::unique_ptr<ProductDetails const> >& productDetails) { (void)productDetails; }
        virtual void PurchasedProductsRetrieved(const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >& purchasedProductDetails) { (void)purchasedProductDetails;  }
        virtual void PurchasedProductsRestored(const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >& purchasedProductDetails) { (void)purchasedProductDetails;  }
        virtual void NewProductPurchased(const PurchasedProductDetails* purchasedProductDetails) { (void)purchasedProductDetails; }
        virtual void PurchaseConsumed(const AZStd::string& purchaseToken) { (void)purchaseToken; }
        virtual void PurchaseCancelled(const PurchasedProductDetails* purchasedProductDetails) { (void)purchasedProductDetails; }
        virtual void PurchaseRefunded(const PurchasedProductDetails* purchasedProductDetails) { (void)purchasedProductDetails; }
        virtual void PurchaseFailed(const PurchasedProductDetails* purchasedProductDetails) { (void)purchasedProductDetails; }
        virtual void HostedContentDownloadComplete(const AZStd::string& transactionId, const AZStd::string& downloadedFileLocation) { (void)downloadedFileLocation; (void)transactionId; }
        virtual void HostedContentDownloadFailed(const AZStd::string& transactionId, const AZStd::string& contentId) { (void)transactionId; (void)contentId; }
    };

    using InAppPurchasesResponseBus = AZ::EBus<InAppPurchasesResponse>;

    // This API should be re-designed to be platform-agnostic, or if that is
    // not possible we should move it to an Android specific include folder.
    // But it can stay here for now because it's not a restricted platform.
    class PurchasedProductDetailsAndroid
        : public PurchasedProductDetails
    {
    public:
        AZ_RTTI(PurchasedProductDetailsAndroid, "{86A7072A-4661-4DAA-A811-F9279B089859}", PurchasedProductDetails);
        
        AZ::u64 GetPurchaseTime() const override { return m_purchaseTime / 1000; }
        const AZStd::string& GetPurchaseSignature() const { return m_purchaseSignature; }
        const AZStd::string& GetPackageName() const { return m_packageName; }
        const AZStd::string& GetPurchaseToken() const { return m_purchaseToken; }
        bool GetIsAutoRenewing() const { return m_autoRenewing; }

        void SetPurchaseSignature(const AZStd::string& purchaseSignature) { m_purchaseSignature = purchaseSignature; }
        void SetPackageName(const AZStd::string& packageName) { m_packageName = packageName; }
        void SetPurchaseToken(const AZStd::string& purchaseToken) { m_purchaseToken = purchaseToken; }
        void SetIsAutoRenewing(bool autoRenewing) { m_autoRenewing = autoRenewing; }

    protected:
        AZStd::string m_purchaseSignature;
        AZStd::string m_packageName;
        AZStd::string m_purchaseToken;
        bool m_autoRenewing;
    };

    // This API should be re-designed to be platform-agnostic, or if that is
    // not possible we should move it to an Apple specific include folder.
    // But it can stay here for now because it's not a restricted platform.
    class PurchasedProductDetailsApple
        : public PurchasedProductDetails
    {
    public:
        AZ_RTTI(PurchasedProductDetailsApple, "{31C108A3-9676-457A-9F1E-B752DBF96BC6}", PurchasedProductDetails);

        const AZStd::string& GetRestoredOrderId() const { return m_restoredOrderId; }
        AZ::u64 GetSubscriptionExpirationTime() const { return m_subscriptionExpirationTime; }
        AZ::u64 GetRestoredPurchaseTime() const { return m_restoredPurchaseTime; }
        bool GetHasDownloads() const { return m_hasDownloads; }

        void SetRestoredOrderId(const AZStd::string& restoredOrderId) { m_restoredOrderId = restoredOrderId; }
        void SetSubscriptionExpirationTime(AZ::u64 subscriptionExpirationTime) { m_subscriptionExpirationTime = subscriptionExpirationTime; }
        void SetRestoredPurchaseTime(AZ::u64 restoredPurchaseTime) { m_restoredPurchaseTime = restoredPurchaseTime; }
        void SetHasDownloads(bool hasDownloads) { m_hasDownloads = hasDownloads; }

    protected:
        AZStd::string m_restoredOrderId;
        AZ::u64 m_subscriptionExpirationTime;
        AZ::u64 m_restoredPurchaseTime;
        bool m_hasDownloads;
    };
}
