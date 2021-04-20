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

#include "Material.h"

// Editor
#include "MaterialHelpers.h"
#include "MaterialManager.h"
#include "UsedResources.h"
#include "ErrorReport.h"
#include "Include/ISourceControl.h"
#include "Undo/IUndoObject.h"


static SInputShaderResources defaultShaderResource;

//////////////////////////////////////////////////////////////////////////
CMaterial::CMaterial(const QString& name, int nFlags)
    : m_highlightFlags(0)
    , m_dccMaterialHash(0)
{
    m_scFileAttributes = SCC_FILE_ATTRIBUTE_NORMAL;

    m_pParent = 0;

    m_shaderResources = defaultShaderResource;
    m_shaderResources.m_LMaterial.m_Opacity = 1;
    m_shaderResources.m_LMaterial.m_Diffuse.Set(1.0f, 1.0f, 1.0f, 1.0f);
    m_shaderResources.m_LMaterial.m_Specular.Set(0.045f, 0.045f, 0.045f, 1.0f);    // default 59 spec + div by Gamma exponent -> lin
    m_shaderResources.m_LMaterial.m_Smoothness = 10.0f;

    m_mtlFlags = nFlags;
    ZeroStruct(m_shaderItem);

    // Default shader.
    m_shaderName = "Illum";
    m_nShaderGenMask = 0;

    m_name = name;
    m_bRegetPublicParams = true;
    m_bKeepPublicParamsValues = false;
    m_bIgnoreNotifyChange = false;
    m_bDummyMaterial = false;

    m_pMatInfo = NULL;
    m_propagationFlags = 0;

    m_allowLayerActivation = true;
}

CMaterial::CMaterial(const CMaterial& rhs)
    : m_scFileAttributes{rhs.m_scFileAttributes}
    , m_pParent{nullptr}
    , m_shaderResources{rhs.m_shaderResources}
    , m_mtlFlags{rhs.m_mtlFlags}
    , m_shaderName{rhs.m_shaderName}
    , m_nShaderGenMask{rhs.m_nShaderGenMask}
    , m_bRegetPublicParams{rhs.m_bRegetPublicParams}
    , m_bKeepPublicParamsValues{rhs.m_bKeepPublicParamsValues}
    , m_bDummyMaterial{rhs.m_bDummyMaterial}
    , m_pMatInfo{nullptr}
    , m_propagationFlags{rhs.m_propagationFlags}
    , m_allowLayerActivation{rhs.m_allowLayerActivation}
    , m_dccMaterialHash(rhs.m_dccMaterialHash)
{
    ZeroStruct(m_shaderItem);
    m_name = rhs.m_name;
}

