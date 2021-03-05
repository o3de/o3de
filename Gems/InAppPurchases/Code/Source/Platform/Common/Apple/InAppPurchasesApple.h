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

#include <InAppPurchases/InAppPurchasesInterface.h>
#include "InAppPurchasesDelegate.h"

namespace InAppPurchases
{
    class InAppPurchasesApple
        : public InAppPurchasesInterface
    {
    public:
        void Initialize() override;
        
        ~InAppPurchasesApple();

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
        InAppPurchasesDelegate* m_delegate;
    };
    


    class ProductDetailsApple
        : public ProductDetails
    {
    public:
        AZ_RTTI(ProductDetailsApple, "{AAF5C20F-482A-45BC-B975-F5864B4C00C5}", ProductDetails);
    };
}