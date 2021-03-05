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

#ifndef CRYINCLUDE_EDITOR_MATERIAL_MATERIAL_H
#define CRYINCLUDE_EDITOR_MATERIAL_MATERIAL_H
#pragma once

#include <IMaterial.h>
#include <IRenderer.h>
#include "BaseLibraryItem.h"
#include "Include/IEditorMaterial.h"
#include "Util/Variable.h"
#include <vector>

// forward declarations,
class CMaterialManager;
class CVarBlock;

enum eMTL_PROPAGATION
{
    MTL_PROPAGATE_OPACITY = 1 << 0,
    MTL_PROPAGATE_LIGHTING = 1 << 1,
    MTL_PROPAGATE_ADVANCED = 1 << 2,
    MTL_PROPAGATE_TEXTURES = 1 << 3,
    MTL_PROPAGATE_SHADER_PARAMS = 1 << 4,
    MTL_PROPAGATE_SHADER_GEN = 1 << 5,
    MTL_PROPAGATE_VERTEX_DEF = 1 << 6,
    MTL_PROPAGATE_LAYER_PRESETS = 1 << 7,
    MTL_PROPAGATE_MATERIAL_SETTINGS = 1 << 8,
    MTL_PROPAGATE_ALL = (
            MTL_PROPAGATE_OPACITY |
            MTL_PROPAGATE_LIGHTING |
            MTL_PROPAGATE_ADVANCED |
            MTL_PROPAGATE_TEXTURES |
            MTL_PROPAGATE_SHADER_PARAMS |
            MTL_PROPAGATE_SHADER_GEN |
            MTL_PROPAGATE_VERTEX_DEF |
            MTL_PROPAGATE_LAYER_PRESETS |
            MTL_PROPAGATE_MATERIAL_SETTINGS),
    MTL_PROPAGATE_RESERVED = 1 << 9
};

/** CMaterial class
        Every Material is a member of material library.
        Materials can have child sub materials,
        Sub materials are applied to the same geometry of the parent material in the other material slots.
*/

struct SMaterialLayerResources
{
    SMaterialLayerResources()
        : m_nFlags(MTL_LAYER_USAGE_REPLACEBASE)
        , m_bRegetPublicParams(true)
        , m_pMatLayer(0)
    {
    }

    uint8 m_nFlags;
    bool m_bRegetPublicParams;
    QString m_shaderName;

    _smart_ptr< IMaterialLayer > m_pMatLayer;
    SInputShaderResources m_shaderResources;
    XmlNodeRef m_publicVarsCache;
};

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
class CRYEDIT_API CMaterial
    : public IEditorMaterial
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    //////////////////////////////////////////////////////////////////////////
    CMaterial(const QString& name, int nFlags = 0);
    CMaterial(const CMaterial& rhs);
    ~CMaterial();

    virtual EDataBaseItemType GetType() const { return EDB_TYPE_MATERIAL; };

    void SetName(const QString& name);

    //////////////////////////////////////////////////////////////////////////
    QString GetFullName() const { return m_name; };

    //////////////////////////////////////////////////////////////////////////
    // File properties of the material.
    //////////////////////////////////////////////////////////////////////////
    QString GetFilename() const;

    //! Collect filenames of texture sources used in material
    //! Return number of filenames
    int GetTextureFilenames(QStringList& outFilenames) const;
    int GetAnyTextureFilenames(QStringList& outFilenames) const;

    void UpdateFileAttributes(bool useSourceControl = true);
    uint32 GetFileAttributes();
    //////////////////////////////////////////////////////////////////////////

    //! Sets one or more material flags from EMaterialFlags enum.
    void SetFlags(int flags) { m_mtlFlags = flags; };
    //! Query this material flags.
    virtual int GetFlags() const { return m_mtlFlags; }
    bool IsMultiSubMaterial() const { return (m_mtlFlags & MTL_FLAG_MULTI_SUBMTL) != 0; };
    bool IsPureChild() const { return (m_mtlFlags & MTL_FLAG_PURE_CHILD) != 0; }

    // Check if material is used.
    bool IsUsed() const { /*return m_nUseCount > 0 || (m_mtlFlags & MTL_FLAG_ALWAYS_USED);*/ return true; };

    virtual void GatherUsedResources(CUsedResources& resources);

    //! Set name of shader used by this material.
    void SetShaderName(const QString& shaderName);
    //! Get name of shader used by this material.
    QString GetShaderName() const { return m_shaderName; };

    virtual SInputShaderResources& GetShaderResources() { return m_shaderResources; };

    //! Get public parameters of material in variable block.
    CVarBlock* GetPublicVars(SInputShaderResources& pShaderResources);

    //! Set the shader public param m_script variable into our own m_script, script contains min/max for a given shader param value
    void SetShaderParamPublicScript();

    //! Sets variable block of public shader parameters.
    //! VarBlock must be in same format as returned by GetPublicVars().
    void SetPublicVars(CVarBlock* pPublicVars, CMaterial* pMtl);

    //! Update names/descriptions in this variable array, return a variable block for replacing
    CVarBlock* UpdateTextureNames(CSmartVariableArray textureVars[EFTT_MAX]);
