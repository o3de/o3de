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

#include "StatObjValidator.h"

// Editor
#include "Material/Material.h"


CStatObjValidator::CStatObjValidator()
    : m_isValid(true)
{
}

template<size_t size>
bool HasPrefix(const char* name, const char (&prefix)[size])
{
    return _strnicmp(name, prefix, size - 1) == 0;
}

struct SMeshMaterialIssue
{
    AZStd::string nodeName;
    AZStd::string description;
    int subMaterialIndex;

    SMeshMaterialIssue()
        : subMaterialIndex(-1)
    {
    }

    SMeshMaterialIssue(const AZStd::string& name, const AZStd::string& description)
        : nodeName(name)
        , description(description)
        , subMaterialIndex(-1)
    {
    }
};

static void ValidateMeshMaterials(std::vector<SMeshMaterialIssue>* issues, IStatObj* pStatObj, CMaterial* pMaterial)
{
    _smart_ptr<IMaterial> pIMaterial = 0;
    if (pMaterial)
    {
        if (pMaterial->GetParent())
        {
            pIMaterial = pMaterial->GetParent()->GetMatInfo();
        }
        else
        {
            pIMaterial = pMaterial->GetMatInfo();
        }
    }

    IIndexedMesh* pIndexedMesh = pStatObj->GetIndexedMesh(true);
    if (pIndexedMesh)
    {
        int breakableSubmeshes = 0;
        int nonbreakableSubmeshes = 0;

        int subsetCount = pIndexedMesh->GetSubSetCount();
        for (int i = 0; i < subsetCount; ++i)
        {
            const SMeshSubset& subset = pIndexedMesh->GetSubSet(i);
            if (subset.nNumVerts == 0)
            {
                continue;
            }

            // Check to see if the material uses multiple uv sets and if the vertex format has the same number of texCoord attributes
            SShaderItem shaderItem = pIMaterial->GetShaderItem(i);
            if (shaderItem.m_pShader)
            {
                size_t materialUVs = shaderItem.m_pShader->GetNumberOfUVSets();
                size_t meshUVs = subset.vertexFormat.GetAttributeUsageCount(AZ::Vertex::AttributeUsage::TexCoord);
                if (materialUVs != meshUVs)
                {
                    const char* meshName = pStatObj->GetRenderMesh() ? pStatObj->GetRenderMesh()->GetSourceName() : "unknown";
                    AZStd::string errorMessage;
                    errorMessage = AZStd::string::format("Material '%s' sub-material %d with %zu uv set(s) was assigned to mesh '%s' with %zu uv set(s). ", pIMaterial->GetName(), i + 1, materialUVs, meshName, meshUVs);

                    AZStd::string recommendedAction;
                    if (materialUVs < meshUVs)
                    {
                        recommendedAction = AZStd::string::format("If you do not intend to use %zu uv sets, remove the extra uv set(s) from the source mesh during the import process. Otherwise, consider checking the desired 'Use uv set 2 for...' shader gen params in the material editor.", meshUVs);
                    }
                    else
                    {
                        recommendedAction = AZStd::string::format("If you intend to use %zu uv sets, include the additional uv set(s) in the source mesh during the import process. Otherwise, consider unchecking the 'Use uv set 2 for...' shader gen params in the material editor.", materialUVs);
                    }
                    errorMessage += recommendedAction;
                    AZ_Warning("Material Editor", false, errorMessage.c_str());
                    SMeshMaterialIssue issue(meshName, errorMessage);
                    issues->push_back(issue);
                }
            }

            _smart_ptr<IMaterial> pSubMaterial = pIMaterial->GetSubMtl(subset.nMatID);
            if (!pSubMaterial)
            {
                continue;
            }

            if (size_t(subset.nMatID) > size_t(pMaterial->GetSubMaterialCount()))
            {
                continue;
            }

            if (pSubMaterial->GetSurfaceType()->GetBreakable2DParams())
            {
                ++breakableSubmeshes;
            }
            else
            {
                ++nonbreakableSubmeshes;
            }
        }
    }


    int subobjectCount = pStatObj->GetSubObjectCount();
    for (int i = 0; i < subobjectCount; ++i)
    {
        const IStatObj::SSubObject* subobject = pStatObj->GetSubObject(i);
        if (subobject->pStatObj)
        {
            ValidateMeshMaterials(issues, subobject->pStatObj, pMaterial);
        }
    }
}

void CStatObjValidator::Validate(IStatObj* statObj, CMaterial* editorMaterial)
{
    m_description = QString();
    m_isValid = true;

    _smart_ptr<IMaterial> pIMaterial = 0;
    if (editorMaterial)
    {
        if (editorMaterial->GetParent())
        {
            pIMaterial = editorMaterial->GetParent()->GetMatInfo();
        }
        else
        {
            pIMaterial = editorMaterial->GetMatInfo();
        }
    }

    const char* errorText = 0;
    const char* nodeName = 0;
    if (statObj && editorMaterial)
    {
        std::vector<SMeshMaterialIssue> issues;
        ValidateMeshMaterials(&issues, statObj, editorMaterial);

        if (!issues.empty())
        {
            m_isValid = false;
        }

        for (size_t i = 0; i < issues.size(); ++i)
        {
            const SMeshMaterialIssue& issue = issues[i];
            if (!m_description.isEmpty())
            {
                m_description += "\n";
            }
            if (!issue.nodeName.empty())
            {
                m_description += "Node ";
                m_description += issue.nodeName.c_str();
                m_description += ":";
            }
            if (issue.subMaterialIndex >= 0)
            {
                m_description += QStringLiteral("SubMaterial %1:").arg(issue.subMaterialIndex + 1);
            }
            if (!issue.nodeName.empty() || issue.subMaterialIndex >= 0)
            {
                m_description += "\n  ";
            }
            if (!issue.description.empty())
            {
                m_description += issue.description.c_str();
            }
        }
    }
}

