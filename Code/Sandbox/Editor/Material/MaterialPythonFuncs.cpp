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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Material support for Python

#include "EditorDefs.h"

#include "MaterialPythonFuncs.h"

// Editor
#include "MaterialManager.h"
#include "ShaderEnum.h"


namespace
{
    void PyMaterialCreate()
    {
        GetIEditor()->GetMaterialManager()->Command_Create();
    }

    void PyMaterialCreateMulti()
    {
        GetIEditor()->GetMaterialManager()->Command_CreateMulti();
    }

    void PyMaterialConvertToMulti()
    {
        GetIEditor()->GetMaterialManager()->Command_ConvertToMulti();
    }

    void PyMaterialDuplicateCurrent()
    {
        GetIEditor()->GetMaterialManager()->Command_Duplicate();
    }

    void PyMaterialMergeSelection()
    {
        GetIEditor()->GetMaterialManager()->Command_Merge();
    }

    void PyMaterialDeleteCurrent()
    {
        GetIEditor()->GetMaterialManager()->Command_Delete();
    }

    void PyMaterialAssignCurrentToSelection()
    {
        CUndo undo("Assign Material To Selection");
        GetIEditor()->GetMaterialManager()->Command_AssignToSelection();
    }

    void PyMaterialResetSelection()
    {
        GetIEditor()->GetMaterialManager()->Command_ResetSelection();
    }

    void PyMaterialSelectObjectsWithCurrent()
    {
        CUndo undo("Select Objects With Current Material");
        GetIEditor()->GetMaterialManager()->Command_SelectAssignedObjects();
    }

    void PyMaterialSetCurrentFromObject()
    {
        GetIEditor()->GetMaterialManager()->Command_SelectFromObject();
    }

    AZStd::vector<AZStd::string> PyGetSubMaterial(const char* pMaterialPath)
    {
        QString materialPath = pMaterialPath;
        CMaterial* pMaterial = GetIEditor()->GetMaterialManager()->LoadMaterial(pMaterialPath, false);
        if (!pMaterial)
        {
            throw std::runtime_error("Invalid multi material.");
        }

        AZStd::vector<AZStd::string> result;
        for (int i = 0; i < pMaterial->GetSubMaterialCount(); i++)
        {
            if (pMaterial->GetSubMaterial(i))
            {
                result.push_back((materialPath + "\\" + pMaterial->GetSubMaterial(i)->GetName()).toUtf8().data());
            }
        }
        return result;
    }

    CMaterial* TryLoadingMaterial(const char* pPathAndMaterialName)
    {
        QString varMaterialPath = pPathAndMaterialName;
        std::deque<QString> splittedMaterialPath;
        for (auto token : varMaterialPath.split(QRegularExpression(R"([\\/])"), Qt::SkipEmptyParts))
        {
            splittedMaterialPath.push_back(token);
        }

        CMaterial* pMaterial  = GetIEditor()->GetMaterialManager()->LoadMaterial(varMaterialPath, false);
        if (!pMaterial)
        {
            QString subMaterialName = splittedMaterialPath.back();
            bool isSubMaterialExist(false);

            varMaterialPath.remove((varMaterialPath.length() - subMaterialName.length() - 1), varMaterialPath.length());
            QString test = varMaterialPath;
            pMaterial  = GetIEditor()->GetMaterialManager()->LoadMaterial(varMaterialPath, false);

            if (!pMaterial)
            {
                throw std::runtime_error("Invalid multi material.");
            }

            for (int i = 0; i < pMaterial->GetSubMaterialCount(); i++)
            {
                if (pMaterial->GetSubMaterial(i)->GetName() == subMaterialName)
                {
                    pMaterial = pMaterial->GetSubMaterial(i);
                    isSubMaterialExist = true;
                }
            }

            if (!pMaterial || !isSubMaterialExist)
            {
                throw std::runtime_error((QString("\"") + subMaterialName + "\" is an invalid sub material.").toUtf8().data());
            }
        }
        GetIEditor()->GetMaterialManager()->SetCurrentMaterial(pMaterial);
        return pMaterial;
    }

    std::deque<QString> PreparePropertyPath(const char* pPathAndPropertyName)
    {
        QString varPathProperty = pPathAndPropertyName;
        std::deque<QString> splittedPropertyPath;
        for (auto token : varPathProperty.split(QRegularExpression(R"([\\/])"), Qt::SkipEmptyParts))
        {
            splittedPropertyPath.push_back(token);
        }

        return splittedPropertyPath;
    }

    //////////////////////////////////////////////////////////////////////////
    // Converter: Enum -> CString (human readable)
    //////////////////////////////////////////////////////////////////////////

    QString TryConvertingSEFResTextureToCString(SEfResTexture* pResTexture)
    {
        if (pResTexture)
        {
            return pResTexture->m_Name.c_str();
        }
        return "";
    }

    QString TryConvertingETEX_TypeToCString(const uint8& texTypeName)
    {
        switch (texTypeName)
        {
        case eTT_2D:
            return "2D";
        case eTT_Cube:
            return "Cube-Map";
        case eTT_NearestCube:
            return "Nearest Cube-Map probe for alpha blended";
        case eTT_Auto2D:
            return "Auto 2D-Map";
        case eTT_Dyn2D:
            return "Dynamic 2D-Map";
        case eTT_User:
            return "From User Params";
        default:
            throw std::runtime_error("Invalid tex type.");
        }
    }

    QString TryConvertingTexFilterToCString(const int& iFilterName)
    {
        switch (iFilterName)
        {
        case FILTER_NONE:
            return "Default";
        case FILTER_POINT:
            return "Point";
        case FILTER_LINEAR:
            return "Linear";
        case FILTER_BILINEAR:
            return "Bilinear";
        case FILTER_TRILINEAR:
            return "Trilinear";
        case FILTER_ANISO2X:
            return "Anisotropic 2x";
        case FILTER_ANISO4X:
            return "Anisotropic 4x";
        case FILTER_ANISO8X:
            return "Anisotropic 8x";
        case FILTER_ANISO16X:
            return "Anisotropic 16x";
        default:
            throw std::runtime_error("Invalid tex filter.");
        }
    }

    QString TryConvertingETexGenTypeToCString(const uint8& texGenType)
    {
        switch (texGenType)
        {
        case ETG_Stream:
            return "Stream";
        case ETG_World:
            return "World";
        case ETG_Camera:
            return "Camera";
        default:
            throw std::runtime_error("Invalid tex gen type.");
        }
    }

    QString TryConvertingETexModRotateTypeToCString(const uint8& rotateType)
    {
        switch (rotateType)
        {
        case ETMR_NoChange:
            return "No Change";
        case ETMR_Fixed:
            return "Fixed Rotation";
        case ETMR_Constant:
            return "Constant Rotation";
        case ETMR_Oscillated:
            return "Oscillated Rotation";
        default:
            throw std::runtime_error("Invalid rotate type.");
        }
    }

    QString TryConvertingETexModMoveTypeToCString(const uint8& oscillatorType)
    {
        switch (oscillatorType)
        {
        case ETMM_NoChange:
            return "No Change";
        case ETMM_Fixed:
            return "Fixed Moving";
        case ETMM_Constant:
            return "Constant Moving";
        case ETMM_Jitter:
            return "Jitter Moving";
        case ETMM_Pan:
            return "Pan Moving";
        case ETMM_Stretch:
            return "Stretch Moving";
        case ETMM_StretchRepeat:
            return "Stretch-Repeat Moving";
        default:
            throw std::runtime_error("Invalid oscillator type.");
        }
    }

    QString TryConvertingEDeformTypeToCString(const EDeformType& deformType)
    {
        switch (deformType)
        {
        case eDT_Unknown:
            return "None";
        case eDT_SinWave:
            return "Sin Wave";
        case eDT_SinWaveUsingVtxColor:
            return "Sin Wave using vertex color";
        case eDT_Bulge:
            return "Bulge";
        case eDT_Squeeze:
            return "Squeeze";
        case eDT_Perlin2D:
            return "Perlin 2D";
        case eDT_Perlin3D:
            return "Perlin 3D";
        case eDT_FromCenter:
            return "From Center";
        case eDT_Bending:
            return "Bending";
        case eDT_ProcFlare:
            return "Proc. Flare";
        case eDT_AutoSprite:
            return "Auto sprite";
        case eDT_Beam:
            return "Beam";
        case eDT_FixedOffset:
            return "FixedOffset";
        default:
            throw std::runtime_error("Invalid deform type.");
        }
    }