// [Shader System] - Do To: add back with map usage:   CVarBlock* UpdateTextureNames(AZStd::unordered_map<ResourceSlotIndex, CSmartVariableArray>& textureVarsMap);

    //////////////////////////////////////////////////////////////////////////
    CVarBlock* GetShaderGenParamsVars();
    void SetShaderGenParamsVars(CVarBlock* pBlock);
    uint64 GetShaderGenMask() { return m_nShaderGenMask; }
    void SetShaderGenMask(uint64 mask) { m_nShaderGenMask = mask; }

    //! Return variable block of shader params.
    SShaderItem& GetShaderItem() { return m_shaderItem; };

    //! Return material layers resources
    SMaterialLayerResources* GetMtlLayerResources() { return m_pMtlLayerResources; };

    //! Get texture map usage mask for shader in this material.
    unsigned int GetTexmapUsageMask() const;

    //! Load new shader.
    bool LoadShader();

    //! Reload shader, update all shader parameters.
    virtual void Update();

    // Reload material settings from file.
    // NOTICE: The function will remove all the sub-materials and recreate them!
    void Reload();

    //! Serialize material settings to xml.
    virtual void Serialize(SerializeContext& ctx);

    //! Assign this material to static geometry.
    void AssignToEntity(IRenderNode* pEntity);

    //////////////////////////////////////////////////////////////////////////
    // Surface types.
    //////////////////////////////////////////////////////////////////////////
    virtual void SetSurfaceTypeName(const QString& surfaceType);
    virtual const QString& GetSurfaceTypeName() const { return m_surfaceType; };
    bool IsBreakable2D() const;

    //////////////////////////////////////////////////////////////////////////
    // Child Sub materials.
    //////////////////////////////////////////////////////////////////////////
    //! Get number of sub materials childs.
    int GetSubMaterialCount() const;
    //! Set number of sub materials childs.
    void SetSubMaterialCount(int nSubMtlsCount);
    //! Get sub material child by index.
    CMaterial* GetSubMaterial(int index) const;
    //! Find sub material index by name
    int FindMaterialIndex(const QString& name);
    // Set a material to the sub materials slot.
    // Use NULL material pointer to clear slot.
    void SetSubMaterial(int nSlot, CMaterial* mtl);
    //! Remove all sub materials, does not change number of sub material slots.
    void ClearAllSubMaterials();

    //! Return pointer to engine material.
    virtual _smart_ptr<IMaterial> GetMatInfo(bool bUseExistingEngineMaterial = true);
    // Clear stored pointer to engine material.
    void ClearMatInfo();

    //! Validate materials for errors.
    void Validate();

    // Check if material file can be modified.
    // Will check file attributes if it is not read only.
    bool CanModify(bool bSkipReadOnly = true);

    // Save material to file.
    virtual bool Save(bool bSkipReadOnly = true, const QString& fullPath = "");

    // Dummy material is just a placeholder item for materials that have not been found on disk.
    void SetDummy(bool bDummy) { m_bDummyMaterial = bDummy; }
    bool IsDummy() const { return m_bDummyMaterial != 0; }
    
    // Called by material manager when material selected as a current material.
    void OnMakeCurrent();

    void SetFromMatInfo(_smart_ptr<IMaterial> pMatInfo);

    // Link a submaterial by name (used for value propagation in CMaterialUI)
    void LinkToMaterial(const QString& name);
    const QString& GetLinkedMaterialName() { return m_linkedMaterial; }

    // Return parent material for submaterial
    CMaterial* GetParent() const {return m_pParent; }

    //! Loads material layers
    bool LoadMaterialLayers();
    //! Updates material layers
    void UpdateMaterialLayers();

    void SetHighlightFlags(int highlightFlags);
    void UpdateHighlighting();
    virtual void DisableHighlightForFrame();
    void RecordUndo(const char* sText, bool bForceUpdate = false);

    int GetPropagationFlags() const { return m_propagationFlags; }
    void SetPropagationFlags(const int flags) { m_propagationFlags = flags; }

    bool LayerActivationAllowed() const { return m_allowLayerActivation; }
    void SetLayerActivation(bool allowed) { m_allowLayerActivation = allowed; }

    uint32 GetDccMaterialHash() const { return m_dccMaterialHash; }
    void SetDccMaterialHash(AZ::u32 hash) { m_dccMaterialHash = hash; }
    void SetShaderItem(const SShaderItem& shaderItem);

