/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace InAppPurchases
{
    /*
    * Store a list of the product's details which are available for purchase
    */
    class ProductDetails
    {
    public:
        AZ_RTTI(ProductDetails, "{D90F4F87-E877-4503-917E-99E9D0A9EE5C}");

        const AZStd::string& GetProductId() const { return m_productId; }
        const AZStd::string& GetProductTitle() const { return m_productName; }
        const AZStd::string& GetProductDescription() const { return m_productDescription; }
        const AZStd::string& GetProductPrice() const { return m_productPrice; }
        const AZStd::string& GetProductCurrencyCode() const { return m_productPriceCurrencyCode; }
        AZ::u64 GetProductPriceMicro() const { return m_productPriceMicro; }
        
        void SetProductId(const AZStd::string& productId) { m_productId = productId; }
        void SetProductTitle(const AZStd::string& productTitle) { m_productName = productTitle; }
        void SetProductDescription(const AZStd::string& productDescription) { m_productDescription = productDescription; }
        void SetProductPrice(const AZStd::string& productPrice) { m_productPrice = productPrice; }
        void SetProductCurrencyCode(const AZStd::string& productCurrencyCode) { m_productPriceCurrencyCode = productCurrencyCode; }
        void SetProductPriceMicro(const AZ::u64& productPriceMicro) { m_productPriceMicro = productPriceMicro; }

        virtual ~ProductDetails() {}
        ProductDetails(const ProductDetails&) = default;
        ProductDetails() = default;
        
    protected:
        AZStd::string m_productId;
        AZStd::string m_productName;
        AZStd::string m_productDescription;
        AZStd::string m_productPrice;
        AZStd::string m_productPriceCurrencyCode;
        AZ::u64 m_productPriceMicro;
    };

    enum PurchaseState
    {
        PURCHASING,
        DEFERRED,
        PURCHASED,
        CANCELLED,
        FAILED,
        RESTORED,
        REFUNDED
    };

    /*
    * Store a list of the purchased product's details
    */
    class PurchasedProductDetails
    {
    public:
        AZ_RTTI(PurchasedProductDetails, "{166DF716-D1C5-4239-BB93-7AFB14FA2400}");

        const AZStd::string& GetProductId() const { return m_productId; }
        const AZStd::string& GetOrderId() const { return m_orderId; }
        const AZStd::string& GetDeveloperPayload() const { return m_developerPayload; }
        virtual AZ::u64 GetPurchaseTime() const { return m_purchaseTime; }
        PurchaseState GetPurchaseState() const { return m_purchaseState; }
        
        void SetProductId(const AZStd::string& productId) { m_productId = productId; }
        void SetOrderId(const AZStd::string& orderId) { m_orderId = orderId; }
        void SetDeveloperPayload(const AZStd::string& developerPayload) { m_developerPayload = developerPayload; }
        void SetPurchaseTime(AZ::u64 purchaseTime) { m_purchaseTime = purchaseTime; }
        void SetPurchaseState(PurchaseState purchaseState) { m_purchaseState = purchaseState; }

        virtual ~PurchasedProductDetails() {}
        PurchasedProductDetails(const PurchasedProductDetails&) = default;
        PurchasedProductDetails() = default;
        
    protected:
        AZStd::string m_productId;
        AZStd::string m_orderId;
        AZStd::string m_developerPayload;
        AZ::u64 m_purchaseTime;
        PurchaseState m_purchaseState;
    };


    /*
    * Class to read from/write to cache
    */
    class InAppPurchasesCache
    {
    public:
        void ClearCachedProductDetails();
        void ClearCachedPurchasedProductDetails();

        void AddProductDetailsToCache(const ProductDetails* productDetails);
        void AddPurchasedProductDetailsToCache(const PurchasedProductDetails* purchasedProductDetails);

        const AZStd::vector<AZStd::unique_ptr<ProductDetails const> >& GetCachedProductDetails() const;
        const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >& GetCachedPurchasedProductDetails() const;

    private:
        AZStd::vector<AZStd::unique_ptr<ProductDetails const> > m_cachedProductDetails;
        AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> > m_cachedPurchasedProducts;
    };


    /*
    * InAppPurchases interface which must be implemented
    * by the platform specific classes for in-app purchasing
    */
    class InAppPurchasesInterface
    {
    public:
        virtual ~InAppPurchasesInterface() { }

        static InAppPurchasesInterface* GetInstance();

        static void DestroyInstance();

        virtual void Initialize() = 0;

        virtual void QueryProductInfo(AZStd::vector<AZStd::string>& productIds) const = 0;
        virtual void QueryProductInfo() const = 0;

        virtual void PurchaseProduct(const AZStd::string& productId, const AZStd::string& developerPayload) const = 0;
        virtual void PurchaseProduct(const AZStd::string& productId) const = 0;

        virtual void QueryPurchasedProducts() const = 0;
        
        virtual void RestorePurchasedProducts() const = 0;
        
        virtual void ConsumePurchase(const AZStd::string& purchaseToken) const = 0;
        
        virtual void FinishTransaction(const AZStd::string& transactionId, bool downloadHostedContent) const = 0;

        virtual InAppPurchasesCache* GetCache() = 0;

    protected:
        InAppPurchasesCache m_cache;

    private:
        static InAppPurchasesInterface* CreateInstance();
        static InAppPurchasesInterface* iapInstance;
    };
}