    QString TryConvertingEWaveFormToCString(const EWaveForm& waveForm)
    {
        switch (waveForm)
        {
        case eWF_None:
            return "None";
        case eWF_Sin:
            return "Sin";
        case eWF_HalfSin:
            return "Half Sin";
        case eWF_Square:
            return "Square";
        case eWF_Triangle:
            return "Triangle";
        case eWF_SawTooth:
            return "Saw Tooth";
        case eWF_InvSawTooth:
            return "Inverse Saw Tooth";
        case eWF_Hill:
            return "Hill";
        case eWF_InvHill:
            return "Inverse Hill";
        default:
            throw std::runtime_error("Invalid wave form.");
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Converter: CString -> Enum
    //////////////////////////////////////////////////////////////////////////

    template<typename STRING>
    // [Shader System TO DO] remove once dynamic slots assingment is in place
    EEfResTextures TryConvertingCStringToEEfResTextures(const STRING& resTextureName)
    {
        if (resTextureName == "Diffuse")
        {
            return EFTT_DIFFUSE;
        }
        else if (resTextureName == "Specular")
        {
            return EFTT_SPECULAR;
        }
        else if (resTextureName == "Bumpmap")
        {
            return EFTT_NORMALS;
        }
        else if (resTextureName == "Heightmap")
        {
            return EFTT_HEIGHT;
        }
        else if (resTextureName == "Environment")
        {
            return EFTT_ENV;
        }
        else if (resTextureName == "Detail")
        {
            return EFTT_DETAIL_OVERLAY;
        }
        else if (resTextureName == "Opacity")
        {
            return EFTT_OPACITY;
        }
        else if (resTextureName == "Decal")
        {
            return EFTT_DECAL_OVERLAY;
        }
        else if (resTextureName == "SubSurface")
        {
            return EFTT_SUBSURFACE;
        }
        else if (resTextureName == "Custom")
        {
            return EFTT_CUSTOM;
        }
        else if (resTextureName == "[1] Custom")
        {
            return EFTT_DIFFUSE;
        }
        else if (resTextureName == "Emittance")
        {
            return EFTT_EMITTANCE;
        }
        else if (resTextureName == "Occlusion")
        {
            return EFTT_OCCLUSION;
        }
        else if (resTextureName == "Specular2")
        {
            return EFTT_SPECULAR_2;
        }
        throw std::runtime_error("Invalid texture name.");

        return EFTT_MAX;
    }

    template<typename STRING>
    ETEX_Type TryConvertingCStringToETEX_Type(const STRING& texTypeName)
    {
        if (texTypeName == "2D")
        {
            return eTT_2D;
        }
        else if (texTypeName == "Cube-Map")
        {
            return eTT_Cube;
        }
        else if (texTypeName == "Nearest Cube-Map probe for alpha blended")
        {
            return eTT_NearestCube;
        }
        else if (texTypeName == "Auto 2D-Map")
        {
            return eTT_Auto2D;
        }
        else if (texTypeName == "Dynamic 2D-Map")
        {
            return eTT_Dyn2D;
        }
        else if (texTypeName == "From User Params")
        {
            return eTT_User;
        }
        throw std::runtime_error("Invalid tex type name.");
    }

    template<typename STRING>
    signed char TryConvertingCStringToTexFilter(const STRING& filterName)
    {
        if (filterName == "Default")
        {
            return FILTER_NONE;
        }
        else if (filterName == "Point")
        {
            return FILTER_POINT;
        }
        else if (filterName == "Linear")
        {
            return FILTER_LINEAR;
        }
        else if (filterName == "Bilinear")
        {
            return FILTER_BILINEAR;
        }
        else if (filterName == "Trilinear")
        {
            return FILTER_TRILINEAR;
        }
        else if (filterName == "Anisotropic 2x")
        {
            return FILTER_ANISO2X;
        }
        else if (filterName == "Anisotropic 4x")
        {
            return FILTER_ANISO4X;
        }
        else if (filterName == "Anisotropic 8x")
        {
            return FILTER_ANISO8X;
        }
        else if (filterName == "Anisotropic 16x")
        {
            return FILTER_ANISO16X;
        }
        throw std::runtime_error("Invalid filter name.");
    }

    template<typename STRING>
    ETexGenType TryConvertingCStringToETexGenType(const STRING& texGenType)
    {
        if (texGenType == "Stream")
        {
            return ETG_Stream;
        }
        else if (texGenType == "World")
        {
            return ETG_World;
        }
        else if (texGenType == "Camera")
        {
            return ETG_Camera;
        }
        throw std::runtime_error("Invalid tex gen type name.");
    }

    template<typename STRING>
    ETexModRotateType TryConvertingCStringToETexModRotateType(const STRING& rotateType)
    {
        if (rotateType == "No Change")
        {
            return ETMR_NoChange;
        }
        else if (rotateType == "Fixed Rotation")
        {
            return ETMR_Fixed;
        }
        else if (rotateType == "Constant Rotation")
        {
            return ETMR_Constant;
        }
        else if (rotateType == "Oscillated Rotation")
        {
            return ETMR_Oscillated;
        }
        throw std::runtime_error("Invalid rotate type name.");
    }

    template<typename STRING>
    ETexModMoveType TryConvertingCStringToETexModMoveType(const STRING& oscillatorType)
    {
        if (oscillatorType == "No Change")
        {
            return ETMM_NoChange;
        }
        else if (oscillatorType == "Fixed Moving")
        {
            return ETMM_Fixed;
        }
        else if (oscillatorType == "Constant Moving")
        {
            return ETMM_Constant;
        }
        else if (oscillatorType == "Jitter Moving")
        {
            return ETMM_Jitter;
        }
        else if (oscillatorType == "Pan Moving")
        {
            return ETMM_Pan;
        }
        else if (oscillatorType == "Stretch Moving")
        {
            return ETMM_Stretch;
        }
        else if (oscillatorType == "Stretch-Repeat Moving")
        {
            return ETMM_StretchRepeat;
        }
        throw std::runtime_error("Invalid oscillator type.");
    }

    template<typename STRING>
    EDeformType TryConvertingCStringToEDeformType(const STRING& deformType)
    {
        if (deformType == "None")
        {
            return eDT_Unknown;
        }
        else if (deformType == "Sin Wave")
        {
            return eDT_SinWave;
        }
        else if (deformType == "Sin Wave using vertex color")
        {
            return eDT_SinWaveUsingVtxColor;
        }
        else if (deformType == "Bulge")
        {
            return eDT_Bulge;
        }
        else if (deformType == "Squeeze")
        {
            return eDT_Squeeze;
        }
        else if (deformType == "Perlin 2D")
        {
            return eDT_Perlin2D;
        }
        else if (deformType == "Perlin 3D")
        {
            return eDT_Perlin3D;
        }
        else if (deformType == "From Center")
        {
            return eDT_FromCenter;
        }
        else if (deformType == "Bending")
        {
            return eDT_Bending;
        }
        else if (deformType == "Proc. Flare")
        {
            return eDT_ProcFlare;
        }
        else if (deformType == "Auto sprite")
        {
            return eDT_AutoSprite;
        }
        else if (deformType == "Beam")
        {
            return eDT_Beam;
        }
        else if (deformType == "FixedOffset")
        {
            return eDT_FixedOffset;
        }
        throw std::runtime_error("Invalid deform type.");
    }

    template <typename STRING>
    EWaveForm TryConvertingCStringToEWaveForm(const STRING& waveForm)
    {
        if (waveForm == "None")
        {
            return eWF_None;
        }
        else if (waveForm == "Sin")
        {
            return eWF_Sin;
        }
        else if (waveForm == "Half Sin")
        {
            return eWF_HalfSin;
        }
        else if (waveForm == "Square")
        {
            return eWF_Square;
        }
        else if (waveForm == "Triangle")
        {
            return eWF_Triangle;
        }
        else if (waveForm == "Saw Tooth")
        {
            return eWF_SawTooth;
        }
        else if (waveForm == "Inverse Saw Tooth")
        {
            return eWF_InvSawTooth;
        }
        else if (waveForm == "Hill")
        {
            return eWF_Hill;
        }
        else if (waveForm == "Inverse Hill")
        {
            return eWF_InvHill;
        }
        throw std::runtime_error("Invalid wave form.");
    }

    //////////////////////////////////////////////////////////////////////////
    // Script parser
    //////////////////////////////////////////////////////////////////////////

    QString ParseUINameFromPublicParamsScript(const char* sUIScript)
    {
        string uiscript = sUIScript;
        string element[3];
        int p1 = 0;
        string itemToken = uiscript.Tokenize(";", p1);
        while (!itemToken.empty())
        {
            int nElements = 0;
            int p2 = 0;
            string token = itemToken.Tokenize(" \t\r\n=", p2);
            while (!token.empty())
            {
                element[nElements++] = token;
                if (nElements == 2)
                {
                    element[nElements] = itemToken.substr(p2);
                    element[nElements].Trim(" =\t\"");
                    break;
                }
                token = itemToken.Tokenize(" \t\r\n=", p2);
            }

            if (_stricmp(element[1], "UIName") == 0)
            {
                return element[2].c_str();
            }
            itemToken = uiscript.Tokenize(";", p1);
        }
        return "";
    }

    std::map<QString, float> ParseValidRangeFromPublicParamsScript(const char* sUIScript)
    {
        string uiscript = sUIScript;
        std::map<QString, float> result;
        bool isUIMinExist(false);
        bool isUIMaxExist(false);
        string element[3];
        int p1 = 0;
        string itemToken = uiscript.Tokenize(";", p1);
        while (!itemToken.empty())
        {
            int nElements = 0;
            int p2 = 0;
            string token = itemToken.Tokenize(" \t\r\n=", p2);
            while (!token.empty())
            {
                element[nElements++] = token;
                if (nElements == 2)
                {
                    element[nElements] = itemToken.substr(p2);
                    element[nElements].Trim(" =\t\"");
                    break;
                }
                token = itemToken.Tokenize(" \t\r\n=", p2);
            }

            if (_stricmp(element[1], "UIMin") == 0)
            {
                result["UIMin"] = atof(element[2]);
                isUIMinExist = true;
            }
            if (_stricmp(element[1], "UIMax") == 0)
            {
                result["UIMax"] = atof(element[2]);
                isUIMaxExist = true;
            }
            itemToken = uiscript.Tokenize(";", p1);
        }
        if (!isUIMinExist || !isUIMaxExist)
        {
            throw std::runtime_error("Invalid range for shader param.");
        }
        return result;
    }

    //////////////////////////////////////////////////////////////////////////
    // Set Flags
    //////////////////////////////////////////////////////////////////////////

    void SetMaterialFlag(CMaterial* pMaterial, const EMaterialFlags& eFlag, const bool& bFlag)
    {
        if (pMaterial->GetFlags() & eFlag && bFlag == false)
        {
            pMaterial->SetFlags(pMaterial->GetFlags() - eFlag);
        }
        else if (!(pMaterial->GetFlags() & eFlag) && bFlag == true)
        {
            pMaterial->SetFlags(pMaterial->GetFlags() | eFlag);
        }
    }

    void SetPropagationFlag(CMaterial* pMaterial, const eMTL_PROPAGATION& eFlag, const bool& bFlag)
    {
        if (pMaterial->GetPropagationFlags() & eFlag && bFlag == false)
        {
            pMaterial->SetPropagationFlags(pMaterial->GetPropagationFlags() - eFlag);
        }
        else if (!(pMaterial->GetPropagationFlags() & eFlag) && bFlag == true)
        {
            pMaterial->SetPropagationFlags(pMaterial->GetPropagationFlags() | eFlag);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    template <typename T>
    bool IsAnyValidRange(const AZStd::any& value, const T& low, const T& high, const QString& invalidTypeMessage, const QString& invalidValueMessage)
    {
        static_assert(AZStd::is_arithmetic<T>::value, "This function only works with numbers.");

        if (!value.is<T>())
        {
            throw std::runtime_error(invalidTypeMessage.toUtf8().data());
        }

        T valueData;
        AZStd::any_numeric_cast(&value, valueData);
        if (valueData < low || valueData > high)
        {
            throw std::runtime_error(invalidValueMessage.toUtf8().data());
        }
        return true;
    }

    template <>
    bool IsAnyValidRange(const AZStd::any& value, const AZ::Color& low, const AZ::Color& high, const QString& invalidTypeMessage, const QString& invalidValueMessage)
    {
        if (!value.is<AZ::Color>())
        {
            throw std::runtime_error(invalidTypeMessage.toUtf8().data());
        }

        const AZ::Color* valueData = AZStd::any_cast<const AZ::Color>(&value);
        if (!valueData)
        {
            throw std::runtime_error(invalidValueMessage.toUtf8().data());
        }

        if (valueData->IsLessThan(low) || valueData->IsGreaterThan(high))
        {
            throw std::runtime_error(invalidValueMessage.toUtf8().data());
        }
        return true;
    }

    bool ValidateProperty(CMaterial* pMaterial, const std::deque<QString>& splittedPropertyPathParam, const AZStd::any& value)
    {
        std::deque<QString> splittedPropertyPath = splittedPropertyPathParam;
        std::deque<QString> splittedPropertyPathSubCategory = splittedPropertyPathParam;
        QString categoryName = splittedPropertyPath.front();
        QString subCategoryName = "";
        QString subSubCategoryName = "";
        QString currentPath = "";
        QString propertyName = splittedPropertyPath.back();

        QString errorMsgInvalidValue = QString("Invalid value for property \"") + propertyName + "\"";
        QString errorMsgInvalidDataType = QString("Invalid data type for property \"") + propertyName + "\"";
        QString errorMsgInvalidPropertyPath = QString("Invalid property path");

        const int iMinColorValue = 0;
        const int iMaxColorValue = 255;

        if (splittedPropertyPathSubCategory.size() == 3)
        {
            splittedPropertyPathSubCategory.pop_back();
            subCategoryName = splittedPropertyPathSubCategory.back();
            currentPath = QString("") + categoryName + "/" + subCategoryName;

            if (
                subCategoryName != "TexType" &&
                subCategoryName != "Filter" &&
                subCategoryName != "IsProjectedTexGen" &&
                subCategoryName != "TexGenType" &&
                subCategoryName != "Wave X" &&
                subCategoryName != "Wave Y" &&
                subCategoryName != "Wave Z" &&
                subCategoryName != "Wave W" &&
                subCategoryName != "Shader1" &&
                subCategoryName != "Shader2" &&
                subCategoryName != "Shader3" &&
                subCategoryName != "Tiling" &&
                subCategoryName != "Rotator" &&
                subCategoryName != "Oscillator" &&
                subCategoryName != "Diffuse" &&
                subCategoryName != "Specular" &&
                subCategoryName != "Bumpmap" &&
                subCategoryName != "Heightmap" &&
                subCategoryName != "Environment" &&
                subCategoryName != "Detail" &&
                subCategoryName != "Opacity" &&
                subCategoryName != "Decal" &&
                subCategoryName != "SubSurface" &&
                subCategoryName != "Custom" &&
                subCategoryName != "[1] Custom"
                )
            {
                throw std::runtime_error((errorMsgInvalidPropertyPath + " (" + currentPath + ")").toUtf8().data());
            }
        }
        else if (splittedPropertyPathSubCategory.size() == 4)
        {
            splittedPropertyPathSubCategory.pop_back();
            subCategoryName = splittedPropertyPathSubCategory.back();
            splittedPropertyPathSubCategory.pop_back();
            subSubCategoryName = splittedPropertyPathSubCategory.back();
            currentPath = categoryName + "/" + subSubCategoryName + "/" + subCategoryName;

            if (
                subSubCategoryName != "Diffuse" &&
                subSubCategoryName != "Specular" &&
                subSubCategoryName != "Bumpmap" &&
                subSubCategoryName != "Heightmap" &&
                subSubCategoryName != "Environment" &&
                subSubCategoryName != "Detail" &&
                subSubCategoryName != "Opacity" &&
                subSubCategoryName != "Decal" &&
                subSubCategoryName != "SubSurface" &&
                subSubCategoryName != "Custom" &&
                subSubCategoryName != "[1] Custom"
                )
            {
                throw std::runtime_error((errorMsgInvalidPropertyPath + " (" + currentPath + ")").toUtf8().data());
            }
            else if (subCategoryName != "Tiling" && subCategoryName != "Rotator" && subCategoryName != "Oscillator")
            {
                throw std::runtime_error((errorMsgInvalidPropertyPath + " (" + currentPath + ")").toUtf8().data());
            }
        }
        else
        {
            currentPath = categoryName;
        }

        if (
            categoryName == "Material Settings" ||
            categoryName == "Opacity Settings" ||
            categoryName == "Lighting Settings" ||
            categoryName == "Advanced" ||
            categoryName == "Texture Maps" ||
            categoryName == "Vertex Deformation" ||
            categoryName == "Layer Presets")
        {
            if (propertyName == "Opacity" || propertyName == "AlphaTest" || propertyName == "Glow Amount")
            {
                // int: 0 < x < 100
                if (!value.is<AZ::s64>())
                {
                    throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                }

                AZ::s64 valueData;
                AZStd::any_numeric_cast(&value, valueData);
                if (valueData < 0 || valueData > 100)
                {
                    throw std::runtime_error(errorMsgInvalidValue.toUtf8().data());
                }
                return true;
            }
            else if (propertyName == "Smoothness" || propertyName == "Glossiness")
            {
                // int: 0 < x < 255
                return IsAnyValidRange<AZ::s64>(value, 0, 255, errorMsgInvalidDataType, errorMsgInvalidValue);
            }
            else if (propertyName == "Specular Level")
            {
                // float: 0.0 < x < 4.0
                return IsAnyValidRange<double>(value, 0.0, 4.0, errorMsgInvalidDataType, errorMsgInvalidValue);
            }
            else if (
                propertyName == "TileU" ||
                propertyName == "TileV" ||
                propertyName == "OffsetU" ||
                propertyName == "OffsetV" ||
                propertyName == "RotateU" ||
                propertyName == "RotateV" ||
                propertyName == "RotateW" ||
                propertyName == "Rate" ||
                propertyName == "Phase" ||
                propertyName == "Amplitude" ||
                propertyName == "CenterU" ||
                propertyName == "CenterV" ||
                propertyName == "RateU" ||
                propertyName == "RateV" ||
                propertyName == "PhaseU" ||
                propertyName == "PhaseV" ||
                propertyName == "AmplitudeU" ||
                propertyName == "AmplitudeV" ||
                propertyName == "Wave Length X" ||
                propertyName == "Wave Length Y" ||
                propertyName == "Wave Length Z" ||
                propertyName == "Wave Length W" ||
                propertyName == "Level" ||
                propertyName == "Amplitude" ||
                propertyName == "Phase" ||
                propertyName == "Frequency")
            {
                // float: 0.0 < x < 100.0
                return IsAnyValidRange<double>(value, 0.0, 100.0, errorMsgInvalidDataType, errorMsgInvalidValue);
            }
            else if (propertyName == "Voxel Coverage")
            {
                // float: 0.0 < x < 1.0
                return IsAnyValidRange<double>(value, 0.0, 1.0, errorMsgInvalidDataType, errorMsgInvalidValue);
            }
            else if (propertyName == "Emissive Intensity")
            {
                // float: 0.0 < x < 200.0
                return IsAnyValidRange<double>(value, 0.0, 200.0, errorMsgInvalidDataType, errorMsgInvalidValue);
            }
            else if (propertyName == "Diffuse Color" || propertyName == "Specular Color" || propertyName == "Emissive Color")
            {
                // intVector(RGB): 0 < x < 255
                return IsAnyValidRange<AZ::Color>(value, AZ::Color::CreateZero(), AZ::Color::CreateOne(), errorMsgInvalidDataType, errorMsgInvalidValue);
            }
            else if (
                propertyName == "Link to Material" ||
                propertyName == "Surface Type" ||
                propertyName == "Diffuse" ||
                propertyName == "Specular" ||
                propertyName == "Bumpmap" ||
                propertyName == "Heightmap" ||
                propertyName == "Environment" ||
                propertyName == "Detail" ||
                propertyName == "Opacity" ||
                propertyName == "Decal" ||
                propertyName == "SubSurface" ||
                propertyName == "Custom" ||
                propertyName == "[1] Custom" ||
                propertyName == "TexType" ||
                propertyName == "Filter" ||
                propertyName == "TexGenType" ||
                propertyName == "Type" ||
                propertyName == "TypeU" ||
                propertyName == "TypeV")
            {
                // string
                if (!value.is<AZStd::string_view>())
                {
                    throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                }
                return true;
            }
            else if (
                propertyName == "Additive" ||
                propertyName == "Allow layer activation" ||
                propertyName == "2 Sided" ||
                propertyName == "No Shadow" ||
                propertyName == "Use Scattering" ||
                propertyName == "Hide After Breaking" ||
                propertyName == "Fog Volume Shading Quality High" ||
                propertyName == "Blend Terrain Color" ||
                propertyName == "Propagate Material Settings" ||
                propertyName == "Propagate Opacity Settings" ||
                propertyName == "Propagate Lighting Settings" ||
                propertyName == "Propagate Advanced Settings" ||
                propertyName == "Propagate Texture Maps" ||
                propertyName == "Propagate Shader Params" ||
                propertyName == "Propagate Shader Generation" ||
                propertyName == "Propagate Vertex Deformation" ||
                propertyName == "Propagate Layer Presets" ||
                propertyName == "IsProjectedTexGen" ||
                propertyName == "IsTileU" ||
                propertyName == "IsTileV" ||
                propertyName == "No Draw")
            {
                // bool
                if (!value.is<bool>())
                {
                    throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                }
                return true;
            }
            else if (propertyName == "Shader" || propertyName == "Shader1" || propertyName == "Shader2" || propertyName == "Shader3")
            {
                // string && valid shader
                if (!value.is<AZStd::string_view>())
                {
                    throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                }

                CShaderEnum* pShaderEnum = GetIEditor()->GetShaderEnum();
                if (!pShaderEnum)
                {
                    throw std::runtime_error("Shader enumerator corrupted.");
                }
                pShaderEnum->EnumShaders();
                for (int i = 0; i < pShaderEnum->GetShaderCount(); i++)
                {
                    if (pShaderEnum->GetShader(i) == AZStd::any_cast<AZStd::string_view>(value).data())
                    {
                        return true;
                    }
                }
            }
            else if (propertyName == "Noise Scale")
            {
                // FloatVec: undefined < x < undefined
                if (!value.is<AZ::Vector3>())
                {
                    throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                }
                return true;
            }
        }
        else if (categoryName == "Shader Params")
        {
            auto& shaderParams = pMaterial->GetShaderResources().m_ShaderParams;
            for (int i = 0; i < shaderParams.size(); i++)
            {
                if (propertyName == ParseUINameFromPublicParamsScript(shaderParams[i].m_Script.c_str()))
                {
                    if (shaderParams[i].m_Type == eType_FLOAT)
                    {
                        // float: valid range (from script)
                        float floatValue;
                        if (!AZStd::any_numeric_cast<float>(&value, floatValue))
                        {
                            throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                        }
                        std::map<QString, float> range = ParseValidRangeFromPublicParamsScript(shaderParams[i].m_Script.c_str());
                        if (floatValue < range["UIMin"] || floatValue > range["UIMax"])
                        {
                            QString errorMsg;
                            errorMsg = QStringLiteral("Invalid value for shader param \"%1\" (min: %2, max: %3)").arg(propertyName).arg(range["UIMin"]).arg(range["UIMax"]);
                            throw std::runtime_error(errorMsg.toUtf8().data());
                        }
                        return true;
                    }
                    else if (shaderParams[i].m_Type == eType_FCOLOR)
                    {
                        return IsAnyValidRange<AZ::Color>(value, AZ::Color::CreateZero(), AZ::Color::CreateOne(), errorMsgInvalidDataType, errorMsgInvalidValue);
                    }
                }
            }
        }
        else if (categoryName == "Shader Generation Params")
        {
            for (int i = 0; i < pMaterial->GetShaderGenParamsVars()->GetNumVariables(); i++)
            {
                if (propertyName == pMaterial->GetShaderGenParamsVars()->GetVariable(i)->GetHumanName())
                {
                    if (!value.is<bool>())
                    {
                        throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                    }
                    return true;
                }
            }
        }
        else
        {
            throw std::runtime_error((errorMsgInvalidPropertyPath + " (" + currentPath + ")").toUtf8().data());
        }
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    template <typename T>
    T PyFetchNumericType(const AZStd::any& value)
    {
        T numericValue;
        AZStd::any_numeric_cast(&value, numericValue);
        return numericValue;
    }

    AZStd::any PyGetProperty(const char* pPathAndMaterialName, const char* pPathAndPropertyName)
    {
        CMaterial* pMaterial = TryLoadingMaterial(pPathAndMaterialName);
        std::deque<QString> splittedPropertyPath = PreparePropertyPath(pPathAndPropertyName);
        std::deque<QString> splittedPropertyPathCategory = splittedPropertyPath;
        QString categoryName = splittedPropertyPath.front();
        QString subCategoryName = "None";
        QString subSubCategoryName = "None";
        QString propertyName = splittedPropertyPath.back();
        QString errorMsgInvalidPropertyPath = "Invalid property path.";
        
        if (splittedPropertyPathCategory.size() == 3)
        {
            splittedPropertyPathCategory.pop_back();
            subCategoryName = splittedPropertyPathCategory.back();
        }
        else if (splittedPropertyPathCategory.size() == 4)
        {
            splittedPropertyPathCategory.pop_back();
            subCategoryName = splittedPropertyPathCategory.back();
            splittedPropertyPathCategory.pop_back();
            subSubCategoryName = splittedPropertyPathCategory.back();
        }

        // ########## Material Settings ##########
        if (categoryName == "Material Settings")
        {
            if (propertyName == "Shader")
            {
                return AZStd::make_any<AZStd::string>(pMaterial->GetShaderName().toUtf8().data());
            }
            else if (propertyName == "Surface Type")
            {
                QString stringValue = pMaterial->GetSurfaceTypeName();
                if (stringValue.startsWith("mat_"))
                {
                    stringValue.remove(0, 4);
                }
                return AZStd::make_any<AZStd::string>(stringValue.toLatin1().data());
            }
            else
            {
                throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid material setting.").toUtf8().data());
            }
        }
        // ########## Opacity Settings ##########
        else if (categoryName == "Opacity Settings")
        {
            if (propertyName == "Opacity")
            {
                AZ::s64 intValue = aznumeric_cast<AZ::s64>(pMaterial->GetShaderResources().m_LMaterial.m_Opacity * 100.0f);
                return AZStd::make_any<AZ::s64>(intValue);
            }
            else if (propertyName == "AlphaTest")
            {
                AZ::s64 intValue = aznumeric_cast<AZ::s64>(pMaterial->GetShaderResources().m_AlphaRef * 100.0f);
                return AZStd::make_any<AZ::s64>(intValue);
            }
            else if (propertyName == "Additive")
            {
                return AZStd::make_any<bool>(pMaterial->GetFlags() & MTL_FLAG_ADDITIVE);
            }
            else
            {
                throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid opacity setting.").toUtf8().data());
            }
        }
        // ########## Lighting Settings ##########
        else if (categoryName == "Lighting Settings")
        {
            if (propertyName == "Diffuse Color")
            {
                QColor col = ColorLinearToGamma(ColorF(
                    pMaterial->GetShaderResources().m_LMaterial.m_Diffuse.r,
                    pMaterial->GetShaderResources().m_LMaterial.m_Diffuse.g,
                    pMaterial->GetShaderResources().m_LMaterial.m_Diffuse.b));
                return AZStd::make_any<AZ::Color>(col.red(), col.green(), col.blue(), 1.0f);
            }
            else if (propertyName == "Specular Color")
            {
                QColor col = ColorLinearToGamma(ColorF(
                    pMaterial->GetShaderResources().m_LMaterial.m_Specular.r / pMaterial->GetShaderResources().m_LMaterial.m_Specular.a,
                    pMaterial->GetShaderResources().m_LMaterial.m_Specular.g / pMaterial->GetShaderResources().m_LMaterial.m_Specular.a,
                    pMaterial->GetShaderResources().m_LMaterial.m_Specular.b / pMaterial->GetShaderResources().m_LMaterial.m_Specular.a));
                return AZStd::make_any<AZ::Color>(col.red(), col.green(), col.blue(), 1.0f);
            }
            else if (propertyName == "Glossiness")
            {
                return AZStd::make_any<float>(pMaterial->GetShaderResources().m_LMaterial.m_Smoothness);
            }
            else if (propertyName == "Specular Level")
            {
                return AZStd::make_any<float>(pMaterial->GetShaderResources().m_LMaterial.m_Specular.a);
            }
            else if (propertyName == "Emissive Color")
            {
                QColor col = ColorLinearToGamma(ColorF(
                    pMaterial->GetShaderResources().m_LMaterial.m_Emittance.r,
                    pMaterial->GetShaderResources().m_LMaterial.m_Emittance.g,
                    pMaterial->GetShaderResources().m_LMaterial.m_Emittance.b));
                return AZStd::make_any<AZ::Color>(col.red(), col.green(), col.blue(), 1.0f);
            }
            else if (propertyName == "Emissive Intensity")
            {
                return AZStd::make_any<float>(pMaterial->GetShaderResources().m_LMaterial.m_Emittance.a);
            }
            else
            {
                throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid lighting setting.").toUtf8().data());
            }
        }
        // ########## Advanced ##########
        else if (categoryName == "Advanced")
        {
            if (propertyName == "Allow layer activation")
            {
                return AZStd::make_any<bool>(pMaterial->LayerActivationAllowed());
            }
            else if (propertyName == "2 Sided")
            {
                return AZStd::make_any<bool>(pMaterial->GetFlags() & MTL_FLAG_2SIDED);
            }
            else if (propertyName == "No Shadow")
            {
                return AZStd::make_any<bool>(pMaterial->GetFlags() & MTL_FLAG_NOSHADOW);
            }
            else if (propertyName == "Use Scattering")
            {
                return AZStd::make_any<bool>(pMaterial->GetFlags() & MTL_FLAG_SCATTER);
            }
            else if (propertyName == "Hide After Breaking")
            {
                return AZStd::make_any<bool>(pMaterial->GetFlags() & MTL_FLAG_HIDEONBREAK);
            }
            else if (propertyName == "Fog Volume Shading Quality High")
            {
                return AZStd::make_any<bool>(pMaterial->GetFlags() & MTL_FLAG_FOG_VOLUME_SHADING_QUALITY_HIGH);
            }
            else if (propertyName == "Blend Terrain Color")
            {
                return AZStd::make_any<bool>(pMaterial->GetFlags() & MTL_FLAG_BLEND_TERRAIN);
            }
            else if (propertyName == "Voxel Coverage")
            {
                return AZStd::make_any<float>(aznumeric_cast<float>(pMaterial->GetShaderResources().m_VoxelCoverage) / 255.0f);
            }
            else if (propertyName == "Link to Material")
            {
                return AZStd::make_any<AZStd::string>(pMaterial->GetMatInfo()->GetMaterialLinkName());
            }
            else if (propertyName == "Propagate Material Settings")
            {
                return AZStd::make_any<bool>(pMaterial->GetPropagationFlags() & MTL_PROPAGATE_MATERIAL_SETTINGS);
            }
            else if (propertyName == "Propagate Opacity Settings")
            {
                return AZStd::make_any<bool>(pMaterial->GetPropagationFlags() & MTL_PROPAGATE_OPACITY);
            }
            else if (propertyName == "Propagate Lighting Settings")
            {
                return AZStd::make_any<bool>(pMaterial->GetPropagationFlags() & MTL_PROPAGATE_LIGHTING);
            }
            else if (propertyName == "Propagate Advanced Settings")
            {
                return AZStd::make_any<bool>(pMaterial->GetPropagationFlags() & MTL_PROPAGATE_ADVANCED);
            }
            else if (propertyName == "Propagate Texture Maps")
            {
                return AZStd::make_any<bool>(pMaterial->GetPropagationFlags() & MTL_PROPAGATE_TEXTURES);
            }
            else if (propertyName == "Propagate Shader Params")
            {
                return AZStd::make_any<bool>(pMaterial->GetPropagationFlags() & MTL_PROPAGATE_SHADER_PARAMS);
            }
            else if (propertyName == "Propagate Shader Generation")
            {
                return AZStd::make_any<bool>(pMaterial->GetPropagationFlags() & MTL_PROPAGATE_SHADER_GEN);
            }
            else if (propertyName == "Propagate Vertex Deformation")
            {
                return AZStd::make_any<bool>(pMaterial->GetPropagationFlags() & MTL_PROPAGATE_VERTEX_DEF);
            }
            else if (propertyName == "Propagate Layer Presets")
            {
                return AZStd::make_any<bool>(pMaterial->GetPropagationFlags() & MTL_PROPAGATE_LAYER_PRESETS);
            }
            else
            {
                throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid advanced setting.").toUtf8().data());
            }
        }
        // ########## Texture Maps ##########
        else if (categoryName == "Texture Maps")
        {
            SInputShaderResources& shaderResources = pMaterial->GetShaderResources();
            // ########## Texture Maps / [name] ##########
            if (splittedPropertyPath.size() == 2)
            {
                uint16 nSlot = aznumeric_cast<uint16>(TryConvertingCStringToEEfResTextures(propertyName));
                SEfResTexture*  pTextureRes = shaderResources.GetTextureResource(nSlot);
                if (!pTextureRes || pTextureRes->m_Name.empty())
                {
                    AZ_Warning("ShadersSystem", false, "PyGetProperty - Error: empty texture slot [%d] (or missing name) for material %s",
                        nSlot, pMaterial->GetName().toStdString().c_str());
                    return AZStd::any();
                }
                else
                {
                    return AZStd::make_any<AZStd::string>(pTextureRes->m_Name);
                }
            }
            // ########## Texture Maps / [TexType | Filter | IsProjectedTexGen | TexGenType ] ##########
            else if (splittedPropertyPath.size() == 3)
            {
                SEfResTexture* pTextureRes = shaderResources.GetTextureResource(TryConvertingCStringToEEfResTextures(subCategoryName));
                if (pTextureRes)
                {
                    if (propertyName == "TexType")
                    {
                        return AZStd::make_any<AZStd::string>(TryConvertingETEX_TypeToCString(pTextureRes->m_Sampler.m_eTexType).toLatin1().data());
                    }
                    else if (propertyName == "Filter")
                    {
                        return AZStd::make_any<AZStd::string>(TryConvertingTexFilterToCString(pTextureRes->m_Filter).toLatin1().data());
                    }
                    else if (propertyName == "IsProjectedTexGen")
                    {
                        return AZStd::make_any<bool>(pTextureRes->AddModificator()->m_bTexGenProjected);
                    }
                    else if (propertyName == "TexGenType")
                    {
                        return AZStd::make_any<AZStd::string>(TryConvertingETexGenTypeToCString(pTextureRes->AddModificator()->m_eTGType).toLatin1().data());
                    }
                    else
                    {
                        throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid property.").toUtf8().data());
                    }
                }
                else
                {
                    uint16 nSlot = aznumeric_cast<uint16>(TryConvertingCStringToEEfResTextures(subCategoryName));
                    AZ_Warning("ShadersSystem", false, "PyGetProperty - Error: empty 'subCategoryName' texture slot [%d] for material %s",
                        nSlot, pMaterial->GetName().toStdString().c_str());
                }
            }
            // ########## Texture Maps / [Tiling | Rotator | Oscillator] ##########
            else if (splittedPropertyPath.size() == 4)
            {
                SEfResTexture*  pTextureRes = shaderResources.GetTextureResource(TryConvertingCStringToEEfResTextures(subSubCategoryName));
                if (pTextureRes)
                {
                    if (subCategoryName == "Tiling")
                    {
                        if (propertyName == "IsTileU")
                        {
                            return AZStd::make_any<bool>(pTextureRes->m_bUTile);
                        }
                        else if (propertyName == "IsTileV")
                        {
                            return AZStd::make_any<bool>(pTextureRes->m_bVTile);
                        }
                        else if (propertyName == "TileU")
                        {
                            return AZStd::make_any<float>(pTextureRes->AddModificator()->m_Tiling[0]);
                        }
                        else if (propertyName == "TileV")
                        {
                            return AZStd::make_any<float>(pTextureRes->AddModificator()->m_Tiling[1]);
                        }
                        else if (propertyName == "OffsetU")
                        {
                            return AZStd::make_any<float>(pTextureRes->AddModificator()->m_Offs[0]);
                        }
                        else if (propertyName == "OffsetV")
                        {
                            return AZStd::make_any<float>(pTextureRes->AddModificator()->m_Offs[1]);
                        }
                        else if (propertyName == "RotateU")
                        {
                            return AZStd::make_any<float>(pTextureRes->AddModificator()->m_Rot[0]);
                        }
                        else if (propertyName == "RotateV")
                        {
                            return AZStd::make_any<float>(pTextureRes->AddModificator()->m_Rot[1]);
                        }
                        else if (propertyName == "RotateW")
                        {
                            return AZStd::make_any<float>(pTextureRes->AddModificator()->m_Rot[2]);
                        }
                        else
                        {
                            throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid property.").toUtf8().data());
                        }
                    }
                    else if (subCategoryName == "Rotator")
                    {
                        if (propertyName == "Type")
                        {
                            return AZStd::make_any<AZStd::string>(TryConvertingETexModRotateTypeToCString(pTextureRes->AddModificator()->m_eRotType).toLatin1().data());
                        }
                        else if (propertyName == "Rate")
                        {
                            return AZStd::make_any<float>(Word2Degr(pTextureRes->AddModificator()->m_RotOscRate[2]));
                        }
                        else if (propertyName == "Phase")
                        {
                            return AZStd::make_any<float>(Word2Degr(pTextureRes->AddModificator()->m_RotOscPhase[2]));
                        }
                        else if (propertyName == "Amplitude")
                        {
                            return AZStd::make_any<float>(Word2Degr(pTextureRes->AddModificator()->m_RotOscAmplitude[2]));
                        }
                        else if (propertyName == "CenterU")
                        {
                            return AZStd::make_any<float>(pTextureRes->AddModificator()->m_RotOscCenter[0]);
                        }
                        else if (propertyName == "CenterV")
                        {
                            return AZStd::make_any<float>(pTextureRes->AddModificator()->m_RotOscCenter[1]);
                        }
                        else
                        {
                            throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid property.").toUtf8().data());
                        }
                    }
                    else if (subCategoryName == "Oscillator")
                    {
                        if (propertyName == "TypeU")
                        {
                            return AZStd::make_any<AZStd::string>(TryConvertingETexModMoveTypeToCString(pTextureRes->AddModificator()->m_eMoveType[0]).toLatin1().data());
                        }
                        else if (propertyName == "TypeV")
                        {
                            return AZStd::make_any<AZStd::string>(TryConvertingETexModMoveTypeToCString(pTextureRes->AddModificator()->m_eMoveType[1]).toLatin1().data());
                        }
                        else if (propertyName == "RateU")
                        {
                            return AZStd::make_any<float>(pTextureRes->AddModificator()->m_OscRate[0]);

                        }
                        else if (propertyName == "RateV")
                        {
                            return AZStd::make_any<float>(pTextureRes->AddModificator()->m_OscRate[1]);
                        }
                        else if (propertyName == "PhaseU")
                        {
                            return AZStd::make_any<float>(pTextureRes->AddModificator()->m_OscPhase[0]);
                        }
                        else if (propertyName == "PhaseV")
                        {
                            return AZStd::make_any<float>(pTextureRes->AddModificator()->m_OscPhase[1]);
                        }
                        else if (propertyName == "AmplitudeU")
                        {
                            return AZStd::make_any<float>(pTextureRes->AddModificator()->m_OscAmplitude[0]);
                        }
                        else if (propertyName == "AmplitudeV")
                        {
                            return AZStd::make_any<float>(pTextureRes->AddModificator()->m_OscAmplitude[1]);
                        }
                        else
                        {
                            throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid property.").toUtf8().data());
                        }
                    }
                    else
                    {
                        throw std::runtime_error((QString("\"") + subCategoryName + "\" is an invalid sub category.").toUtf8().data());
                    }
                }
                else
                {
                    uint16 nSlot = aznumeric_cast<uint16>(TryConvertingCStringToEEfResTextures(subSubCategoryName));
                    AZ_Warning("ShadersSystem", false, "PyGetProperty - Error: empty 'subSubCategoryName' texture slot [%d] for material %s",
                        nSlot, pMaterial->GetName().toStdString().c_str());
                }
            }
            else
            {
                throw std::runtime_error(errorMsgInvalidPropertyPath.toUtf8().data());
            }
        }
        // ########## Shader Params ##########
        else if (categoryName == "Shader Params")
        {
            auto& shaderParams = pMaterial->GetShaderResources().m_ShaderParams;

            for (int i = 0; i < shaderParams.size(); i++)
            {
                if (propertyName == ParseUINameFromPublicParamsScript(shaderParams[i].m_Script.c_str()))
                {
                    if (shaderParams[i].m_Type == eType_FLOAT)
                    {
                        return AZStd::make_any<float>(shaderParams[i].m_Value.m_Float);
                    }
                    else if (shaderParams[i].m_Type == eType_FCOLOR)
                    {
                        QColor col = ColorLinearToGamma(ColorF(
                            shaderParams[i].m_Value.m_Vector[0],
                            shaderParams[i].m_Value.m_Vector[1],
                            shaderParams[i].m_Value.m_Vector[2]));
                        return AZStd::make_any<AZ::Color>(col.red(), col.green(), col.blue(), 1.0f);
                    }
                }
            }

            throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid shader param.").toUtf8().data());
        }
        // ########## Shader Generation Params ##########
        else if (categoryName == "Shader Generation Params")
        {
            for (int i = 0; i < pMaterial->GetShaderGenParamsVars()->GetNumVariables(); i++)
            {
                if (propertyName == pMaterial->GetShaderGenParamsVars()->GetVariable(i)->GetHumanName())
                {
                    // get the current Boolean value for this shader variable and return so that the std::runtime_error() will not throw to indicate failure
                    bool boolValue = false;
                    pMaterial->GetShaderGenParamsVars()->GetVariable(i)->Get(boolValue);
                    return AZStd::make_any<bool>(boolValue);
                }
            }

            // not matching property was found, so throw a std::runtime_error() to indcate an error
            throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid shader generation param.").toUtf8().data());
        }
        // ########## Vertex Deformation ##########
        else if (categoryName == "Vertex Deformation")
        {
            // ########## Vertex Deformation / [ Type | Wave Length X | Wave Length Y | Wave Length Z | Wave Length W | Noise Scale ] ##########
            if (splittedPropertyPath.size() == 2)
            {
                if (propertyName == "Type")
                {
                    return AZStd::make_any<AZStd::string>(TryConvertingEDeformTypeToCString(pMaterial->GetShaderResources().m_DeformInfo.m_eType).toLatin1().data());
                }
                else if (propertyName == "Wave Length X")
                {
                    return AZStd::make_any<float>(pMaterial->GetShaderResources().m_DeformInfo.m_fDividerX);
                }
                else if (propertyName == "Noise Scale")
                {
                    return AZStd::make_any<AZ::Vector3>(
                        pMaterial->GetShaderResources().m_DeformInfo.m_vNoiseScale[0],
                        pMaterial->GetShaderResources().m_DeformInfo.m_vNoiseScale[1],
                        pMaterial->GetShaderResources().m_DeformInfo.m_vNoiseScale[2]);
                }
                else
                {
                    throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid property.").toUtf8().data());
                }
            }
            // ########## Vertex Deformation / [ Wave X ] ##########
            else if (splittedPropertyPath.size() == 3)
            {
                if (subCategoryName == "Wave X")
                {
                    SWaveForm2 currentWaveForm;
                    if (subCategoryName == "Wave X")
                    {
                        currentWaveForm = pMaterial->GetShaderResources().m_DeformInfo.m_WaveX;
                    }

                    if (propertyName == "Type")
                    {
                        return AZStd::make_any<AZStd::string>(TryConvertingEWaveFormToCString(currentWaveForm.m_eWFType).toLatin1().data());
                    }
                    else if (propertyName == "Level")
                    {
                        return AZStd::make_any<float>(currentWaveForm.m_Level);
                    }
                    else if (propertyName == "Amplitude")
                    {
                        return AZStd::make_any<float>(currentWaveForm.m_Amp);
                    }
                    else if (propertyName == "Phase")
                    {
                        return AZStd::make_any<float>(currentWaveForm.m_Phase);
                    }
                    else if (propertyName == "Frequency")
                    {
                        return AZStd::make_any<float>(currentWaveForm.m_Freq);
                    }
                    else
                    {
                        throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid property.").toUtf8().data());
                    }
                }
                else
                {
                    throw std::runtime_error((QString("\"") + categoryName + "\" is an invalid category.").toUtf8().data());
                }
            }
            else
            {
                throw std::runtime_error(errorMsgInvalidPropertyPath.toUtf8().data());
            }
        }
        // ########## Layer Presets ##########
        else if (categoryName == "Layer Presets")
        {
            // names are "Shader1", "Shader2" and "Shader3", because all have the name "Shader" in material editor
            if (splittedPropertyPath.size() == 2)
            {
                int shaderNumber = -1;
                if (propertyName == "Shader1")
                {
                    shaderNumber = 0;
                }
                else if (propertyName == "Shader2")
                {
                    shaderNumber = 1;
                }
                else if (propertyName == "Shader3")
                {
                    shaderNumber = 2;
                }
                else
                {
                    throw std::runtime_error("Invalid shader.");
                }

                return AZStd::make_any<AZStd::string>(pMaterial->GetMtlLayerResources()[shaderNumber].m_shaderName.toLatin1().data());
            }
            else if (splittedPropertyPath.size() == 3)
            {
                if (propertyName == "No Draw")
                {
                    int shaderNumber = -1;
                    if (subCategoryName == "Shader1")
                    {
                        shaderNumber = 0;
                    }
                    else if (subCategoryName == "Shader2")
                    {
                        shaderNumber = 1;
                    }
                    else if (subCategoryName == "Shader3")
                    {
                        shaderNumber = 2;
                    }
                    else
                    {
                        throw std::runtime_error("Invalid shader.");
                    }
                    return AZStd::make_any<bool>(pMaterial->GetMtlLayerResources()[shaderNumber].m_nFlags & MTL_LAYER_USAGE_NODRAW);
                }
            }
        }

        throw std::runtime_error(errorMsgInvalidPropertyPath.toUtf8().data());
        return AZStd::any();
    }

    void PySetProperty(const char* pPathAndMaterialName, const char* pPathAndPropertyName, const AZStd::any& value)
    {
        CMaterial* pMaterial = TryLoadingMaterial(pPathAndMaterialName);
        std::deque<QString> splittedPropertyPath = PreparePropertyPath(pPathAndPropertyName);
        std::deque<QString> splittedPropertyPathCategory = splittedPropertyPath;
        QString categoryName = splittedPropertyPath.front();
        QString subCategoryName = "None";
        QString subSubCategoryName = "None";
        QString propertyName = splittedPropertyPath.back();
        QString errorMsgInvalidPropertyPath = "Invalid property path.";

        if (!ValidateProperty(pMaterial, splittedPropertyPath, value))
        {
            throw std::runtime_error("Invalid property.");
        }

        QString undoMsg = "Set Material Property";
        CUndo undo(undoMsg.toUtf8().data());
        pMaterial->RecordUndo(undoMsg.toUtf8().data(), true);

        if (splittedPropertyPathCategory.size() == 3)
        {
            splittedPropertyPathCategory.pop_back();
            subCategoryName = splittedPropertyPathCategory.back();
        }
        else if (splittedPropertyPathCategory.size() == 4)
        {
            splittedPropertyPathCategory.pop_back();
            subCategoryName = splittedPropertyPathCategory.back();
            splittedPropertyPathCategory.pop_back();
            subSubCategoryName = splittedPropertyPathCategory.back();
        }

        // ########## Material Settings ##########
        if (categoryName == "Material Settings")
        {
            if (propertyName == "Shader")
            {
                pMaterial->SetShaderName(AZStd::any_cast<AZStd::string_view>(value).data());
            }
            else if (propertyName == "Surface Type")
            {
                bool isSurfaceExist(false);
                QString realSurfacename = "";
                ISurfaceTypeEnumerator* pSurfaceTypeEnum = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager()->GetEnumerator();
                if (pSurfaceTypeEnum)
                {
                    for (ISurfaceType* pSurfaceType = pSurfaceTypeEnum->GetFirst(); pSurfaceType; pSurfaceType = pSurfaceTypeEnum->GetNext())
                    {
                        QString surfaceName = pSurfaceType->GetName();
                        realSurfacename = surfaceName;
                        if (surfaceName.left(4) == "mat_")
                        {
                            surfaceName.remove(0, 4);
                        }
                        if (surfaceName == AZStd::any_cast<AZStd::string_view>(value).data())
                        {
                            isSurfaceExist = true;
                            pMaterial->SetSurfaceTypeName(realSurfacename);
                        }
                    }

                    if (!isSurfaceExist)
                    {
                        throw std::runtime_error("Invalid surface type name.");
                    }
                }
                else
                {
                    throw std::runtime_error("Surface Type Enumerator corrupted.");
                }
            }
        }
        // ########## Opacity Settings ##########
        else if (categoryName == "Opacity Settings")
        {
            if (propertyName == "Opacity")
            {
                pMaterial->GetShaderResources().m_LMaterial.m_Opacity = PyFetchNumericType<float>(value) / 100.0f;
            }
            else if (propertyName == "AlphaTest")
            {
                pMaterial->GetShaderResources().m_AlphaRef = PyFetchNumericType<float>(value) / 100.0f;
            }
            else if (propertyName == "Additive")
            {
                SetMaterialFlag(pMaterial, MTL_FLAG_ADDITIVE, PyFetchNumericType<bool>(value));
            }
        }
        // ########## Lighting Settings ##########
        else if (categoryName == "Lighting Settings")
        {
            if (propertyName == "Diffuse Color")
            {
                const AZ::Color* color = AZStd::any_cast<AZ::Color>(&value);
                pMaterial->GetShaderResources().m_LMaterial.m_Diffuse = ColorGammaToLinear(QColor(color->GetR8(), color->GetG8(), color->GetB8()));
            }
            else if (propertyName == "Specular Color")
            {
                const AZ::Color* color = AZStd::any_cast<AZ::Color>(&value);
                ColorF colorFloat = ColorGammaToLinear(QColor(color->GetR8(), color->GetG8(), color->GetB8()));
                colorFloat.a = pMaterial->GetShaderResources().m_LMaterial.m_Specular.a;
                colorFloat.r *= colorFloat.a;
                colorFloat.g *= colorFloat.a;
                colorFloat.b *= colorFloat.a;
                pMaterial->GetShaderResources().m_LMaterial.m_Specular = colorFloat;
            }
            else if (propertyName == "Glossiness" || propertyName == "Smoothness")
            {
                pMaterial->GetShaderResources().m_LMaterial.m_Smoothness = PyFetchNumericType<float>(value);
            }
            else if (propertyName == "Specular Level")
            {
                const float localVariableAlpha = PyFetchNumericType<float>(value);
                ColorF colorFloat = pMaterial->GetShaderResources().m_LMaterial.m_Specular;
                colorFloat.r *= localVariableAlpha;
                colorFloat.g *= localVariableAlpha;
                colorFloat.b *= localVariableAlpha;
                colorFloat.a = 1.0f;
                pMaterial->GetShaderResources().m_LMaterial.m_Specular = colorFloat;
            }
            else if (propertyName == "Emissive Color")
            {
                const AZ::Color* color = AZStd::any_cast<AZ::Color>(&value);
                float emissiveIntensity = pMaterial->GetShaderResources().m_LMaterial.m_Emittance.a;
                pMaterial->GetShaderResources().m_LMaterial.m_Emittance = ColorGammaToLinear(QColor(color->GetR8(), color->GetG8(), color->GetB8()));
                pMaterial->GetShaderResources().m_LMaterial.m_Emittance.a = emissiveIntensity;
            }
            else if (propertyName == "Emissive Intensity")
            {
                pMaterial->GetShaderResources().m_LMaterial.m_Emittance.a = PyFetchNumericType<float>(value);
            }
        }
        // ########## Advanced ##########
        else if (categoryName == "Advanced")
        {
            if (propertyName == "Allow layer activation")
            {
                pMaterial->SetLayerActivation(PyFetchNumericType<bool>(value));
            }
            else if (propertyName == "2 Sided")
            {
                SetMaterialFlag(pMaterial, MTL_FLAG_2SIDED, PyFetchNumericType<bool>(value));
            }
            else if (propertyName == "No Shadow")
            {
                SetMaterialFlag(pMaterial, MTL_FLAG_NOSHADOW, PyFetchNumericType<bool>(value));
            }
            else if (propertyName == "Use Scattering")
            {
                SetMaterialFlag(pMaterial, MTL_FLAG_SCATTER, PyFetchNumericType<bool>(value));
            }
            else if (propertyName == "Hide After Breaking")
            {
                SetMaterialFlag(pMaterial, MTL_FLAG_HIDEONBREAK, PyFetchNumericType<bool>(value));
            }
            else if (propertyName == "Fog Volume Shading Quality High")
            {
                SetMaterialFlag(pMaterial, MTL_FLAG_FOG_VOLUME_SHADING_QUALITY_HIGH, PyFetchNumericType<bool>(value));
            }
            else if (propertyName == "Blend Terrain Color")
            {
                SetMaterialFlag(pMaterial, MTL_FLAG_BLEND_TERRAIN, PyFetchNumericType<bool>(value));
            }
            else if (propertyName == "Voxel Coverage")
            {
                pMaterial->GetShaderResources().m_VoxelCoverage = aznumeric_cast<uint8>(PyFetchNumericType<float>(value) * 255.0f);
            }
            else if (propertyName == "Link to Material")
            {
                pMaterial->GetMatInfo()->SetMaterialLinkName(AZStd::any_cast<AZStd::string_view>(value).data());
            }
            else if (propertyName == "Propagate Material Settings")
            {
                SetPropagationFlag(pMaterial, MTL_PROPAGATE_MATERIAL_SETTINGS, PyFetchNumericType<bool>(value));
            }
            else if (propertyName == "Propagate Opacity Settings")
            {
                SetPropagationFlag(pMaterial, MTL_PROPAGATE_OPACITY, PyFetchNumericType<bool>(value));
            }
            else if (propertyName == "Propagate Lighting Settings")
            {
                SetPropagationFlag(pMaterial, MTL_PROPAGATE_LIGHTING, PyFetchNumericType<bool>(value));
            }
            else if (propertyName == "Propagate Advanced Settings")
            {
                SetPropagationFlag(pMaterial, MTL_PROPAGATE_ADVANCED, PyFetchNumericType<bool>(value));
            }
            else if (propertyName == "Propagate Texture Maps")
            {
                SetPropagationFlag(pMaterial, MTL_PROPAGATE_TEXTURES, PyFetchNumericType<bool>(value));
            }
            else if (propertyName == "Propagate Shader Params")
            {
                if (PyFetchNumericType<bool>(value))
                {
                    SetPropagationFlag(pMaterial, MTL_PROPAGATE_MATERIAL_SETTINGS, true);
                }
                SetPropagationFlag(pMaterial, MTL_PROPAGATE_SHADER_PARAMS, PyFetchNumericType<bool>(value));
            }
            else if (propertyName == "Propagate Shader Generation")
            {
                if (PyFetchNumericType<bool>(value))
                {
                    SetPropagationFlag(pMaterial, MTL_PROPAGATE_MATERIAL_SETTINGS, true);
                }
                SetPropagationFlag(pMaterial, MTL_PROPAGATE_SHADER_GEN, PyFetchNumericType<bool>(value));
            }
            else if (propertyName == "Propagate Vertex Deformation")
            {
                SetPropagationFlag(pMaterial, MTL_PROPAGATE_VERTEX_DEF, PyFetchNumericType<bool>(value));
            }
            else if (propertyName == "Propagate Layer Presets")
            {
                SetPropagationFlag(pMaterial, MTL_PROPAGATE_LAYER_PRESETS, PyFetchNumericType<bool>(value));
            }
        }
        // ########## Texture Maps ##########
        else if (categoryName == "Texture Maps")
        {
            // ########## Texture Maps / [name] ##########
            SInputShaderResources& shaderResources = pMaterial->GetShaderResources();
            if (splittedPropertyPath.size() == 2)
            {
                uint16 nSlot = aznumeric_cast<uint16>(TryConvertingCStringToEEfResTextures(propertyName));
                auto stringValue = AZStd::any_cast<AZStd::string_view>(value);
                if (stringValue.empty())
                {
                    AZ_Warning("ShadersSystem", false, "PySetProperty - Error: empty texture [%d] name for material %s",
                        nSlot, pMaterial->GetName().toStdString().c_str());
                }
                // notice that the following is an insertion operation if the index did not exist in the map
                shaderResources.m_TexturesResourcesMap[nSlot].m_Name = stringValue.data();
            }
            // ########## Texture Maps / [TexType | Filter | IsProjectedTexGen | TexGenType ] ##########
            else if (splittedPropertyPath.size() == 3)
            {
                uint16 nSlot = aznumeric_cast<uint16>(TryConvertingCStringToEEfResTextures(subCategoryName));
                // notice that each of the following will add the texture slot if did not exist yet
                if (propertyName == "TexType")
                {
                    shaderResources.m_TexturesResourcesMap[nSlot].m_Sampler.m_eTexType = TryConvertingCStringToETEX_Type(AZStd::any_cast<AZStd::string_view>(value));
                }
                else if (propertyName == "Filter")
                {
                    shaderResources.m_TexturesResourcesMap[nSlot].m_Filter = TryConvertingCStringToTexFilter(AZStd::any_cast<AZStd::string_view>(value));
                }
                else if (propertyName == "IsProjectedTexGen")
                {
                    shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_bTexGenProjected = PyFetchNumericType<bool>(value);
                }
                else if (propertyName == "TexGenType")
                {
                    shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_eTGType = TryConvertingCStringToETexGenType(AZStd::any_cast<AZStd::string_view>(value));
                }
            }
            // ########## Texture Maps / [Tiling | Rotator | Oscillator] ##########
            else if (splittedPropertyPath.size() == 4)
            {
                uint16 nSlot = aznumeric_cast<uint16>(TryConvertingCStringToEEfResTextures(subSubCategoryName));
                if (subCategoryName == "Tiling")
                {
                    if (propertyName == "IsTileU")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].m_bUTile = PyFetchNumericType<bool>(value);
                    }
                    else if (propertyName == "IsTileV")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].m_bVTile = PyFetchNumericType<bool>(value);
                    }
                    else if (propertyName == "TileU")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_Tiling[0] = PyFetchNumericType<float>(value);
                    }
                    else if (propertyName == "TileV")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_Tiling[1] = PyFetchNumericType<float>(value);
                    }
                    else if (propertyName == "OffsetU")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_Offs[0] = PyFetchNumericType<float>(value);
                    }
                    else if (propertyName == "OffsetV")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_Offs[1] = PyFetchNumericType<float>(value);
                    }
                    else if (propertyName == "RotateU")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_Rot[0] = Degr2Word(PyFetchNumericType<float>(value));
                    }
                    else if (propertyName == "RotateV")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_Rot[1] = Degr2Word(PyFetchNumericType<float>(value));
                    }
                    else if (propertyName == "RotateW")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_Rot[2] = Degr2Word(PyFetchNumericType<float>(value));
                    }
                }
                else if (subCategoryName == "Rotator")
                {
                    if (propertyName == "Type")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_eRotType = TryConvertingCStringToETexModRotateType(AZStd::any_cast<AZStd::string_view>(value));
                    }
                    else if (propertyName == "Rate")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_RotOscRate[2] = Degr2Word(PyFetchNumericType<float>(value));
                    }
                    else if (propertyName == "Phase")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_RotOscPhase[2] = Degr2Word(PyFetchNumericType<float>(value));
                    }
                    else if (propertyName == "Amplitude")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_RotOscAmplitude[2] = Degr2Word(PyFetchNumericType<float>(value));
                    }
                    else if (propertyName == "CenterU")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_RotOscCenter[0] = PyFetchNumericType<float>(value);
                    }
                    else if (propertyName == "CenterV")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_RotOscCenter[1] = PyFetchNumericType<float>(value);
                    }
                }
                else if (subCategoryName == "Oscillator")
                {
                    if (propertyName == "TypeU")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_eMoveType[0] = TryConvertingCStringToETexModMoveType(AZStd::any_cast<AZStd::string_view>(value));
                    }
                    else if (propertyName == "TypeV")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_eMoveType[1] = TryConvertingCStringToETexModMoveType(AZStd::any_cast<AZStd::string_view>(value));
                    }
                    else if (propertyName == "RateU")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_OscRate[0] = PyFetchNumericType<float>(value);
                    }
                    else if (propertyName == "RateV")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_OscRate[1] = PyFetchNumericType<float>(value);
                    }
                    else if (propertyName == "PhaseU")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_OscPhase[0] = PyFetchNumericType<float>(value);
                    }
                    else if (propertyName == "PhaseV")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_OscPhase[1] = PyFetchNumericType<float>(value);
                    }
                    else if (propertyName == "AmplitudeU")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_OscAmplitude[0] = PyFetchNumericType<float>(value);
                    }
                    else if (propertyName == "AmplitudeV")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_OscAmplitude[1] = PyFetchNumericType<float>(value);
                    }
                }
            }
        }
        // ########## Shader Params ##########
        else if (categoryName == "Shader Params")
        {
            auto& shaderParams = pMaterial->GetShaderResources().m_ShaderParams;

            for (int i = 0; i < shaderParams.size(); i++)
            {
                if (propertyName == ParseUINameFromPublicParamsScript(shaderParams[i].m_Script.c_str()))
                {
                    if (shaderParams[i].m_Type == eType_FLOAT)
                    {
                        shaderParams[i].m_Value.m_Float = PyFetchNumericType<float>(value);
                        break;
                    }
                    else if (shaderParams[i].m_Type == eType_FCOLOR)
                    {
                        const AZ::Color* color = AZStd::any_cast<AZ::Color>(&value);
                        ColorF colorLinear = ColorGammaToLinear(QColor(color->GetR8(), color->GetG8(), color->GetB8()));
                        shaderParams[i].m_Value.m_Vector[0] = colorLinear.r;
                        shaderParams[i].m_Value.m_Vector[1] = colorLinear.g;
                        shaderParams[i].m_Value.m_Vector[2] = colorLinear.b;
                        break;
                    }
                    else
                    {
                        throw std::runtime_error("Invalid data type (Shader Params)");
                    }
                }
            }
        }
        // ########## Shader Generation Params ##########
        else if (categoryName == "Shader Generation Params")
        {
            for (int i = 0; i < pMaterial->GetShaderGenParamsVars()->GetNumVariables(); i++)
            {
                if (propertyName == pMaterial->GetShaderGenParamsVars()->GetVariable(i)->GetHumanName())
                {
                    CVarBlock* shaderGenBlock = pMaterial->GetShaderGenParamsVars();
                    shaderGenBlock->GetVariable(i)->Set(PyFetchNumericType<bool>(value));
                    pMaterial->SetShaderGenParamsVars(shaderGenBlock);
                    break;
                }
            }
        }
        // ########## Vertex Deformation ##########
        else if (categoryName == "Vertex Deformation")
        {
            // ########## Vertex Deformation / [ Type | Wave Length X | Noise Scale ] ##########
            if (splittedPropertyPath.size() == 2)
            {
                if (propertyName == "Type")
                {
                    pMaterial->GetShaderResources().m_DeformInfo.m_eType = TryConvertingCStringToEDeformType(AZStd::any_cast<AZStd::string_view>(value));
                }
                else if (propertyName == "Wave Length X")
                {
                    pMaterial->GetShaderResources().m_DeformInfo.m_fDividerX = PyFetchNumericType<float>(value);
                }
                else if (propertyName == "Noise Scale")
                {
                    const AZ::Vector3* vecValue = AZStd::any_cast<AZ::Vector3>(&value);
                    pMaterial->GetShaderResources().m_DeformInfo.m_vNoiseScale[0] = vecValue->GetX();
                    pMaterial->GetShaderResources().m_DeformInfo.m_vNoiseScale[1] = vecValue->GetY();
                    pMaterial->GetShaderResources().m_DeformInfo.m_vNoiseScale[2] = vecValue->GetZ();
                }
            }
            // ########## Vertex Deformation / [ Wave X ] ##########
            else if (splittedPropertyPath.size() == 3)
            {
                if (subCategoryName == "Wave X")
                {
                    SWaveForm2& currentWaveForm = pMaterial->GetShaderResources().m_DeformInfo.m_WaveX;

                    if (propertyName == "Type")
                    {
                        currentWaveForm.m_eWFType = TryConvertingCStringToEWaveForm(AZStd::any_cast<AZStd::string_view>(value));
                    }
                    else if (propertyName == "Level")
                    {
                        currentWaveForm.m_Level = PyFetchNumericType<float>(value);
                    }
                    else if (propertyName == "Amplitude")
                    {
                        currentWaveForm.m_Amp = PyFetchNumericType<float>(value);
                    }
                    else if (propertyName == "Phase")
                    {
                        currentWaveForm.m_Phase = PyFetchNumericType<float>(value);
                    }
                    else if (propertyName == "Frequency")
                    {
                        currentWaveForm.m_Freq = PyFetchNumericType<float>(value);
                    }
                }
            }
        }
        // ########## Layer Presets ##########
        else if (categoryName == "Layer Presets")
        {
            // names are "Shader1", "Shader2" and "Shader3", because all have the name "Shader" in material editor
            if (splittedPropertyPath.size() == 2)
            {
                int shaderNumber = -1;
                if (propertyName == "Shader1")
                {
                    shaderNumber = 0;
                }
                else if (propertyName == "Shader2")
                {
                    shaderNumber = 1;
                }
                else if (propertyName == "Shader3")
                {
                    shaderNumber = 2;
                }

                pMaterial->GetMtlLayerResources()[shaderNumber].m_shaderName = AZStd::any_cast<AZStd::string_view>(value).data();
            }
            else if (splittedPropertyPath.size() == 3)
            {
                if (propertyName == "No Draw")
                {
                    int shaderNumber = -1;
                    if (subCategoryName == "Shader1")
                    {
                        shaderNumber = 0;
                    }
                    else if (subCategoryName == "Shader2")
                    {
                        shaderNumber = 1;
                    }
                    else if (subCategoryName == "Shader3")
                    {
                        shaderNumber = 2;
                    }

                    if (pMaterial->GetMtlLayerResources()[shaderNumber].m_nFlags & MTL_LAYER_USAGE_NODRAW && PyFetchNumericType<bool>(value) == false)
                    {
                        pMaterial->GetMtlLayerResources()[shaderNumber].m_nFlags = pMaterial->GetMtlLayerResources()[shaderNumber].m_nFlags - MTL_LAYER_USAGE_NODRAW;
                    }
                    else if (!(pMaterial->GetMtlLayerResources()[shaderNumber].m_nFlags & MTL_LAYER_USAGE_NODRAW) && PyFetchNumericType<bool>(value) == true)
                    {
                        pMaterial->GetMtlLayerResources()[shaderNumber].m_nFlags = pMaterial->GetMtlLayerResources()[shaderNumber].m_nFlags | MTL_LAYER_USAGE_NODRAW;
                    }
                }
            }
        }

        pMaterial->Update();
        pMaterial->Save();
        GetIEditor()->GetMaterialManager()->OnUpdateProperties(pMaterial, true);
    }
}

