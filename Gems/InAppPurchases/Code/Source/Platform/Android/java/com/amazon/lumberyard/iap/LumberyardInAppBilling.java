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

package com.amazon.lumberyard.iap;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.IntentSender.SendIntentException;
import android.content.res.Resources;
import android.os.Build;
import android.os.Bundle;
import android.os.Looper;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClientStateListener;
import com.android.billingclient.api.BillingFlowParams;
import com.android.billingclient.api.ConsumeResponseListener;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.PurchasesUpdatedListener;
import com.android.billingclient.api.SkuDetails;
import com.android.billingclient.api.SkuDetailsParams;
import com.android.billingclient.api.SkuDetailsResponseListener;


public class LumberyardInAppBilling implements PurchasesUpdatedListener
{
    public LumberyardInAppBilling(Activity activity)
    {
        m_activity = activity;

        m_packageName = m_activity.getPackageName();

        Resources resources = m_activity.getResources();
        int stringId = resources.getIdentifier("public_key", "string", m_activity.getPackageName());
        m_appPublicKey = resources.getString(stringId);

        m_setupDone = false;

        final LumberyardInAppBilling iapInstance = this;

        if (!IsKindleDevice())
        {
            (new Thread(new Runnable()
            {
                public void run()
                {
                    Looper.prepare();
                    m_billingClient = BillingClient.newBuilder(m_activity).setListener(iapInstance).build();
                    m_billingClient.startConnection(new BillingClientStateListener()
                    {
                        @Override
                        public void onBillingSetupFinished(int responseCode)
                        {
                            Log.d(s_tag, "Service connected");
                    
                            if (!m_billingClient.isReady())
                            {
                                Log.d(s_tag, m_packageName + " IN_APP items not supported!");
                                return;
                            }
                    
                            int subscriptionsResponseCode = m_billingClient.isFeatureSupported(BillingClient.FeatureType.SUBSCRIPTIONS);
                            if (!VerifyResponseCode(subscriptionsResponseCode))
                            {
                                return;
                            }
                    
                            subscriptionsResponseCode = m_billingClient.isFeatureSupported(BillingClient.FeatureType.SUBSCRIPTIONS_UPDATE);
                            if (!VerifyResponseCode(subscriptionsResponseCode))
                            {
                                return;
                            }
                    
                            m_setupDone = true;
                        }
                    
                        @Override
                        public void onBillingServiceDisconnected()
                        {
                            Log.d(s_tag, "Service disconnected");
                        }
                    });
                    Looper.loop();
                }
            })).start();
        }

        Log.d(s_tag, "Instance created");
    }


    public static native void nativeProductInfoRetrieved(Object[] productDetails);
    public static native void nativePurchasedProductsRetrieved(Object[] purchasedProductDetails);
    public static native void nativeNewProductPurchased(Object[] purchaseReceipt);
    public static native void nativePurchaseConsumed(String purchaseToken);

    public void UnbindService()
    {
        if (!m_setupDone)
        {
            Log.e(s_tag, "Not initialized!");
            return;
        }

        m_setupDone = false;

        m_billingClient.endConnection();
        m_billingClient = null;
    }


    public void QueryProductInfo(final String[] skuListArray)
    {
        if (!m_setupDone)
        {
            Log.e(s_tag, "Not initialized!");
            return;
        }

        m_numResponses = 0;
        final ArrayList<ProductDetails> responseList = new ArrayList<>();

        List<String> skuList = Arrays.asList(skuListArray);

        SkuDetailsResponseListener responseListener = new SkuDetailsResponseListener()
        {
            @Override
            public void onSkuDetailsResponse(int responseCode, List<SkuDetails> skuDetailsList)
            {
                m_numResponses++;
                if (!VerifyResponseCode(responseCode))
                {
                    return;
                }

                for (SkuDetails skuDetails : skuDetailsList)
                {
                    ProductDetails productDetails = new ProductDetails();
                    productDetails.m_productId = skuDetails.getSku();
                    productDetails.m_title = skuDetails.getTitle();
                    productDetails.m_description = skuDetails.getDescription();
                    productDetails.m_price = skuDetails.getPrice();
                    productDetails.m_currencyCode = skuDetails.getPriceCurrencyCode();
                    productDetails.m_type = skuDetails.getType();
                    productDetails.m_priceMicro = skuDetails.getPriceAmountMicros();
                    responseList.add(productDetails);
                }

                // Wait for responses for both subscriptions and regular products
                if (m_numResponses == 2)
                {
                    nativeProductInfoRetrieved(responseList.toArray());
                }
            }
        };

        SkuDetailsParams.Builder params = SkuDetailsParams.newBuilder();
        params.setSkusList(skuList).setType(BillingClient.SkuType.INAPP);
        m_billingClient.querySkuDetailsAsync(params.build(), responseListener);
        params.setSkusList(skuList).setType(BillingClient.SkuType.SUBS);
        m_billingClient.querySkuDetailsAsync(params.build(), responseListener);
    }


