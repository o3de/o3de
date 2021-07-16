/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#import <StoreKit/StoreKit.h>

@interface InAppPurchasesDelegate : NSObject<SKProductsRequestDelegate, SKPaymentTransactionObserver>
@property (strong, nonatomic) SKProductsRequest* m_productsRequest;
@property (strong, nonatomic) NSMutableArray* m_products;
@property (strong, nonatomic) NSMutableArray* m_unfinishedTransactions;
@property (strong, nonatomic) NSMutableArray* m_unfinishedDownloads;
@property (strong, nonatomic) SKReceiptRefreshRequest* m_receiptRefreshRequest;
-(void) requestProducts:(NSMutableArray*) productIds;
-(void) purchaseProduct:(NSString*) productId withUserName:(NSString*) userName;
-(void) finishTransaction:(NSString*) transactionId;
-(void) downloadAppleHostedContentAndFinishTransaction:(NSString*) transactionId;
-(void) restorePurchasedProducts;
-(void) refreshAppReceipt;
-(void) initialize;
-(void) deinitialize;
@end
