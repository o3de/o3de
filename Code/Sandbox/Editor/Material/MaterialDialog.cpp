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

#include "EditorDefs.h"

#include "MaterialDialog.h"

// Qt
#include <QStatusBar>
#include <QComboBox>
#include <QToolBar>
#include <QMenuBar>
#include <QSplitter>
#include <QAbstractEventDispatcher>
#include <QMessageBox>
#include <QLabel>

// AzToolsFramework
#include <AzToolsFramework/API/ViewPaneOptions.h>       // for AzToolsFramework::ViewPaneOptions

// Editor
#include "IEditor.h"
#include "EditTool.h"
#include "MaterialImageListCtrl.h"
#include "MaterialManager.h"
#include "MaterialHelpers.h"
#include "ShaderEnum.h"
#include "MatEditPreviewDlg.h"
#include "Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.h"
#include "Include/IObjectManager.h"
#include "Objects/BaseObject.h"
#include "Settings.h"
#include "Objects/SelectionGroup.h"
#include "LyViewPaneNames.h"


const QString EDITOR_OBJECTS_PATH("Objects\\Editor\\");

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions opts;
    opts.shortcut = QKeySequence(Qt::Key_M);
    opts.canHaveMultipleInstances = true;

    AzToolsFramework::RegisterViewPane<CMaterialDialog>(MATERIAL_EDITOR_NAME, LyViewPane::CategoryTools, opts);

    GetIEditor()->GetSettingsManager()->AddToolVersion(MATERIAL_EDITOR_NAME, MATERIAL_EDITOR_VER);
}

const GUID& CMaterialDialog::GetClassID()
{
    static const GUID guid =
    {
        0xc7891863, 0x1665, 0x45ac, { 0xae, 0x51, 0x48, 0x66, 0x71, 0xbc, 0x8b, 0x12 }
    };
    return guid;
}

inline float RoundDegree(float val)
{
    return (float)((int)(val * 100 + 0.5f)) * 0.01f;
}

//////////////////////////////////////////////////////////////////////////
// Material structures.
//////////////////////////////////////////////////////////////////////////

#ifndef _countof
#define _countof(array) (sizeof(array) / sizeof(array[0]))
#endif

struct STextureVars
{
    CSmartVariable<bool> is_tile[2];

    CSmartVariableEnum<int> etcgentype;
    CSmartVariableEnum<int> etcmrotatetype;
    CSmartVariableEnum<int> etcmumovetype;
    CSmartVariableEnum<int> etcmvmovetype;
    CSmartVariableEnum<int> etextype;
    CSmartVariableEnum<int> filter;

    CSmartVariable<bool> is_tcgprojected;
    CSmartVariable<float> tiling[3];
    CSmartVariable<float> rotate[3];
    CSmartVariable<float> offset[3];
    CSmartVariable<float> tcmuoscrate;
    CSmartVariable<float> tcmvoscrate;
    CSmartVariable<float> tcmuoscamplitude;
    CSmartVariable<float> tcmvoscamplitude;
    CSmartVariable<float> tcmuoscphase;
    CSmartVariable<float> tcmvoscphase;
    CSmartVariable<float> tcmrotoscrate;
    CSmartVariable<float> tcmrotoscamplitude;
    CSmartVariable<float> tcmrotoscphase;
    CSmartVariable<float> tcmrotosccenter[2];

    CSmartVariableArray tableTiling;
    CSmartVariableArray tableOscillator;
    CSmartVariableArray tableRotator;

    void Reset()
    {
        SEfTexModificator defaultTextureCoordinateModifier;
        SEfResTexture defaultTextureResource;
        for (int i = 0; i < 2; i++)
        {
            *is_tile[i] = defaultTextureResource.GetTiling(i);
            *tcmrotosccenter[i] = defaultTextureCoordinateModifier.m_RotOscCenter[i];
        }

        for (int i = 0; i < 3; i++)
        {
            *rotate[i] = RoundDegree(Word2Degr(defaultTextureCoordinateModifier.m_Rot[i]));
            *tiling[i] = defaultTextureCoordinateModifier.m_Tiling[i];
            *offset[i] = defaultTextureCoordinateModifier.m_Offs[i];
        }

        etcgentype = defaultTextureCoordinateModifier.m_eTGType;
        etcmrotatetype = defaultTextureCoordinateModifier.m_eRotType;
        etcmumovetype = defaultTextureCoordinateModifier.m_eMoveType[0];
        etcmvmovetype = defaultTextureCoordinateModifier.m_eMoveType[1];
        etextype = defaultTextureResource.m_Sampler.m_eTexType;
        filter = defaultTextureResource.m_Filter;
        is_tcgprojected = defaultTextureCoordinateModifier.m_bTexGenProjected;

        tcmuoscrate = defaultTextureCoordinateModifier.m_OscRate[0];
        tcmvoscrate = defaultTextureCoordinateModifier.m_OscRate[1];

        tcmuoscamplitude = defaultTextureCoordinateModifier.m_OscAmplitude[0];
        tcmvoscamplitude = defaultTextureCoordinateModifier.m_OscAmplitude[1];

        tcmuoscphase = defaultTextureCoordinateModifier.m_OscPhase[0];
        tcmvoscphase = defaultTextureCoordinateModifier.m_OscPhase[1];

        tcmrotoscrate = RoundDegree(Word2Degr(defaultTextureCoordinateModifier.m_RotOscRate[2]));
        tcmrotoscamplitude = RoundDegree(Word2Degr(defaultTextureCoordinateModifier.m_RotOscAmplitude[2]));
        tcmrotoscphase = RoundDegree(Word2Degr(defaultTextureCoordinateModifier.m_RotOscPhase[2]));
    }
};

struct SMaterialLayerVars
{
    CSmartVariable<bool> bNoDraw; // disable layer rendering (useful in some cases)
    CSmartVariable<bool> bFadeOut; // fade out layer rendering and parent rendering
    CSmartVariableEnum<QString> shader; // shader layer name
};

struct SVertexWaveFormUI
{
    CSmartVariableArray table;
    CSmartVariableEnum<int> waveFormType;
    CSmartVariable<float> level;
    CSmartVariable<float> amplitude;
    CSmartVariable<float> phase;
    CSmartVariable<float> frequency;
};

//////////////////////////////////////////////////////////////////////////
struct SVertexModUI
{
    CSmartVariableEnum<int> type;
    CSmartVariable<float>   fDividerX;
    CSmartVariable<float>   fDividerY;
    CSmartVariable<float>   fDividerZ;
    CSmartVariable<float>   fDividerW;
    CSmartVariable<Vec3>    vNoiseScale;
    SVertexWaveFormUI wave;
};

/** User Interface definition of material.
*/
class CMaterialUI
{
public:
    CSmartVariableEnum<QString> shader;
    CSmartVariable<bool> bNoShadow;
    CSmartVariable<bool> bAdditive;
    CSmartVariable<bool> bWire;
    CSmartVariable<bool> b2Sided;
    CSmartVariable<float> opacity;
    CSmartVariable<float> alphaTest;
    CSmartVariable<float> emissiveIntensity;
    CSmartVariable<float> voxelCoverage;
    CSmartVariable<float> heatAmount;
    CSmartVariable<bool> bScatter;
    CSmartVariable<bool> bHideAfterBreaking;
    CSmartVariable<bool> bFogVolumeShadingQualityHigh;
    CSmartVariable<bool> bBlendTerrainColor;
    //CSmartVariable<bool> bTranslucenseLayer;
    CSmartVariableEnum<QString> surfaceType;

    CSmartVariable<bool> allowLayerActivation;

    //////////////////////////////////////////////////////////////////////////
    // Material Value Propagation for dynamic material switches, as for instance
    // used by breakable glass
    //////////////////////////////////////////////////////////////////////////
    CSmartVariableEnum<QString> matPropagate;
    CSmartVariable<bool> bPropagateMaterialSettings;
    CSmartVariable<bool> bPropagateOpactity;
    CSmartVariable<bool> bPropagateLighting;
    CSmartVariable<bool> bPropagateAdvanced;
    CSmartVariable<bool> bPropagateTexture;
    CSmartVariable<bool> bPropagateVertexDef;
    CSmartVariable<bool> bPropagateShaderParams;
    CSmartVariable<bool> bPropagateLayerPresets;
    CSmartVariable<bool> bPropagateShaderGenParams;

    //////////////////////////////////////////////////////////////////////////
    // Lighting
    //////////////////////////////////////////////////////////////////////////
    CSmartVariable<Vec3> diffuse;               // Diffuse color 0..1
    CSmartVariable<Vec3> specular;              // Specular color 0..1
    CSmartVariable<float> smoothness;       // Specular shininess.
    CSmartVariable<Vec3> emissiveCol;       // Emissive color 0..1

    //////////////////////////////////////////////////////////////////////////
    // Textures.
    //////////////////////////////////////////////////////////////////////////
    CSmartVariableArray     textureVars[EFTT_MAX];
    CSmartVariableArray     advancedTextureGroup[EFTT_MAX];
    STextureVars            textures[EFTT_MAX];

    //////////////////////////////////////////////////////////////////////////
    // Material layers settings
    //////////////////////////////////////////////////////////////////////////

    // 8 max for now. change this later
    SMaterialLayerVars materialLayers[MTL_LAYER_MAX_SLOTS];

    //////////////////////////////////////////////////////////////////////////

    SVertexModUI vertexMod;

    CSmartVariableArray tableShader;
    CSmartVariableArray tableOpacity;
    CSmartVariableArray tableLighting;
    CSmartVariableArray tableTexture;
    CSmartVariableArray tableAdvanced;
    CSmartVariableArray tableVertexMod;
    CSmartVariableArray tableEffects;

    CSmartVariableArray tableShaderParams;
    CSmartVariableArray tableShaderGenParams;

    CVarEnumList<int>* enumTexType;
    CVarEnumList<int>* enumTexGenType;
    CVarEnumList<int>* enumTexModRotateType;
    CVarEnumList<int>* enumTexModUMoveType;
    CVarEnumList<int>* enumTexModVMoveType;
    CVarEnumList<int>* enumTexFilterType;

    CVarEnumList<int>* enumVertexMod;
    CVarEnumList<int>* enumWaveType;

    //////////////////////////////////////////////////////////////////////////
    int texUsageMask;

    CVarBlockPtr m_vars;

    typedef std::map<QString, MaterialChangeFlags> TVarChangeNotifications;
    TVarChangeNotifications m_varChangeNotifications;

    //////////////////////////////////////////////////////////////////////////
    void SetFromMaterial(CMaterial* mtl);
    void SetToMaterial(CMaterial* mtl, int propagationFlags = MTL_PROPAGATE_ALL);
    void SetTextureNames(CMaterial* mtl);

    void SetShaderResources(const SInputShaderResources& srTextures, bool bSetTextures = true);
    void GetShaderResources(SInputShaderResources& sr, int propagationFlags);

    void SetVertexDeform(const SInputShaderResources& sr);
    void GetVertexDeform(SInputShaderResources& sr, int propagationFlags);

    void PropagateFromLinkedMaterial(CMaterial* mtl);
    void PropagateToLinkedMaterial(CMaterial* mtl, CVarBlockPtr pShaderParamsBlock);
    void NotifyObjectsAboutMaterialChange(IVariable* var);


    //////////////////////////////////////////////////////////////////////////
    CMaterialUI()
    {
    }

    ~CMaterialUI()
    {
    }