    public void PurchaseProduct(String productSku, String developerPayload, String productType)
    {
        if (!m_setupDone)
        {
            Log.e(s_tag, "Not initialized!");
            return;
        }

        BillingFlowParams flowParams = BillingFlowParams.newBuilder().setSku(productSku).setType(productType).build();
        int responseCode = m_billingClient.launchBillingFlow(m_activity, flowParams);

        if (!VerifyResponseCode(responseCode))
        {
            return;
        }

        Log.d(s_tag, "Purchase flow initiated.");
    }


    @Override
    public void onPurchasesUpdated(int responseCode, List<Purchase> purchases)
    {
        if (!VerifyResponseCode(responseCode))
        {
            return;
        }

        ArrayList<PurchasedProductDetails> purchasedProducts = new ArrayList<>();

        ParsePurchasedProducts(purchases, purchasedProducts);

        nativeNewProductPurchased(purchasedProducts.toArray());
    }


    public void QueryPurchasedProducts()
    {
        if (!m_setupDone)
        {
            Log.e(s_tag, "Not initialized!");
            return;
        }

        ArrayList<PurchasedProductDetails> purchasedProducts = new ArrayList<>();

        if (!QueryPurchasedProductsBySkuType(BillingClient.SkuType.INAPP, purchasedProducts) || !QueryPurchasedProductsBySkuType(BillingClient.SkuType.SUBS, purchasedProducts))
        {
            return;
        }

        nativePurchasedProductsRetrieved(purchasedProducts.toArray());
    }


    public void ConsumePurchase(final String purchaseToken)
    {
        if (!m_setupDone)
        {
            Log.e(s_tag, "Not initialized!");
            return;
        }

        ConsumeResponseListener listener = new ConsumeResponseListener()
        {
            @Override
            public void onConsumeResponse(int responseCode, String purchaseToken)
            {
                if (!VerifyResponseCode(responseCode))
                {
                    return;
                }

                nativePurchaseConsumed(purchaseToken);
            }
        };

        m_billingClient.consumeAsync(purchaseToken, listener);
    }


    public boolean IsKindleDevice()
    {
        if (Build.MANUFACTURER.equals("Amazon") && Build.MODEL.contains("KF"))
        {
            Log.e(s_tag, "Kindle devices not currently supported");
            return true;
        }
        return false;
    }


    private boolean QueryPurchasedProductsBySkuType(String skuType, ArrayList<PurchasedProductDetails> purchasedProducts)
    {
        Purchase.PurchasesResult purchasesResult = m_billingClient.queryPurchases(skuType);

        if (!VerifyResponseCode(purchasesResult.getResponseCode()))
        {
            return false;
        }

        ParsePurchasedProducts(purchasesResult.getPurchasesList(), purchasedProducts);

        return true;
    }


    private void ParsePurchasedProducts(List<Purchase> purchases, ArrayList<PurchasedProductDetails> purchasedProducts)
    {
        for (Purchase purchase : purchases)
        {
            PurchasedProductDetails purchasedProductDetails = new PurchasedProductDetails();
            purchasedProductDetails.m_productId = purchase.getSku();
            purchasedProductDetails.m_orderId = purchase.getOrderId();
            purchasedProductDetails.m_packageName = purchase.getPackageName();
            purchasedProductDetails.m_purchaseToken = purchase.getPurchaseToken();
            purchasedProductDetails.m_signature = purchase.getSignature();
            purchasedProductDetails.m_purchaseTime = purchase.getPurchaseTime();
            purchasedProductDetails.m_isAutoRenewing = purchase.isAutoRenewing();
            purchasedProducts.add(purchasedProductDetails);
        }
    }


    private boolean VerifyResponseCode(int responseCode)
    {
        if (responseCode != BillingClient.BillingResponse.OK)
        {
            Log.d(s_tag, m_packageName + " returned error code " + BILLING_RESPONSE_RESULT_STRINGS[responseCode]);
            return false;
        }

        return true;
    }


    public class ProductDetails
    {
        public String m_title;
        public String m_description;
        public String m_productId;
        public String m_price;
        public String m_currencyCode;
        public String m_type;
        public long m_priceMicro;
    };

    public class PurchasedProductDetails
    {
        public String m_productId;
        public String m_orderId;
        public String m_packageName;
        public String m_purchaseToken;
        public String m_signature;
        public long m_purchaseTime;
        public boolean m_isAutoRenewing;
    };

    private static final String[] BILLING_RESPONSE_RESULT_STRINGS = {
        "Billing response result is ok",
        "User cancelled the request",
        "The requested service is unavailable",
        "Billing is currently unavailable",
        "The requested item is unavailable",
        "Developer error",
        "Error",
        "The item being purchased is already owned by the user",
        "The item is not owned by the user"
    };

    private static final String s_tag = "LumberyardInAppBilling";

    private Activity m_activity;

    private BillingClient m_billingClient;

    private String m_appPublicKey;

    private String m_packageName;

    private boolean m_setupDone;

    private int m_numResponses;
}