private:
    void UpdateMatInfo();
    void CheckSpecialConditions();

    void NotifyChanged();

private:
    //////////////////////////////////////////////////////////////////////////
    // Variables.
    //////////////////////////////////////////////////////////////////////////
    QString m_shaderName;
    QString m_surfaceType;
    QString m_linkedMaterial;

    //! Material flags.
    int m_mtlFlags;

    // Hash for DCC material attributes, used to check if .dccmtl has changed
    // If so, the source .mtl file will need to be rebuilt
    uint32 m_dccMaterialHash;

    // Parent material, Only valid for Pure Childs.
    CMaterial* m_pParent;

    //////////////////////////////////////////////////////////////////////////
    // Shader resources.
    //////////////////////////////////////////////////////////////////////////
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    SShaderItem m_shaderItem;
    SInputShaderResources m_shaderResources;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    //CVarBlockPtr m_shaderParamsVar;
    //! Common shader flags.
    uint64 m_nShaderGenMask;
    QString m_pszShaderGenMask;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    SMaterialLayerResources m_pMtlLayerResources[MTL_LAYER_MAX_SLOTS];

    _smart_ptr<IMaterial> m_pMatInfo;

    XmlNodeRef m_publicVarsCache;

    //! Array of sub materials.
    std::vector<_smart_ptr<CMaterial> > m_subMaterials;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    int m_nUseCount;
    uint32 m_scFileAttributes;

    unsigned char m_highlightFlags;

    // The propagation flags are a bit combination of the MTL_PROPAGATION enum above
    // and determine which properties get propagated to an optional linked material
    // during ui editing
    int m_propagationFlags;

    //! Material Used in level.
    int m_bDummyMaterial       : 1; // Dummy material, name specified but material file not found.
    int m_bIgnoreNotifyChange  : 1; // Do not send notifications about changes.
    int m_bRegetPublicParams   : 1;
    int m_bKeepPublicParamsValues : 1;

    bool m_allowLayerActivation;
};

#endif // CRYINCLUDE_EDITOR_MATERIAL_MATERIAL_H
