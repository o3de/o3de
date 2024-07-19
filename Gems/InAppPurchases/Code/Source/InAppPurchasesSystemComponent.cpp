/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <time.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/error/en.h>

#include "InAppPurchases/InAppPurchasesResponseBus.h"
#include "InAppPurchasesSystemComponent.h"

namespace InAppPurchases
{
    void SystemComponent::Init()
    {
    }

    void SystemComponent::Activate()
    {
        m_productInfoIndex = 0;
        m_purchasedProductInfoIndex = 0;
        InAppPurchasesRequestBus::Handler::BusConnect();
        InAppPurchasesResponseAccessorBus::Handler::BusConnect();
    }

    void SystemComponent::Deactivate()
    {
        // The instance is created on the first call to GetInstance()
        InAppPurchasesInterface::DestroyInstance();
        InAppPurchasesRequestBus::Handler::BusDisconnect();
        InAppPurchasesResponseAccessorBus::Handler::BusDisconnect();
    }

    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<SystemComponent>("InAppPurchases", "Adds support for in app purchases on iOS and Android")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<InAppPurchasesRequestBus>("InAppPurchasesRequestBus")
                ->Event("Initialize", &InAppPurchasesRequestBus::Events::Initialize)
                ->Event("QueryProductInfo", &InAppPurchasesRequestBus::Events::QueryProductInfo)
                ->Event("QueryProductInfoFromJson", &InAppPurchasesRequestBus::Events::QueryProductInfoFromJson)
                ->Event("PurchaseProductWithDeveloperPayload", &InAppPurchasesRequestBus::Events::PurchaseProductWithDeveloperPayload)
                ->Event("PurchaseProduct", &InAppPurchasesRequestBus::Events::PurchaseProduct)
                ->Event("QueryPurchasedProducts", &InAppPurchasesRequestBus::Events::QueryPurchasedProducts)
                ->Event("ConsumePurchase", &InAppPurchasesRequestBus::Events::ConsumePurchase)
                ->Event("FinishTransaction", &InAppPurchasesRequestBus::Events::FinishTransaction)
                ;