namespace AzToolsFramework
{
    void MaterialPythonFuncsHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // this will put these methods into the 'azlmbr.legacy.material' module
            auto addLegacyMaterial = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Legacy/Material")
                    ->Attribute(AZ::Script::Attributes::Module, "legacy.material");
            };
            addLegacyMaterial(behaviorContext->Method("create", PyMaterialCreate, nullptr, "Creates a material."));
            addLegacyMaterial(behaviorContext->Method("create_multi", PyMaterialCreateMulti, nullptr, "Creates a multi-material."));
            addLegacyMaterial(behaviorContext->Method("convert_to_multi", PyMaterialConvertToMulti, nullptr, "Converts the selected material to a multi-material."));
            addLegacyMaterial(behaviorContext->Method("duplicate_current", PyMaterialDuplicateCurrent, nullptr, "Duplicates the current material."));
            addLegacyMaterial(behaviorContext->Method("merge_selection", PyMaterialMergeSelection, nullptr, "Merges the selected materials."));
            addLegacyMaterial(behaviorContext->Method("delete_current", PyMaterialDeleteCurrent, nullptr, "Deletes the current material."));
            addLegacyMaterial(behaviorContext->Method("get_submaterial", PyGetSubMaterial, nullptr, "Gets sub materials of a material."));
            addLegacyMaterial(behaviorContext->Method("get_property", PyGetProperty, nullptr, "Gets a property of a material."));
            addLegacyMaterial(behaviorContext->Method("set_property", PySetProperty, nullptr, "Sets a property of a material."));
        }
    }
}
