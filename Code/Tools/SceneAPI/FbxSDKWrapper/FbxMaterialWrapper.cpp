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
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/MathUtils.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/FbxSDKWrapper/FbxMaterialWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxPropertyWrapper.h>

namespace
{
    const char* s_physicalisedAttributeName = "physicalize";
    const char* s_proxyNoDraw = "ProxyNoDraw";
}

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxMaterialWrapper::FbxMaterialWrapper(FbxSurfaceMaterial* fbxMaterial)
            : SDKMaterial::MaterialWrapper(fbxMaterial)
        {
            AZ_Assert(fbxMaterial, "Invalid FbxSurfaceMaterial input to initialize FbxMaterialWrapper");
        }

        AZStd::string FbxMaterialWrapper::GetName() const
        {
            return m_fbxMaterial->GetInitialName();
        }

        AZ::Vector3 FbxMaterialWrapper::GetDiffuseColor() const
        {
            if (m_fbxMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
            {
                FbxSurfaceLambert* lambertMat = static_cast<FbxSurfaceLambert*>(m_fbxMaterial);
                const FbxDouble3 fbxValue = lambertMat->Diffuse.Get();
                const float power = static_cast<float>(lambertMat->DiffuseFactor.Get());
                return power * AZ::Vector3(static_cast<float>(fbxValue[0]), static_cast<float>(fbxValue[1]), static_cast<float>(fbxValue[2]));
            }
            else if (m_fbxMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
            {
                FbxSurfacePhong* phongMat = static_cast<FbxSurfacePhong*>(m_fbxMaterial);
                const FbxDouble3 fbxValue = phongMat->Diffuse.Get();
                const float power = static_cast<float>(phongMat->DiffuseFactor.Get());
                return power * AZ::Vector3(static_cast<float>(fbxValue[0]), static_cast<float>(fbxValue[1]), static_cast<float>(fbxValue[2]));
            }

            return AZ::Vector3::CreateOne();
        }

        AZ::Vector3 FbxMaterialWrapper::GetSpecularColor() const
        {
            if (m_fbxMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
            {
                FbxSurfacePhong* phongMat = static_cast<FbxSurfacePhong*>(m_fbxMaterial);
                const FbxDouble3 fbxValue = phongMat->Specular.Get();
                const float power = static_cast<float>(phongMat->SpecularFactor.Get());
                return power * AZ::Vector3(static_cast<float>(fbxValue[0]), static_cast<float>(fbxValue[1]), static_cast<float>(fbxValue[2]));
            }

            return AZ::Vector3::CreateZero();
        }

        AZ::Vector3 FbxMaterialWrapper::GetEmissiveColor() const
        {
            if (m_fbxMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
            {
                FbxSurfaceLambert* lambertMat = static_cast<FbxSurfaceLambert*>(m_fbxMaterial);
                const FbxDouble3 fbxValue = lambertMat->Emissive.Get();
                const float power = static_cast<float>(lambertMat->EmissiveFactor.Get());
                return power * AZ::Vector3(static_cast<float>(fbxValue[0]), static_cast<float>(fbxValue[1]), static_cast<float>(fbxValue[2]));
            }
            else if (m_fbxMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
            {
                FbxSurfacePhong* phongMat = static_cast<FbxSurfacePhong*>(m_fbxMaterial);
                const FbxDouble3 fbxValue = phongMat->Emissive.Get();
                const float power = static_cast<float>(phongMat->EmissiveFactor.Get());
                return power * AZ::Vector3(static_cast<float>(fbxValue[0]), static_cast<float>(fbxValue[1]), static_cast<float>(fbxValue[2]));
            }

            return AZ::Vector3::CreateZero();
        }

        float FbxMaterialWrapper::GetShininess() const
        {
            if (m_fbxMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
            {
                FbxSurfacePhong* phongMat = static_cast<FbxSurfacePhong*>(m_fbxMaterial);
                return static_cast<float>(phongMat->Shininess.Get());
            }

            return 10.f;
        }

        AZ::u64 FbxMaterialWrapper::GetUniqueId() const
        {
            return m_fbxMaterial->GetUniqueID();
        }

        float FbxMaterialWrapper::GetOpacity() const
        {
            // FBX materials are erroneously reporting a TransparencyFactor of 1.0 (fully transparent)
            // even for values that are 0.0 in Maya. It is instead storing it in the components
            // for TransparentColor, so extract from there instead.

            if (m_fbxMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
            {
                FbxSurfaceLambert* lambertMat = static_cast<FbxSurfaceLambert*>(m_fbxMaterial);
                const FbxDouble3 fbxValue = lambertMat->TransparentColor.Get();
                return 1.f - AZ::GetMin<float>(AZ::GetMin<float>(static_cast<float>(fbxValue[0]), static_cast<float>(fbxValue[1])), static_cast<float>(fbxValue[2]));
            }
            else if (m_fbxMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
            {
                FbxSurfacePhong* phongMat = static_cast<FbxSurfacePhong*>(m_fbxMaterial);
                const FbxDouble3 fbxValue = phongMat->TransparentColor.Get();
                return 1.f - AZ::GetMin<float>(AZ::GetMin<float>(static_cast<float>(fbxValue[0]), static_cast<float>(fbxValue[1])), static_cast<float>(fbxValue[2]));
            }

            return 1.f;
        }

        AZStd::string FbxMaterialWrapper::GetTextureFileName(const char* textureType) const
        {
            FbxFileTexture* fileTexture = nullptr;
            FbxProperty property = m_fbxMaterial->FindProperty(textureType);
            FbxDataType propertyType = property.GetPropertyDataType();

            /// Engine currently doesn't support multiple textures. Right now we only use first texture of first layer.
            int layeredTextureCount = property.GetSrcObjectCount<FbxLayeredTexture>();
            if (layeredTextureCount > 0)
            {
                FbxLayeredTexture* layeredTexture = FbxCast<FbxLayeredTexture>(property.GetSrcObject<FbxLayeredTexture>(0));
                int textureCount = layeredTexture->GetSrcObjectCount<FbxTexture>();
                if (textureCount > 0)
                {
                    fileTexture = FbxCast<FbxFileTexture>(layeredTexture->GetSrcObject<FbxTexture>(0));
                }
            }
            else
            {
                int textureCount = property.GetSrcObjectCount<FbxTexture>();
                if (textureCount > 0)
                {
                    fileTexture = FbxCast<FbxFileTexture>(property.GetSrcObject<FbxTexture>(0));
                }
            }

            return fileTexture ? fileTexture->GetFileName() : AZStd::string();
        }

        AZStd::string FbxMaterialWrapper::GetTextureFileName(const AZStd::string& textureType) const
        {
            return GetTextureFileName(textureType.c_str());
        }

        AZStd::string FbxMaterialWrapper::GetTextureFileName(MaterialMapType textureType) const
        {
            switch (textureType)
            {
            case MaterialMapType::Diffuse:
                return GetTextureFileName(FbxSurfaceMaterial::sDiffuse);
            case MaterialMapType::Specular:
                return GetTextureFileName(FbxSurfaceMaterial::sSpecular);
            case MaterialMapType::Bump:
                return GetTextureFileName(FbxSurfaceMaterial::sBump);
            case MaterialMapType::Normal:
                return GetTextureFileName(FbxSurfaceMaterial::sNormalMap);
            default:
                AZ_TraceContext("Unknown value", aznumeric_cast<int>(textureType));
                AZ_TracePrintf(SceneAPI::Utilities::WarningWindow, "Unrecognized MaterialMapType retrieved");
                return AZStd::string();
            }
        }
    } // namespace FbxSDKWrapper
} // namespace AZ