            behaviorContext->EBus<InAppPurchasesResponseAccessorBus>("InAppPurchasesResponseAccessorBus")
                ->Event("NextProduct", &InAppPurchasesResponseAccessorBus::Events::GetNextProduct)
                ->Event("PreviousProduct", &InAppPurchasesResponseAccessorBus::Events::GetPreviousProduct)
                ->Event("NextPurchasedProduct", &InAppPurchasesResponseAccessorBus::Events::GetNextPurchasedProduct)
                ->Event("PreviousPurchasedProduct", &InAppPurchasesResponseAccessorBus::Events::GetPreviousPurchasedProduct)
                ->Event("ProductId", &InAppPurchasesResponseAccessorBus::Events::GetProductId)
                ->Event("ProductTitle", &InAppPurchasesResponseAccessorBus::Events::GetProductTitle)
                ->Event("ProductDescription", &InAppPurchasesResponseAccessorBus::Events::GetProductDescription)
                ->Event("ProductPrice", &InAppPurchasesResponseAccessorBus::Events::GetProductPrice)
                ->Event("ProductCurrencyCode", &InAppPurchasesResponseAccessorBus::Events::GetProductCurrencyCode)
                ->Event("ProductPriceMicro", &InAppPurchasesResponseAccessorBus::Events::GetProductPriceMicro)
                ->Event("PurchasedProductId", &InAppPurchasesResponseAccessorBus::Events::GetPurchasedProductId)
                ->Event("PurchaseTime", &InAppPurchasesResponseAccessorBus::Events::GetPurchaseTime)
                ->Event("OrderId", &InAppPurchasesResponseAccessorBus::Events::GetOrderId)
                ->Event("DeveloperPayload", &InAppPurchasesResponseAccessorBus::Events::GetDeveloperPayload)
                ->Event("PurchaseSignature", &InAppPurchasesResponseAccessorBus::Events::GetPurchaseSignature)
                ->Event("PackageName", &InAppPurchasesResponseAccessorBus::Events::GetPackageName)
                ->Event("PurchaseToken", &InAppPurchasesResponseAccessorBus::Events::GetPurchaseToken)
                ->Event("IsAutoRenewing", &InAppPurchasesResponseAccessorBus::Events::IsAutoRenewing)
                ->Event("RestoredOrderId", &InAppPurchasesResponseAccessorBus::Events::GetRestoredOrderId)
                ->Event("SubscriptionExpirationTime", &InAppPurchasesResponseAccessorBus::Events::GetSubscriptionExpirationTime)
                ->Event("RestoredPurchaseTime", &InAppPurchasesResponseAccessorBus::Events::GetRestoredPurchaseTime)
                ->Event("HasDownloads", &InAppPurchasesResponseAccessorBus::Events::HasDownloads)
                ->Event("IsProductOwned", &InAppPurchasesResponseAccessorBus::Events::IsProductOwned)
                ->Event("ResetIndices", &InAppPurchasesResponseAccessorBus::Events::ResetIndices)
                ;
        }
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("InAppPurchasesService"));
    }

    void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("InAppPurchasesService"));
    }

    void SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void SystemComponent::Initialize()
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->Initialize();
        }
    }

    void SystemComponent::QueryProductInfoById(const AZStd::string& productId) const
    {
        AZStd::vector<AZStd::string> productIds;
        productIds.push_back(productId);
        SystemComponent::QueryProductInfoByIds(productIds);
    }

    void SystemComponent::QueryProductInfoByIds(AZStd::vector<AZStd::string>& productIds) const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->QueryProductInfo(productIds);
        }
    }

    void SystemComponent::QueryProductInfo() const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->QueryProductInfo();
        }
    }

    void SystemComponent::QueryProductInfoFromJson(const AZStd::string& jsonFilePath) const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            AZ::IO::FileIOBase* fileReader = AZ::IO::FileIOBase::GetInstance();

            AZStd::string fileBuffer;

            AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
            AZ::u64 fileSize = 0;
            if (!fileReader->Open(jsonFilePath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, fileHandle))
            {
                AZ::Failure(AZStd::string::format("Failed to read %s - unable to open file", jsonFilePath.c_str()));
                return;
            }

            if ((!fileReader->Size(fileHandle, fileSize)) || (fileSize == 0))
            {
                fileReader->Close(fileHandle);
                AZ::Failure(AZStd::string::format("Failed to read %s - file truncated.", jsonFilePath.c_str()));
                return;
            }
            fileBuffer.resize(fileSize);
            if (!fileReader->Read(fileHandle, fileBuffer.data(), fileSize, true))
            {
                fileBuffer.resize(0);
                fileReader->Close(fileHandle);
                AZ::Failure(AZStd::string::format("Failed to read %s - file read failed.", jsonFilePath.c_str()));
                return;
            }
            fileReader->Close(fileHandle);
            rapidjson::Document doc;

            doc.Parse(fileBuffer.data());

            if (doc.HasParseError())
            {
                AZ_Warning("LumberyardInAppBilling", false, "Failed to parse product_ids: %s\n", rapidjson::GetParseError_En(doc.GetParseError()));
                return;
            }

            AZStd::vector<AZStd::string> productIdsVec;
            if (doc.HasMember("product_ids") && doc["product_ids"].GetType() == rapidjson::kArrayType)
            {
                rapidjson::Value& productIdsArray = doc["product_ids"];
                for (rapidjson::Value::ConstValueIterator itr = productIdsArray.Begin(); itr != productIdsArray.End(); itr++)
                {
                    if ((*itr).HasMember("id"))
                    {
                        productIdsVec.push_back((*itr)["id"].GetString());
                    }
                }
                InAppPurchasesInterface::GetInstance()->QueryProductInfo(productIdsVec);
            }
            else
            {
                AZ_Warning("O3DEInAppPurchases", false, "The JSON string provided does not contain an array named ProductIds!(Property *has* to be an array)");
            }
        }
    }

    const AZStd::vector<AZStd::unique_ptr<ProductDetails const> >* SystemComponent::GetCachedProductInfo() const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            return &InAppPurchasesInterface::GetInstance()->GetCache()->GetCachedProductDetails();
        }
        return nullptr;
    }

    const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* SystemComponent::GetCachedPurchasedProductInfo() const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            return &InAppPurchasesInterface::GetInstance()->GetCache()->GetCachedPurchasedProductDetails();
        }
        return nullptr;
    }

    void SystemComponent::PurchaseProductWithDeveloperPayload(const AZStd::string& productId, const AZStd::string& developerPayload) const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->PurchaseProduct(productId, developerPayload);
        }
    }

    void SystemComponent::PurchaseProduct(const AZStd::string& productId) const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->PurchaseProduct(productId);
        }
    }

    void SystemComponent::QueryPurchasedProducts() const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->QueryPurchasedProducts();
        }
    }

    void SystemComponent::RestorePurchasedProducts() const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->RestorePurchasedProducts();
        }
    }

    void SystemComponent::ConsumePurchase(const AZStd::string& purchaseToken) const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->ConsumePurchase(purchaseToken);
        }
    }

    void SystemComponent::FinishTransaction(const AZStd::string& transactionId, bool downloadHostedContent) const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->FinishTransaction(transactionId, downloadHostedContent);
        }
    }

    void SystemComponent::ClearCachedProductDetails()
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->GetCache()->ClearCachedProductDetails();
        }
    }

    void SystemComponent::ClearCachedPurchasedProductDetails()
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->GetCache()->ClearCachedPurchasedProductDetails();
        }
    }

    bool SystemComponent::GetNextProduct()
    {
        m_productInfoIndex++;

        const AZStd::vector<AZStd::unique_ptr<ProductDetails const> >* productDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(productDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedProductInfo);

        if (productDetails != nullptr)
        {
            if (m_productInfoIndex >= productDetails->size())
            {
                m_productInfoIndex = 0;
            }

            if (productDetails->size() > 0)
            {
                return true;
            }
        }

        return false;
    }

    bool SystemComponent::GetPreviousProduct()
    {
        m_productInfoIndex--;

        const AZStd::vector<AZStd::unique_ptr<ProductDetails const> >* productDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            productDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedProductInfo);

        if (productDetails != nullptr)
        {
            if (m_productInfoIndex < 0)
            {
                m_productInfoIndex = static_cast<int>(productDetails->size() - 1);
            }

            if (productDetails->size() > 0)
            {
                return true;
            }
        }

        return false;
    }

    bool SystemComponent::GetNextPurchasedProduct()
    {
        m_purchasedProductInfoIndex++;

        const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* purchasedProductDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            purchasedProductDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedPurchasedProductInfo);

        if (purchasedProductDetails != nullptr)
        {
            if (m_purchasedProductInfoIndex >= purchasedProductDetails->size())
            {
                m_purchasedProductInfoIndex = 0;
            }

            if (purchasedProductDetails->size() > 0)
            {
                return true;
            }
        }

        return false;
    }

    bool SystemComponent::GetPreviousPurchasedProduct()
    {
        m_purchasedProductInfoIndex--;

        const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* purchasedProductDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            purchasedProductDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedPurchasedProductInfo);

        if (purchasedProductDetails != nullptr)
        {
            if (m_purchasedProductInfoIndex < 0)
            {
                m_purchasedProductInfoIndex = static_cast<int>(purchasedProductDetails->size() - 1);
            }

            if (purchasedProductDetails->size() > 0)
            {
                return true;
            }
        }

        return false;
    }

    AZStd::string SystemComponent::GetProductId()
    {
        const AZStd::vector<AZStd::unique_ptr<ProductDetails const> >* productDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            productDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedProductInfo);

        if (productDetails != nullptr && m_productInfoIndex >= 0 && m_productInfoIndex < productDetails->size())
        {
            return productDetails->at(m_productInfoIndex)->GetProductId();
        }

        return "";
    }

    AZStd::string SystemComponent::GetProductTitle()
    {
        const AZStd::vector<AZStd::unique_ptr<ProductDetails const> >* productDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            productDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedProductInfo);

        if (productDetails != nullptr && m_productInfoIndex >= 0 && m_productInfoIndex < productDetails->size())
        {
            return productDetails->at(m_productInfoIndex)->GetProductTitle();
        }

        return "";
    }

    AZStd::string SystemComponent::GetProductDescription()
    {
        const AZStd::vector<AZStd::unique_ptr<ProductDetails const> >* productDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            productDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedProductInfo);

        if (productDetails != nullptr && m_productInfoIndex >= 0 && m_productInfoIndex < productDetails->size())
        {
            return productDetails->at(m_productInfoIndex)->GetProductDescription();
        }

        return "";
    }

    AZStd::string SystemComponent::GetProductPrice()
    {
        const AZStd::vector<AZStd::unique_ptr<ProductDetails const> >* productDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            productDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedProductInfo);

        if (productDetails != nullptr && m_productInfoIndex >= 0 && m_productInfoIndex < productDetails->size())
        {
            return productDetails->at(m_productInfoIndex)->GetProductPrice();
        }

        return "";
    }

    AZStd::string SystemComponent::GetProductCurrencyCode()
    {
        const AZStd::vector<AZStd::unique_ptr<ProductDetails const> >* productDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            productDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedProductInfo);

        if (productDetails != nullptr && m_productInfoIndex >= 0 && m_productInfoIndex < productDetails->size())
        {
            return productDetails->at(m_productInfoIndex)->GetProductCurrencyCode();
        }

        return "";
    }

    AZ::u64 SystemComponent::GetProductPriceMicro()
    {
        const AZStd::vector<AZStd::unique_ptr<ProductDetails const> >* productDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            productDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedProductInfo);

        if (productDetails != nullptr && m_productInfoIndex >= 0 && m_productInfoIndex < productDetails->size())
        {
            return productDetails->at(m_productInfoIndex)->GetProductPriceMicro();
        }

        return 0;
    }

    AZStd::string SystemComponent::GetPurchasedProductId()
    {
        const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* purchasedProductDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            purchasedProductDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedPurchasedProductInfo);

        if (purchasedProductDetails != nullptr && m_purchasedProductInfoIndex >= 0 && m_purchasedProductInfoIndex < purchasedProductDetails->size())
        {
            return purchasedProductDetails->at(m_purchasedProductInfoIndex)->GetProductId();
        }

        return "";
    }

    AZStd::string SystemComponent::GetOrderId()
    {
        const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* purchasedProductDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            purchasedProductDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedPurchasedProductInfo);

        if (purchasedProductDetails != nullptr && m_purchasedProductInfoIndex >= 0 && m_purchasedProductInfoIndex < purchasedProductDetails->size())
        {
            return purchasedProductDetails->at(m_purchasedProductInfoIndex)->GetOrderId();
        }

        return "";
    }

    AZStd::string SystemComponent::GetDeveloperPayload()
    {
        const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* purchasedProductDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            purchasedProductDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedPurchasedProductInfo);

        if (purchasedProductDetails != nullptr && m_purchasedProductInfoIndex >= 0 && m_purchasedProductInfoIndex < purchasedProductDetails->size())
        {
            return purchasedProductDetails->at(m_purchasedProductInfoIndex)->GetDeveloperPayload();
        }

        return "";
    }

    AZStd::string SystemComponent::GetPurchaseTime()
    {
        const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* purchasedProductDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            purchasedProductDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedPurchasedProductInfo);

        AZStd::string date;

        if (purchasedProductDetails != nullptr && m_purchasedProductInfoIndex >= 0 && m_purchasedProductInfoIndex < purchasedProductDetails->size())
        {
            AZ::u64 time = purchasedProductDetails->at(m_purchasedProductInfoIndex)->GetPurchaseTime();
            time_t t = static_cast<time_t>(time);
            tm local;
            azlocaltime(&t, &local);

            date.resize(24); // 3 (day) + 3 (month) + 2 (date) + 8 (time) + 4 (year) + 4 (spaces)
            strftime(date.data(), date.size(), "%a %b %d %H:%M:%S %Y", &local);
        }

        return date;
    }

    AZStd::string SystemComponent::GetPurchaseSignature()
    {
        const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* purchasedProductDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            purchasedProductDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedPurchasedProductInfo);
        if (purchasedProductDetails != nullptr && m_purchasedProductInfoIndex >= 0 && m_purchasedProductInfoIndex < purchasedProductDetails->size())
        {
            const PurchasedProductDetailsAndroid* purchasedProductsAndroid = azrtti_cast<const PurchasedProductDetailsAndroid*>(purchasedProductDetails->at(m_purchasedProductInfoIndex).get());
            if (purchasedProductsAndroid)
            {
                return purchasedProductsAndroid->GetPurchaseSignature();
            }
        }

        return "";
    }

    AZStd::string SystemComponent::GetPackageName()
    {
        const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* purchasedProductDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            purchasedProductDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedPurchasedProductInfo);
        if (purchasedProductDetails != nullptr && m_purchasedProductInfoIndex >= 0 && m_purchasedProductInfoIndex < purchasedProductDetails->size())
        {
            const PurchasedProductDetailsAndroid* purchasedProductsAndroid = azrtti_cast<const PurchasedProductDetailsAndroid*>(purchasedProductDetails->at(m_purchasedProductInfoIndex).get());
            if (purchasedProductsAndroid)
            {
                return purchasedProductsAndroid->GetPackageName();
            }
        }

        return "";
    }

    AZStd::string SystemComponent::GetPurchaseToken()
    {
        const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* purchasedProductDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            purchasedProductDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedPurchasedProductInfo);
        if (purchasedProductDetails != nullptr && m_purchasedProductInfoIndex >= 0 && m_purchasedProductInfoIndex < purchasedProductDetails->size())
        {
            const PurchasedProductDetailsAndroid* purchasedProductsAndroid = azrtti_cast<const PurchasedProductDetailsAndroid*>(purchasedProductDetails->at(m_purchasedProductInfoIndex).get());
            if (purchasedProductsAndroid)
            {
                return purchasedProductsAndroid->GetPurchaseToken();
            }
        }

        return "";
    }

    bool SystemComponent::IsAutoRenewing()
    {
        const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* purchasedProductDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            purchasedProductDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedPurchasedProductInfo);
        if (purchasedProductDetails != nullptr && m_purchasedProductInfoIndex >= 0 && m_purchasedProductInfoIndex < purchasedProductDetails->size())
        {
            const PurchasedProductDetailsAndroid* purchasedProductsAndroid = azrtti_cast<const PurchasedProductDetailsAndroid*>(purchasedProductDetails->at(m_purchasedProductInfoIndex).get());
            if (purchasedProductsAndroid)
            {
                return purchasedProductsAndroid->GetIsAutoRenewing();
            }
        }

        return false;
    }

    AZStd::string SystemComponent::GetRestoredOrderId()
    {
        const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* purchasedProductDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            purchasedProductDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedPurchasedProductInfo);
        if (purchasedProductDetails != nullptr && m_purchasedProductInfoIndex >= 0 && m_purchasedProductInfoIndex < purchasedProductDetails->size())
        {
            const PurchasedProductDetailsApple* purchasedProductsApple = azrtti_cast<const PurchasedProductDetailsApple*>(purchasedProductDetails->at(m_purchasedProductInfoIndex).get());
            if (purchasedProductsApple)
            {
                return purchasedProductsApple->GetRestoredOrderId();
            }
        }

        return "";
    }

    AZ::u64 SystemComponent::GetSubscriptionExpirationTime()
    {
        const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* purchasedProductDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            purchasedProductDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedPurchasedProductInfo);
        if (purchasedProductDetails != nullptr && m_purchasedProductInfoIndex >= 0 && m_purchasedProductInfoIndex < purchasedProductDetails->size())
        {
            const PurchasedProductDetailsApple* purchasedProductsApple = azrtti_cast<const PurchasedProductDetailsApple*>(purchasedProductDetails->at(m_purchasedProductInfoIndex).get());
            if (purchasedProductsApple)
            {
                return purchasedProductsApple->GetSubscriptionExpirationTime();
            }
        }

        return 0;
    }

    AZ::u64 SystemComponent::GetRestoredPurchaseTime()
    {
        const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* purchasedProductDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            purchasedProductDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedPurchasedProductInfo);
        if (purchasedProductDetails != nullptr && m_purchasedProductInfoIndex >= 0 && m_purchasedProductInfoIndex < purchasedProductDetails->size())
        {
            const PurchasedProductDetailsApple* purchasedProductsApple = azrtti_cast<const PurchasedProductDetailsApple*>(purchasedProductDetails->at(m_purchasedProductInfoIndex).get());
            if (purchasedProductsApple)
            {
                return purchasedProductsApple->GetRestoredPurchaseTime();
            }
        }

        return 0;
    }

    bool SystemComponent::HasDownloads()
    {
        const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* purchasedProductDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            purchasedProductDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedPurchasedProductInfo);
        if (purchasedProductDetails != nullptr && m_purchasedProductInfoIndex >= 0 && m_purchasedProductInfoIndex < purchasedProductDetails->size())
        {
            const PurchasedProductDetailsApple* purchasedProductsApple = azrtti_cast<const PurchasedProductDetailsApple*>(purchasedProductDetails->at(m_purchasedProductInfoIndex).get());
            if (purchasedProductsApple)
            {
                return purchasedProductsApple->GetHasDownloads();
            }
        }

        return false;
    }

    bool SystemComponent::IsProductOwned()
    {
        const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* purchasedProductDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            purchasedProductDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedPurchasedProductInfo);

        const AZStd::vector<AZStd::unique_ptr<ProductDetails const> >* productDetails = nullptr;
        InAppPurchases::InAppPurchasesRequestBus::BroadcastResult(
            productDetails, &InAppPurchases::InAppPurchasesRequestBus::Events::GetCachedProductInfo);

        if (purchasedProductDetails != nullptr && productDetails != nullptr)
        {
            if (m_productInfoIndex >= 0 && m_productInfoIndex < productDetails->size())
            {
                const AZStd::string& productId = productDetails->at(m_productInfoIndex)->GetProductId();
                for (int i = 0; i < purchasedProductDetails->size(); i++)
                {
                    if (productId.compare(purchasedProductDetails->at(i)->GetProductId()) == 0)
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    void SystemComponent::ResetIndices()
    {
        m_productInfoIndex = 0;
        m_purchasedProductInfoIndex = 0;
    }
}
