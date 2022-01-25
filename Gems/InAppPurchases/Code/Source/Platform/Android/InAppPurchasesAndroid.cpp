/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "InAppPurchasesAndroid.h"

#include "InAppPurchasesModule.h"

#include <InAppPurchases/InAppPurchasesResponseBus.h>
#include <AzCore/NativeUI/NativeUIRequests.h>

#include <AzCore/Android/JNI/JNI.h>
#include <AzCore/Android/Utils.h>
#include <AzCore/EBus/BusImpl.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/error/en.h>
#include <AzCore/IO/FileIO.h>


namespace InAppPurchases
{
    static bool IsFieldIdValid(jfieldID fid)
    {
        if (fid == NULL)
        {
            return false;
        }

        return true;
    }

    static PurchasedProductDetailsAndroid* ParseReceiptDetails(JNIEnv* env, jobjectArray jpurchasedProductDetails, int index)
    {
        jobject jpurchasedProduct = env->GetObjectArrayElement(jpurchasedProductDetails, index);

        const int NUM_FIELDS_PURCHASED_PRODUCTS = 7;
        jfieldID fid[NUM_FIELDS_PURCHASED_PRODUCTS];
        jclass cls = env->GetObjectClass(jpurchasedProduct);
        fid[0] = env->GetFieldID(cls, "m_productId", "Ljava/lang/String;");
        fid[1] = env->GetFieldID(cls, "m_orderId", "Ljava/lang/String;");
        fid[2] = env->GetFieldID(cls, "m_packageName", "Ljava/lang/String;");
        fid[3] = env->GetFieldID(cls, "m_purchaseToken", "Ljava/lang/String;");
        fid[4] = env->GetFieldID(cls, "m_signature", "Ljava/lang/String;");
        fid[5] = env->GetFieldID(cls, "m_purchaseTime", "J");
        fid[6] = env->GetFieldID(cls, "m_isAutoRenewing", "Z");

        for (int i = 0; i < NUM_FIELDS_PURCHASED_PRODUCTS; i++)
        {
            if (!IsFieldIdValid(fid[i]))
            {
                AZ_TracePrintf("LumberyardInAppBilling", "Invaild FieldId in PurchasedProductDetails\n");
                return nullptr;
            }
        }

        PurchasedProductDetailsAndroid* purchasedProductDetails = new PurchasedProductDetailsAndroid();

        purchasedProductDetails->SetProductId(AZ::Android::JNI::ConvertJstringToString(static_cast<jstring>(env->GetObjectField(jpurchasedProduct, fid[0]))));
        purchasedProductDetails->SetOrderId(AZ::Android::JNI::ConvertJstringToString(static_cast<jstring>(env->GetObjectField(jpurchasedProduct, fid[1]))));
        purchasedProductDetails->SetPackageName(AZ::Android::JNI::ConvertJstringToString(static_cast<jstring>(env->GetObjectField(jpurchasedProduct, fid[2]))));
        purchasedProductDetails->SetPurchaseToken(AZ::Android::JNI::ConvertJstringToString(static_cast<jstring>(env->GetObjectField(jpurchasedProduct, fid[3]))));
        purchasedProductDetails->SetPurchaseSignature(AZ::Android::JNI::ConvertJstringToString(static_cast<jstring>(env->GetObjectField(jpurchasedProduct, fid[4]))));
        purchasedProductDetails->SetPurchaseTime(env->GetLongField(jpurchasedProduct, fid[5]));
        purchasedProductDetails->SetIsAutoRenewing(env->GetBooleanField(jpurchasedProduct, fid[6]));

        return purchasedProductDetails;
    }

