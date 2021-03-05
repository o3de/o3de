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

#include "InAppPurchases/InAppPurchasesBus.h"
#include <AzCore/Component/Component.h>

namespace InAppPurchases
{
    class SystemComponent
        : public AZ::Component
        , public InAppPurchasesRequestBus::Handler
        , public InAppPurchasesResponseAccessorBus::Handler
    {
    public:
        AZ_COMPONENT(SystemComponent, "{D0ABA496-16A7-4090-98AB-6D372BE7BD45}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ////////////////////////////////////////////////////////////////////////
        // InAppPurchasesRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        void Initialize() override;

        void QueryProductInfoById(const AZStd::string& productId) const override;
        void QueryProductInfoByIds(AZStd::vector<AZStd::string>& productIds) const override;
        void QueryProductInfo() const override;
        void QueryProductInfoFromJson(const AZStd::string& filePath) const override;

        const AZStd::vector<AZStd::unique_ptr<ProductDetails const> >* GetCachedProductInfo() const override;
        const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* GetCachedPurchasedProductInfo() const override;

        void PurchaseProductWithDeveloperPayload(const AZStd::string& productId, const AZStd::string& developerPayload) const override;
        void PurchaseProduct(const AZStd::string& productId) const override;
        
        void QueryPurchasedProducts() const override;
        
        void RestorePurchasedProducts() const override;
        
        void ConsumePurchase(const AZStd::string& purchaseToken) const override;
        
        void FinishTransaction(const AZStd::string& transactionId, bool downloadHostedContent) const override;

        void ClearCachedProductDetails() override;
        void ClearCachedPurchasedProductDetails() override;

        // Response Accessors
        bool GetNextProduct() override;
        bool GetPreviousProduct() override;
        bool GetNextPurchasedProduct() override;
        bool GetPreviousPurchasedProduct() override;
        
        AZStd::string GetProductId() override;
        AZStd::string GetProductTitle() override;
        AZStd::string GetProductDescription() override;
        AZStd::string GetProductPrice() override;
        AZStd::string GetProductCurrencyCode() override;
        AZ::u64 GetProductPriceMicro() override;
        
        AZStd::string GetPurchasedProductId() override;
        AZStd::string GetOrderId() override;
        AZStd::string GetDeveloperPayload() override;
        AZStd::string GetPurchaseTime() override;
        AZStd::string GetPurchaseSignature() override;
        AZStd::string GetPackageName() override;
        AZStd::string GetPurchaseToken() override;
        bool IsAutoRenewing() override;
        AZStd::string GetRestoredOrderId() override;
        AZ::u64 GetSubscriptionExpirationTime() override;
        AZ::u64 GetRestoredPurchaseTime() override;
        bool HasDownloads() override;
        bool IsProductOwned() override;
        
        void ResetIndices() override;

    protected:

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:

        int m_productInfoIndex;
        int m_purchasedProductInfoIndex;
    };
}