    //////////////////////////////////////////////////////////////////////////
    CVarBlock* CreateVars()
    {
        m_vars = new CVarBlock;

        //////////////////////////////////////////////////////////////////////////
        // Init enums.
        //////////////////////////////////////////////////////////////////////////
        enumTexType = new CVarEnumList<int>();
        enumTexType->AddItem("2D", eTT_2D);
        enumTexType->AddItem("Cube-Map", eTT_Cube);
        enumTexType->AddItem("Nearest Cube-Map probe for alpha blended", eTT_NearestCube);
        enumTexType->AddItem("Dynamic 2D-Map", eTT_Dyn2D);
        enumTexType->AddItem("From User Params", eTT_User);

        enumTexGenType = new CVarEnumList<int>();
        enumTexGenType->AddItem("Stream", ETG_Stream);
        enumTexGenType->AddItem("World", ETG_World);
        enumTexGenType->AddItem("Camera", ETG_Camera);

        enumTexModRotateType = new CVarEnumList<int>();
        enumTexModRotateType->AddItem("No Change", ETMR_NoChange);
        enumTexModRotateType->AddItem("Fixed Rotation", ETMR_Fixed);
        enumTexModRotateType->AddItem("Constant Rotation", ETMR_Constant);
        enumTexModRotateType->AddItem("Oscillated Rotation", ETMR_Oscillated);

        enumTexModUMoveType = new CVarEnumList<int>();
        enumTexModUMoveType->AddItem("No Change", ETMM_NoChange);
        enumTexModUMoveType->AddItem("Fixed Moving", ETMM_Fixed);
        enumTexModUMoveType->AddItem("Constant Moving", ETMM_Constant);
        enumTexModUMoveType->AddItem("Jitter Moving", ETMM_Jitter);
        enumTexModUMoveType->AddItem("Pan Moving", ETMM_Pan);
        enumTexModUMoveType->AddItem("Stretch Moving", ETMM_Stretch);
        enumTexModUMoveType->AddItem("Stretch-Repeat Moving", ETMM_StretchRepeat);

        enumTexModVMoveType = new CVarEnumList<int>();
        enumTexModVMoveType->AddItem("No Change", ETMM_NoChange);
        enumTexModVMoveType->AddItem("Fixed Moving", ETMM_Fixed);
        enumTexModVMoveType->AddItem("Constant Moving", ETMM_Constant);
        enumTexModVMoveType->AddItem("Jitter Moving", ETMM_Jitter);
        enumTexModVMoveType->AddItem("Pan Moving", ETMM_Pan);
        enumTexModVMoveType->AddItem("Stretch Moving", ETMM_Stretch);
        enumTexModVMoveType->AddItem("Stretch-Repeat Moving", ETMM_StretchRepeat);

        enumTexFilterType = new CVarEnumList<int>();
        enumTexFilterType->AddItem("Default", FILTER_NONE);
        enumTexFilterType->AddItem("Point", FILTER_POINT);
        enumTexFilterType->AddItem("Linear", FILTER_LINEAR);
        enumTexFilterType->AddItem("Bilinear", FILTER_BILINEAR);
        enumTexFilterType->AddItem("Trilinear", FILTER_TRILINEAR);
        enumTexFilterType->AddItem("Anisotropic 2x", FILTER_ANISO2X);
        enumTexFilterType->AddItem("Anisotropic 4x", FILTER_ANISO4X);
        enumTexFilterType->AddItem("Anisotropic 8x", FILTER_ANISO8X);
        enumTexFilterType->AddItem("Anisotropic 16x", FILTER_ANISO16X);

        //////////////////////////////////////////////////////////////////////////
        // Vertex Mods.
        //////////////////////////////////////////////////////////////////////////
        enumVertexMod = new CVarEnumList<int>();
        enumVertexMod->AddItem("None", eDT_Unknown);
        enumVertexMod->AddItem("Sin Wave", eDT_SinWave);
        enumVertexMod->AddItem("Sin Wave using vertex color", eDT_SinWaveUsingVtxColor);
        enumVertexMod->AddItem("Bulge", eDT_Bulge);
        enumVertexMod->AddItem("Squeeze", eDT_Squeeze);
        enumVertexMod->AddItem("FixedOffset", eDT_FixedOffset);

        //////////////////////////////////////////////////////////////////////////

        enumWaveType = new CVarEnumList<int>();
        enumWaveType->AddItem("Sin", eWF_Sin);

        //////////////////////////////////////////////////////////////////////////
        // Fill shaders enum.
        //////////////////////////////////////////////////////////////////////////
        CVarEnumList<QString>* enumShaders = new CVarEnumList<QString>();
        {
            CShaderEnum* pShaderEnum = GetIEditor()->GetShaderEnum();
            pShaderEnum->EnumShaders();
            for (int i = 0; i < pShaderEnum->GetShaderCount(); i++)
            {
                QString shaderName = pShaderEnum->GetShader(i);
                if (shaderName.contains("_Overlay", Qt::CaseInsensitive))
                {
                    continue;
                }
                enumShaders->AddItem(shaderName, shaderName);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // Fill surface types.
        //////////////////////////////////////////////////////////////////////////
        CVarEnumList<QString>* enumSurfaceTypes = new CVarEnumList<QString>();
        {
            QStringList types;
            types.push_back(""); // Push empty surface type.
            ISurfaceTypeEnumerator* pSurfaceTypeEnum = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager()->GetEnumerator();
            if (pSurfaceTypeEnum)
            {
                for (ISurfaceType* pSurfaceType = pSurfaceTypeEnum->GetFirst(); pSurfaceType; pSurfaceType = pSurfaceTypeEnum->GetNext())
                {
                    types.push_back(pSurfaceType->GetName());
                }
                std::sort(types.begin(), types.end());
                for (int i = 0; i < types.size(); i++)
                {
                    QString name = types[i];
                    if (name.left(4) == "mat_")
                    {
                        name.remove(0, 4);
                    }
                    enumSurfaceTypes->AddItem(name, types[i]);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // Init tables.
        //////////////////////////////////////////////////////////////////////////
        AddVariable(m_vars, tableShader, "Material Settings", "");
        AddVariable(m_vars, tableOpacity, "Opacity Settings", "");
        AddVariable(m_vars, tableLighting, "Lighting Settings", "");
        AddVariable(m_vars, tableAdvanced, "Advanced", "");
        AddVariable(m_vars, tableTexture, "Texture Maps", "");
        AddVariable(m_vars, tableShaderParams, "Shader Params", "");
        AddVariable(m_vars, tableShaderGenParams, "Shader Generation Params", "");
        AddVariable(m_vars, tableVertexMod, "Vertex Deformation", "");

        tableTexture->SetFlags(tableTexture->GetFlags() | IVariable::UI_ROLLUP2);
        tableVertexMod->SetFlags(tableVertexMod->GetFlags() | IVariable::UI_ROLLUP2 | IVariable::UI_COLLAPSED);
        tableAdvanced->SetFlags(tableAdvanced->GetFlags() | IVariable::UI_COLLAPSED);
        tableShaderGenParams->SetFlags(tableShaderGenParams->GetFlags() | IVariable::UI_ROLLUP2 | IVariable::UI_COLLAPSED);
        tableShaderParams->SetFlags(tableShaderParams->GetFlags() | IVariable::UI_ROLLUP2);


        //////////////////////////////////////////////////////////////////////////
        // Shader.
        //////////////////////////////////////////////////////////////////////////
        AddVariable(tableShader, shader, "Shader", "Selects shader type for specific surface response and options");
        AddVariable(tableShader, surfaceType, "Surface Type", "Defines how entities interact with surfaces using the material effects system");
        m_varChangeNotifications["Surface Type"] = MATERIALCHANGE_SURFACETYPE;

        shader->SetEnumList(enumShaders);

        surfaceType->SetEnumList(enumSurfaceTypes);

        // Properties that use this scriptingDescription are based on what's available in MaterialHelpers::SetGetMaterialParamVec3 and MaterialHelpers::SetGetMaterialParamFloat.
        // This should match what's done in MaterialHelpers.cpp AddRealNameToDescription().
        auto scriptingDescription = [](const AZStd::string& scriptAccessibleName, const AZStd::string& description) { return description + "\n(Script Param Name = " + scriptAccessibleName + ")"; };

        //////////////////////////////////////////////////////////////////////////
        // Opacity.
        //////////////////////////////////////////////////////////////////////////
        AddVariable(tableOpacity, opacity, "Opacity",
            scriptingDescription("opacity", "Sets the transparency amount. Uses 0-99 to set Alpha Blend and 100 for Opaque and Alpha Test.").c_str(), IVariable::DT_PERCENT);
        AddVariable(tableOpacity, alphaTest, "AlphaTest",
            scriptingDescription("alpha", "Uses the alpha mask and refines the transparent edge. Uses 0-50 to bias toward white or 50-100 to bias toward black.").c_str(), IVariable::DT_PERCENT);
        AddVariable(tableOpacity, bAdditive, "Additive", "Adds material color to the background color resulting in a brighter transparent surface");
        opacity->SetLimits(0, 100, 1, true, true);
        alphaTest->SetLimits(0, 100, 1, true, true);

        //////////////////////////////////////////////////////////////////////////
        // Lighting.
        //////////////////////////////////////////////////////////////////////////
        AddVariable(tableLighting, diffuse,             "Diffuse Color (Tint)",         scriptingDescription("diffuse",             "Tints the material diffuse color. Physically based materials should be left at white").c_str(), IVariable::DT_COLOR);
        AddVariable(tableLighting, specular,            "Specular Color",               scriptingDescription("specular",            "Reflective and shininess intensity and color of reflective highlights").c_str(), IVariable::DT_COLOR);
        AddVariable(tableLighting, smoothness,          "Smoothness",                   scriptingDescription("shininess",           "Smoothness or glossiness simulating how light bounces off the surface").c_str());
        AddVariable(tableLighting, emissiveIntensity,   "Emissive Intensity (kcd/m2)",  scriptingDescription("emissive_intensity",  "Brightness simulating light emitting from the surface making an object glow").c_str());
        AddVariable(tableLighting, emissiveCol,         "Emissive Color",               scriptingDescription("emissive_color",      "Tints the emissive color").c_str(), IVariable::DT_COLOR);
        emissiveIntensity->SetLimits(0, EMISSIVE_INTENSITY_SOFT_MAX, 1, true, false);
        smoothness->SetLimits(0, 255, 1, true, true);

        //////////////////////////////////////////////////////////////////////////
        // Init texture variables.
        //////////////////////////////////////////////////////////////////////////
        for (EEfResTextures texId = EEfResTextures(0); texId < EFTT_MAX; texId = EEfResTextures(texId + 1))
        {
            if (!MaterialHelpers::IsAdjustableTexSlot(texId))
            {
                continue;
            }

            InitTextureVars(texId, MaterialHelpers::LookupTexName(texId), MaterialHelpers::LookupTexDesc(texId));
        }

        //AddVariable( tableAdvanced,bWire,"Wireframe" );
        AddVariable(tableAdvanced, allowLayerActivation, "Allow layer activation", "");
        AddVariable(tableAdvanced, b2Sided, "2 Sided", "Enables both sides of mesh faces to render");
        AddVariable(tableAdvanced, bNoShadow, "No Shadow", "Disables casting shadows from mesh faces");
        AddVariable(tableAdvanced, bScatter, "Use Scattering", "Deprecated");
        AddVariable(tableAdvanced, bHideAfterBreaking, "Hide After Breaking", "Causes the object to disappear after procedurally breaking");
        AddVariable(tableAdvanced, bFogVolumeShadingQualityHigh, "Fog Volume Shading Quality High", "high fog volume shading quality behaves more accurately with fog volumes.");
        AddVariable(tableAdvanced, bBlendTerrainColor, "Blend Terrain Color", "");
        AddVariable(tableAdvanced, voxelCoverage, "Voxel Coverage", "Fine tunes occlusion amount for svogi feature. Higher values occlude more closely to object shape.");
        voxelCoverage->SetLimits(0, 1.0f);

        //////////////////////////////////////////////////////////////////////////
        // Material Value Propagation for dynamic material switches, as for instance
        // used by breakable glass
        //////////////////////////////////////////////////////////////////////////
        AddVariable(tableAdvanced, matPropagate, "Link to Material", "");
        AddVariable(tableAdvanced, bPropagateMaterialSettings, "Propagate Material Settings", "");
        AddVariable(tableAdvanced, bPropagateOpactity, "Propagate Opacity Settings", "");
        AddVariable(tableAdvanced, bPropagateLighting, "Propagate Lighting Settings", "");
        AddVariable(tableAdvanced, bPropagateAdvanced, "Propagate Advanced Settings", "");
        AddVariable(tableAdvanced, bPropagateTexture, "Propagate Texture Maps", "");
        AddVariable(tableAdvanced, bPropagateShaderParams, "Propagate Shader Params", "");
        AddVariable(tableAdvanced, bPropagateShaderGenParams, "Propagate Shader Generation", "");
        AddVariable(tableAdvanced, bPropagateVertexDef, "Propagate Vertex Deformation", "");

        //////////////////////////////////////////////////////////////////////////
        // Init Vertex Deformation.
        //////////////////////////////////////////////////////////////////////////
        vertexMod.type->SetEnumList(enumVertexMod);
        AddVariable(tableVertexMod, vertexMod.type, "Type", "Choose method to define how the vertices will deform");
        AddVariable(tableVertexMod, vertexMod.fDividerX, "Wave Length", "Length of wave deformation");

        AddVariable(tableVertexMod, vertexMod.wave.table, "Parameters", "Fine tunes how the vertices deform");

        vertexMod.wave.waveFormType->SetEnumList(enumWaveType);
        AddVariable(vertexMod.wave.table, vertexMod.wave.waveFormType, "Type", "Sin type will include vertex color in calculation");
        AddVariable(vertexMod.wave.table, vertexMod.wave.level, "Level", "Scales the object equally in xyz");
        AddVariable(vertexMod.wave.table, vertexMod.wave.amplitude, "Amplitude", "Strength of vertex deformation (vertex color: b, normal: z)");
        AddVariable(vertexMod.wave.table, vertexMod.wave.phase, "Phase", "Offset of vertex deformation (vertex color: r, normal: x)");
        AddVariable(vertexMod.wave.table, vertexMod.wave.frequency, "Frequency", "Speed of vertex animation (vertex color: g, normal: y)");

        return m_vars;
    }

private:
    //////////////////////////////////////////////////////////////////////////
    void InitTextureVars(int id, const QString& name, const QString& desc)
    {
        textureVars[id]->SetFlags(IVariable::UI_BOLD);
        textureVars[id]->SetFlags(textureVars[id]->GetFlags() | IVariable::UI_AUTO_EXPAND);
        advancedTextureGroup[id]->SetFlags(advancedTextureGroup[id]->GetFlags() | IVariable::UI_COLLAPSED);
        AddVariable(tableTexture, *textureVars[id], name.toUtf8().data(), desc.toUtf8().data(), IVariable::DT_TEXTURE);
        AddVariable(*textureVars[id], *advancedTextureGroup[id], "Advanced", "Controls UV tiling, offset, and rotation as well as texture filtering");

        AddVariable(*advancedTextureGroup[id], textures[id].etextype, "TexType", "");
        AddVariable(*advancedTextureGroup[id], textures[id].filter, "Filter", "Sets texture smoothing method to determine texture pixel quality");

        AddVariable(*advancedTextureGroup[id], textures[id].is_tcgprojected, "IsProjectedTexGen", "");
        AddVariable(*advancedTextureGroup[id], textures[id].etcgentype, "TexGenType", "Controls UV projection behavior");

        if (IsTextureModifierSupportedForTextureMap(static_cast<EEfResTextures>(id)))
        {
            //////////////////////////////////////////////////////////////////////////
            // Tiling table.
            AddVariable(*advancedTextureGroup[id], textures[id].tableTiling, "Tiling", "Controls UV tiling, offset, and rotation");
            {
                CVariableArray& table = textures[id].tableTiling;
                table.SetFlags(IVariable::UI_BOLD);
                AddVariable(table, *textures[id].is_tile[0], "IsTileU", "Enables UV tiling on U");
                AddVariable(table, *textures[id].is_tile[1], "IsTileV", "Enables UV tiling on V");
                AddVariable(table, *textures[id].tiling[0], "TileU", "Multiplies tiled projection on U");
                AddVariable(table, *textures[id].tiling[1], "TileV", "Multiplies tiled projection on V");
                AddVariable(table, *textures[id].offset[0], "OffsetU", "Offsets texture projection on U");
                AddVariable(table, *textures[id].offset[1], "OffsetV", "Offsets texture projection on V");
                AddVariable(table, *textures[id].rotate[0], "RotateU", "Rotates texture projection on U");
                AddVariable(table, *textures[id].rotate[1], "RotateV", "Rotates texture projection on V");
                AddVariable(table, *textures[id].rotate[2], "RotateW", "Rotates texture projection on W");
            }

            //////////////////////////////////////////////////////////////////////////
            // Rotator tables.
            AddVariable(*advancedTextureGroup[id], textures[id].tableRotator, "Rotator", "Controls the animated UV rotation");
            {
                CVariableArray& table = textures[id].tableRotator;
                table.SetFlags(IVariable::UI_BOLD);
                AddVariable(table, textures[id].etcmrotatetype, "Type", "Controls the behavior of UV rotation");
                AddVariable(table, textures[id].tcmrotoscrate, "Rate", "Sets the speed (number of complete cycles per unit of time) of rotation");
                AddVariable(table, textures[id].tcmrotoscphase, "Phase", "Sets the initial offset of rotation");
                AddVariable(table, textures[id].tcmrotoscamplitude, "Amplitude", "Sets the strength (maximum value) of rotation");
                AddVariable(table, *textures[id].tcmrotosccenter[0], "CenterU", "Sets the center of rotation along U");
                AddVariable(table, *textures[id].tcmrotosccenter[1], "CenterV", "Sets the center of rotation along V");
            }

            //////////////////////////////////////////////////////////////////////////
            // Oscillator table
            AddVariable(*advancedTextureGroup[id], textures[id].tableOscillator, "Oscillator", "Controls the animated UV oscillation");
            {
                CVariableArray& table = textures[id].tableOscillator;
                table.SetFlags(IVariable::UI_BOLD);
                AddVariable(table, textures[id].etcmumovetype, "TypeU", "Sets the behavior of oscillation in the U direction");
                AddVariable(table, textures[id].etcmvmovetype, "TypeV", "Sets the behavior of oscillation in the V direction");
                AddVariable(table, textures[id].tcmuoscrate, "RateU", "Sets the speed (number of complete cycles per unit of time) of oscillation in U");
                AddVariable(table, textures[id].tcmvoscrate, "RateV", "Sets the speed (number of complete cycles per unit of time) of oscillation in V");
                AddVariable(table, textures[id].tcmuoscphase, "PhaseU", "Sets the initial offset of oscillation in U");
                AddVariable(table, textures[id].tcmvoscphase, "PhaseV", "Sets the initial offset of oscillation in V");
                AddVariable(table, textures[id].tcmuoscamplitude, "AmplitudeU", "Sets the strength (maximum value) of oscillation in U");
                AddVariable(table, textures[id].tcmvoscamplitude, "AmplitudeV", "Sets the strength (maximum value) of oscillation in V");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // Assign enums tables to variable.
        //////////////////////////////////////////////////////////////////////////
        textures[id].etextype->SetEnumList(enumTexType);
        textures[id].etcgentype->SetEnumList(enumTexGenType);
        textures[id].etcmrotatetype->SetEnumList(enumTexModRotateType);
        textures[id].etcmumovetype->SetEnumList(enumTexModUMoveType);
        textures[id].etcmvmovetype->SetEnumList(enumTexModVMoveType);
        textures[id].filter->SetEnumList(enumTexFilterType);
    }
    //////////////////////////////////////////////////////////////////////////

    void AddVariable(CVariableBase& varArray, CVariableBase& var, const char* varName, const char* varTooltip, unsigned char dataType = IVariable::DT_SIMPLE)
    {
        if (varName)
        {
            var.SetName(varName);
        }
        if (varTooltip)
        {
            var.SetDescription(varTooltip);
        }
        var.SetDataType(dataType);
        varArray.AddVariable(&var);
    }
    //////////////////////////////////////////////////////////////////////////
    void AddVariable(CVarBlock* vars, CVariableBase& var, const char* varName, const char* varTooltip, unsigned char dataType = IVariable::DT_SIMPLE)
    {
        if (varName)
        {
            var.SetName(varName);
        }
        if (varTooltip)
        {
            var.SetDescription(varTooltip);
        }
        var.SetDataType(dataType);
        vars->AddVariable(&var);
    }

    void SetTextureResources(const SEfResTexture *pTextureRes, uint16 tex, bool bSetTextures);
    void GetTextureResources(SInputShaderResources& sr, int texid, int propagationFlags);
    void ResetTextureResources(uint16 tex);
    Vec4 ToVec4(const ColorF& col) { return Vec4(col.r, col.g, col.b, col.a); }
    Vec3 ToVec3(const ColorF& col) { return Vec3(col.r, col.g, col.b); }
    ColorF ToCFColor(const Vec3& col) { return ColorF(col); }
    ColorF ToCFColor(const Vec4& col) { return ColorF(col); }
};

//////////////////////////////////////////////////////////////////////////
void CMaterialUI::NotifyObjectsAboutMaterialChange(IVariable* var)
{
    if (!var)
    {
        return;
    }

    TVarChangeNotifications::iterator it = m_varChangeNotifications.find(var->GetName());
    if (it == m_varChangeNotifications.end())
    {
        return;
    }

    CMaterial* pMaterial = GetIEditor()->GetMaterialManager()->GetCurrentMaterial();
    if (!pMaterial)
    {
        return;
    }

    // Get a parent, if we are editing submaterial
    if (pMaterial->GetParent() != 0)
    {
        pMaterial = pMaterial->GetParent();
    }

    CBaseObjectsArray objects;
    GetIEditor()->GetObjectManager()->GetObjects(objects);
    int numObjects = objects.size();
    for (int i = 0; i < numObjects; ++i)
    {
        CBaseObject* pObject = objects[i];
        if (pObject->GetRenderMaterial() == pMaterial)
        {
            pObject->OnMaterialChanged(it->second);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialUI::SetShaderResources(const SInputShaderResources& srTextures, bool bSetTextures)
{
    alphaTest = srTextures.m_AlphaRef;
    voxelCoverage = (float) srTextures.m_VoxelCoverage / 255.0f;

    diffuse = ToVec3(srTextures.m_LMaterial.m_Diffuse);
    specular = ToVec3(srTextures.m_LMaterial.m_Specular);
    emissiveCol = ToVec3(srTextures.m_LMaterial.m_Emittance);
    emissiveIntensity = srTextures.m_LMaterial.m_Emittance.a;
    opacity = srTextures.m_LMaterial.m_Opacity;
    smoothness = srTextures.m_LMaterial.m_Smoothness;

    SetVertexDeform(srTextures);


    for (EEfResTextures texId = EEfResTextures(0); texId < EFTT_MAX; texId = EEfResTextures(texId + 1))
    {
        if (!MaterialHelpers::IsAdjustableTexSlot(texId))
        {
            continue;
        }

        auto foundIter = srTextures.m_TexturesResourcesMap.find((ResourceSlotIndex)texId);
        if (foundIter != srTextures.m_TexturesResourcesMap.end())
        {
            const SEfResTexture*    pTextureRes = const_cast<SEfResTexture*>(&foundIter->second);
            SetTextureResources(pTextureRes, texId, bSetTextures);
        }
        else
        {
            ResetTextureResources(texId);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialUI::GetShaderResources(SInputShaderResources& sr, int propagationFlags)
{
    if (propagationFlags & MTL_PROPAGATE_OPACITY)
    {
        sr.m_LMaterial.m_Opacity = opacity;
        sr.m_AlphaRef = alphaTest;
    }

    if (propagationFlags & MTL_PROPAGATE_ADVANCED)
    {
        sr.m_VoxelCoverage = int_round(voxelCoverage * 255.0f);
    }

    if (propagationFlags & MTL_PROPAGATE_LIGHTING)
    {
        sr.m_LMaterial.m_Diffuse = ToCFColor(diffuse);
        sr.m_LMaterial.m_Specular = ToCFColor(specular);
        sr.m_LMaterial.m_Emittance = ColorF(emissiveCol, emissiveIntensity);
        sr.m_LMaterial.m_Smoothness = smoothness;
    }

    GetVertexDeform(sr, propagationFlags);

    for (EEfResTextures texId = EEfResTextures(0); texId < EFTT_MAX; texId = EEfResTextures(texId + 1))
    {
        if (!MaterialHelpers::IsAdjustableTexSlot(texId))
        {
            continue;
        }

        GetTextureResources(sr, texId, propagationFlags);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialUI::SetTextureResources( const SEfResTexture *pTextureRes, uint16 texSlot, bool bSetTextures)
{
    /*
    // Enable/Disable texture map, depending on the mask.
    int flags = textureVars[tex].GetFlags();
    if ((1 << tex) & texUsageMask)
    flags &= ~IVariable::UI_DISABLED;
    else
    flags |= IVariable::UI_DISABLED;
    textureVars[tex].SetFlags( flags );
    */

    if (bSetTextures)
    {
        QString texFilename = pTextureRes->m_Name.c_str();
        texFilename = Path::ToUnixPath(texFilename);
        textureVars[texSlot]->Set(texFilename);
    }

    //textures[tex].amount = pTextureRes->m_Amount;
    *textures[texSlot].is_tile[0] = pTextureRes->m_bUTile;
    *textures[texSlot].is_tile[1] = pTextureRes->m_bVTile;

    *textures[texSlot].tiling[0] = pTextureRes->GetTiling(0);
    *textures[texSlot].tiling[1] = pTextureRes->GetTiling(1);
    *textures[texSlot].offset[0] = pTextureRes->GetOffset(0);
    *textures[texSlot].offset[1] = pTextureRes->GetOffset(1);
    textures[texSlot].filter = (int)pTextureRes->m_Filter;
    textures[texSlot].etextype = pTextureRes->m_Sampler.m_eTexType;

    if (pTextureRes->m_Ext.m_pTexModifier)
    {
        textures[texSlot].etcgentype = pTextureRes->m_Ext.m_pTexModifier->m_eTGType;
        textures[texSlot].etcmumovetype = pTextureRes->m_Ext.m_pTexModifier->m_eMoveType[0];
        textures[texSlot].etcmvmovetype = pTextureRes->m_Ext.m_pTexModifier->m_eMoveType[1];
        textures[texSlot].etcmrotatetype = pTextureRes->m_Ext.m_pTexModifier->m_eRotType;
        textures[texSlot].is_tcgprojected = pTextureRes->m_Ext.m_pTexModifier->m_bTexGenProjected;
        textures[texSlot].tcmuoscrate = pTextureRes->m_Ext.m_pTexModifier->m_OscRate[0];
        textures[texSlot].tcmuoscphase = pTextureRes->m_Ext.m_pTexModifier->m_OscPhase[0];
        textures[texSlot].tcmuoscamplitude = pTextureRes->m_Ext.m_pTexModifier->m_OscAmplitude[0];
        textures[texSlot].tcmvoscrate = pTextureRes->m_Ext.m_pTexModifier->m_OscRate[1];
        textures[texSlot].tcmvoscphase = pTextureRes->m_Ext.m_pTexModifier->m_OscPhase[1];
        textures[texSlot].tcmvoscamplitude = pTextureRes->m_Ext.m_pTexModifier->m_OscAmplitude[1];

        for (int i = 0; i < 3; i++)
        {
            *textures[texSlot].rotate[i] = RoundDegree(Word2Degr(pTextureRes->m_Ext.m_pTexModifier->m_Rot[i]));
        }
        textures[texSlot].tcmrotoscrate = RoundDegree(Word2Degr(pTextureRes->m_Ext.m_pTexModifier->m_RotOscRate[2]));
        textures[texSlot].tcmrotoscphase = RoundDegree(Word2Degr(pTextureRes->m_Ext.m_pTexModifier->m_RotOscPhase[2]));
        textures[texSlot].tcmrotoscamplitude = RoundDegree(Word2Degr(pTextureRes->m_Ext.m_pTexModifier->m_RotOscAmplitude[2]));
        *textures[texSlot].tcmrotosccenter[0] = pTextureRes->m_Ext.m_pTexModifier->m_RotOscCenter[0];
        *textures[texSlot].tcmrotosccenter[1] = pTextureRes->m_Ext.m_pTexModifier->m_RotOscCenter[1];
    }
    else
    {
        textures[texSlot].etcgentype = 0;
        textures[texSlot].etcmumovetype = 0;
        textures[texSlot].etcmvmovetype = 0;
        textures[texSlot].etcmrotatetype = 0;
        textures[texSlot].is_tcgprojected = false;
        textures[texSlot].tcmuoscrate = 0;
        textures[texSlot].tcmuoscphase = 0;
        textures[texSlot].tcmuoscamplitude = 0;
        textures[texSlot].tcmvoscrate = 0;
        textures[texSlot].tcmvoscphase = 0;
        textures[texSlot].tcmvoscamplitude = 0;

        for (int i = 0; i < 3; i++)
        {
            *textures[texSlot].rotate[i] = 0;
        }

        textures[texSlot].tcmrotoscrate = 0;
        textures[texSlot].tcmrotoscphase = 0;
        textures[texSlot].tcmrotoscamplitude = 0;
        *textures[texSlot].tcmrotosccenter[0] = 0;
        *textures[texSlot].tcmrotosccenter[1] = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialUI::ResetTextureResources(uint16 texSlot)
{
    QString texFilename = "";
    textureVars[texSlot]->Set(texFilename);
    textures[texSlot].Reset();
}

void CMaterialUI::GetTextureResources(SInputShaderResources& sr, int tex, int propagationFlags)
{
    if ((propagationFlags & MTL_PROPAGATE_TEXTURES) == 0)
    {
        return;
    }

    QString             texFilename;
    textureVars[tex]->Get(texFilename);
    if (texFilename.isEmpty())
    {
        // Remove the texture if the path was cleared in the UI
        sr.m_TexturesResourcesMap.erase(tex);

        // If the normal map/second normal map has been cleared in the UI,
        // we must also clear the smoothness/second smoothness since smoothness lives in the alpha of the normal
        if (tex == EFTT_NORMALS)
        {
            sr.m_TexturesResourcesMap.erase(EFTT_SMOOTHNESS);
        }
        // EFTT_CUSTOM_SECONDARY is the 2nd normal
        if (tex == EFTT_CUSTOM_SECONDARY)
        {
            sr.m_TexturesResourcesMap.erase(EFTT_SECOND_SMOOTHNESS);
        }
        return;
    }
    texFilename = Path::ToUnixPath(texFilename);

    // Clear any texture resource that has no associated file
    if (texFilename.size() > AZ_MAX_PATH_LEN)
    {
        AZ_Error("Material Editor", false, "Texture path exceeds the maximium allowable length of %d.", AZ_MAX_PATH_LEN);
        return;
    }

    // The following line will insert the slot if did not exist.
    SEfResTexture*      pTextureRes = &(sr.m_TexturesResourcesMap[tex]);
    pTextureRes->m_Name = texFilename.toUtf8().data();

    //pTextureRes->m_Amount = textures[tex].amount;
    pTextureRes->m_bUTile = *textures[tex].is_tile[0];
    pTextureRes->m_bVTile = *textures[tex].is_tile[1];
    SEfTexModificator& texm = *pTextureRes->AddModificator();
    texm.m_bTexGenProjected = textures[tex].is_tcgprojected;

    texm.m_Tiling[0] = *textures[tex].tiling[0];
    texm.m_Tiling[1] = *textures[tex].tiling[1];
    texm.m_Offs[0] = *textures[tex].offset[0];
    texm.m_Offs[1] = *textures[tex].offset[1];
    pTextureRes->m_Filter = (int)textures[tex].filter;
    pTextureRes->m_Sampler.m_eTexType = textures[tex].etextype;
    texm.m_eRotType = textures[tex].etcmrotatetype;
    texm.m_eTGType = textures[tex].etcgentype;
    texm.m_eMoveType[0] = textures[tex].etcmumovetype;
    texm.m_eMoveType[1] = textures[tex].etcmvmovetype;
    texm.m_OscRate[0] = textures[tex].tcmuoscrate;
    texm.m_OscPhase[0] = textures[tex].tcmuoscphase;
    texm.m_OscAmplitude[0] = textures[tex].tcmuoscamplitude;
    texm.m_OscRate[1] = textures[tex].tcmvoscrate;
    texm.m_OscPhase[1] = textures[tex].tcmvoscphase;
    texm.m_OscAmplitude[1] = textures[tex].tcmvoscamplitude;

    for (int i = 0; i < 3; i++)
    {
        texm.m_Rot[i] = Degr2Word(*textures[tex].rotate[i]);
    }
    texm.m_RotOscRate[2] = Degr2Word(textures[tex].tcmrotoscrate);
    texm.m_RotOscPhase[2] = Degr2Word(textures[tex].tcmrotoscphase);
    texm.m_RotOscAmplitude[2] = Degr2Word(textures[tex].tcmrotoscamplitude);
    texm.m_RotOscCenter[0] = *textures[tex].tcmrotosccenter[0];
    texm.m_RotOscCenter[1] = *textures[tex].tcmrotosccenter[1];
    texm.m_RotOscCenter[2] = 0.0f;
}

//////////////////////////////////////////////////////////////////////////
void CMaterialUI::SetVertexDeform(const SInputShaderResources& sr)
{
    vertexMod.type = (int)sr.m_DeformInfo.m_eType;
    vertexMod.fDividerX = sr.m_DeformInfo.m_fDividerX;
    vertexMod.vNoiseScale = sr.m_DeformInfo.m_vNoiseScale;

    vertexMod.wave.waveFormType = EWaveForm::eWF_Sin;
    vertexMod.wave.amplitude = sr.m_DeformInfo.m_WaveX.m_Amp;
    vertexMod.wave.level = sr.m_DeformInfo.m_WaveX.m_Level;
    vertexMod.wave.phase = sr.m_DeformInfo.m_WaveX.m_Phase;
    vertexMod.wave.frequency = sr.m_DeformInfo.m_WaveX.m_Freq;
}

//////////////////////////////////////////////////////////////////////////
void CMaterialUI::GetVertexDeform(SInputShaderResources& sr, int propagationFlags)
{
    if ((propagationFlags & MTL_PROPAGATE_VERTEX_DEF) == 0)
    {
        return;
    }

    sr.m_DeformInfo.m_eType = (EDeformType)((int)vertexMod.type);
    sr.m_DeformInfo.m_fDividerX = vertexMod.fDividerX;
    sr.m_DeformInfo.m_vNoiseScale = vertexMod.vNoiseScale;

    sr.m_DeformInfo.m_WaveX.m_eWFType = (EWaveForm)((int)vertexMod.wave.waveFormType);
    sr.m_DeformInfo.m_WaveX.m_Amp = vertexMod.wave.amplitude;
    sr.m_DeformInfo.m_WaveX.m_Level = vertexMod.wave.level;
    sr.m_DeformInfo.m_WaveX.m_Phase = vertexMod.wave.phase;
    sr.m_DeformInfo.m_WaveX.m_Freq = vertexMod.wave.frequency;
}

void CMaterialUI::PropagateToLinkedMaterial(CMaterial* mtl, CVarBlockPtr pShaderParams)
{
    if (!mtl)
    {
        return;
    }
    CMaterial* subMtl = NULL, * parentMtl = mtl->GetParent();
    const QString& linkedMaterialName = matPropagate;
    int propFlags = 0;

    if (parentMtl)
    {
        for (int i = 0; i < parentMtl->GetSubMaterialCount(); ++i)
        {
            CMaterial* pMtl = parentMtl->GetSubMaterial(i);
            if (pMtl && pMtl != mtl && pMtl->GetFullName() == linkedMaterialName)
            {
                subMtl = pMtl;
                break;
            }
        }
    }
    if (!linkedMaterialName.isEmpty() && subMtl)
    {
        // Ensure that the linked material is cleared if it can't be found anymore
        mtl->LinkToMaterial(linkedMaterialName);
    }
    // Note: It's only allowed to propagate the shader params and shadergen params
    // if we also propagate the actual shader to the linked material as well, else
    // bogus values will be set
    bPropagateShaderParams = (int)bPropagateShaderParams & - (int)bPropagateMaterialSettings;
    bPropagateShaderGenParams = (int)bPropagateShaderGenParams & - (int)bPropagateMaterialSettings;

    propFlags |= MTL_PROPAGATE_MATERIAL_SETTINGS & - (int)bPropagateMaterialSettings;
    propFlags |= MTL_PROPAGATE_OPACITY & - (int)bPropagateOpactity;
    propFlags |= MTL_PROPAGATE_LIGHTING & - (int)bPropagateLighting;
    propFlags |= MTL_PROPAGATE_ADVANCED & - (int)bPropagateAdvanced;
    propFlags |= MTL_PROPAGATE_TEXTURES & - (int)bPropagateTexture;
    propFlags |= MTL_PROPAGATE_SHADER_PARAMS & - (int)bPropagateShaderParams;
    propFlags |= MTL_PROPAGATE_SHADER_GEN & - (int)bPropagateShaderGenParams;
    propFlags |= MTL_PROPAGATE_VERTEX_DEF & - (int)bPropagateVertexDef;
    propFlags |= MTL_PROPAGATE_LAYER_PRESETS & - (int)bPropagateLayerPresets;
    mtl->SetPropagationFlags(propFlags);

    if (subMtl)
    {
        SetToMaterial(subMtl, propFlags | MTL_PROPAGATE_RESERVED);
        if (propFlags & MTL_PROPAGATE_SHADER_PARAMS)
        {
            if (CVarBlock* pPublicVars = subMtl->GetPublicVars(mtl->GetShaderResources()))
            {
                subMtl->SetPublicVars(pPublicVars, subMtl);
            }
        }
        if (propFlags & MTL_PROPAGATE_SHADER_GEN)
        {
            subMtl->SetShaderGenParamsVars(mtl->GetShaderGenParamsVars());
        }
        subMtl->Update();
        subMtl->UpdateMaterialLayers();
    }
}

void CMaterialUI::PropagateFromLinkedMaterial(CMaterial* mtl)
{
    if (!mtl)
    {
        return;
    }
    CMaterial* subMtl = NULL, * parentMtl = mtl->GetParent();
    const QString& linkedMaterialName = mtl->GetLinkedMaterialName();
    //CVarEnumList<CString> *enumMtls  = new CVarEnumList<CString>;
    if (parentMtl)
    {
        for (int i = 0; i < parentMtl->GetSubMaterialCount(); ++i)
        {
            CMaterial* pMtl = parentMtl->GetSubMaterial(i);
            if (!pMtl || pMtl == mtl)
            {
                continue;
            }
            const QString& subMtlName = pMtl->GetFullName();
            //enumMtls->AddItem(subMtlName, subMtlName);
            if (subMtlName == linkedMaterialName)
            {
                subMtl = pMtl;
                break;
            }
        }
    }
    matPropagate = QString();
    //matPropagate.SetEnumList(enumMtls);
    if (!linkedMaterialName.isEmpty() && !subMtl)
    {
        // Ensure that the linked material is cleared if it can't be found anymore
        mtl->LinkToMaterial(QString());
    }
    else
    {
        matPropagate = linkedMaterialName;
    }
    bPropagateMaterialSettings = mtl->GetPropagationFlags() & MTL_PROPAGATE_MATERIAL_SETTINGS;
    bPropagateOpactity = mtl->GetPropagationFlags() & MTL_PROPAGATE_OPACITY;
    bPropagateLighting = mtl->GetPropagationFlags() & MTL_PROPAGATE_LIGHTING;
    bPropagateTexture = mtl->GetPropagationFlags() & MTL_PROPAGATE_TEXTURES;
    bPropagateAdvanced = mtl->GetPropagationFlags() & MTL_PROPAGATE_ADVANCED;
    bPropagateVertexDef = mtl->GetPropagationFlags() & MTL_PROPAGATE_VERTEX_DEF;
    bPropagateShaderParams = mtl->GetPropagationFlags() & MTL_PROPAGATE_SHADER_PARAMS;
    bPropagateLayerPresets = mtl->GetPropagationFlags() & MTL_PROPAGATE_LAYER_PRESETS;
    bPropagateShaderGenParams = mtl->GetPropagationFlags() & MTL_PROPAGATE_SHADER_GEN;
}

void CMaterialUI::SetFromMaterial(CMaterial* mtlIn)
{
    QString shaderName = mtlIn->GetShaderName();
    if (!shaderName.isEmpty())
    {
        // Capitalize first letter.
        shaderName = shaderName[0].toUpper() + shaderName.mid(1);
    }

    shader = shaderName;

    int mtlFlags = mtlIn->GetFlags();
    bNoShadow = (mtlFlags & MTL_FLAG_NOSHADOW);
    bAdditive = (mtlFlags & MTL_FLAG_ADDITIVE);
    bWire = (mtlFlags & MTL_FLAG_WIRE);
    b2Sided = (mtlFlags & MTL_FLAG_2SIDED);
    bScatter = (mtlFlags & MTL_FLAG_SCATTER);
    bHideAfterBreaking = (mtlFlags & MTL_FLAG_HIDEONBREAK);
    bFogVolumeShadingQualityHigh = (mtlFlags & MTL_FLAG_FOG_VOLUME_SHADING_QUALITY_HIGH);
    bBlendTerrainColor = (mtlFlags & MTL_FLAG_BLEND_TERRAIN);
    texUsageMask = mtlIn->GetTexmapUsageMask();

    allowLayerActivation = mtlIn->LayerActivationAllowed();

    // Detail, decal and custom textures are always active.
    const uint32 nDefaultFlagsEFTT = (1 << EFTT_DETAIL_OVERLAY) | (1 << EFTT_DECAL_OVERLAY) | (1 << EFTT_CUSTOM) | (1 << EFTT_CUSTOM_SECONDARY);
    texUsageMask |= nDefaultFlagsEFTT;
    if ((texUsageMask & (1 << EFTT_NORMALS)))
    {
        texUsageMask |= 1 << EFTT_NORMALS;
    }

    surfaceType = mtlIn->GetSurfaceTypeName();
    SetShaderResources(mtlIn->GetShaderResources(), true);

    // Propagate settings and properties to a sub material if edited
    PropagateFromLinkedMaterial(mtlIn);

    // set each material layer
    SMaterialLayerResources* pMtlLayerResources = mtlIn->GetMtlLayerResources();
    for (int l(0); l < MTL_LAYER_MAX_SLOTS; ++l)
    {
        materialLayers[l].shader = pMtlLayerResources[l].m_shaderName;
        materialLayers[l].bNoDraw = pMtlLayerResources[l].m_nFlags & MTL_LAYER_USAGE_NODRAW;
        materialLayers[l].bFadeOut = pMtlLayerResources[l].m_nFlags & MTL_LAYER_USAGE_FADEOUT;
    }
}

void CMaterialUI::SetToMaterial(CMaterial* mtl, int propagationFlags)
{
    int mtlFlags = mtl->GetFlags();

    if (propagationFlags & MTL_PROPAGATE_ADVANCED)
    {
        if (bNoShadow)
        {
            mtlFlags |= MTL_FLAG_NOSHADOW;
        }
        else
        {
            mtlFlags &= ~MTL_FLAG_NOSHADOW;
        }
    }

    if (propagationFlags & MTL_PROPAGATE_OPACITY)
    {
        if (bAdditive)
        {
            mtlFlags |= MTL_FLAG_ADDITIVE;
        }
        else
        {
            mtlFlags &= ~MTL_FLAG_ADDITIVE;
        }
    }

    if (bWire)
    {
        mtlFlags |= MTL_FLAG_WIRE;
    }
    else
    {
        mtlFlags &= ~MTL_FLAG_WIRE;
    }

    if (propagationFlags & MTL_PROPAGATE_ADVANCED)
    {
        if (b2Sided)
        {
            mtlFlags |= MTL_FLAG_2SIDED;
        }
        else
        {
            mtlFlags &= ~MTL_FLAG_2SIDED;
        }

        if (bScatter)
        {
            mtlFlags |= MTL_FLAG_SCATTER;
        }
        else
        {
            mtlFlags &= ~MTL_FLAG_SCATTER;
        }

        if (bHideAfterBreaking)
        {
            mtlFlags |= MTL_FLAG_HIDEONBREAK;
        }
        else
        {
            mtlFlags &= ~MTL_FLAG_HIDEONBREAK;
        }

        if (bFogVolumeShadingQualityHigh)
        {
            mtlFlags |= MTL_FLAG_FOG_VOLUME_SHADING_QUALITY_HIGH;
        }
        else
        {
            mtlFlags &= ~MTL_FLAG_FOG_VOLUME_SHADING_QUALITY_HIGH;
        }

        if (bBlendTerrainColor)
        {
            mtlFlags |= MTL_FLAG_BLEND_TERRAIN;
        }
        else
        {
            mtlFlags &= ~MTL_FLAG_BLEND_TERRAIN;
        }
    }

    mtl->SetFlags(mtlFlags);

    mtl->SetLayerActivation(allowLayerActivation);

    // set each material layer
    if (propagationFlags & MTL_PROPAGATE_LAYER_PRESETS)
    {
        SMaterialLayerResources* pMtlLayerResources = mtl->GetMtlLayerResources();
        for (int l(0); l < MTL_LAYER_MAX_SLOTS; ++l)
        {
            if (pMtlLayerResources[l].m_shaderName != materialLayers[l].shader)
            {
                pMtlLayerResources[l].m_shaderName = materialLayers[l].shader;
                pMtlLayerResources[l].m_bRegetPublicParams = true;
            }

            if (materialLayers[l].bNoDraw)
            {
                pMtlLayerResources[l].m_nFlags |= MTL_LAYER_USAGE_NODRAW;
            }
            else
            {
                pMtlLayerResources[l].m_nFlags &= ~MTL_LAYER_USAGE_NODRAW;
            }

            if (materialLayers[l].bFadeOut)
            {
                pMtlLayerResources[l].m_nFlags |= MTL_LAYER_USAGE_FADEOUT;
            }
            else
            {
                pMtlLayerResources[l].m_nFlags &= ~MTL_LAYER_USAGE_FADEOUT;
            }
        }
    }

    if (propagationFlags & MTL_PROPAGATE_MATERIAL_SETTINGS)
    {
        mtl->SetSurfaceTypeName(surfaceType);
        // If shader name is different reload shader.
        mtl->SetShaderName(shader);
    }

    GetShaderResources(mtl->GetShaderResources(), propagationFlags);
}

void CMaterialUI::SetTextureNames(CMaterial* mtl)
{
    SInputShaderResources& sr = mtl->GetShaderResources();

    for ( auto& iter : sr.m_TexturesResourcesMap )
    {
        uint16      texId = iter.first;
        if (!MaterialHelpers::IsAdjustableTexSlot((EEfResTextures)texId))
        {
            continue;
        }

        SEfResTexture*      pTextureRes = &(iter.second);
        textureVars[texId]->Set(pTextureRes->m_Name.c_str());
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CMtlPickCallback
    : public IPickObjectCallback
{
public:
    CMtlPickCallback() { m_bActive = true; };
    //! Called when object picked.
    virtual void OnPick(CBaseObject* picked)
    {
        m_bActive = false;
        CMaterial* pMtl = picked->GetMaterial();
        if (pMtl)
        {
            GetIEditor()->OpenMaterialLibrary(pMtl);
        }
        delete this;
    }
    //! Called when pick mode canceled.
    virtual void OnCancelPick()
    {
        m_bActive = false;
        delete this;
    }
    //! Return true if specified object is pickable.
    virtual bool OnPickFilter(CBaseObject* filterObject)
    {
        // Check if object have material.
        if (filterObject->GetMaterial())
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    static bool IsActive() { return m_bActive; };
private:
    static bool m_bActive;
};
bool CMtlPickCallback::m_bActive = false;
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// CMaterialDialog implementation.
//////////////////////////////////////////////////////////////////////////
CMaterialDialog::CMaterialDialog(QWidget* parent /* = 0 */)
    : QMainWindow(parent)
    , m_wndMtlBrowser(0)
{
    m_propsCtrl = new TwoColumnPropertyControl;
    m_propsCtrl->Setup(true, 150);
    m_propsCtrl->SetSavedStateKey("MaterialDialog");
    m_propsCtrl->setMinimumWidth(460);

    m_placeHolderLabel = new QLabel(tr("Select a material in the Material Editor hierarchy to view properties"));
    m_placeHolderLabel->setMinimumHeight(250);
    m_placeHolderLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    SEventLog toolEvent(MATERIAL_EDITOR_NAME, "", MATERIAL_EDITOR_VER);
    GetIEditor()->GetSettingsManager()->RegisterEvent(toolEvent);

    m_pMatManager = GetIEditor()->GetMaterialManager();

    m_shaderGenParamsVars = 0;

    m_textureSlots = 0;

    m_pMaterialUI = new CMaterialUI;

    m_bForceReloadPropsCtrl = true;

    m_pMaterialImageListModel.reset(new QMaterialImageListModel);

    m_pMaterialImageListCtrl.reset(new CMaterialImageListCtrl);
    m_pMaterialImageListCtrl->setModel(m_pMaterialImageListModel.data());

    // Immediately create dialog.
    OnInitDialog();

    GetIEditor()->RegisterNotifyListener(this);
    m_pMatManager->AddListener(this);
    m_propsCtrl->SetUndoCallback(AZStd::bind(&CMaterialDialog::OnUndo, this, AZStd::placeholders::_1));
    m_propsCtrl->SetStoreUndoByItems(false);

    // KDAB_TODO: hack until we have proper signal coming from the IEDitor
    connect(QCoreApplication::eventDispatcher(), &QAbstractEventDispatcher::awake, this, &CMaterialDialog::UpdateActions);
}

//////////////////////////////////////////////////////////////////////////
CMaterialDialog::~CMaterialDialog()
{
    m_pMatManager->RemoveListener(this);
    GetIEditor()->UnregisterNotifyListener(this);
    m_wndMtlBrowser->SetImageListCtrl(NULL);

    delete m_pMaterialUI;
    m_vars = 0;
    m_publicVars = 0;
    m_shaderGenParamsVars = 0;
    m_textureSlots = 0;

    m_propsCtrl->ClearUndoCallback();
    m_propsCtrl->RemoveAllItems();

    SEventLog toolEvent(MATERIAL_EDITOR_NAME, "", MATERIAL_EDITOR_VER);
    GetIEditor()->GetSettingsManager()->UnregisterEvent(toolEvent);
}

BOOL CMaterialDialog::OnInitDialog()
{
    setWindowTitle(tr(LyViewPane::MaterialEditor));
    if (gEnv->p3DEngine)
    {
        ISurfaceTypeManager* pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
        if (pSurfaceTypeManager)
        {
            pSurfaceTypeManager->LoadSurfaceTypes();
        }
    }

    InitToolbar(IDR_DB_MATERIAL_BAR);

    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);

    // hide menu bar
    menuBar()->hide();

    // Create status bar.
    {
        m_statusBar = this->statusBar();
        m_statusBar->setSizeGripEnabled(false);
    }

    QSplitter* centralWidget = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(centralWidget);

    QSplitter* rightWidget = new QSplitter(Qt::Vertical, centralWidget);
    centralWidget->addWidget(rightWidget);

    rightWidget->addWidget(m_propsCtrl);

    m_vars = m_pMaterialUI->CreateVars();
    m_propsCtrl->AddVarBlock(m_vars);

    m_propsCtrl->setEnabled(false);
    m_propsCtrl->hide();

    //////////////////////////////////////////////////////////////////////////
    // Preview Pane
    //////////////////////////////////////////////////////////////////////////
    {
        rightWidget->insertWidget(0, m_pMaterialImageListCtrl.data());

        int h = m_pMaterialImageListCtrl->sizeHint().height();
        m_pMaterialImageListCtrl->hide();
        rightWidget->setSizes({h, height() - h });
    }

    rightWidget->addWidget(m_placeHolderLabel);
    m_placeHolderLabel->setAlignment(Qt::AlignCenter);

    //////////////////////////////////////////////////////////////////////////
    // Browser Pane
    //////////////////////////////////////////////////////////////////////////
    if (!m_wndMtlBrowser)
    {
        m_wndMtlBrowser = new MaterialBrowserWidget(this);
        m_wndMtlBrowser->SetListener(this);
        m_wndMtlBrowser->SetImageListCtrl(m_pMaterialImageListCtrl.data());
        //m_wndMtlBrowser->resize(width() / 3, height());

        centralWidget->insertWidget(0, m_wndMtlBrowser);

        int w = m_wndMtlBrowser->sizeHint().height();
        centralWidget->setSizes({ w, width() - w });
        centralWidget->setStretchFactor(0, 0);
        centralWidget->setStretchFactor(1, 1);

        // Start the background processing of material files after the widget has been initialized
        m_wndMtlBrowser->StartRecordUpdateJobs();
    }

    // Set the image list control to give stretch priority to the other widgets. This is both to avoid resizing the
    // image list control when the window is resized and to avoid an issue with the QSplitter resizing the image list
    // control when enabling/disabling the other two widgets.
    const int materialImageControlIndex = 0;
    const int materialImagePropertiesControlIndex = 1;
    const int materialPlaceholderLabelIndex = 2;
    rightWidget->setStretchFactor(materialImageControlIndex, 0);
    rightWidget->setStretchFactor(materialImagePropertiesControlIndex, 1);
    rightWidget->setStretchFactor(materialPlaceholderLabelIndex, 1);

    resize(1200, 800);

    return true; // return true unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CMaterialDialog::closeEvent(QCloseEvent *ev)
{
    // We call save before running any dtors, as it might trigger a modal dialog / nested event loop
    // asking to overwrite files, and that causes a crash
    m_wndMtlBrowser->SaveCurrentMaterial();
    ev->accept(); // All good, dialog will close now
}

//////////////////////////////////////////////////////////////////////////
// Create the toolbar
void CMaterialDialog::InitToolbar([[maybe_unused]] UINT nToolbarResID)
{
    // detect if the new viewport interaction model is enabled and give
    // feedback to the user that certain operations are not yet compatible
    const bool newViewportInteractionModelEnabled = GetIEditor()->IsNewViewportInteractionModelEnabled();
    const char* const newViewportInteractionModelWarning =
        "This option is currently not available with the new Viewport Interaction Model enabled";

    m_toolbar = addToolBar(tr("Material ToolBar"));
    m_toolbar->setFloatable(false);

    QIcon assignselectionIcon;
    assignselectionIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_assignselection_normal.png" }, QIcon::Normal);
    assignselectionIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_assignselection_active.png" }, QIcon::Active);
    assignselectionIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_assignselection_disabled.png" }, QIcon::Disabled);

    m_assignToSelectionAction =
        m_toolbar->addAction(assignselectionIcon,
            newViewportInteractionModelEnabled
                ? tr(newViewportInteractionModelWarning)
                : tr("Assign Item to Selected Objects"),
            this, SLOT(OnAssignMaterialToSelection()));

    QIcon resetIcon;
    resetIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_reset_normal.png" }, QIcon::Normal);
    resetIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_reset_active.png" }, QIcon::Active);
    resetIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_reset_disabled.png" }, QIcon::Disabled);

    m_resetAction =
        m_toolbar->addAction(resetIcon,
            newViewportInteractionModelEnabled
                ? tr(newViewportInteractionModelWarning)
                : tr("Reset Material on Selection to Default"),
            this, SLOT(OnResetMaterialOnSelection()));

    QIcon getfromselectionIcon;
    getfromselectionIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_getfromselection_normal.png" }, QIcon::Normal);
    getfromselectionIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_getfromselection_active.png" }, QIcon::Active);
    getfromselectionIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_getfromselection_disabled.png" }, QIcon::Disabled);

    m_getFromSelectionAction =
        m_toolbar->addAction(
            getfromselectionIcon,
            newViewportInteractionModelEnabled
                ? tr(newViewportInteractionModelWarning)
                : tr("Get Properties From Selection"),
            this, SLOT(OnGetMaterialFromSelection()));

    QIcon pickIcon;
    pickIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_pick_normal.png" }, QIcon::Normal);
    pickIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_pick_active.png" }, QIcon::Active);
    pickIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_pick_disabled.png" }, QIcon::Disabled);

    m_pickAction = m_toolbar->addAction(
        pickIcon,
        newViewportInteractionModelEnabled
            ? tr(newViewportInteractionModelWarning)
            : tr("Pick Material from Object"),
        this, SLOT(OnPickMtl()));

    m_pickAction->setCheckable(true);

    if (newViewportInteractionModelEnabled)
    {
        m_pickAction->setEnabled(false);
    }

    QAction* sepAction = m_toolbar->addSeparator();
    m_filterTypeSelection = new QComboBox(this);
    m_filterTypeSelection->addItem(tr("All Materials"));
    m_filterTypeSelection->addItem(tr("Used In Level"));
    m_filterTypeSelection->setMinimumWidth(150);
    QAction* cbAction = m_toolbar->addWidget(m_filterTypeSelection);
    m_filterTypeSelection->setCurrentIndex(0);
    connect(m_filterTypeSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(OnChangedBrowserListType(int)));
    m_toolbar->addSeparator();
    QIcon addIcon;
    addIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_add_normal.png" }, QIcon::Normal);
    addIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_add_active.png" }, QIcon::Active);
    addIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_add_disabled.png" }, QIcon::Disabled);
    m_addAction = m_toolbar->addAction(addIcon, tr("Add New Item"), this, SLOT(OnAddItem()));
    QIcon saveIcon;
    saveIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_save_normal.png" }, QIcon::Normal);
    saveIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_save_active.png" }, QIcon::Active);
    saveIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_save_disabled.png" }, QIcon::Disabled);
    m_saveAction = m_toolbar->addAction(saveIcon, tr("Save Item"), this, SLOT(OnSaveItem()));
    QIcon removeIcon;
    removeIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_remove_normal.png" }, QIcon::Normal);
    removeIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_remove_active.png" }, QIcon::Active);
    removeIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_remove_disabled.png" }, QIcon::Disabled);
    m_removeAction = m_toolbar->addAction(removeIcon, tr("Remove Item"), this, SLOT(OnDeleteItem()));
    m_toolbar->addSeparator();
    QIcon copyIcon;
    copyIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_copy_normal.png" }, QIcon::Normal);
    copyIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_copy_active.png" }, QIcon::Active);
    copyIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_copy_disabled.png" }, QIcon::Disabled);
    m_copyAction = m_toolbar->addAction(copyIcon, tr("Copy Material"), this, SLOT(OnCopy()));
    QIcon pasteIcon;
    pasteIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_paste_normal.png" }, QIcon::Normal);
    pasteIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_paste_active.png" }, QIcon::Active);
    pasteIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_paste_disabled.png" }, QIcon::Disabled);
    m_pasteAction = m_toolbar->addAction(pasteIcon, tr("Paste Material"), this, SLOT(OnPaste()));
    m_toolbar->addSeparator();
    QIcon previewIcon;
    previewIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_preview_normal.png" }, QIcon::Normal);
    previewIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_preview_active.png" }, QIcon::Active);
    previewIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_preview_disabled.png" }, QIcon::Disabled);
    m_previewAction = m_toolbar->addAction(previewIcon, tr("Open Large Material Preview Window"), this, SLOT(OnMaterialPreview()));
    m_toolbar->addSeparator();
    QIcon resetViewportIcon;
    resetViewportIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_reset_viewport_normal.png" }, QIcon::Normal);
    resetViewportIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_reset_viewport_active.png" }, QIcon::Active);
    resetViewportIcon.addPixmap(QPixmap{ ":/MaterialDialog/ToolBar/images/materialdialog_reset_viewport_disabled.png" }, QIcon::Disabled);
    m_resetViewporAction = m_toolbar->addAction(resetViewportIcon, tr("Reset Material Viewport"), this, SLOT(OnResetMaterialViewport()));

    UpdateActions();
    setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);

    connect(m_toolbar, &QToolBar::orientationChanged, m_toolbar, [=](Qt::Orientation orientation)
        {
            if (orientation == Qt::Vertical)
            {
                m_toolbar->removeAction(cbAction);
            }
            else
            {
                m_toolbar->insertAction(sepAction, cbAction);
            }
        });
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::ReloadItems()
{
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::OnAddItem()
{
    m_wndMtlBrowser->OnAddNewMaterial();
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::OnSaveItem()
{
    CMaterial* pMtl = GetSelectedMaterial();
    if (pMtl)
    {
        CMaterial* parent = pMtl->GetParent();

        if (!pMtl->Save(false))
        {
            if (!parent)
            {
                QMessageBox::warning(this, QString(), tr("The material file cannot be saved. The file is located in a PAK archive or access is denied"));
            }
        }

        if (parent)
        {
            //The reload function will clear all the sub-material references, and re-create them.
            //Thus pMtl will point to old sub-material that should be deleted instead.
            //So we need to set m_pMatManager's current material to the new one.
            int index = -1;

            //Find the corresponding sub-material and record its index
            for (int i = 0; i < parent->GetSubMaterialCount(); i++)
            {
                if (parent->GetSubMaterial(i) == pMtl)
                {
                    index = i;
                    break;
                }
            }
            pMtl->Reload();

            if (index >= 0 && index < parent->GetSubMaterialCount())
            {
                m_pMatManager->SetCurrentMaterial(parent->GetSubMaterial(index));
            }
            else //If we can't find the sub-material, use parent instead
            {
                m_pMatManager->SetCurrentMaterial(parent);
            }
        }
        else
        {
            pMtl->Reload();
        }

    }
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::OnDeleteItem()
{
    m_wndMtlBrowser->DeleteItem();
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::SetMaterialVars([[maybe_unused]] CMaterial* mtl)
{
}


//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::UpdateShaderParamsUI(CMaterial* pMtl)
{
    //////////////////////////////////////////////////////////////////////////
    // Shader Gen Mask.
    //////////////////////////////////////////////////////////////////////////
    IVariable* shaderGenParamsContainerVar = m_pMaterialUI->tableShaderGenParams.GetVar();
    if (m_propsCtrl->FindVariable(shaderGenParamsContainerVar))
    {
        m_shaderGenParamsVars = pMtl->GetShaderGenParamsVars();
        m_propsCtrl->ReplaceVarBlock(shaderGenParamsContainerVar, m_shaderGenParamsVars);
    }

    //////////////////////////////////////////////////////////////////////////
    // Shader Public Params.
    //////////////////////////////////////////////////////////////////////////
    IVariable* publicVars = m_pMaterialUI->tableShaderParams.GetVar();
    if (m_propsCtrl->FindVariable(publicVars))
    {
        bool bNeedUpdateMaterialFromUI = false;
        CVarBlockPtr pPublicVars = pMtl->GetPublicVars(pMtl->GetShaderResources());
        if (m_publicVars && pPublicVars)
        {
            // list of shader parameters depends on list of shader generation parameters
            // we need to keep values of vars which not presented in every combinations,
            // but probably adjusted by user, to keep his work.
            // m_excludedPublicVars is used for these values
            if (m_excludedPublicVars.pMaterial)
            {
                if (m_excludedPublicVars.pMaterial != pMtl)
                {
                    m_excludedPublicVars.vars.DeleteAllVariables();
                }
                else
                {
                    // find new presented vars in pPublicVars, which not existed in old m_publicVars
                    for (int j = pPublicVars->GetNumVariables() - 1; j >= 0; --j)
                    {
                        IVariable* pVar = pPublicVars->GetVariable(j);
                        bool isVarExist = false;
                        for (int i = m_publicVars->GetNumVariables() - 1; i >= 0; --i)
                        {
                            IVariable* pOldVar = m_publicVars->GetVariable(i);
                            if (!QString::compare(pOldVar->GetName(), pVar->GetName()))
                            {
                                isVarExist = true;
                                break;
                            }
                        }
                        if (!isVarExist) // var exist in new pPublicVars block, but not in previous (m_publicVars)
                        {
                            // try to find value for this var inside "excluded vars" collection
                            for (int i = m_excludedPublicVars.vars.GetNumVariables() - 1; i >= 0; --i)
                            {
                                IVariable* pStoredVar = m_excludedPublicVars.vars.GetVariable(i);
                                if (!QString::compare(pStoredVar->GetName(), pVar->GetName()) && pVar->GetDataType() == pStoredVar->GetDataType())
                                {
                                    pVar->CopyValue(pStoredVar);
                                    m_excludedPublicVars.vars.DeleteVariable(pStoredVar);
                                    bNeedUpdateMaterialFromUI = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            // We only want to collect vars if the old and new block are part of the same
            // material, otherwise we are storing state from one material to an other.
            if (m_excludedPublicVars.pMaterial == pMtl)
            {
                // collect excluded vars from old block (m_publicVars)
                // which exist in m_publicVars but not in a new generated pPublicVars block
                for (int i = m_publicVars->GetNumVariables() - 1; i >= 0; --i)
                {
                    IVariable* pOldVar = m_publicVars->GetVariable(i);
                    bool isVarExist = false;
                    for (int j = pPublicVars->GetNumVariables() - 1; j >= 0; --j)
                    {
                        IVariable* pVar = pPublicVars->GetVariable(j);
                        if (!QString::compare(pOldVar->GetName(), pVar->GetName()))
                        {
                            isVarExist = true;
                            break;
                        }
                    }
                    if (!isVarExist)
                    {
                        m_excludedPublicVars.vars.AddVariable(pOldVar->Clone(false));
                    }
                }
            }
            m_excludedPublicVars.pMaterial = pMtl;
        }

        m_publicVars = pPublicVars;
        if (m_publicVars)
        {
            m_publicVars->Sort();
        }

        m_propsCtrl->ReplaceVarBlock(publicVars, m_publicVars);

        if (m_publicVars && bNeedUpdateMaterialFromUI)
        {
            pMtl->SetPublicVars(m_publicVars, pMtl);
        }
    }
    IVariable* textureSlotsVar = m_pMaterialUI->tableTexture.GetVar();
    if (m_propsCtrl->FindVariable(textureSlotsVar))
    {
        m_textureSlots = pMtl->UpdateTextureNames(m_pMaterialUI->textureVars);
        m_propsCtrl->ReplaceVarBlock(textureSlotsVar, m_textureSlots);
    }

    //////////////////////////////////////////////////////////////////////////
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::SelectItem(CBaseLibraryItem* item, bool bForceReload)
{
    static bool bNoRecursiveSelect = false;
    if (bNoRecursiveSelect)
    {
        return;
    }

    bool bChanged = item != m_pPrevSelectedItem || bForceReload;

    if (!bChanged)
    {
        return;
    }

    m_pPrevSelectedItem = item;

    // Empty preview control.
    //m_previewCtrl.SetEntity(0);
    m_pMatManager->SetCurrentMaterial((CMaterial*)item);

    if (!item)
    {
        m_statusBar->clearMessage();
        m_propsCtrl->setEnabled(false);
        m_propsCtrl->hide();
        m_pMaterialImageListCtrl->hide();
        m_placeHolderLabel->setText(tr("Select a material in the Material Editor hierarchy to view properties"));
        m_placeHolderLabel->show();
        return;
    }

    // Render preview geometry with current material
    CMaterial* mtl = (CMaterial*)item;

    QString statusText;
    if (mtl->IsPureChild() && mtl->GetParent())
    {
        statusText = mtl->GetParent()->GetName() + " [" + mtl->GetName() + "]";
    }
    else
    {
        statusText = mtl->GetName();
    }


    if (mtl->IsDummy())
    {
        statusText += " (Not Found)";
    }
    else if (!mtl->CanModify())
    {
        statusText += " (Read Only)";
    }
    m_statusBar->showMessage(statusText);

    if (mtl->IsMultiSubMaterial())
    {
        // Cannot edit it.
        m_propsCtrl->setEnabled(false);
        m_propsCtrl->EnableUpdateCallback(false);
        m_propsCtrl->hide();

        m_placeHolderLabel->setText(tr("Select a material to view properties"));
        m_placeHolderLabel->show();

        //return;
    }
    else
    {
        m_propsCtrl->setEnabled(true);
        m_propsCtrl->EnableUpdateCallback(false);
        m_propsCtrl->show();
        m_placeHolderLabel->hide();
    }
    m_pMaterialImageListCtrl->show();

    if (m_bForceReloadPropsCtrl)
    {
        // CPropertyCtrlEx skip OnPaint and another methods for redraw
        // OnSize method is forced to invalidate control for redraw
        m_propsCtrl->InvalidateCtrl();
        m_bForceReloadPropsCtrl = false;
    }

    UpdatePreview();

    // Update variables.
    m_propsCtrl->EnableUpdateCallback(false);
    m_pMaterialUI->SetFromMaterial(mtl);
    m_propsCtrl->EnableUpdateCallback(true);

    mtl->SetShaderParamPublicScript();

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Set Shader Gen Params.
    //////////////////////////////////////////////////////////////////////////
    UpdateShaderParamsUI(mtl);
    //////////////////////////////////////////////////////////////////////////

    m_propsCtrl->SetUpdateCallback(AZStd::bind(&CMaterialDialog::OnUpdateProperties, this, AZStd::placeholders::_1));
    m_propsCtrl->EnableUpdateCallback(true);

    if (mtl->IsDummy())
    {
        m_propsCtrl->setEnabled(false);
    }
    else
    {
        m_propsCtrl->setEnabled(true);
        m_propsCtrl->SetGrayed(!mtl->CanModify());
    }
    if (mtl)
    {
        m_pMaterialImageListCtrl->SelectMaterial(mtl);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::OnUpdateProperties(IVariable* var)
{
    CMaterial* mtl = GetSelectedMaterial();
    if (!mtl)
    {
        return;
    }

    bool bShaderChanged = (m_pMaterialUI->shader == var);
    bool bShaderGenMaskChanged = false;
    if (m_shaderGenParamsVars)
    {
        bShaderGenMaskChanged = m_shaderGenParamsVars->IsContainsVariable(var);
    }

    bool bMtlLayersChanged = false;
    SMaterialLayerResources* pMtlLayerResources = mtl->GetMtlLayerResources();
    int nCurrLayer = -1;

    // Check for shader changes
    for (int l(0); l < MTL_LAYER_MAX_SLOTS; ++l)
    {
        if ((m_pMaterialUI->materialLayers[l].shader == var))
        {
            bMtlLayersChanged = true;
            nCurrLayer = l;
            break;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Assign modified Shader Gen Params to shader.
    //////////////////////////////////////////////////////////////////////////
    if (bShaderGenMaskChanged)
    {
        mtl->SetShaderGenParamsVars(m_shaderGenParamsVars);
    }
    //////////////////////////////////////////////////////////////////////////
    // Invalidate material and save changes.
    //m_pMatManager->MarkMaterialAsModified(mtl);
    //

    mtl->RecordUndo("Material parameter", true);
    m_pMaterialUI->SetToMaterial(mtl);
    mtl->Update();

    //
    //////////////////////////////////////////////////////////////////////////
    // Assign new public vars to material.
    // Must be after material update.
    //////////////////////////////////////////////////////////////////////////

    GetIEditor()->SuspendUndo();

    if (m_publicVars != NULL && !bShaderChanged)
    {
        mtl->SetPublicVars(m_publicVars, mtl);
    }

    /*
      bool bUpdateLayers = false;
      for(int l(0); l < MTL_LAYER_MAX_SLOTS; ++l)
      {
        if ( m_varsMtlLayersShaderParams[l] != NULL && l != nCurrLayer)
        {
          SMaterialLayerResources *pCurrResource = pTemplateMtl ? &pTemplateMtl->GetMtlLayerResources()[l] : &pMtlLayerResources[l];
          SShaderItem &pCurrShaderItem = pCurrResource->m_pMatLayer->GetShaderItem();
          CVarBlock* pVarBlock = pTemplateMtl ? pTemplateMtl->GetPublicVars( pCurrResource->m_shaderResources ) : m_varsMtlLayersShaderParams[l];
          mtl->SetPublicVars( pVarBlock, pCurrResource->m_shaderResources, pCurrShaderItem.m_pShaderResources, pCurrShaderItem.m_pShader);
          bUpdateLayers = true;
        }
      }
    */
    //if( bUpdateLayers )
    {
        mtl->UpdateMaterialLayers();
    }

    m_pMaterialUI->PropagateToLinkedMaterial(mtl, m_shaderGenParamsVars);
    if (var)
    {
        GetIEditor()->GetMaterialManager()->HighlightedMaterialChanged(mtl);
        m_pMaterialUI->NotifyObjectsAboutMaterialChange(var);
    }

    GetIEditor()->ResumeUndo();

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    if (bShaderChanged || bShaderGenMaskChanged || bMtlLayersChanged)
    {
        m_pMaterialUI->SetFromMaterial(mtl);
    }
    //m_pMaterialUI->SetTextureNames( mtl );

    UpdatePreview();

    // When shader changed.
    if (bShaderChanged || bShaderGenMaskChanged || bMtlLayersChanged)
    {
        //////////////////////////////////////////////////////////////////////////
        // Set material layers params
        //////////////////////////////////////////////////////////////////////////
        /*
            if( bMtlLayersChanged) // only update changed shader in material layers
            {
              SMaterialLayerResources *pCurrResource = &pMtlLayerResources[nCurrLayer];

              // delete old property item
              if ( m_varsMtlLayersShaderParamsItems[nCurrLayer] )
              {
                m_propsCtrl->DeleteItem( m_varsMtlLayersShaderParamsItems[nCurrLayer] );
                m_varsMtlLayersShaderParamsItems[nCurrLayer] = 0;
              }

              m_varsMtlLayersShaderParams[nCurrLayer] = mtl->GetPublicVars( pCurrResource->m_shaderResources );

              if ( m_varsMtlLayersShaderParams[nCurrLayer] )
              {
                m_varsMtlLayersShaderParamsItems[nCurrLayer] = m_propsCtrl->AddVarBlockAt( m_varsMtlLayersShaderParams[nCurrLayer], "Shader Params", m_varsMtlLayersShaderItems[nCurrLayer] );
              }
            }
                */

        UpdateShaderParamsUI(mtl);
    }

    if (bShaderGenMaskChanged || bShaderChanged || bMtlLayersChanged)
    {
        m_propsCtrl->InvalidateCtrl();
    }

    m_pMaterialImageListModel->InvalidateMaterial(mtl);
}

//////////////////////////////////////////////////////////////////////////
CMaterial* CMaterialDialog::GetSelectedMaterial()
{
    CBaseLibraryItem* pItem = m_pMatManager->GetCurrentMaterial();
    return (CMaterial*)pItem;
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::OnAssignMaterialToSelection()
{
    CUndo undo("Assign Material To Selection");
    GetIEditor()->GetMaterialManager()->Command_AssignToSelection();
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::OnSelectAssignedObjects()
{
    CUndo undo("Select Objects With Current Material");
    GetIEditor()->GetMaterialManager()->Command_SelectAssignedObjects();
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::OnResetMaterialOnSelection()
{
    GetIEditor()->GetMaterialManager()->Command_ResetSelection();
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::OnGetMaterialFromSelection()
{
    GetIEditor()->GetMaterialManager()->Command_SelectFromObject();
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::DeleteItem([[maybe_unused]] CBaseLibraryItem* pItem)
{
    m_wndMtlBrowser->DeleteItem();
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////

void CMaterialDialog::UpdateActions()
{
    if (isHidden())
    {
        return;
    }

    CMaterial* mtl = GetSelectedMaterial();
    if (mtl && mtl->CanModify(false))
    {
        m_saveAction->setEnabled(true);
    }
    else
    {
        m_saveAction->setEnabled(false);
    }

    if (GetIEditor()->GetEditTool() && GetIEditor()->GetEditTool()->GetClassDesc() && QString::compare(GetIEditor()->GetEditTool()->GetClassDesc()->ClassName(), "EditTool.PickMaterial") == 0)
    {
        m_pickAction->setChecked(true);
    }
    else
    {
        m_pickAction->setChecked(false);
    }

    if (mtl && (!GetIEditor()->GetSelection()->IsEmpty() || GetIEditor()->IsInPreviewMode()))
    {
        m_assignToSelectionAction->setEnabled(true);
    }
    else
    {
        m_assignToSelectionAction->setEnabled(false);
    }

    if (!GetIEditor()->GetSelection()->IsEmpty() || GetIEditor()->IsInPreviewMode())
    {
        m_resetAction->setEnabled(true);
        m_getFromSelectionAction->setEnabled(true);
    }
    else
    {
        m_resetAction->setEnabled(false);
        m_getFromSelectionAction->setEnabled(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::OnPickMtl()
{
    if (GetIEditor()->GetEditTool() && QString::compare(GetIEditor()->GetEditTool()->GetClassDesc()->ClassName(), "EditTool.PickMaterial") == 0)
    {
        GetIEditor()->SetEditTool(NULL);
    }
    else
    {
        GetIEditor()->SetEditTool("EditTool.PickMaterial");
    }
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::OnCopy()
{
    m_wndMtlBrowser->OnCopy();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::OnPaste()
{
    m_wndMtlBrowser->OnPaste();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::OnMaterialPreview()
{
    if (!m_pPreviewDlg)
    {
        m_pPreviewDlg = new CMatEditPreviewDlg(this);
        m_pPreviewDlg->show();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CMaterialDialog::SetItemName(CBaseLibraryItem* item, const QString& groupName, const QString& itemName)
{
    assert(item);
    // Make prototype name.
    QString fullName = groupName + "/" + itemName;
    IDataBaseItem* pOtherItem = m_pMatManager->FindItemByName(fullName);
    if (pOtherItem && pOtherItem != item)
    {
        // Ensure uniqness of name.
        Warning("Duplicate Item Name %s", fullName.toUtf8().data());
        return false;
    }
    else
    {
        item->SetName(fullName);
    }
    return true;
}


//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::OnBrowserSelectItem(IDataBaseItem* pItem, bool bForce)
{
    SelectItem((CBaseLibraryItem*)pItem, bForce);
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::UpdatePreview()
{
};

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::OnChangedBrowserListType(int sel)
{
    m_wndMtlBrowser->ShowOnlyLevelMaterials(sel == 1);
    m_pMatManager->SetCurrentMaterial(0);
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::OnUndo(IVariable* pVar)
{
    if (!m_pMatManager->GetCurrentMaterial())
    {
        return;
    }

    QString undoName;
    if (pVar)
    {
        undoName = tr("%1 modified").arg(pVar->GetName());
    }
    else
    {
        undoName = tr("Material parameter was modified");
    }

    if (!CUndo::IsRecording())
    {
        if (!CUndo::IsSuspended())
        {
            CUndo undo(undoName.toUtf8().data());
            m_pMatManager->GetCurrentMaterial()->RecordUndo(undoName.toUtf8().data(), true);
        }
    }
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialDialog::OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event)
{
    switch (event)
    {
    case EDB_ITEM_EVENT_UPDATE_PROPERTIES:
        if (pItem && pItem == m_pMatManager->GetCurrentMaterial())
        {
            SelectItem(m_pMatManager->GetCurrentMaterial(), true);
        }
        break;
    }
}

// If an object is selected or de-selected, update the available actions in the Material Editor toolbar
void CMaterialDialog::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnSelectionChange:
        UpdateActions();
        break;
    case eNotify_OnCloseScene:
    case eNotify_OnEndNewScene:
    case eNotify_OnEndSceneOpen:
        m_filterTypeSelection->setCurrentIndex(0);
        break;
    }
}

void CMaterialDialog::OnResetMaterialViewport()
{
    m_pMaterialImageListCtrl->LoadModel();
}

#include <Material/moc_MaterialDialog.cpp>