    void ProductInfoRetrieved(JNIEnv* env, jobject obj, jobjectArray jproductDetails)
    {
        int numProducts = env->GetArrayLength(jproductDetails);

        InAppPurchasesInterface::GetInstance()->GetCache()->ClearCachedProductDetails();

        const int NUM_FIELDS_PRODUCTS = 7;
        jfieldID fid[NUM_FIELDS_PRODUCTS];
        jclass cls;
        if (numProducts > 0)
        {
            cls = env->GetObjectClass(env->GetObjectArrayElement(jproductDetails, 0));
            fid[0] = env->GetFieldID(cls, "m_productId", "Ljava/lang/String;");
            fid[1] = env->GetFieldID(cls, "m_type", "Ljava/lang/String;");
            fid[2] = env->GetFieldID(cls, "m_price", "Ljava/lang/String;");
            fid[3] = env->GetFieldID(cls, "m_currencyCode", "Ljava/lang/String;");
            fid[4] = env->GetFieldID(cls, "m_title", "Ljava/lang/String;");
            fid[5] = env->GetFieldID(cls, "m_description", "Ljava/lang/String;");
            fid[6] = env->GetFieldID(cls, "m_priceMicro", "J");
        }

        for (int i = 0; i < NUM_FIELDS_PRODUCTS; i++)
        {
            if (!IsFieldIdValid(fid[i]))
            {
                AZ_TracePrintf("LumberyardInAppBilling", "Invaild FieldId in ProductDetails\n");
                return;
            }
        }

        for (int i = 0; i < numProducts; i++)
        {
            jobject jproduct = env->GetObjectArrayElement(jproductDetails, i);

            ProductDetailsAndroid* productDetails = new ProductDetailsAndroid();

            productDetails->SetProductId(AZ::Android::JNI::ConvertJstringToString(static_cast<jstring>(env->GetObjectField(jproduct, fid[0]))));
            productDetails->SetProductType(AZ::Android::JNI::ConvertJstringToString(static_cast<jstring>(env->GetObjectField(jproduct, fid[1]))));
            productDetails->SetProductPrice(AZ::Android::JNI::ConvertJstringToString(static_cast<jstring>(env->GetObjectField(jproduct, fid[2]))));
            productDetails->SetProductCurrencyCode(AZ::Android::JNI::ConvertJstringToString(static_cast<jstring>(env->GetObjectField(jproduct, fid[3]))));
            productDetails->SetProductTitle(AZ::Android::JNI::ConvertJstringToString(static_cast<jstring>(env->GetObjectField(jproduct, fid[4]))));
            productDetails->SetProductDescription(AZ::Android::JNI::ConvertJstringToString(static_cast<jstring>(env->GetObjectField(jproduct, fid[5]))));
            productDetails->SetProductPriceMicro(env->GetLongField(jproduct, fid[6]));
            InAppPurchasesInterface::GetInstance()->GetCache()->AddProductDetailsToCache(productDetails);
        }
        EBUS_EVENT(InAppPurchasesResponseBus, ProductInfoRetrieved, InAppPurchasesInterface::GetInstance()->GetCache()->GetCachedProductDetails());
    }

    void PurchasedProductsRetrieved(JNIEnv* env, jobject object, jobjectArray jpurchasedProductDetails)
    {
        InAppPurchasesInterface::GetInstance()->GetCache()->ClearCachedPurchasedProductDetails();
        int numPurchasedProducts = env->GetArrayLength(jpurchasedProductDetails);
        for (int i = 0; i < numPurchasedProducts; i++)
        {
            PurchasedProductDetailsAndroid* purchasedProduct = ParseReceiptDetails(env, jpurchasedProductDetails, i);
            if (purchasedProduct != nullptr)
            {
                InAppPurchasesInterface::GetInstance()->GetCache()->AddPurchasedProductDetailsToCache(purchasedProduct);
            }
        }
        EBUS_EVENT(InAppPurchasesResponseBus, PurchasedProductsRetrieved, InAppPurchasesInterface::GetInstance()->GetCache()->GetCachedPurchasedProductDetails());
    }

    void NewProductPurchased(JNIEnv* env, jobject object, jobjectArray jpurchaseReceipt)
    {
        PurchasedProductDetailsAndroid* purchasedProduct = ParseReceiptDetails(env, jpurchaseReceipt, 0);

        if (purchasedProduct != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->GetCache()->AddPurchasedProductDetailsToCache(purchasedProduct);
            EBUS_EVENT(InAppPurchasesResponseBus, NewProductPurchased, purchasedProduct);
        }
    }

    void PurchaseConsumed(JNIEnv* env, jobject object, jstring jpurchaseToken)
    {
        const char* purchaseToken = env->GetStringUTFChars(jpurchaseToken, nullptr);
        EBUS_EVENT(InAppPurchasesResponseBus, PurchaseConsumed, AZStd::string(purchaseToken, env->GetStringUTFLength(jpurchaseToken)));
        env->ReleaseStringUTFChars(jpurchaseToken, purchaseToken);
    }

