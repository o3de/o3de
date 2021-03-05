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