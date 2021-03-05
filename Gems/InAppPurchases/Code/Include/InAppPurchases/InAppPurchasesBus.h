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

#include <AzCore/EBus/EBus.h>
#include "InAppPurchases/InAppPurchasesInterface.h"

namespace InAppPurchases
{
    class InAppPurchasesRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void Initialize() = 0;

        // All Queries and Purchase requests will send a response to the InAppPurchasesResponseBus
        // Implement that class to receive those responses
        // Request product details for a single product or a group of products
        virtual void QueryProductInfoById(const AZStd::string& productId) const = 0;
        virtual void QueryProductInfoByIds(AZStd::vector<AZStd::string>& productIds) const = 0;
        virtual void QueryProductInfo() const = 0;
        virtual void QueryProductInfoFromJson(const AZStd::string& filePath) const = 0;

        virtual const AZStd::vector<AZStd::unique_ptr<ProductDetails const> >* GetCachedProductInfo() const = 0;
        virtual const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* GetCachedPurchasedProductInfo() const = 0;

        // Purchase a product
        virtual void PurchaseProductWithDeveloperPayload(const AZStd::string& productId, const AZStd::string& developerPayload) const = 0;
        virtual void PurchaseProduct(const AZStd::string& productId) const = 0;

        // Request list of products already purchased by the user
        virtual void QueryPurchasedProducts() const = 0;
        
        virtual void RestorePurchasedProducts() const = 0;
        
        // This should be called when a user buys any consumable product(like virtual currency). Otherwise, the user will not be able to buy this product again.
        virtual void ConsumePurchase(const AZStd::string& purchaseToken) const = 0;
        
        // This should be called for all transactions when you've processed on the purchase details and delivered the content.
        virtual void FinishTransaction(const AZStd::string& transactionId, bool downloadHostedContent) const = 0;

        virtual void ClearCachedProductDetails() = 0;
        virtual void ClearCachedPurchasedProductDetails() = 0;
    };

    using InAppPurchasesRequestBus = AZ::EBus<InAppPurchasesRequests>;

    class InAppPurchasesResponseAccessor
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual bool GetNextProduct() { return false; }
        virtual bool GetPreviousProduct() { return false; }
        virtual bool GetNextPurchasedProduct() { return false; }
        virtual bool GetPreviousPurchasedProduct() { return false; }

        virtual AZStd::string GetProductId() { return ""; }
        virtual AZStd::string GetProductTitle() { return ""; }
        virtual AZStd::string GetProductDescription() { return ""; }
        virtual AZStd::string GetProductPrice() { return ""; }
        virtual AZStd::string GetProductCurrencyCode() { return ""; }
        virtual AZ::u64 GetProductPriceMicro() { return 0; }

        virtual AZStd::string GetPurchasedProductId() { return ""; }
        virtual AZStd::string GetOrderId() { return ""; }
        virtual AZStd::string GetDeveloperPayload() { return ""; }
        virtual AZStd::string GetPurchaseSignature() { return ""; }
        virtual AZStd::string GetPurchaseToken() { return""; }
        virtual AZStd::string GetRestoredOrderId() { return ""; }
        virtual AZStd::string GetPackageName() { return ""; }
        virtual AZStd::string GetPurchaseTime() { return ""; }
        virtual AZ::u64 GetSubscriptionExpirationTime() { return 0; }
        virtual AZ::u64 GetRestoredPurchaseTime() { return 0; }
        virtual bool IsAutoRenewing() { return false; }
        virtual bool HasDownloads() { return false; }
        virtual bool IsProductOwned() { return false; }

        virtual void ResetIndices() {}
    };

    using InAppPurchasesResponseAccessorBus = AZ::EBus<InAppPurchasesResponseAccessor>;
}