    static JNINativeMethod methods[] = {
        { "nativeProductInfoRetrieved", "([Ljava/lang/Object;)V", (void*)ProductInfoRetrieved },
        { "nativePurchasedProductsRetrieved", "([Ljava/lang/Object;)V", (void*)PurchasedProductsRetrieved },
        { "nativeNewProductPurchased", "([Ljava/lang/Object;)V", (void*)NewProductPurchased },
        { "nativePurchaseConsumed", "(Ljava/lang/String;)V", (void*)PurchaseConsumed }
    };

    InAppPurchasesInterface* InAppPurchasesInterface::CreateInstance()
    {
        return new InAppPurchasesAndroid();
    }

    void InAppPurchasesAndroid::Initialize()
    {
        JNIEnv* env = AZ::Android::JNI::GetEnv();
        jobject activityObject = AZ::Android::Utils::GetActivityRef();

        jclass billingClass = AZ::Android::JNI::LoadClass("com/amazon/lumberyard/iap/LumberyardInAppBilling");
        jmethodID mid = env->GetMethodID(billingClass, "<init>", "(Landroid/app/Activity;)V");

        jobject billingInstance = env->NewObject(billingClass, mid, activityObject);
        m_billingInstance = env->NewGlobalRef(billingInstance);

        env->RegisterNatives(billingClass, methods, sizeof(methods) / sizeof(methods[0]));

        mid = env->GetMethodID(billingClass, "IsKindleDevice", "()Z");
        jboolean result = env->CallBooleanMethod(m_billingInstance, mid);
        if (result)
        {
            EBUS_EVENT(AZ::NativeUI::NativeUIRequestBus, DisplayOkDialog, "Kindle Device Detected", "IAP currently unsupported on Kindle devices", false);
        }

        env->DeleteGlobalRef(billingClass);
        env->DeleteLocalRef(billingInstance);
    }

    InAppPurchasesAndroid::~InAppPurchasesAndroid()
    {
        JNIEnv* env = AZ::Android::JNI::GetEnv();
        jclass billingClass = env->GetObjectClass(m_billingInstance);
        jmethodID mid = env->GetMethodID(billingClass, "UnbindService", "()V");
        env->CallVoidMethod(m_billingInstance, mid);

        env->DeleteLocalRef(billingClass);
        env->DeleteGlobalRef(m_billingInstance);
    }

    void InAppPurchasesAndroid::QueryProductInfo(AZStd::vector<AZStd::string>& productIds) const
    {
        JNIEnv* env = AZ::Android::JNI::GetEnv();

        jobjectArray jproductIds = static_cast<jobjectArray>(env->NewObjectArray(productIds.size(), env->FindClass("java/lang/String"), env->NewStringUTF("")));
        for (int i = 0; i < productIds.size(); i++)
        {
            env->SetObjectArrayElement(jproductIds, i, env->NewStringUTF(productIds[i].c_str()));
        }

        jclass billingClass = env->GetObjectClass(m_billingInstance);
        jmethodID mid = env->GetMethodID(billingClass, "QueryProductInfo", "([Ljava/lang/String;)V");
        env->CallVoidMethod(m_billingInstance, mid, jproductIds);

        env->DeleteLocalRef(jproductIds);
        env->DeleteLocalRef(billingClass);
    }