//////////////////////////////////////////////////////////////////////////
CMaterial::~CMaterial()
{
    if (IsModified())
    {
        Save(false);
    }

    // Release used shader.
    SAFE_RELEASE(m_shaderItem.m_pShader);
    SAFE_RELEASE(m_shaderItem.m_pShaderResources);

    if (m_pMatInfo)
    {
        m_pMatInfo->SetUserData(0);
        m_pMatInfo = 0;
    }

    if (!m_subMaterials.empty())
    {
        for (int i = 0; i < m_subMaterials.size(); i++)
        {
            if (m_subMaterials[i])
            {
                m_subMaterials[i]->m_pParent = NULL;
            }
        }

        m_subMaterials.clear();
    }

    if (!IsPureChild() && !(GetFlags() & MTL_FLAG_UIMATERIAL))
    {
        // Unregister this material from manager.
        // Don't use here local variable m_pManager. Manager can be destroyed.
        if (GetIEditor()->GetMaterialManager())
        {
            GetIEditor()->GetMaterialManager()->DeleteItem(this);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::SetName(const QString& name)
{
    if (name != m_name)
    {
        QString oldName = GetFullName();
        m_name = name;

        if (!IsPureChild())
        {
            if (GetIEditor()->GetMaterialManager())
            {
                GetIEditor()->GetMaterialManager()->OnRenameItem(this, oldName);
            }

            if (m_pMatInfo)
            {
                GetIEditor()->Get3DEngine()->GetMaterialManager()->RenameMaterial(m_pMatInfo, GetName().toUtf8().data());
            }
        }
        else
        {
            if (m_pMatInfo)
            {
                m_pMatInfo->SetName(m_name.toUtf8().data());
            }
        }

        NotifyChanged();
    }

    if (m_shaderItem.m_pShaderResources)
    {
        // Only for correct warning message purposes.
        m_shaderItem.m_pShaderResources->SetMaterialName(m_name.toUtf8().data());
    }
}


//////////////////////////////////////////////////////////////////////////
QString CMaterial::GetFilename() const
{
    return GetIEditor()->GetMaterialManager()->MaterialToFilename(IsPureChild() && m_pParent ?  m_pParent->m_name : m_name);
}

//////////////////////////////////////////////////////////////////////////
int CMaterial::GetTextureFilenames(QStringList& outFilenames) const
{
    for (auto& iter : m_shaderResources.m_TexturesResourcesMap )
    {
        const SEfResTexture*    pTexture = (const SEfResTexture*) &(iter.second);
        QString                 name = QtUtil::ToQString(pTexture->m_Name);
        if (name.isEmpty())
        {
            AZ_Warning("Shaders System", false, "Error:  CMaterial::GetTextureFilenames - texture slot name does not exist for slot %d", iter.first );
            continue;
        }

        // Collect image filenames
        if (IResourceCompilerHelper::IsSourceImageFormatSupported(name.toUtf8().data()))
        {
            stl::push_back_unique(outFilenames, Path::GamePathToFullPath(name));
        }

        // collect source files used in DCC tools
        QString dccFilename;
        if (CFileUtil::CalculateDccFilename(name, dccFilename))
        {
            stl::push_back_unique(outFilenames, Path::GamePathToFullPath(dccFilename));
        }
    }

    if (IsMultiSubMaterial())
    {
        for (int i = 0; i < GetSubMaterialCount(); ++i)
        {
            CMaterial* pSubMtl = GetSubMaterial(i);
            if (pSubMtl)
            {
                pSubMtl->GetTextureFilenames(outFilenames);
            }
        }
    }

    return outFilenames.size();
}

//////////////////////////////////////////////////////////////////////////
int CMaterial::GetAnyTextureFilenames(QStringList& outFilenames) const
{
    for ( auto& iter : m_shaderResources.m_TexturesResourcesMap )
    {
        QString name = QtUtil::ToQString( iter.second.m_Name);
        if (name.isEmpty())
        {
            continue;
        }

        // Collect any filenames
        stl::push_back_unique(outFilenames, Path::GamePathToFullPath(name));
    }

    if (IsMultiSubMaterial())
    {
        for (int i = 0; i < GetSubMaterialCount(); ++i)
        {
            CMaterial* pSubMtl = GetSubMaterial(i);
            if (pSubMtl)
            {
                pSubMtl->GetAnyTextureFilenames(outFilenames);
            }
        }
    }

    return outFilenames.size();
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::UpdateFileAttributes(bool useSourceControl)
{
    QString filename = GetFilename();
    if (filename.isEmpty())
    {
        return;
    }

    m_scFileAttributes = CFileUtil::GetAttributes(filename.toUtf8().data(), useSourceControl);
}

//////////////////////////////////////////////////////////////////////////
uint32 CMaterial::GetFileAttributes()
{
    if (IsDummy())
    {
        return m_scFileAttributes;
    }

    if (IsPureChild() && m_pParent)
    {
        return m_pParent->GetFileAttributes();
    }

    UpdateFileAttributes();
    return m_scFileAttributes;
};

//////////////////////////////////////////////////////////////////////////
void CMaterial::SetShaderName(const QString& shaderName)
{
    if (m_shaderName != shaderName)
    {
        m_bRegetPublicParams = true;
        m_bKeepPublicParamsValues = false;

        RecordUndo("Change Shader");
    }

    m_shaderName = shaderName;
    if (QString::compare(m_shaderName, "nodraw", Qt::CaseInsensitive) == 0)
    {
        m_mtlFlags |= MTL_FLAG_NODRAW;
    }
    else
    {
        m_mtlFlags &= ~MTL_FLAG_NODRAW;
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::CheckSpecialConditions()
{
    if (QString::compare(m_shaderName, "nodraw", Qt::CaseInsensitive) == 0)
    {
        m_mtlFlags |= MTL_FLAG_NODRAW;
    }
    else
    {
        m_mtlFlags &= ~MTL_FLAG_NODRAW;
    }

    // If environment texture name have auto/nearest cubemap in it, force material to use auto cube-map for it.
    SEfResTexture*      pTextureRes = m_shaderResources.GetTextureResource(EFTT_ENV);
    if (!pTextureRes)
        return;

    if (!pTextureRes->m_Name.empty())
    {
        const char*         sAtPos;
        sAtPos = strstr(pTextureRes->m_Name.c_str(), "auto_2d");
        if (sAtPos)
        {
            pTextureRes->m_Sampler.m_eTexType = eTT_Auto2D;   // Force Auto-2D
        }
        sAtPos = strstr(pTextureRes->m_Name.c_str(), "nearest_cubemap");
        if (sAtPos)
        {
            pTextureRes->m_Sampler.m_eTexType = eTT_NearestCube;  // Force Nearest Cubemap
        }
    }

    // Force auto 2D map if user sets texture type
    if (pTextureRes->m_Sampler.m_eTexType == eTT_Auto2D)
    {
        pTextureRes->m_Name = "auto_2d";
    }

    // Force nearest cube map if user sets texture type
    if (pTextureRes->m_Sampler.m_eTexType == eTT_NearestCube)
    {
        pTextureRes->m_Name = "nearest_cubemap";
        m_mtlFlags |= MTL_FLAG_REQUIRE_NEAREST_CUBEMAP;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CMaterial::LoadShader()
{
    if (m_bDummyMaterial)
    {
        return true;
    }

    CheckSpecialConditions();

    GetIEditor()->GetErrorReport()->SetCurrentValidatorItem(this);

    m_shaderResources.m_ResFlags = m_mtlFlags;

    QString sShader = m_shaderName;
    if (sShader.isEmpty())
    {
        sShader = "<Default>";
    }

    QByteArray n = m_name.toUtf8();
    m_shaderResources.m_szMaterialName = n.data();
    SShaderItem newShaderItem = GetIEditor()->GetRenderer()->EF_LoadShaderItem(sShader.toUtf8().data(), false, 0, &m_shaderResources, m_nShaderGenMask);

    // Shader not found
    if (newShaderItem.m_pShader && (newShaderItem.m_pShader->GetFlags() & EF_NOTFOUND) != 0)
    {
        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Failed to load shader \"%s\" in material \"%s\"", newShaderItem.m_pShader->GetName(), m_name.toUtf8().constData());
    }

    // Release previously used shader (Must be After new shader is loaded, for speed).
    SAFE_RELEASE(m_shaderItem.m_pShader);
    SAFE_RELEASE(m_shaderItem.m_pShaderResources);

    m_shaderItem = newShaderItem;
    if (!m_shaderItem.m_pShader)
    {
        CErrorRecord err;

        err.error = QObject::tr("Failed to Load Shader %1").arg(m_shaderName);
        err.pItem = this;

        GetIEditor()->GetErrorReport()->ReportError(err);
        GetIEditor()->GetErrorReport()->SetCurrentValidatorItem(NULL);
        return false;
    }

    IShader* pShader = m_shaderItem.m_pShader;
    m_nShaderGenMask = pShader->GetGenerationMask();
    if (pShader->GetFlags() & EF_NOPREVIEW)
    {
        m_mtlFlags |= MTL_FLAG_NOPREVIEW;
    }
    else
    {
        m_mtlFlags &= ~MTL_FLAG_NOPREVIEW;
    }

    //////////////////////////////////////////////////////////////////////////
    // Reget shader parms.
    //////////////////////////////////////////////////////////////////////////
    if (m_bRegetPublicParams)
    {
        if (m_bKeepPublicParamsValues)
        {
            m_bKeepPublicParamsValues = false;
            m_publicVarsCache = XmlHelpers::CreateXmlNode("PublicParams");

            MaterialHelpers::SetXmlFromShaderParams(m_shaderResources, m_publicVarsCache);
        }

        m_shaderResources.m_ShaderParams = pShader->GetPublicParams();
        m_bRegetPublicParams = false;
    }

    //////////////////////////////////////////////////////////////////////////
    // If we have XML node with public parameters loaded, apply it on shader parms.
    //////////////////////////////////////////////////////////////////////////
    if (m_publicVarsCache)
    {
        MaterialHelpers::SetShaderParamsFromXml(m_shaderResources, m_publicVarsCache);
        GetIEditor()->GetMaterialManager()->OnUpdateProperties(this, false);
        m_publicVarsCache = 0;
    }

    //////////////////////////////////////////////////////////////////////////
    // Set shader parms.
    if (m_shaderItem.m_pShaderResources)
    {
        m_shaderItem.m_pShaderResources->SetShaderParams(&m_shaderResources, m_shaderItem.m_pShader);
    }
    //////////////////////////////////////////////////////////////////////////

    gEnv->pRenderer->UpdateShaderItem(&m_shaderItem, nullptr);

    //////////////////////////////////////////////////////////////////////////
    // Set Shader Params for material layers
    //////////////////////////////////////////////////////////////////////////
    if (m_pMatInfo)
    {
        UpdateMatInfo();
    }

    GetIEditor()->GetMaterialManager()->OnLoadShader(this);
    GetIEditor()->GetErrorReport()->SetCurrentValidatorItem(NULL);

    return true;
}

bool CMaterial::LoadMaterialLayers()
{
    if (!m_pMatInfo)
    {
        return false;
    }

    if (m_shaderItem.m_pShader && m_shaderItem.m_pShaderResources)
    {
        // mask generation for base material shader
        uint32 nMaskGenBase = m_shaderItem.m_pShader->GetGenerationMask();
        SShaderGen* pShaderGenBase = m_shaderItem.m_pShader->GetGenerationParams();

        for (uint32 l(0); l < MTL_LAYER_MAX_SLOTS; ++l)
        {
            SMaterialLayerResources* pCurrLayer = &m_pMtlLayerResources[l];
            pCurrLayer->m_nFlags |= MTL_FLAG_NODRAW;
            if (!pCurrLayer->m_shaderName.isEmpty())
            {
                if (QString::compare(pCurrLayer->m_shaderName, "nodraw", Qt::CaseInsensitive) == 0)
                {
                    // no shader = skip layer
                    pCurrLayer->m_shaderName.clear();
                    continue;
                }

                IShader* pNewShader = GetIEditor()->GetRenderer()->EF_LoadShader(pCurrLayer->m_shaderName.toUtf8().data(), 0);

                // Check if shader loaded
                if (!pNewShader || (pNewShader->GetFlags() & EF_NOTFOUND) != 0)
                {
                    CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Failed to load material layer shader \"%s\" in material \"%s\"", pCurrLayer->m_shaderName.toUtf8().constData(), m_pMatInfo->GetName());
                    if (!pNewShader)
                    {
                        continue;
                    }
                }

                if (!pCurrLayer->m_pMatLayer)
                {
                    pCurrLayer->m_pMatLayer = m_pMatInfo->CreateLayer();
                }

                // mask generation for base material shader
                uint64 nMaskGenLayer = 0;
                SShaderGen* pShaderGenLayer = pNewShader->GetGenerationParams();
                if (pShaderGenBase && pShaderGenLayer)
                {
                    for (int nLayerBit(0); nLayerBit < pShaderGenLayer->m_BitMask.size(); ++nLayerBit)
                    {
                        SShaderGenBit* pLayerBit = pShaderGenLayer->m_BitMask[nLayerBit];

                        for (int nBaseBit(0); nBaseBit < pShaderGenBase->m_BitMask.size(); ++nBaseBit)
                        {
                            SShaderGenBit* pBaseBit = pShaderGenBase->m_BitMask[nBaseBit];

                            // Need to check if flag name is common to both shaders (since flags values can be different), if so activate it on this layer
                            if (nMaskGenBase & pBaseBit->m_Mask)
                            {
                                if (!pLayerBit->m_ParamName.empty() && !pBaseBit->m_ParamName.empty())
                                {
                                    if (pLayerBit->m_ParamName ==  pBaseBit->m_ParamName)
                                    {
                                        nMaskGenLayer |= pLayerBit->m_Mask;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                // Reload with proper flags
                SShaderItem newShaderItem = GetIEditor()->GetRenderer()->EF_LoadShaderItem(pCurrLayer->m_shaderName.toUtf8().data(), false, 0, &pCurrLayer->m_shaderResources, nMaskGenLayer);
                if (!newShaderItem.m_pShader || (newShaderItem.m_pShader->GetFlags() & EF_NOTFOUND) != 0)
                {
                    CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Failed to load material layer shader \"%s\" in material \"%s\"", pCurrLayer->m_shaderName.toUtf8().data(), m_pMatInfo->GetName());
                    if (!newShaderItem.m_pShader)
                    {
                        continue;
                    }
                }

                SShaderItem& pCurrShaderItem = pCurrLayer->m_pMatLayer->GetShaderItem();

                if (newShaderItem.m_pShader)
                {
                    newShaderItem.m_pShader->AddRef();
                }

                // Release previously used shader (Must be After new shader is loaded, for speed).
                SAFE_RELEASE(pCurrShaderItem.m_pShader);
                SAFE_RELEASE(pCurrShaderItem.m_pShaderResources);
                SAFE_RELEASE(newShaderItem.m_pShaderResources);

                pCurrShaderItem.m_pShader = newShaderItem.m_pShader;
                // Copy resources from base material
                pCurrShaderItem.m_pShaderResources = m_shaderItem.m_pShaderResources->Clone();
                pCurrShaderItem.m_nTechnique = newShaderItem.m_nTechnique;
                pCurrShaderItem.m_nPreprocessFlags = newShaderItem.m_nPreprocessFlags;

                // set default params
                if (pCurrLayer->m_bRegetPublicParams)
                {
                    pCurrLayer->m_shaderResources.m_ShaderParams = pCurrShaderItem.m_pShader->GetPublicParams();
                }

                pCurrLayer->m_bRegetPublicParams = false;

                if (pCurrLayer->m_publicVarsCache)
                {
                    MaterialHelpers::SetShaderParamsFromXml(pCurrLayer->m_shaderResources, pCurrLayer->m_publicVarsCache);
                    pCurrLayer->m_publicVarsCache = 0;
                }

                if (pCurrShaderItem.m_pShaderResources)
                {
                    pCurrShaderItem.m_pShaderResources->SetShaderParams(&pCurrLayer->m_shaderResources, pCurrShaderItem.m_pShader);
                }

                // Activate layer
                pCurrLayer->m_nFlags &= ~MTL_FLAG_NODRAW;
            }
        }

        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////

void CMaterial::UpdateMaterialLayers()
{
    if (m_pMatInfo && m_shaderItem.m_pShaderResources)
    {
        m_pMatInfo->SetLayerCount(MTL_LAYER_MAX_SLOTS);

        uint8 nMaterialLayerFlags = 0;

        for (int l(0); l < MTL_LAYER_MAX_SLOTS; ++l)
        {
            SMaterialLayerResources* pCurrLayer = &m_pMtlLayerResources[l];
            if (pCurrLayer && !pCurrLayer->m_shaderName.isEmpty() && pCurrLayer->m_pMatLayer)
            {
                pCurrLayer->m_pMatLayer->SetFlags(pCurrLayer->m_nFlags);
                m_pMatInfo->SetLayer(l, pCurrLayer->m_pMatLayer);

                if ((pCurrLayer->m_nFlags & MTL_LAYER_USAGE_NODRAW))
                {
                    if (!QString::compare(pCurrLayer->m_shaderName, "frozenlayerwip", Qt::CaseInsensitive))
                    {
                        nMaterialLayerFlags |= MTL_LAYER_FROZEN;
                    }
                }
            }
        }

        if (m_shaderItem.m_pShaderResources)
        {
            m_shaderItem.m_pShaderResources->SetMtlLayerNoDrawFlags(nMaterialLayerFlags);
        }
    }
}

void CMaterial::UpdateMatInfo()
{
    if (m_pMatInfo)
    {
        // Mark material invalid.
        m_pMatInfo->SetFlags(m_mtlFlags);
        m_pMatInfo->SetShaderItem(m_shaderItem);
        m_pMatInfo->SetShaderName(m_shaderName.toUtf8().constData());
        m_pMatInfo->SetSurfaceType(m_surfaceType.toUtf8().constData());

        LoadMaterialLayers();
        UpdateMaterialLayers();

        m_pMatInfo->SetMaterialLinkName(m_linkedMaterial.toUtf8().data());

        if (IsMultiSubMaterial())
        {
            m_pMatInfo->SetSubMtlCount(m_subMaterials.size());
            for (unsigned int i = 0; i < m_subMaterials.size(); i++)
            {
                if (m_subMaterials[i])
                {
                    m_pMatInfo->SetSubMtl(i, m_subMaterials[i]->GetMatInfo());
                }
                else
                {
                    m_pMatInfo->SetSubMtl(i, NULL);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CVarBlock* CMaterial::GetPublicVars(SInputShaderResources& pShaderResources)
{
    return MaterialHelpers::GetPublicVars(pShaderResources);
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::SetShaderParamPublicScript()
{
    IShader* pShader = m_shaderItem.m_pShader;

    if (!pShader)
    {
        return;
    }

    if (m_shaderResources.m_ShaderParams.size() == 0 || pShader->GetPublicParams().size() == 0)
    {
        return;
    }

    // We want to inspect public shader param and paste the m_script into our shader resource param script
    for (int i = 0; i < m_shaderResources.m_ShaderParams.size(); ++i)
    {
        SShaderParam &currentShaderParam = m_shaderResources.m_ShaderParams.at(i);
        for (int j = 0; j < pShader->GetPublicParams().size(); ++j)
        {
            const SShaderParam &publicShaderParam = pShader->GetPublicParams().at(j);
            if ((currentShaderParam.m_Name == publicShaderParam.m_Name) && (currentShaderParam.m_Type == publicShaderParam.m_Type))
            {
                currentShaderParam.m_Script = publicShaderParam.m_Script;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::SetPublicVars(CVarBlock* pPublicVars, CMaterial* pMtl)
{
    if (!pMtl->GetShaderResources().m_ShaderParams.empty())
    {
        RecordUndo("Set Public Vars");
    }

    MaterialHelpers::SetPublicVars(pPublicVars, pMtl->GetShaderResources(), pMtl->GetShaderItem().m_pShaderResources, pMtl->GetShaderItem().m_pShader);

    GetIEditor()->GetMaterialManager()->OnUpdateProperties(this, false);
}

//////////////////////////////////////////////////////////////////////////
CVarBlock* CMaterial::GetShaderGenParamsVars()
{
    return MaterialHelpers::GetShaderGenParamsVars(GetShaderItem().m_pShader, m_nShaderGenMask);
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::SetShaderGenParamsVars(CVarBlock* pBlock)
{
    RecordUndo("Change Shader GenMask");

    uint64 nGenMask = MaterialHelpers::SetShaderGenParamsVars(GetShaderItem().m_pShader, pBlock);
    if (m_nShaderGenMask != nGenMask)
    {
        m_bRegetPublicParams = true;
        m_bKeepPublicParamsValues = true;
        m_nShaderGenMask = nGenMask;
    }
}

//////////////////////////////////////////////////////////////////////////
unsigned int CMaterial::GetTexmapUsageMask() const
{
    int mask = 0;
    if (m_shaderItem.m_pShader)
    {
        IShader* pTempl = m_shaderItem.m_pShader;
        if (pTempl)
        {
            mask = pTempl->GetUsedTextureTypes();
        }
    }
    return mask;
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::Update()
{
    // Reload shader item with new resources and shader.
    LoadShader();

    // Mark library as modified.
    SetModified();

    GetIEditor()->SetModifiedFlag();

    // When modifying pure child, mark his parent as modified.
    if (IsPureChild() && m_pParent)
    {
        m_pParent->SetModified();
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CMaterial::Serialize(SerializeContext& ctx)
{
    //CBaseLibraryItem::Serialize( ctx );

    XmlNodeRef node = ctx.node;
    if (ctx.bLoading)
    {
        m_bIgnoreNotifyChange = true;
        m_bRegetPublicParams = true;

        SInputShaderResources& sr = m_shaderResources;
        m_shaderResources = defaultShaderResource;

        // Loading
        int flags = m_mtlFlags;
        if (node->getAttr("MtlFlags", flags))
        {
            m_mtlFlags &= ~(MTL_FLAGS_SAVE_MASK);
            m_mtlFlags |= (flags & (MTL_FLAGS_SAVE_MASK));
        }

        uint32 dccMaterialHash = 0;
        node->getAttr("DccMaterialHash", dccMaterialHash);
        SetDccMaterialHash(dccMaterialHash);

        if (!IsMultiSubMaterial())
        {
            node->getAttr("Shader", m_shaderName);
            node->getAttr("GenMask", m_nShaderGenMask);

            if (!(m_mtlFlags & MTL_64BIT_SHADERGENMASK))
            {
                uint32 nShaderGenMask = 0;
                node->getAttr("GenMask", nShaderGenMask);
                m_nShaderGenMask = nShaderGenMask;
            }
            else
            {
                node->getAttr("GenMask", m_nShaderGenMask);
            }

            // Remap flags if needed
            if (!(m_mtlFlags & MTL_64BIT_SHADERGENMASK))
            {
                m_nShaderGenMask = GetIEditor()->GetRenderer()->EF_GetRemapedShaderMaskGen(m_shaderName.toUtf8().data(), m_nShaderGenMask);
                m_mtlFlags |= MTL_64BIT_SHADERGENMASK;
            }

            if (node->getAttr("StringGenMask", m_pszShaderGenMask))
            {
                m_nShaderGenMask = GetIEditor()->GetRenderer()->EF_GetShaderGlobalMaskGenFromString(m_shaderName.toUtf8().data(), m_pszShaderGenMask.toUtf8().data(), m_nShaderGenMask);           // get common mask gen
            }
            else
            {
                // version doens't has string gen mask yet ? Remap flags if needed
                m_nShaderGenMask = GetIEditor()->GetRenderer()->EF_GetRemapedShaderMaskGen(m_shaderName.toUtf8().data(), m_nShaderGenMask, ((m_mtlFlags & MTL_64BIT_SHADERGENMASK) != 0));
            }
            m_mtlFlags |= MTL_64BIT_SHADERGENMASK;

            node->getAttr("SurfaceType", m_surfaceType);
            node->getAttr("LayerAct", m_allowLayerActivation);

            MaterialHelpers::SetLightingFromXml(sr, node);

            MaterialHelpers::SetTexturesFromXml(sr, node);
            MaterialHelpers::MigrateXmlLegacyData(sr, node);
        }

        //////////////////////////////////////////////////////////////////////////
        // Check if we have a link name and if any propagation settings were
        // present
        //////////////////////////////////////////////////////////////////////////
        XmlNodeRef pLinkName = node->findChild("MaterialLinkName");
        if (pLinkName)
        {
            m_linkedMaterial = pLinkName->getAttr("name");
        }
        else
        {
            m_linkedMaterial = QString();
        }

        XmlNodeRef pPropagationFlags = node->findChild("MaterialPropagationFlags");
        if (pPropagationFlags)
        {
            pPropagationFlags->getAttr("flags", m_propagationFlags);
        }
        else
        {
            m_propagationFlags = 0;
        }

        //////////////////////////////////////////////////////////////////////////
        // Check if we have vertex deform.
        //////////////////////////////////////////////////////////////////////////
        MaterialHelpers::SetVertexDeformFromXml(m_shaderResources, node);

        // Serialize sub materials.

        const auto ResizeSubMaterials = [this](size_t count)
        {
            for (size_t i = count; i < m_subMaterials.size(); ++i)
            {
                if (auto& pSubMtl = m_subMaterials[i])
                {
                    pSubMtl->m_pParent = nullptr;
                }
            }
            m_subMaterials.resize(count);
        };

        XmlNodeRef childsNode = node->findChild("SubMaterials");
        if (childsNode && !ctx.bIgnoreChilds)
        {
            QString name;
            int nSubMtls = childsNode->getChildCount();
            ResizeSubMaterials(nSubMtls);
            for (int i = 0; i < nSubMtls; i++)
            {
                auto& pSubMtl = m_subMaterials[i];
                XmlNodeRef mtlNode = childsNode->getChild(i);
                if (mtlNode->isTag("Material"))
                {
                    mtlNode->getAttr("Name", name);
                    if (pSubMtl && pSubMtl->IsPureChild())
                    {
                        pSubMtl->SetName(name);
                    }
                    else
                    {
                        if (pSubMtl)
                        {
                            pSubMtl->m_pParent = nullptr;
                        }

                        pSubMtl = new CMaterial(name, MTL_FLAG_PURE_CHILD);
                        pSubMtl->m_pParent = this;
                    }

                    SerializeContext childCtx(ctx);
                    childCtx.node = mtlNode;
                    pSubMtl->Serialize(childCtx);

                    pSubMtl->m_shaderResources.m_SortPrio = nSubMtls - i - 1;
                }
                else
                {
                    if (pSubMtl)
                    {
                        pSubMtl->m_pParent = nullptr;
                        pSubMtl = nullptr;
                    }

                    if (mtlNode->getAttr("Name", name))
                    {
                        CMaterial* pMtl = GetIEditor()->GetMaterialManager()->LoadMaterial(name);
                        if (pMtl && !pMtl->IsMultiSubMaterial())
                        {
                            pSubMtl = pMtl;
                        }
                    }
                }
            }

            m_subMaterials.erase(std::remove(m_subMaterials.begin(), m_subMaterials.end(), nullptr), m_subMaterials.end());
        }
        else
        {
            ResizeSubMaterials(0);
        }

        UpdateMatInfo();

        //////////////////////////////////////////////////////////////////////////
        // Load public parameters.
        //////////////////////////////////////////////////////////////////////////
        m_publicVarsCache = node->findChild("PublicParams");

        //////////////////////////////////////////////////////////////////////////
        // Load material layers data
        //////////////////////////////////////////////////////////////////////////
        XmlNodeRef mtlLayersNode = node->findChild("MaterialLayers");
        if (mtlLayersNode)
        {
            int nChildCount = min((int) MTL_LAYER_MAX_SLOTS, (int)  mtlLayersNode->getChildCount());
            for (int l = 0; l < nChildCount; ++l)
            {
                XmlNodeRef layerNode = mtlLayersNode->getChild(l);
                if (layerNode)
                {
                    if (layerNode->getAttr("Name", m_pMtlLayerResources[l].m_shaderName))
                    {
                        m_pMtlLayerResources[l].m_bRegetPublicParams = true;

                        bool bNoDraw = false;
                        layerNode->getAttr("NoDraw", bNoDraw);

                        m_pMtlLayerResources[l].m_publicVarsCache = layerNode->findChild("PublicParams");

                        if (bNoDraw)
                        {
                            m_pMtlLayerResources[l].m_nFlags |= MTL_LAYER_USAGE_NODRAW;
                        }
                        else
                        {
                            m_pMtlLayerResources[l].m_nFlags &= ~MTL_LAYER_USAGE_NODRAW;
                        }


                        bool bFadeOut = false;
                        layerNode->getAttr("FadeOut", bFadeOut);
                        if (bFadeOut)
                        {
                            m_pMtlLayerResources[l].m_nFlags |= MTL_LAYER_USAGE_FADEOUT;
                        }
                        else
                        {
                            m_pMtlLayerResources[l].m_nFlags &= ~MTL_LAYER_USAGE_FADEOUT;
                        }
                    }
                }
            }
        }

        if (ctx.bUndo)
        {
            LoadShader();
            UpdateMatInfo();
        }

        m_bIgnoreNotifyChange = false;

        // If copy pasting or undo send update event.
        if (ctx.bCopyPaste || ctx.bUndo)
        {
            NotifyChanged();
        }

        // NotifyChanged calls SetModified but since we just loaded it, its not actually changed.
        SetModified(false);
    }
    else // If !ctx.bLoading
    {
        int extFlags = MTL_64BIT_SHADERGENMASK;
        {
            const QString& name = GetName();
            const size_t len = name.length();
            if (len > 4 && strstri(name.toUtf8().data() + (len - 4), "_con"))
            {
                extFlags |= MTL_FLAG_CONSOLE_MAT;
            }
        }

        // Saving.
        node->setAttr("MtlFlags", m_mtlFlags | extFlags);
        node->setAttr("DccMaterialHash", GetDccMaterialHash());

        if (!IsMultiSubMaterial())
        {
            // store shader gen bit mask string
            m_pszShaderGenMask = GetIEditor()->GetRenderer()->EF_GetStringFromShaderGlobalMaskGen(m_shaderName.toUtf8().data(), m_nShaderGenMask).c_str();

            node->setAttr("Shader", m_shaderName.toUtf8().data());
            node->setAttr("GenMask", m_nShaderGenMask);
            node->setAttr("StringGenMask", m_pszShaderGenMask.toUtf8().data());
            node->setAttr("SurfaceType", m_surfaceType.toUtf8().data());

            //if (!m_shaderName.IsEmpty() && (stricmp(m_shaderName,"nodraw") != 0))
            {
                MaterialHelpers::SetXmlFromLighting(m_shaderResources, node);
                MaterialHelpers::SetXmlFromTextures(m_shaderResources, node);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // Save out the link name if present and the propagation flags
        //////////////////////////////////////////////////////////////////////////
        if (!m_linkedMaterial.isEmpty())
        {
            XmlNodeRef pLinkName = node->newChild("MaterialLinkName");
            pLinkName->setAttr("name", m_linkedMaterial.toUtf8().data());
        }

        if (m_propagationFlags)
        {
            XmlNodeRef pPropagationFlags = node->newChild("MaterialPropagationFlags");
            pPropagationFlags->setAttr("flags", m_propagationFlags);
        }

        //////////////////////////////////////////////////////////////////////////
        // Check if we have vertex deform.
        //////////////////////////////////////////////////////////////////////////
        MaterialHelpers::SetXmlFromVertexDeform(m_shaderResources, node);

        if (GetSubMaterialCount() > 0)
        {
            // Serialize sub materials.

            // Let's not serialize empty submaterials at the end of the list.
            // Note that IDs of the remaining submaterials stay intact.
            int count = GetSubMaterialCount();
            while (count > 0 && !GetSubMaterial(count - 1))
            {
                --count;
            }

            XmlNodeRef childsNode = node->newChild("SubMaterials");

            for (int i = 0; i < count; ++i)
            {
                CMaterial* const pSubMtl = GetSubMaterial(i);
                if (pSubMtl && pSubMtl->IsPureChild())
                {
                    XmlNodeRef mtlNode = childsNode->newChild("Material");
                    mtlNode->setAttr("Name", pSubMtl->GetName().toUtf8().data());
                    SerializeContext childCtx(ctx);
                    childCtx.node = mtlNode;
                    pSubMtl->Serialize(childCtx);
                }
                else
                {
                    XmlNodeRef mtlNode = childsNode->newChild("MaterialRef");
                    if (pSubMtl)
                    {
                        mtlNode->setAttr("Name", pSubMtl->GetName().toUtf8().data());
                    }
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // Save public parameters.
        //////////////////////////////////////////////////////////////////////////
        if (m_publicVarsCache)
        {
            node->addChild(m_publicVarsCache);
        }
        else if (!m_shaderResources.m_ShaderParams.empty())
        {
            XmlNodeRef publicsNode = node->newChild("PublicParams");
            MaterialHelpers::SetXmlFromShaderParams(m_shaderResources, publicsNode);
        }

        //////////////////////////////////////////////////////////////////////////
        // Save material layers data
        //////////////////////////////////////////////////////////////////////////

        bool bMaterialLayers = false;
        for (int l(0); l < MTL_LAYER_MAX_SLOTS; ++l)
        {
            if (!m_pMtlLayerResources[l].m_shaderName.isEmpty())
            {
                bMaterialLayers = true;
                break;
            }
        }

        if (bMaterialLayers)
        {
            XmlNodeRef mtlLayersNode = node->newChild("MaterialLayers");
            for (int l(0); l < MTL_LAYER_MAX_SLOTS; ++l)
            {
                XmlNodeRef layerNode = mtlLayersNode->newChild("Layer");
                if (!m_pMtlLayerResources[l].m_shaderName.isEmpty())
                {
                    layerNode->setAttr("Name", m_pMtlLayerResources[l].m_shaderName.toUtf8().data());
                    layerNode->setAttr("NoDraw", m_pMtlLayerResources[l].m_nFlags & MTL_LAYER_USAGE_NODRAW);
                    layerNode->setAttr("FadeOut", m_pMtlLayerResources[l].m_nFlags & MTL_LAYER_USAGE_FADEOUT);

                    if (m_pMtlLayerResources[l].m_publicVarsCache)
                    {
                        layerNode->addChild(m_pMtlLayerResources[l].m_publicVarsCache);
                    }
                    else if (!m_pMtlLayerResources[l].m_shaderResources.m_ShaderParams.empty())
                    {
                        XmlNodeRef publicsNode = layerNode->newChild("PublicParams");
                        MaterialHelpers::SetXmlFromShaderParams(m_pMtlLayerResources[l].m_shaderResources, publicsNode);
                    }
                }
            }
        }

        if (GetSubMaterialCount() == 0 || GetParent())
        {
            node->setAttr("LayerAct", m_allowLayerActivation);
        }
    }
}

/*
//////////////////////////////////////////////////////////////////////////
void CMaterial::SerializePublics( XmlNodeRef &node,bool bLoading )
{
if (bLoading)
{
}
else
{
if (m_shaderParams.empty())
return;
XmlNodeRef publicsNode = node->newChild( "PublicParams" );

for (int i = 0; i < m_shaderParams.size(); i++)
{
XmlNodeRef paramNode = node->newChild( "Param" );
SShaderParam *pParam = &m_shaderParams[i];
paramNode->setAttr( "Name",pParam->m_Name );
switch (pParam->m_Type)
{
case eType_BYTE:
paramNode->setAttr( "Value",(int)pParam->m_Value.m_Byte );
paramNode->setAttr( "Type",(int)pParam->m_Value.m_Byte );
break;
case eType_SHORT:
paramNode->setAttr( "Value",(int)pParam->m_Value.m_Short );
break;
case eType_INT:
paramNode->setAttr( "Value",(int)pParam->m_Value.m_Int );
break;
case eType_FLOAT:
paramNode->setAttr( "Value",pParam->m_Value.m_Float );
break;
case eType_STRING:
paramNode->setAttr( "Value",pParam->m_Value.m_String );
break;
case eType_FCOLOR:
paramNode->setAttr( "Value",Vec3(pParam->m_Value.m_Color[0],pParam->m_Value.m_Color[1],pParam->m_Value.m_Color[2]) );
break;
case eType_VECTOR:
paramNode->setAttr( "Value",Vec3(pParam->m_Value.m_Vector[0],pParam->m_Value.m_Vector[1],pParam->m_Value.m_Vector[2]) );
break;
}
}
}
}
*/

//////////////////////////////////////////////////////////////////////////
void CMaterial::AssignToEntity(IRenderNode* pEntity)
{
    assert(pEntity);
    pEntity->SetMaterial(GetMatInfo());
}

//////////////////////////////////////////////////////////////////////////
bool CMaterial::IsBreakable2D() const
{
    if ((GetFlags() & MTL_FLAG_NODRAW) != 0)
    {
        return false;
    }

    const QString& surfaceTypeName = GetSurfaceTypeName();
    if (ISurfaceTypeManager* pSurfaceManager = GetIEditor()->Get3DEngine()->GetMaterialManager()->GetSurfaceTypeManager())
    {
        ISurfaceType* pSurfaceType = pSurfaceManager->GetSurfaceTypeByName(surfaceTypeName.toUtf8().data());
        if (pSurfaceType && pSurfaceType->GetBreakable2DParams() != 0)
        {
            return true;
        }
    }

    int count = GetSubMaterialCount();
    for (int i = 0; i < count; ++i)
    {
        const CMaterial* pSub = GetSubMaterial(i);
        if (!pSub)
        {
            continue;
        }
        if (pSub->IsBreakable2D())
        {
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::SetFromMatInfo(_smart_ptr<IMaterial> pMatInfo)
{
    assert(pMatInfo);

    m_shaderName = "";

    ClearMatInfo();
    SetModified(true);

    m_mtlFlags = pMatInfo->GetFlags();
    if (m_mtlFlags & MTL_FLAG_MULTI_SUBMTL)
    {
        // Create sub materials.
        SetSubMaterialCount(pMatInfo->GetSubMtlCount());
        for (int i = 0; i < GetSubMaterialCount(); i++)
        {
            _smart_ptr<IMaterial> pChildMatInfo = pMatInfo->GetSubMtl(i);
            if (!pChildMatInfo)
            {
                continue;
            }

            if (pChildMatInfo->GetFlags() & MTL_FLAG_PURE_CHILD)
            {
                CMaterial* existingChild = GetSubMaterial(i);
                if (existingChild)
                {
                    existingChild->SetFromMatInfo(pChildMatInfo);
                }
                else
                {
                    CMaterial* pChild = new CMaterial(pChildMatInfo->GetName(), pChildMatInfo->GetFlags());
                    pChild->SetFromMatInfo(pChildMatInfo);
                    SetSubMaterial(i, pChild);
                }
            }
            else
            {
                CMaterial* pChild = GetIEditor()->GetMaterialManager()->LoadMaterial(pChildMatInfo->GetName());
                pChild->SetFromMatInfo(pChildMatInfo);
                SetSubMaterial(i, pChild);
            }
        }
    }
    else
    {
        SetShaderItem(pMatInfo->GetShaderItem());

        if (m_shaderItem.m_pShaderResources)
        {
            m_shaderResources = SInputShaderResources(m_shaderItem.m_pShaderResources);
        }
        if (m_shaderItem.m_pShader)
        {
            // Get name of template.
            IShader* pTemplShader = m_shaderItem.m_pShader;
            if (pTemplShader)
            {
                m_nShaderGenMask = pTemplShader->GetGenerationMask();
            }
        }
        m_shaderName = pMatInfo->GetShaderName();
        ISurfaceType* pSurfaceType = pMatInfo->GetSurfaceType();
        if (pSurfaceType)
        {
            m_surfaceType = pSurfaceType->GetName();
        }
        else
        {
            m_surfaceType = "";
        }
    }

    // Mark as not modified.
    SetModified(false);

    // Material link names
    if (const char* szLinkName = pMatInfo->GetMaterialLinkName())
    {
        m_linkedMaterial = szLinkName;
    }

    //////////////////////////////////////////////////////////////////////////
    // Assign mat info.
    m_pMatInfo = pMatInfo;
    m_pMatInfo->SetUserData(this);
    AddRef(); // Let IMaterial keep a reference to us.
}

//////////////////////////////////////////////////////////////////////////
int CMaterial::GetSubMaterialCount() const
{
    return m_subMaterials.size();
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::SetSubMaterialCount(int nSubMtlsCount)
{
    RecordUndo("Multi Material Change");
    m_subMaterials.resize(nSubMtlsCount);
    UpdateMatInfo();
    NotifyChanged();
}

//////////////////////////////////////////////////////////////////////////
CMaterial* CMaterial::GetSubMaterial(int index) const
{
    const int nSubMats = m_subMaterials.size();
    assert(index >= 0 && index < nSubMats);

    if (index < 0 || index >= nSubMats)
    {
        return NULL;
    }

    return m_subMaterials[index];
}

//////////////////////////////////////////////////////////////////////////
int CMaterial::FindMaterialIndex(const QString& name)
{
    for (int i = 0; i < m_subMaterials.size(); ++i)
    {
        if (m_subMaterials[i]->GetName().compare(name) == 0)
        {
            return i;
        }
    }

    return -1;
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::SetSubMaterial(int nSlot, CMaterial* mtl)
{
    RecordUndo("Multi Material Change");
    assert(nSlot >= 0 && nSlot < m_subMaterials.size());
    if (mtl)
    {
        if (mtl->IsMultiSubMaterial())
        {
            return;
        }
        if (mtl->IsPureChild())
        {
            mtl->m_pParent = this;
        }
    }

    if (m_subMaterials[nSlot])
    {
        m_subMaterials[nSlot]->m_pParent = NULL;
    }
    m_subMaterials[nSlot] = mtl;

    if (!m_subMaterials[nSlot])
    {
        m_subMaterials.erase(m_subMaterials.begin() + nSlot);
    }

    UpdateMatInfo();
    NotifyChanged();
}

//////////////////////////////////////////////////////////////////////////
// This method will populate for the material editor the name and tool tip of the 
// different textures of the current material 
//CVarBlock* CMaterial::UpdateTextureNames(AZStd::unordered_map<ResourceSlotIndex, CSmartVariableArray>& textureVarsMap)
CVarBlock* CMaterial::UpdateTextureNames(CSmartVariableArray textureVars[EFTT_MAX])
{
    CVarBlock*          pTextureSlots = new CVarBlock;
    IShader*            pTemplShader = m_shaderItem.m_pShader;
    int                 nTech = max(0, m_shaderItem.m_nTechnique);
    SShaderTexSlots*    pShaderSlots = pTemplShader ? pTemplShader->GetUsedTextureSlots(nTech) : nullptr;

    for (EEfResTextures nTexSlot = EEfResTextures(0); nTexSlot < EFTT_MAX; nTexSlot = EEfResTextures(nTexSlot + 1))
    {
        if (!MaterialHelpers::IsAdjustableTexSlot((EEfResTextures)nTexSlot))
        {   // do not take into account virtual slots (such as smoothness - normal's alpha)
            // in theory this case should not happen as it is filtered from the source list.
            continue;
        }

        IVariable*          pVar = textureVars[nTexSlot].GetVar();
        SShaderTextureSlot* pSlot = pShaderSlots ? pShaderSlots->m_UsedTextureSlots[nTexSlot] : nullptr;

        // If slot is NULL, fall back to default name - name here is the context name (i.e. diffuse, normal..) 
        // and not the actual texture file name
        pVar->SetName(pSlot && pSlot->m_Name.length() ? pSlot->m_Name.c_str() : MaterialHelpers::LookupTexName((EEfResTextures) nTexSlot));
        pVar->SetDescription(pSlot && pSlot->m_Description.length() ? pSlot->m_Description.c_str() : MaterialHelpers::LookupTexDesc((EEfResTextures)nTexSlot));

        int flags = pVar->GetFlags();

        // TODO: slot->m_TexType gives expected sampler type (2D vs Cube etc). Could check/extract this here.

        // Not sure why this needs COLLAPSED added again, but without this all the slots expand
        flags |= IVariable::UI_COLLAPSED;

        //clear the auto-expand flag if there is no texture assigned.
        SEfResTexture*  pTextureRes = m_shaderResources.GetTextureResource(nTexSlot);
        bool            noTextureName = (!pTextureRes ? true : pTextureRes->m_Name.empty());

        if (noTextureName)
        {
            flags &= ~IVariable::UI_AUTO_EXPAND;
        }

        // if slot is NULL, but we have reflection information, this slot isn't used - make the variable invisible
        // unless there's a texture in the slot
        if (pShaderSlots && !pSlot && noTextureName)
        {
            flags |= IVariable::UI_INVISIBLE;
        }
        else
        {
            flags &= ~IVariable::UI_INVISIBLE;
        }

        pVar->SetFlags(flags);

        pTextureSlots->AddVariable(pVar);
    }

    return pTextureSlots;
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::ClearAllSubMaterials()
{
    RecordUndo("Multi Material Change");
    for (int i = 0; i < m_subMaterials.size(); i++)
    {
        if (m_subMaterials[i])
        {
            m_subMaterials[i]->m_pParent = NULL;
            m_subMaterials[i] = NULL;
        }
    }
    UpdateMatInfo();
    NotifyChanged();
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::Validate()
{
    if (IsDummy())
    {
        CErrorRecord err;
        err.error = QObject::tr("Material %1 file not found").arg(GetName());
        err.pItem = this;
        GetIEditor()->GetErrorReport()->ReportError(err);
    }
    // Reload shader.
    LoadShader();

    // Validate sub materials.
    for (int i = 0; i < m_subMaterials.size(); i++)
    {
        if (m_subMaterials[i])
        {
            m_subMaterials[i]->Validate();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::GatherUsedResources(CUsedResources& resources)
{
    if (!IsUsed())
    {
        return;
    }

    SInputShaderResources&  sr = GetShaderResources();
    for (auto iter = sr.m_TexturesResourcesMap.begin(); iter != sr.m_TexturesResourcesMap.end(); ++iter)
    {
        SEfResTexture* pTexture = &iter->second;
        if (!pTexture->m_Name.empty())
        {
            resources.Add(pTexture->m_Name.c_str());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CMaterial::CanModify(bool bSkipReadOnly)
{
    if (m_bDummyMaterial)
    {
        return false;
    }

    if (IsPureChild() && GetParent())
    {
        return GetParent()->CanModify(bSkipReadOnly);
    }

    if (bSkipReadOnly)
    {
        // If read only or in pack, do not save.
        if (m_scFileAttributes & (SCC_FILE_ATTRIBUTE_READONLY | SCC_FILE_ATTRIBUTE_INPAK))
        {
            return false;
        }

        // Managed file must be checked out.
        if ((m_scFileAttributes & SCC_FILE_ATTRIBUTE_MANAGED) && !(m_scFileAttributes & SCC_FILE_ATTRIBUTE_CHECKEDOUT))
        {
            return false;
        }
    }
    else
    {
        // Only if in pack.
        if (m_scFileAttributes & (SCC_FILE_ATTRIBUTE_INPAK))
        {
            return false;
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CMaterial::Save(bool bSkipReadOnly, const QString& fullPath)
{
    // Save our parent
    if (IsPureChild())
    {
        if (m_pParent)
        {
            return m_pParent->Save(bSkipReadOnly);
        }
        return false;
    }

    if (m_mtlFlags & MTL_FLAG_UIMATERIAL)
    {
        return false;
    }

    GetFileAttributes();

    if (bSkipReadOnly && IsModified())
    {
        // If read only or in pack, do not save.
        if (m_scFileAttributes & (SCC_FILE_ATTRIBUTE_READONLY | SCC_FILE_ATTRIBUTE_INPAK))
        {
            gEnv->pLog->LogError("Can't save material %s (read-only)", GetName().toUtf8().constData());
        }

        // Managed file must be checked out.
        if ((m_scFileAttributes & SCC_FILE_ATTRIBUTE_MANAGED) && !(m_scFileAttributes & SCC_FILE_ATTRIBUTE_CHECKEDOUT))
        {
            gEnv->pLog->LogError("Can't save material %s (need to check out)", GetName().toUtf8().constData());
        }
    }

    if (!CanModify(bSkipReadOnly))
    {
        return false;
    }


    // If filename is empty do not not save.
    if (GetFilename().isEmpty())
    {
        return false;
    }

    // Save material XML to a file that corresponds to the material name with extension .mtl.
    XmlNodeRef mtlNode = XmlHelpers::CreateXmlNode("Material");
    CBaseLibraryItem::SerializeContext ctx(mtlNode, false);
    Serialize(ctx);

    bool saveSucceeded = false;
    if (fullPath.isEmpty())
    {
        // If no filepath was specified, get the filename using the relative path/unique identifier of this material
        saveSucceeded = XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), mtlNode, GetFilename().toUtf8().data());
    }
    else
    {
        // If a filepath was specified, save to the specified location
        saveSucceeded = XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), mtlNode, fullPath.toUtf8().data());
    }

    if (saveSucceeded)
    {
        // If material successfully saved, clear modified flag.
        SetModified(false);
        for (int i = 0; i < GetSubMaterialCount(); ++i)
        {
            CMaterial* pSubMaterial = GetSubMaterial(i);
            if (pSubMaterial)
            {
                pSubMaterial->SetModified(false);
            }
        }
    }
    else
    {
        AZ_Warning("Material Editor", false, "Material '%s' failed to save successfully. Check that the file is writable and has been successfully checked out in source control.", m_name.toUtf8().data());
    }

    return saveSucceeded;
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::ClearMatInfo()
{
    m_pMatInfo = nullptr;
}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CMaterial::GetMatInfo(bool bUseExistingEngineMaterial)
{
    if (!m_pMatInfo)
    {
        if (m_bDummyMaterial)
        {
            m_pMatInfo = GetIEditor()->Get3DEngine()->GetMaterialManager()->GetDefaultMaterial();
            AddRef(); // Always keep dummy materials.
            return m_pMatInfo;
        }

        if (!IsMultiSubMaterial() && !m_shaderItem.m_pShader)
        {
            LoadShader();
        }

        if (!IsPureChild())
        {
            if (bUseExistingEngineMaterial)
            {
                m_pMatInfo = GetIEditor()->Get3DEngine()->GetMaterialManager()->FindMaterial(GetName().toUtf8().data());
            }

            if (!m_pMatInfo)
            {
                m_pMatInfo = GetIEditor()->Get3DEngine()->GetMaterialManager()->CreateMaterial(GetName().toUtf8().data(), m_mtlFlags);
            }
        }
        else
        {
            // Pure child should not be registered with the name.
            m_pMatInfo = GetIEditor()->Get3DEngine()->GetMaterialManager()->CreateMaterial("", m_mtlFlags);
            m_pMatInfo->SetName(GetName().toUtf8().data());
        }
        m_mtlFlags = m_pMatInfo->GetFlags();
        UpdateMatInfo();

        if (m_pMatInfo->GetUserData() != this)
        {
            m_pMatInfo->SetUserData(this);
            AddRef(); // Let IMaterial keep a reference to us.
        }
    }

    return m_pMatInfo;
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::NotifyChanged()
{
    if (m_bIgnoreNotifyChange)
    {
        return;
    }

    if (!CanModify() && !IsModified() && CUndo::IsRecording())
    {
        // Display Warning message.
        Warning("Modifying read only material %s\r\nChanges will not be saved!", GetName().toUtf8().data());
    }

    SetModified();

    GetIEditor()->GetMaterialManager()->OnItemChanged(this);
}

//////////////////////////////////////////////////////////////////////////
class CUndoMaterial
    : public IUndoObject
{
public:
    CUndoMaterial(CMaterial* pMaterial, const char* undoDescription, bool bForceUpdate)
    {
        assert(pMaterial);

        // Stores the current state of this object.
        m_undoDescription = undoDescription;

        m_bIsSubMaterial = pMaterial->IsPureChild();

        if (m_bIsSubMaterial)
        {
            CMaterial* pParentMaterial = pMaterial->GetParent();
            assert(pParentMaterial && !pParentMaterial->IsPureChild());
            if (pParentMaterial && !pParentMaterial->IsPureChild())
            {
                bool bFound = false;
                const int subMaterialCount = pParentMaterial->GetSubMaterialCount();
                for (int i = 0; i < subMaterialCount; ++i)
                {
                    CMaterial* pSubMaterial = pParentMaterial->GetSubMaterial(i);
                    if (pSubMaterial == pMaterial)
                    {
                        bFound = true;
                        m_subMaterialName = pSubMaterial->GetName();
                        break;
                    }
                }
                assert(bFound);
                m_mtlName = pParentMaterial->GetName();
            }
        }
        else
        {
            m_mtlName = pMaterial->GetName();
        }

        // Save material XML to a file that corresponds to the material name with extension .mtl.
        m_undo = XmlHelpers::CreateXmlNode("Material");
        CBaseLibraryItem::SerializeContext ctx(m_undo, false);
        pMaterial->Serialize(ctx);
        m_bForceUpdate = bForceUpdate;
    }

protected:
    virtual void Release() { delete this; };

    virtual int GetSize()
    {
        return sizeof(*this) + m_undoDescription.length() + m_mtlName.length();
    }

    virtual QString GetDescription() { return m_undoDescription; };

    virtual void Undo(bool bUndo)
    {
        CMaterial* pMaterial = GetMaterial();

        assert(pMaterial);
        if (!pMaterial)
        {
            return;
        }

        if (bUndo)
        {
            // Save current object state.
            m_redo = XmlHelpers::CreateXmlNode("Material");
            CBaseLibraryItem::SerializeContext ctx(m_redo, false);
            pMaterial->Serialize(ctx);
        }

        CBaseLibraryItem::SerializeContext ctx(m_undo, true);
        ctx.bUndo = bUndo;
        pMaterial->Serialize(ctx);

        if (m_bForceUpdate && bUndo)
        {
            GetIEditor()->GetMaterialManager()->OnUpdateProperties(pMaterial, true);
        }
    }

    virtual void Redo()
    {
        CMaterial* pMaterial = GetMaterial();

        if (!pMaterial)
        {
            return;
        }

        CBaseLibraryItem::SerializeContext ctx(m_redo, true);
        ctx.bUndo = true;
        pMaterial->Serialize(ctx);

        if (m_bForceUpdate)
        {
            GetIEditor()->GetMaterialManager()->OnUpdateProperties(pMaterial, true);
        }
    }

private:
    CMaterial* GetMaterial()
    {
        CMaterial* pMaterial = (CMaterial*)GetIEditor()->GetMaterialManager()->FindItemByName(m_mtlName);
        assert(pMaterial);

        if (pMaterial && m_bIsSubMaterial)
        {
            bool bFound = false;
            const int subMaterialCount = pMaterial->GetSubMaterialCount();
            for (int i = 0; i < subMaterialCount; ++i)
            {
                CMaterial* pSubMaterial = pMaterial->GetSubMaterial(i);
                if (pSubMaterial && (pSubMaterial->GetName() == m_subMaterialName))
                {
                    bFound = true;
                    pMaterial = pSubMaterial;
                    break;
                }
            }
            assert(bFound && pMaterial);
        }

        return pMaterial;
    }

    QString m_undoDescription;
    QString m_mtlName;
    bool m_bIsSubMaterial;
    QString m_subMaterialName;
    XmlNodeRef m_undo;
    XmlNodeRef m_redo;
    bool m_bForceUpdate;
};

//////////////////////////////////////////////////////////////////////////
void CMaterial::RecordUndo(const char* sText, bool bForceUpdate)
{
    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoMaterial(this, sText, bForceUpdate));
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::OnMakeCurrent()
{
    UpdateFileAttributes(false);

    // If Shader not yet loaded, load it now.
    if (!m_shaderItem.m_pShader)
    {
        LoadShader();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::SetSurfaceTypeName(const QString& surfaceType)
{
    m_surfaceType = surfaceType;
    UpdateMatInfo();
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::Reload()
{
    if (IsPureChild())
    {
        if (m_pParent)
        {
            m_pParent->Reload();
        }
        return;
    }
    if (IsDummy())
    {
        return;
    }

    XmlNodeRef mtlNode = GetISystem()->LoadXmlFromFile(GetFilename().toUtf8().data());
    if (!mtlNode)
    {
        return;
    }
    CBaseLibraryItem::SerializeContext serCtx(mtlNode, true);
    serCtx.bUndo = true; // Simulate undo.
    Serialize(serCtx);

    // was called by Simulate undo.
    //UpdateMatInfo();
    //NotifyChanged();
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::LinkToMaterial(const QString& name)
{
    m_linkedMaterial = name;
    UpdateMatInfo();
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::DisableHighlightForFrame()
{
    SetHighlightFlags(0);
}

static ColorF Interpolate(const ColorF& a, const ColorF& b, float phase)
{
    const float oneMinusPhase = 1.0f - phase;
    return ColorF(b.r * phase + a.r * oneMinusPhase,
        b.g * phase + a.g * oneMinusPhase,
        b.b * phase + a.b * oneMinusPhase,
        b.a * phase + a.a * oneMinusPhase);
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::UpdateHighlighting()
{
    if (!(GetFlags() & MTL_FLAG_NODRAW))
    {
        const CInputLightMaterial& original = m_shaderResources.m_LMaterial;
        CInputLightMaterial lm = original;

        ColorF highlightColor(0.0f, 0.0f, 0.0f, 1.0f);
        float highlightIntensity = 0.0f;

        MAKE_SURE(GetIEditor()->GetMaterialManager(), return );
        GetIEditor()->GetMaterialManager()->GetHighlightColor(&highlightColor, &highlightIntensity, m_highlightFlags);

        if (m_shaderItem.m_pShaderResources)
        {
            ColorF diffuseColor = Interpolate(original.m_Diffuse, highlightColor, highlightIntensity);
            ColorF emissiveColor = Interpolate(original.m_Emittance, highlightColor, highlightIntensity);
            ColorF specularColor = Interpolate(original.m_Specular, highlightColor, highlightIntensity);

            // [Shader System TO DO] remove this hard coded association!
            m_shaderItem.m_pShaderResources->SetColorValue(EFTT_DIFFUSE, diffuseColor);
            m_shaderItem.m_pShaderResources->SetColorValue(EFTT_SPECULAR, specularColor);
            m_shaderItem.m_pShaderResources->SetColorValue(EFTT_EMITTANCE, emissiveColor);

            m_shaderItem.m_pShaderResources->UpdateConstants(m_shaderItem.m_pShader);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterial::SetHighlightFlags(int highlightFlags)
{
    m_highlightFlags = highlightFlags;

    UpdateHighlighting();
}


void CMaterial::SetShaderItem(const SShaderItem& shaderItem)
{
    SAFE_RELEASE(m_shaderItem.m_pShader);
    SAFE_RELEASE(m_shaderItem.m_pShaderResources);

    m_shaderItem = shaderItem;
    if (m_shaderItem.m_pShader)
    {
        m_shaderItem.m_pShader->AddRef();
    }
    if (m_shaderItem.m_pShaderResources)
    {
        m_shaderItem.m_pShaderResources->AddRef();
    }
}
