/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "InAppPurchasesApple.h"

#include "InAppPurchasesModule.h"

#include <InAppPurchases/InAppPurchasesResponseBus.h>

#include <AzCore/JSON/document.h>

#include <openssl/pkcs7.h>
#include <openssl/objects.h>

#include <Payload.h>

namespace
{
    enum ReceiptAttributeTypes
    {
        InAppPurchaseReceipt = 17,
        ProductId = 1702,
        RestoredTransactionId = 1703,
        RestoredPurchaseTime = 1704,
        OriginalTransactionId = 1705,
        OriginalPurchaseTime = 1706,
        SubscriptionExpirationDate = 1708,
    };
    
    bool ParseReceipt(void* encodedPayload, size_t size, InAppPurchases::PurchasedProductDetailsApple* purchasedProductDetails = nullptr)
    {
        Payload_t* payload = nullptr;
        asn_DEF_Payload.ber_decoder(nullptr, &asn_DEF_Payload, (void**)&payload, encodedPayload, size, 0);
        
        if (!payload)
        {
            AZ_TracePrintf("O3DEInAppPurchases", "Payload is null!");
            return false;
        }
        
        for (int i = 0; i < payload->list.count; i++)
        {
            ReceiptAttribute_t* attrib = payload->list.array[i];
            switch (attrib->type)
            {
                case ReceiptAttributeTypes::InAppPurchaseReceipt:
                {
                    ParseReceipt(attrib->value.buf, attrib->value.size, new InAppPurchases::PurchasedProductDetailsApple());
                    break;
                }

                case ReceiptAttributeTypes::ProductId:
                {
                    purchasedProductDetails->SetProductId(AZStd::string(reinterpret_cast<const char*>(attrib->value.buf)).substr(2));
                    break;
                }

                case ReceiptAttributeTypes::RestoredTransactionId:
                {
                    purchasedProductDetails->SetRestoredOrderId(AZStd::string(reinterpret_cast<const char*>(attrib->value.buf)).substr(2));
                    break;
                }

                case ReceiptAttributeTypes::OriginalTransactionId:
                {
                    purchasedProductDetails->SetOrderId(AZStd::string(reinterpret_cast<const char*>(attrib->value.buf)).substr(2));
                    break;
                }

                case ReceiptAttributeTypes::RestoredPurchaseTime:
                case ReceiptAttributeTypes::OriginalPurchaseTime:
                {
                    AZStd::string dateString = AZStd::string(reinterpret_cast<const char*>(attrib->value.buf)).substr(2);
                    NSString* dateNSString = [NSString stringWithCString:dateString.c_str() encoding:NSUTF8StringEncoding];
                    NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
                    formatter.dateFormat = @"yyyy-MM-dd'T'HH:mm:ss'Z'";
                    [formatter setTimeZone:[NSTimeZone timeZoneWithName:@"GMT"]];
                    if (attrib->type == ReceiptAttributeTypes::RestoredPurchaseTime)
                    {
                        purchasedProductDetails->SetRestoredPurchaseTime([[formatter dateFromString:dateNSString] timeIntervalSince1970]);
                    }
                    else
                    {
                        purchasedProductDetails->SetPurchaseTime([[formatter dateFromString:dateNSString] timeIntervalSince1970]);
                    }
                    break;
                }

                case ReceiptAttributeTypes::SubscriptionExpirationDate:
                {
                    AZStd::string expiration = AZStd::string(reinterpret_cast<const char*>(attrib->value.buf));
                    // Products that are not subscriptions still have escape sequences. So we can't check for empty string.
                    if (expiration.size() <= 2)
                    {
                        purchasedProductDetails->SetSubscriptionExpirationTime(0);
                    }
                    else
                    {
                        NSString* expirationString = [NSString stringWithCString:expiration.substr(2).c_str() encoding:NSUTF8StringEncoding];
                        NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
                        formatter.dateFormat = @"yyyy-MM-dd'T'HH:mm:ss'Z'";
                        [formatter setTimeZone:[NSTimeZone timeZoneWithName:@"GMT"]];
                        purchasedProductDetails->SetSubscriptionExpirationTime([[formatter dateFromString:expirationString] timeIntervalSince1970]);
                    }
                    break;
                }
                    
                default:
                    break;
            }
        }
        
        if (purchasedProductDetails != nullptr)
        {
            purchasedProductDetails->SetDeveloperPayload("");
            purchasedProductDetails->SetPurchaseState(InAppPurchases::PurchaseState::PURCHASED);
            InAppPurchases::InAppPurchasesInterface::GetInstance()->GetCache()->AddPurchasedProductDetailsToCache(purchasedProductDetails);
        }
        
        return true;
    }
}