    void InAppPurchasesAndroid::QueryProductInfo() const
    {
        AZ::IO::FileIOBase* fileReader = AZ::IO::FileIOBase::GetInstance();

        AZStd::string fileBuffer;

        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        AZ::u64 fileSize = 0;
        if (!fileReader->Open("@products@/product_ids.json", AZ::IO::OpenMode::ModeRead, fileHandle))
        {
            AZ_TracePrintf("LumberyardInAppBilling", "Unable to open file product_ids.json\n");
            return;
        }

        if ((!fileReader->Size(fileHandle, fileSize)) || (fileSize == 0))
        {
            AZ_TracePrintf("LumberyardInAppBilling", "Unable to read file product_ids.json - file truncated\n");
            fileReader->Close(fileHandle);
            return;
        }
        fileBuffer.resize(fileSize);
        if (!fileReader->Read(fileHandle, fileBuffer.data(), fileSize, true))
        {
            fileBuffer.resize(0);
            fileReader->Close(fileHandle);
            AZ_TracePrintf("LumberyardInAppBilling", "Failed to read file product_ids.json\n");
            return;
        }
        fileReader->Close(fileHandle);

        rapidjson::Document document;
        document.Parse(fileBuffer.data());
        if (document.HasParseError())
        {
            [[maybe_unused]] const char* errorStr = rapidjson::GetParseError_En(document.GetParseError());
            AZ_TracePrintf("LumberyardInAppBilling", "Failed to parse product_ids.json: %s\n", errorStr);
            return;
        }

        const rapidjson::Value& productList = document["product_ids"];
        const auto& end = productList.End();
        AZStd::vector<AZStd::string> productIds;
        for (auto it = productList.Begin(); it != end; it++)
        {
            const auto& elem = *it;
            productIds.push_back(elem["id"].GetString());
        }

        QueryProductInfo(productIds);
    }

    void InAppPurchasesAndroid::PurchaseProduct(const AZStd::string& productId, const AZStd::string& developerPayload) const
    {
        JNIEnv* env = AZ::Android::JNI::GetEnv();

        const AZStd::vector <AZStd::unique_ptr<ProductDetails const> >& cachedProductDetails = InAppPurchasesInterface::GetInstance()->GetCache()->GetCachedProductDetails();
        AZStd::string productType = "";
        for (int i = 0; i < cachedProductDetails.size(); i++)
        {
            const ProductDetailsAndroid* productDetails = azrtti_cast<const ProductDetailsAndroid*>(cachedProductDetails[i].get());
            const AZStd::string& cachedProductId = productDetails->GetProductId();
            if (cachedProductId.compare(productId) == 0)
            {
                productType = productDetails->GetProductType();
                break;
            }
        }

        if (productType.empty())
        {
            AZ_TracePrintf("LumberyardInAppBilling", "Failed to find product with id: %s", productId.c_str());
            return;
        }

        jstring jproductId = env->NewStringUTF(productId.c_str());
        jstring jdeveloperPayload = env->NewStringUTF(developerPayload.c_str());
        jstring jproductType = env->NewStringUTF(productType.c_str());

        jclass billingClass = env->GetObjectClass(m_billingInstance);
        jmethodID mid = env->GetMethodID(billingClass, "PurchaseProduct", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
        env->CallVoidMethod(m_billingInstance, mid, jproductId, jdeveloperPayload, jproductType);

        env->DeleteLocalRef(jproductId);
        env->DeleteLocalRef(jdeveloperPayload);
        env->DeleteLocalRef(jproductType);
        env->DeleteLocalRef(billingClass);
    }

    void InAppPurchasesAndroid::PurchaseProduct(const AZStd::string& productId) const
    {
        PurchaseProduct(productId, "");
    }

    void InAppPurchasesAndroid::QueryPurchasedProducts() const
    {
        JNIEnv* env = AZ::Android::JNI::GetEnv();

        jclass billingClass = env->GetObjectClass(m_billingInstance);
        jmethodID mid = env->GetMethodID(billingClass, "QueryPurchasedProducts", "()V");
        env->CallVoidMethod(m_billingInstance, mid);

        env->DeleteLocalRef(billingClass);
    }

    void InAppPurchasesAndroid::RestorePurchasedProducts() const
    {

    }

    void InAppPurchasesAndroid::ConsumePurchase(const AZStd::string& purchaseToken) const
    {
        JNIEnv* env = AZ::Android::JNI::GetEnv();

        jclass billingClass = env->GetObjectClass(m_billingInstance);
        jmethodID mid = env->GetMethodID(billingClass, "ConsumePurchase", "(Ljava/lang/String;)V");
        jstring jpurchaseToken = env->NewStringUTF(purchaseToken.c_str());
        env->CallVoidMethod(m_billingInstance, mid, jpurchaseToken);

        env->DeleteLocalRef(jpurchaseToken);
        env->DeleteLocalRef(billingClass);
    }

    void InAppPurchasesAndroid::FinishTransaction(const AZStd::string& transactionId, bool downloadHostedContent) const
    {

    }

    InAppPurchasesCache* InAppPurchasesAndroid::GetCache()
    {
        return &m_cache;
    }
}
