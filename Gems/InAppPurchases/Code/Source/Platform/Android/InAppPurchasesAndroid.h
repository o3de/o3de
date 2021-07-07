/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <InAppPurchases/InAppPurchasesInterface.h>

#include <jni.h>

namespace InAppPurchases
{
    class InAppPurchasesAndroid
        : public InAppPurchasesInterface
    {
    public:
        void Initialize() override;
        
        ~InAppPurchasesAndroid();

        void QueryProductInfo(AZStd::vector<AZStd::string>& productIds) const override;
        void QueryProductInfo() const override;

        void PurchaseProduct(const AZStd::string& productId, const AZStd::string& developerPayload) const override;
        void PurchaseProduct(const AZStd::string& productId) const override;

        void QueryPurchasedProducts() const override;
        
        void RestorePurchasedProducts() const override;
        
        void ConsumePurchase(const AZStd::string& purchaseToken) const override;
        
        void FinishTransaction(const AZStd::string& transactionId, bool downloadHostedContent) const override;

        InAppPurchasesCache* GetCache() override;

    private:
        jobject m_billingInstance;
    };
    


    class ProductDetailsAndroid
        : public ProductDetails
    {
    public:
        AZ_RTTI(ProductDetailsAndroid, "{59A14DA4-B224-4BBD-B43E-8C7BC2EEFEB5}", ProductDetails);

        const AZStd::string& GetProductType() const { return m_productType; }
        
        void SetProductType(const AZStd::string& productType) { m_productType = productType; }
        
    protected:
        AZStd::string m_productType;
    };
}