namespace InAppPurchases
{
    InAppPurchasesInterface* InAppPurchasesInterface::CreateInstance()
    {
        return new InAppPurchasesApple();
    }

    void InAppPurchasesApple::Initialize()
    {
        m_delegate = [[InAppPurchasesDelegate alloc] init];
        [m_delegate initialize];
    }

    InAppPurchasesApple::~InAppPurchasesApple()
    {
        [m_delegate deinitialize];
        [m_delegate release];
    }

    void InAppPurchasesApple::QueryProductInfo(AZStd::vector<AZStd::string>& productIds) const
    {        
        NSMutableArray* productIdStrings = [[NSMutableArray alloc] init];
        for (int i = 0; i < productIds.size(); i++)
        {
            NSString* productId = [NSString stringWithCString:productIds[i].c_str() encoding:NSUTF8StringEncoding];
            [productIdStrings addObject:productId];
        }
        
        [m_delegate requestProducts:productIdStrings];
        [productIdStrings release];
    }

    void InAppPurchasesApple::QueryProductInfo() const
    {
        NSURL* url = [[NSBundle mainBundle] URLForResource:@"product_ids" withExtension:@"plist"];
        if (url != nil)
        {
            NSMutableArray* productIds = [NSMutableArray arrayWithContentsOfURL:url];
            if (productIds != nil)
            {
                [m_delegate requestProducts:productIds];
            }
            else
            {
                AZ_TracePrintf("O3DEInAppPurchases", "Unable to find any product ids in product_ids.plist");
            }
        }
        else
        {
            AZ_TracePrintf("O3DEInAppPurchases", "product_ids.plist does not exist");
        }
    }

    void InAppPurchasesApple::PurchaseProduct(const AZStd::string& productId, const AZStd::string& developerPayload) const
    {
        NSString* productIdString = [NSString stringWithCString:productId.c_str() encoding:NSUTF8StringEncoding];
        if (!developerPayload.empty())
        {
            NSString* developerPayloadString = [NSString stringWithCString:developerPayload.c_str() encoding:NSUTF8StringEncoding];
            [m_delegate purchaseProduct:productIdString withUserName:developerPayloadString];
        }
        else
        {
            [m_delegate purchaseProduct:productIdString withUserName:nil];
        }
    }

    void InAppPurchasesApple::PurchaseProduct(const AZStd::string& productId) const
    {
        PurchaseProduct(productId, "");
    }

    void InAppPurchasesApple::RestorePurchasedProducts() const
    {
        InAppPurchasesInterface::GetInstance()->GetCache()->ClearCachedPurchasedProductDetails();
        [m_delegate restorePurchasedProducts];
    }
    
    void InAppPurchasesApple::QueryPurchasedProducts() const
    {
        [m_delegate refreshAppReceipt];
        InAppPurchasesInterface::GetInstance()->GetCache()->ClearCachedPurchasedProductDetails();
        NSURL* url = [[NSBundle mainBundle] appStoreReceiptURL];
        const char* receiptPath = [[url.absoluteString substringFromIndex:7] cStringUsingEncoding:NSASCIIStringEncoding];
        FILE* fp = fopen(receiptPath, "rb");
        if (!fp)
        {
            AZ_TracePrintf("O3DEInAppPurchases", "Unable to open receipt!");
            return;
        }
        
        PKCS7* p7 = d2i_PKCS7_fp(fp, nullptr);
        fclose(fp);
        if (!p7)
        {
            AZ_TracePrintf("O3DEInAppPurchases", "PKCS7 container is null!");
            return;
        }
        
        if (PKCS7_type_is_data(p7->d.sign->contents))
        {
            if (ParseReceipt(p7->d.sign->contents->d.data->data, p7->d.sign->contents->d.data->length))
            {
                EBUS_EVENT(InAppPurchasesResponseBus, PurchasedProductsRetrieved, InAppPurchasesInterface::GetInstance()->GetCache()->GetCachedPurchasedProductDetails());
            }
        }
    }
    
    void InAppPurchasesApple::ConsumePurchase(const AZStd::string& purchaseToken) const
    {
        
    }
    
    void InAppPurchasesApple::FinishTransaction(const AZStd::string& transactionId, bool downloadHostedContent) const
    {
        NSString* transactionIdString = [NSString stringWithCString:transactionId.c_str() encoding:NSASCIIStringEncoding];
        
        if (downloadHostedContent)
        {
            [m_delegate downloadAppleHostedContentAndFinishTransaction:transactionIdString];
        }
        else
        {
            [m_delegate finishTransaction:transactionIdString];
        }
    }

    InAppPurchasesCache* InAppPurchasesApple::GetCache()
    {
        return &m_cache;
    }
}
