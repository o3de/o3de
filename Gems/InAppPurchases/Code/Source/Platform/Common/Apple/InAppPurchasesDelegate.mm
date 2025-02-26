/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "InAppPurchasesDelegate.h"
#include "InAppPurchasesApple.h"
#include <InAppPurchases/InAppPurchasesInterface.h>
#include <InAppPurchases/InAppPurchasesResponseBus.h>

#include <CommonCrypto/CommonCrypto.h>

#include <AzCore/std/string/conversions.h>

@implementation InAppPurchasesDelegate

#if defined(CARBONATED)  // PR375
-(const char*) getStateName: (SKPaymentTransactionState) state
{
    switch (state)
    {
        case SKPaymentTransactionStatePurchasing: return "purchasing";
        case SKPaymentTransactionStatePurchased: return "purchased";
        case SKPaymentTransactionStateFailed: return "failed";
        case SKPaymentTransactionStateRestored: return "restored";
        case SKPaymentTransactionStateDeferred: return "deferred";
    }
    return "unknown";
}
#endif

-(NSString*) convertPriceToString:(NSDecimalNumber*) price forLocale:(NSLocale*) locale
{
    NSNumberFormatter* numberFormatter = [[NSNumberFormatter alloc] init];
    [numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
    [numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
    [numberFormatter setLocale:locale];
    NSString* formattedPrice = [numberFormatter stringFromNumber:price];
    [numberFormatter release];
    return formattedPrice;
}

-(InAppPurchases::PurchasedProductDetailsApple*) parseTransactionDetails:(SKPaymentTransaction*) transaction isRestored:(bool) restored
{
    InAppPurchases::PurchasedProductDetailsApple* purchasedProductDetails = new InAppPurchases::PurchasedProductDetailsApple();
    SKPayment* payment = transaction.payment;
    purchasedProductDetails->SetProductId([payment.productIdentifier UTF8String]);
    if ([payment.applicationUsername length] > 0)
    {
#if defined(CARBONATED)  // PR375
        AZ_Info("O3DEInAppPurchases", "SetDeveloperPayload %s", [payment.applicationUsername UTF8String]);
#endif
        purchasedProductDetails->SetDeveloperPayload([payment.applicationUsername UTF8String]);
    }
    else
    {
        purchasedProductDetails->SetDeveloperPayload("");
    }
    
#if defined(CARBONATED)  // PR375
    const auto& cachedProductDetails = InAppPurchases::InAppPurchasesInterface::GetInstance()->GetCache()->GetCachedProductDetails();
    for(const auto& productDetails : cachedProductDetails)
    {
        if(productDetails && productDetails->GetProductId() == purchasedProductDetails->GetProductId())
        {
            AZ_Info("O3DEInAppPurchases", "Set price %s", productDetails->GetProductPrice().c_str());
            
            purchasedProductDetails->SetProductPrice(productDetails->GetProductPrice());
            purchasedProductDetails->SetProductPriceMicro(productDetails->GetProductPriceMicro());
            purchasedProductDetails->SetProductCurrencyCode(productDetails->GetProductCurrencyCode());
            break;
        }
    }
#endif
    
    if (restored)
    {
        purchasedProductDetails->SetRestoredOrderId([transaction.transactionIdentifier cStringUsingEncoding:NSASCIIStringEncoding]);
        purchasedProductDetails->SetOrderId([transaction.originalTransaction.transactionIdentifier cStringUsingEncoding:NSASCIIStringEncoding]);
        purchasedProductDetails->SetPurchaseTime([transaction.originalTransaction.transactionDate timeIntervalSince1970]);
        purchasedProductDetails->SetRestoredPurchaseTime([transaction.transactionDate timeIntervalSince1970]);
    }
    else
    {
#if defined(CARBONATED)  // PR375
        // there is no order ID for a cancelled transaction
        if (transaction.transactionIdentifier != nil)
        {
            purchasedProductDetails->SetOrderId([transaction.transactionIdentifier cStringUsingEncoding:NSASCIIStringEncoding]);
        }
        else
        {
            AZ_Info("O3DEInAppPurchases", "No transaction id");
            purchasedProductDetails->SetOrderId("");
        }
#else
        purchasedProductDetails->SetOrderId([transaction.transactionIdentifier cStringUsingEncoding:NSASCIIStringEncoding]);
#endif
        purchasedProductDetails->SetPurchaseTime([transaction.transactionDate timeIntervalSince1970]);
        purchasedProductDetails->SetRestoredOrderId("");
        purchasedProductDetails->SetRestoredPurchaseTime(0);
    }

    purchasedProductDetails->SetHasDownloads((transaction.downloads != nil) ? true:false);
    
    return purchasedProductDetails;
}

-(NSString*) hashUserName:(NSString*) userName
{
    const int HASH_SIZE = 32;
    unsigned char hashedChars[HASH_SIZE];
    const char* userNameString = [userName UTF8String];
    size_t userNameLength = strlen(userNameString);
    
    if (userNameLength > UINT32_MAX)
    {
        AZ_TracePrintf("O3DEInAppPurchases", "Username too long to hash:%s", [userName cStringUsingEncoding:NSASCIIStringEncoding]);
        return nil;
    }
    
    CC_SHA256(userNameString, (CC_LONG)userNameLength, hashedChars);
    
    NSMutableString* userNameHash = [[NSMutableString alloc] init];
    
    for (int i =0; i < HASH_SIZE; i++)
    {
        if (i != 0 && i % 4 == 0)
        {
            [userNameHash appendString:@"-"];
        }
        
        [userNameHash appendFormat:@"%02x", hashedChars[i]];
    }
    
    return userNameHash;
}

-(void) initialize
{
    self.m_products = [[NSMutableArray alloc] init];
    self.m_unfinishedTransactions = [[NSMutableArray alloc] init];
    self.m_unfinishedDownloads = [[NSMutableArray alloc] init];
    [[SKPaymentQueue defaultQueue] addTransactionObserver:self];
}

-(void) deinitialize
{
    [self.m_products removeAllObjects];
    [self.m_products release];
    
    [self.m_unfinishedTransactions removeAllObjects];
    [self.m_unfinishedTransactions release];
    
    [self.m_unfinishedDownloads removeAllObjects];
    [self.m_unfinishedDownloads release];
    
    if (self.m_productsRequest != nil)
    {
        [self.m_productsRequest release];
    }
    
    if (self.m_receiptRefreshRequest != nil)
    {
        [self.m_receiptRefreshRequest release];
    }
}

-(void) requestProducts:(NSMutableArray*) productIds
{
    self.m_productsRequest = [[SKProductsRequest alloc] initWithProductIdentifiers:[NSSet setWithArray:productIds]];
    self.m_productsRequest.delegate = self;
    [self.m_productsRequest start];
}

-(void) productsRequest:(SKProductsRequest*) request didReceiveResponse:(SKProductsResponse*) response
{
    for ([[maybe_unused]] NSString* invalidId in response.invalidProductIdentifiers)
    {
        AZ_TracePrintf("O3DEInAppPurchases:", "Invalid product ID:", [invalidId cStringUsingEncoding:NSASCIIStringEncoding]);
    }
    
    InAppPurchases::InAppPurchasesInterface::GetInstance()->GetCache()->ClearCachedProductDetails();
    [self.m_products removeAllObjects];
    
    for (SKProduct* product in response.products)
    {
        InAppPurchases::ProductDetailsApple* productDetails = new InAppPurchases::ProductDetailsApple;
      
        productDetails->SetProductId([product.productIdentifier UTF8String]);
        productDetails->SetProductTitle([product.localizedTitle UTF8String]);
        
#if defined(CARBONATED)  // PR375
        const AZStd::string description = product.localizedDescription ? [product.localizedDescription UTF8String] : "(no desc)";
        productDetails->SetProductDescription(description);
#else
        productDetails->SetProductDescription([product.localizedDescription UTF8String]);
#endif
        productDetails->SetProductPrice([[self convertPriceToString:product.price forLocale:product.priceLocale] UTF8String]);
        productDetails->SetProductCurrencyCode([[product.priceLocale objectForKey:NSLocaleCurrencyCode] UTF8String]);
        AZStd::string priceMicro = [[NSString stringWithFormat:@"%@", product.price] UTF8String];
        productDetails->SetProductPriceMicro((AZStd::stof(priceMicro) * 1000000));
        
        InAppPurchases::InAppPurchasesInterface::GetInstance()->GetCache()->AddProductDetailsToCache(productDetails);
        [self.m_products addObject:product];
    }
    
    EBUS_EVENT(InAppPurchases::InAppPurchasesResponseBus, ProductInfoRetrieved, InAppPurchases::InAppPurchasesInterface::GetInstance()->GetCache()->GetCachedProductDetails());
    
    [self.m_productsRequest release];
    self.m_productsRequest = nil;
}

-(void) purchaseProduct:(NSString *) productId withUserName:(NSString *) userName
{
    SKProduct* productToPurchase = nil;
    for (SKProduct* product in self.m_products)
    {
        if ([product.productIdentifier isEqualToString:productId])
        {
            productToPurchase = product;
            break;
        }
    }
    
    if (productToPurchase != nil)
    {
        SKMutablePayment* payment = [SKMutablePayment paymentWithProduct:productToPurchase];
        payment.quantity = 1;
        
        if ([userName length] > 0)
        {
            NSString* userNameHash = [self hashUserName:userName];
            payment.applicationUsername = userNameHash;
        }
        [[SKPaymentQueue defaultQueue] addPayment:payment];
    }
    else
    {
        AZ_TracePrintf("O3DEInAppPurchases", "Invalid product ID:%s", [productId cStringUsingEncoding:NSASCIIStringEncoding]);
    }
}

-(void) paymentQueue:(SKPaymentQueue*) queue updatedTransactions:(nonnull NSArray*) transactions
{
    bool isRestoredTransaction = false;
    
    for (SKPaymentTransaction* transaction in transactions)
    {
        switch (transaction.transactionState)
        {
            case SKPaymentTransactionStatePurchasing:
            {
                AZ_TracePrintf("O3DEInAppPurchases", "Transaction in progress");
                break;
            }
                
            case SKPaymentTransactionStateDeferred:
            {
                AZ_TracePrintf("O3DEInAppPurchases", "Transaction deferred");
                break;
            }
                
            case SKPaymentTransactionStateFailed:
            {
#if defined(CARBONATED)  // PR375
                AZ_TracePrintf("O3DEInAppPurchases", "Transaction failed");
#endif
                if ([self.m_unfinishedTransactions containsObject:transaction] == false)
                {
                    AZ_TracePrintf("O3DEInAppPurchases", "Transaction failed! Error: %s", [[transaction.error localizedDescription] cStringUsingEncoding:NSASCIIStringEncoding]);
                    InAppPurchases::PurchasedProductDetailsApple* productDetails = [self parseTransactionDetails:transaction isRestored:false];
#if defined(CARBONATED)  // PR375
                    if(transaction.error.code == SKErrorPaymentCancelled)
                    {
                        AZ_TracePrintf("O3DEInAppPurchases", "Transaction payment cancelled");
                        productDetails->SetPurchaseState(InAppPurchases::PurchaseState::CANCELLED);
                        EBUS_EVENT(InAppPurchases::InAppPurchasesResponseBus, PurchaseCancelled, productDetails);
                    }
                    else
                    {
                        productDetails->SetPurchaseState(InAppPurchases::PurchaseState::FAILED);
                        [self.m_unfinishedTransactions addObject:transaction];
                        AZ_TracePrintf("O3DEInAppPurchases", "SKError = %d", transaction.error.code);
                        EBUS_EVENT(InAppPurchases::InAppPurchasesResponseBus, PurchaseFailed, productDetails);
                    }
#else
                    productDetails->SetPurchaseState(InAppPurchases::PurchaseState::FAILED);
#endif
                    
#if defined(CARBONATED)  // PR375
                    InAppPurchases::InAppPurchasesInterface::GetInstance()->GetCache()->AddPurchasedProductDetailsToCache(productDetails);
#endif
                    
#if defined(CARBONATED)  // PR375
                    // ..
#else
                    delete productDetails;
#endif
                }
#if defined(CARBONATED)  // PR375
                else
                {
                    AZ_TracePrintf("O3DEInAppPurchases", "There is an unfinished transaction with %s, state %s, so do nothing",
                                   [transaction.transactionIdentifier UTF8String], [self getStateName:transaction.transactionState]);
                }
#endif
            }
                break;
                
            case SKPaymentTransactionStatePurchased:
            {
#if defined(CARBONATED)  // PR375
                AZ_TracePrintf("O3DEInAppPurchases", "Transaction purchased");
#endif
                if ([self.m_unfinishedTransactions containsObject:transaction] == false)
                {
                    AZ_TracePrintf("O3DEInAppPurchases", "Transaction succeeded");
                    InAppPurchases::PurchasedProductDetailsApple* productDetails = [self parseTransactionDetails:transaction isRestored:false];
                    productDetails->SetPurchaseState(InAppPurchases::PurchaseState::PURCHASED);
                    InAppPurchases::InAppPurchasesInterface::GetInstance()->GetCache()->AddPurchasedProductDetailsToCache(productDetails);
                    [self.m_unfinishedTransactions addObject:transaction];
                    for (SKDownload* download in transaction.downloads)
                    {
                        if ([self.m_unfinishedDownloads containsObject:download.contentIdentifier] == false)
                        {
                            [self.m_unfinishedDownloads addObject:download.contentIdentifier];
                        }
                    }
                    EBUS_EVENT(InAppPurchases::InAppPurchasesResponseBus, NewProductPurchased, productDetails);
                }
#if defined(CARBONATED)  // PR375
                else
                {
                    AZ_TracePrintf("O3DEInAppPurchases", "There is an unfinished transaction with %s, state %s, so do nothing",
                                   [transaction.transactionIdentifier UTF8String], [self getStateName:transaction.transactionState]);
                }
#endif
            }
                break;
                
            case SKPaymentTransactionStateRestored:
            {
#if defined(CARBONATED)  // PR375
                AZ_TracePrintf("O3DEInAppPurchases", "Transaction restored");
#endif
                if ([self.m_unfinishedTransactions containsObject:transaction] == false)
                {
                    AZ_TracePrintf("O3DEInAppPurchases", "Transaction restored");
                    InAppPurchases::PurchasedProductDetailsApple* productDetails = [self parseTransactionDetails:transaction isRestored:true];
                    productDetails->SetPurchaseState(InAppPurchases::PurchaseState::RESTORED);
                    InAppPurchases::InAppPurchasesInterface::GetInstance()->GetCache()->AddPurchasedProductDetailsToCache(productDetails);
                    [self.m_unfinishedTransactions addObject:transaction];
                    for (SKDownload* download in transaction.downloads)
                    {
                        if ([self.m_unfinishedDownloads containsObject:download.contentIdentifier] == false)
                        {
                            [self.m_unfinishedDownloads addObject:download.contentIdentifier];
                        }
                    }
                    isRestoredTransaction = true;
                }
#if defined(CARBONATED)  // PR375
                else
                {
                    AZ_TracePrintf("O3DEInAppPurchases", "There is an unfinished transaction with %s, state %s, so do nothing",
                                   [transaction.transactionIdentifier UTF8String], [self getStateName:transaction.transactionState]);
                }
#endif
            }
                break;
            default:
            {
                AZ_TracePrintf("O3DEInAppPurchases", "Transaction unknown");
            }
                break;
        }
    }
    
    if (isRestoredTransaction)
    {
        EBUS_EVENT(InAppPurchases::InAppPurchasesResponseBus, PurchasedProductsRestored, InAppPurchases::InAppPurchasesInterface::GetInstance()->GetCache()->GetCachedPurchasedProductDetails());
    }
}

-(void) finishTransaction:(NSString*) transactionId
{
    for (SKPaymentTransaction* transaction in self.m_unfinishedTransactions)
    {
        if ([transaction.transactionIdentifier isEqualToString:transactionId])
        {
            [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
            [self.m_unfinishedTransactions removeObject:transaction];
            
#if defined(CARBONATED)  // PR375
            AZStd::string orderId = [transactionId UTF8String];
            InAppPurchases::InAppPurchasesInterface::GetInstance()->GetCache()->RemovePurchasedProductDetails(orderId);
#endif
            
            return;
        }
    }

#if defined(CARBONATED)  // PR375
    AZ_Error("O3DEInAppPurchases", false, "No unfinished transaction found with ID: %s", [transactionId cStringUsingEncoding:NSASCIIStringEncoding]);
#else
    AZ_TracePrintf("O3DEInAppPurchases", "No unfinished transaction found with ID: %s", [transactionId cStringUsingEncoding:NSASCIIStringEncoding]);
#endif
}

-(void) downloadAppleHostedContentAndFinishTransaction:(NSString*) transactionId
{
    SKPaymentTransaction* requiredTransaction = nil;
    for (SKPaymentTransaction* transaction in self.m_unfinishedTransactions)
    {
        if ([transaction.transactionIdentifier isEqualToString:transactionId])
        {
            requiredTransaction = transaction;
            break;
        }
    }
    
    if (requiredTransaction != nil)
    {
        if (requiredTransaction.downloads != nil)
        {
            [[SKPaymentQueue defaultQueue] startDownloads:requiredTransaction.downloads];
        }
        else
        {
            [self finishTransaction:requiredTransaction.transactionIdentifier];
        }
    }
    else
    {
#if defined(CARBONATED)  // PR375
        AZ_Error("O3DEInAppPurchases", false, "No unfinished transaction found with ID: %s", [transactionId cStringUsingEncoding:NSASCIIStringEncoding]);
#else
        AZ_TracePrintf("O3DEInAppPurchases", "No unfinished transaction found with ID: %s", [transactionId cStringUsingEncoding:NSASCIIStringEncoding]);
#endif
    }
}

-(void) paymentQueue:(SKPaymentQueue*) queue updatedDownloads:(NSArray<SKDownload*>*) downloads
{
    for (SKDownload* download in downloads)
    {
        bool wasDownloadHandled = true;
        for (NSString* contentIdentifier in self.m_unfinishedDownloads)
        {
            if ([contentIdentifier isEqualToString:download.contentIdentifier])
            {
                wasDownloadHandled = false;
                break;
            }
        }
        
        if (wasDownloadHandled)
        {
            continue;
        }

#if (defined(TARGET_OS_OSX) && TARGET_OS_OSX) || (defined(__IPHONE_12_0) && __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_12_0)
        switch(download.state)
#else
        switch(download.downloadState)
#endif    
        {
            case SKDownloadStateFinished:
            {
                AZStd::string pathToDownloadedContent = [download.contentURL.absoluteString cStringUsingEncoding:NSASCIIStringEncoding];
                AZStd::string transactionId = [download.transaction.transactionIdentifier cStringUsingEncoding:NSASCIIStringEncoding];
                EBUS_EVENT(InAppPurchases::InAppPurchasesResponseBus, HostedContentDownloadComplete, transactionId, pathToDownloadedContent);
                [self.m_unfinishedDownloads removeObject:download.contentIdentifier];
            }
                break;
                
            case SKDownloadStateFailed:
            {
                AZ_TracePrintf("O3DEInAppPurchases", "Download failed with error: %s", [[download.error localizedDescription] cStringUsingEncoding:NSASCIIStringEncoding]);
                AZStd::string transactionId = [download.transaction.transactionIdentifier cStringUsingEncoding:NSASCIIStringEncoding];
                AZStd::string contentId = [download.contentIdentifier UTF8String];
                EBUS_EVENT(InAppPurchases::InAppPurchasesResponseBus, HostedContentDownloadFailed, transactionId, contentId);
                [self.m_unfinishedDownloads removeObject:download.contentIdentifier];
            }
                break;
                
            case SKDownloadStateCancelled:
            {
                AZ_TracePrintf("O3DEInAppPurchases", "Download cancelled: %s", [download.contentIdentifier cStringUsingEncoding:NSASCIIStringEncoding]);
                [self.m_unfinishedDownloads removeObject:download.contentIdentifier];
            }
                break;
                
            case SKDownloadStateActive:
            case SKDownloadStateWaiting:
            case SKDownloadStatePaused:
                break;
        }
    }
    
    if ([self.m_unfinishedDownloads count] == 0)
    {
        [self finishTransaction:[downloads objectAtIndex:0].transaction.transactionIdentifier];
    }
}

-(void) restorePurchasedProducts
{
    [[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
}

-(void) refreshAppReceipt
{
    self.m_receiptRefreshRequest = [[SKReceiptRefreshRequest alloc] init];
    self.m_receiptRefreshRequest.delegate = self;
    [self.m_receiptRefreshRequest start];
}

-(void) request:(SKRequest*) request didFailWithError:(NSError*) error
{
    AZ_TracePrintf("O3DEInAppPurchases", "Request failed with error: %s", [[error localizedDescription] cStringUsingEncoding:NSASCIIStringEncoding]);
}

-(void) requestDidFinish:(SKRequest *)request
{
    if ([request isKindOfClass:[SKReceiptRefreshRequest class]])
    {
        [self.m_receiptRefreshRequest release];
        self.m_receiptRefreshRequest = nil;
    }
}

@end
