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

#include "LensFlareUtil.h"

// Editor
#include "Clipboard.h"
#include "Objects/EntityObject.h"
#include "Objects/SelectionGroup.h"
#include "Include/IObjectManager.h"


namespace LensFlareUtil
{
    IOpticsElementBasePtr CreateOptics(const XmlNodeRef& xmlNode)
    {
        IOpticsElementBasePtr pOpticsElement = NULL;

        const char* typeName;
        if (!xmlNode->getAttr("Type", &typeName))
        {
            return NULL;
        }

        bool bEnable(true);
        xmlNode->getAttr("Enable", bEnable);

        EFlareType flareType;
        if (!GetFlareType(typeName, flareType))
        {
            return NULL;
        }

        pOpticsElement = gEnv->pOpticsManager->Create(flareType);
        if (pOpticsElement == NULL)
        {
            return NULL;
        }

        pOpticsElement->SetEnabled(bEnable);

        if (!FillOpticsFromXML(pOpticsElement, xmlNode))
        {
            return NULL;
        }

        return pOpticsElement;
    }

    IOpticsElementBasePtr CreateOptics(IOpticsElementBasePtr pOptics, bool bForceTypeToGroup)
    {
        if (pOptics == NULL)
        {
            return NULL;
        }
        IOpticsElementBasePtr pNewOptics = NULL;
        if (bForceTypeToGroup)
        {
            if (pOptics->GetType() == eFT_Root)
            {
                pNewOptics = gEnv->pOpticsManager->Create(eFT_Group);
            }
        }
        if (pNewOptics == NULL)
        {
            pNewOptics = gEnv->pOpticsManager->Create(pOptics->GetType());
            if (pNewOptics == NULL)
            {
                return NULL;
            }
        }
        CopyOptics(pOptics, pNewOptics, true);
        return pNewOptics;
    }

    bool FillOpticsFromXML(IOpticsElementBasePtr pOpticsElement, const XmlNodeRef& xmlNode)
    {
        if (!pOpticsElement)
        {
            return false;
        }

        const char* name;
        if (!xmlNode->getAttr("Name", &name))
        {
            return false;
        }

        const char* typeName;
        if (!xmlNode->getAttr("Type", &typeName))
        {
            return false;
        }
        EFlareType flareType;
        if (!GetFlareType(typeName, flareType))
        {
            return false;
        }
        if (flareType != pOpticsElement->GetType())
        {
            return false;
        }

        pOpticsElement->SetName(name);

        bool bEnable(true);
        xmlNode->getAttr("Enable", bEnable);
        pOpticsElement->SetEnabled(bEnable);

        XmlNodeRef pParamNode = xmlNode->findChild("Params");
        if (pParamNode == NULL)
        {
            return false;
        }

        LensFlareUtil::FillParams(pParamNode, pOpticsElement);
        return true;
    }

    bool CreateXmlData(IOpticsElementBasePtr pOptics, XmlNodeRef& pOutNode)
    {
        pOutNode = gEnv->pSystem->CreateXmlNode("FlareItem");

        if (pOptics == NULL || pOutNode == NULL)
        {
            return false;
        }

        QString typeName;
        if (!GetFlareTypeName(typeName, pOptics))
        {
            return false;
        }

        pOutNode->setAttr("Name", pOptics->GetName().c_str());
        pOutNode->setAttr("Type", typeName.toUtf8().data());
        pOutNode->setAttr("Enable", pOptics->IsEnabled());

        XmlNodeRef pParamNode = pOutNode->createNode("Params");

        CVarBlockPtr pVar;
        SetVariablesTemplateFromOptics(pOptics, pVar);

        pVar->Serialize(pParamNode, false);
        pOutNode->addChild(pParamNode);

        for (int i = 0, iElementCount(pOptics->GetElementCount()); i < iElementCount; ++i)
        {
            XmlNodeRef pNode = NULL;
            IOpticsElementBasePtr pChildOptics = pOptics->GetElementAt(i);
            if (!CreateXmlData(pChildOptics, pNode))
            {
                continue;
            }

            pOutNode->addChild(pNode);
        }

        return true;
    }

    void SetVariablesTemplateFromOptics(IOpticsElementBasePtr pOptics, CVarBlockPtr& pRootVar, std::vector<IVariable::OnSetCallback*>& funcs)
    {
        if (pOptics == NULL)
        {
            return;
        }

        SetVariablesTemplateFromOptics(pOptics, pRootVar);

        for (int i = 0, iNumVariables(pRootVar->GetNumVariables()); i < iNumVariables; ++i)
        {
            IVariable* pVariable = pRootVar->GetVariable(i);
            if (pVariable == NULL)
            {
                continue;
            }
            for (int k = 0, iNumVariables2(pVariable->GetNumVariables()); k < iNumVariables2; ++k)
            {
                IVariable* pChildVariable(pVariable->GetVariable(k));
                if (pChildVariable == NULL)
                {
                    continue;
                }
                for (int ii = 0, iSize(funcs.size()); ii < iSize; ++ii)
                {
                    pChildVariable->AddOnSetCallback(funcs[ii]);
                }
            }
        }
    }

    extern void SetVariablesTemplateFromOptics(IOpticsElementBasePtr pOptics, CVarBlockPtr& pRootVar)
    {
        if (pOptics == NULL)
        {
            return;
        }

        AZStd::vector<FuncVariableGroup> paramGroups = pOptics->GetEditorParamGroups();
        pRootVar = new CVarBlock;

        for (int i = 0; i < paramGroups.size(); ++i)
        {
            CSmartVariableArray variableArray;
            FuncVariableGroup* pGroup = &paramGroups[i];
            if (pGroup == NULL)
            {
                continue;
            }

            QString displayGroupName(pGroup->GetHumanName());
            if (displayGroupName == "Common")
            {
                FlareInfoArray::Props flareInfo = FlareInfoArray::Get();
                int nTypeIndex = (int)pOptics->GetType();
                if (nTypeIndex >= 0 && nTypeIndex < flareInfo.size)
                {
                    displayGroupName += " : ";
                    displayGroupName += flareInfo.p[nTypeIndex].name;
                }
            }

            AddVariable(pRootVar, variableArray, pGroup->GetName(), displayGroupName.toUtf8().data(), "");

            for (int k = 0; k < pGroup->GetVariableCount(); ++k)
            {
                IFuncVariable* pFuncVar = pGroup->GetVariable(k);
                bool bHardMinLimitation = false;

                const std::pair<float, float> range(pFuncVar->GetMin(), pFuncVar->GetMax());

                if (pFuncVar->paramType == e_FLOAT)
                {
                    CSmartVariable<float> floatVar;
                    AddVariable(variableArray, floatVar, pFuncVar->name.c_str(), pFuncVar->humanName.c_str(), pFuncVar->description.c_str());
                    floatVar->SetLimits(range.first, range.second, 0, bHardMinLimitation, false);
                    floatVar->Set(pFuncVar->GetFloat());
                    floatVar->SetUserData(MakeFuncKey(i, k));
                }
                else if (pFuncVar->paramType == e_INT)
                {
                    CSmartVariable<int> intVar;
                    AddVariable(variableArray, intVar, pFuncVar->name.c_str(), pFuncVar->humanName.c_str(), pFuncVar->description.c_str());

                    bHardMinLimitation = HaveParameterLowBoundary(pFuncVar->name.c_str());
                    intVar->SetLimits(range.first, range.second, 0, bHardMinLimitation, false);

                    intVar->Set(pFuncVar->GetInt());
                    intVar->SetUserData(MakeFuncKey(i, k));
                }
                else if (pFuncVar->paramType == e_BOOL)
                {
                    CSmartVariable<bool> boolVar;
                    AddVariable(variableArray, boolVar, pFuncVar->name.c_str(), pFuncVar->humanName.c_str(), pFuncVar->description.c_str());
                    boolVar->Set(pFuncVar->GetBool());
                    boolVar->SetUserData(MakeFuncKey(i, k));
                }
                else if (pFuncVar->paramType == e_VEC2)
                {
                    CSmartVariable<Vec2> vec2Var;
                    AddVariable(variableArray, vec2Var, pFuncVar->name.c_str(), pFuncVar->humanName.c_str(), pFuncVar->description.c_str());
                    vec2Var->SetLimits(range.first, range.second, 0, bHardMinLimitation, false);
                    vec2Var->Set(pFuncVar->GetVec2());
                    vec2Var->SetUserData(MakeFuncKey(i, k));
                }
                else if (pFuncVar->paramType == e_VEC3)
                {
                    CSmartVariable<Vec3> vec3Var;
                    AddVariable(variableArray, vec3Var, pFuncVar->name.c_str(), pFuncVar->humanName.c_str(), pFuncVar->description.c_str());
                    vec3Var->SetLimits(range.first, range.second, 0, bHardMinLimitation, false);
                    vec3Var->Set(pFuncVar->GetVec3());
                    vec3Var->SetUserData(MakeFuncKey(i, k));
                }
                else if (pFuncVar->paramType == e_VEC4)
                {
                    CSmartVariable<Vec4> vec4Var;
                    AddVariable(variableArray, vec4Var, pFuncVar->name.c_str(), pFuncVar->humanName.c_str(), pFuncVar->description.c_str());
                    vec4Var->SetLimits(range.first, range.second, 0, bHardMinLimitation, false);
                    vec4Var->Set(pFuncVar->GetVec4());
                    vec4Var->SetUserData(MakeFuncKey(i, k));
                }
                else if (pFuncVar->paramType == e_COLOR)
                {
                    CSmartVariable<Vec3> colorVar;
                    AddVariable(variableArray, colorVar, pFuncVar->name.c_str(), pFuncVar->humanName.c_str(), pFuncVar->description.c_str(), IVariable::DT_COLOR);
                    ColorF color(pFuncVar->GetColorF());
                    Vec3 colorVec3(color.r, color.g, color.b);
                    colorVar->Set(colorVec3);
                    colorVar->SetUserData(MakeFuncKey(i, k));

                    CSmartVariable<int> alphaVar;
                    QString alphaName(pFuncVar->name.c_str());
                    alphaName += ".alpha";
                    QString alphaHumanName(pFuncVar->humanName.c_str());
                    alphaHumanName += " [alpha]";
                    AddVariable(variableArray, alphaVar, alphaName.toUtf8().data(), alphaHumanName.toUtf8().data(), pFuncVar->description.c_str());
                    alphaVar->SetLimits(0, 255, 0, bHardMinLimitation, false);
                    alphaVar->Set(color.a * 255.0f);
                    alphaVar->SetUserData(MakeFuncKey(i, k));
                }
                else if (pFuncVar->paramType == e_MATRIX33)
                {
                    // Reserved part
                    // This part is for future because Matrix33 type is suppose to be provided in a renderer side
                }
                else if (pFuncVar->paramType == e_TEXTURE2D || pFuncVar->paramType == e_TEXTURE3D || pFuncVar->paramType ==  e_TEXTURE_CUBE)
                {
                    CSmartVariable<QString> textureVar;
                    ITexture* pTexture = pFuncVar->GetTexture();
                    if (pTexture)
                    {
                        QString textureName = pTexture->GetName();
                        textureVar->Set(textureName);
                    }
                    AddVariable(variableArray, textureVar, pFuncVar->name.c_str(), pFuncVar->humanName.c_str(), pFuncVar->description.c_str(), IVariable::DT_TEXTURE);
                    textureVar->SetUserData(MakeFuncKey(i, k));
                }
            }
        }
    }

    void AddOptics(IOpticsElementBasePtr pParentOptics, const XmlNodeRef& xmlNode)
    {
        if (xmlNode == NULL)
        {
            return;
        }

        if (strcmp(xmlNode->getTag(), "FlareItem"))
        {
            return;
        }

        IOpticsElementBasePtr pOptics = CreateOptics(xmlNode);
        if (pOptics == NULL)
        {
            return;
        }
        pParentOptics->AddElement(pOptics);

        for (int i = 0, iChildCount(xmlNode->getChildCount()); i < iChildCount; ++i)
        {
            AddOptics(pOptics, xmlNode->getChild(i));
        }
    }

    void CopyVariable(IFuncVariable* pSrcVar, IFuncVariable* pDestVar)
    {
        if (!pSrcVar || !pDestVar)
        {
            return;
        }

        if (pSrcVar->paramType != pDestVar->paramType)
        {
            return;
        }

        if (pSrcVar->paramType == e_FLOAT)
        {
            float value = pSrcVar->GetFloat();
            pDestVar->InvokeSetter((void*)&value);
        }
        else if (pSrcVar->paramType == e_BOOL)
        {
            bool value = pSrcVar->GetBool();
            pDestVar->InvokeSetter((void*)&value);
        }
        else if (pSrcVar->paramType == e_INT)
        {
            int value = pSrcVar->GetInt();
            if (HaveParameterLowBoundary(pSrcVar->name.c_str()))
            {
                BoundaryProcess(value);
            }
            pDestVar->InvokeSetter((void*)&value);
        }
        else if (pSrcVar->paramType == e_VEC2)
        {
            Vec2 value = pSrcVar->GetVec2();
            pDestVar->InvokeSetter((void*)&value);
        }
        else if (pSrcVar->paramType == e_VEC3)
        {
            Vec3 value = pSrcVar->GetVec3();
            pDestVar->InvokeSetter((void*)&value);
        }
        else if (pSrcVar->paramType == e_VEC4)
        {
            Vec4 value = pSrcVar->GetVec4();
            pDestVar->InvokeSetter((void*)&value);
        }
        else if (pSrcVar->paramType == e_COLOR)
        {
            ColorF value = pSrcVar->GetColorF();
            pDestVar->InvokeSetter((void*)&value);
        }
        else if (pSrcVar->paramType == e_MATRIX33)
        {
            Matrix33 value = pSrcVar->GetMatrix33();
            pDestVar->InvokeSetter((void*)&value);
        }
        else if (pSrcVar->paramType == e_TEXTURE2D || pSrcVar->paramType == e_TEXTURE3D || pSrcVar->paramType == e_TEXTURE_CUBE)
        {
            ITexture* value = pSrcVar->GetTexture();
            pDestVar->InvokeSetter((void*)value);
        }
    }

    void CopyOpticsAboutSameOpticsType(IOpticsElementBasePtr pSrcOptics, IOpticsElementBasePtr pDestOptics)
    {
        if (pSrcOptics->GetType() != pDestOptics->GetType())
        {
            return;
        }

        AZStd::vector<FuncVariableGroup> srcVarGroups = pSrcOptics->GetEditorParamGroups();
        AZStd::vector<FuncVariableGroup> destVarGroups = pDestOptics->GetEditorParamGroups();

        if (srcVarGroups.size() != destVarGroups.size())
        {
            return;
        }

        pDestOptics->SetEnabled(pSrcOptics->IsEnabled());

        for (int i = 0, iVarGroupSize(srcVarGroups.size()); i < iVarGroupSize; ++i)
        {
            FuncVariableGroup srcVarGroup = srcVarGroups[i];
            FuncVariableGroup destVarGroup = destVarGroups[i];

            if (srcVarGroup.GetVariableCount() != destVarGroup.GetVariableCount())
            {
                continue;
            }

            for (int k = 0, iVarSize(srcVarGroup.GetVariableCount()); k < iVarSize; ++k)
            {
                IFuncVariable* pSrcVar = srcVarGroup.GetVariable(k);
                IFuncVariable* pDestVar = destVarGroup.GetVariable(k);
                CopyVariable(pSrcVar, pDestVar);
            }
        }
    }

    bool FindGroupByName(AZStd::vector<FuncVariableGroup>& groupList, const QString& name, int& outIndex)
    {
        for (int i = 0, iVarGroupSize(groupList.size()); i < iVarGroupSize; ++i)
        {
            if (name == groupList[i].GetName())
            {
                outIndex = i;
                return true;
            }
        }
        return false;
    }

    void CopyOpticsAboutDifferentOpticsType(IOpticsElementBasePtr pSrcOptics, IOpticsElementBasePtr pDestOptics)
    {
        if (pSrcOptics->GetType() == pDestOptics->GetType())
        {
            return;
        }

        AZStd::vector<FuncVariableGroup> srcVarGroups = pSrcOptics->GetEditorParamGroups();
        AZStd::vector<FuncVariableGroup> destVarGroups = pDestOptics->GetEditorParamGroups();

        pDestOptics->SetEnabled(pSrcOptics->IsEnabled());

        for (int i = 0, iVarGroupSize(destVarGroups.size()); i < iVarGroupSize; ++i)
        {
            FuncVariableGroup destVarGroup = destVarGroups[i];
            int nIndex = 0;
            if (!FindGroupByName(srcVarGroups, destVarGroup.GetName(), nIndex))
            {
                continue;
            }
            FuncVariableGroup srcVarGroup = srcVarGroups[nIndex];

            for (int k = 0, iVarSize(destVarGroup.GetVariableCount()); k < iVarSize; ++k)
            {
                IFuncVariable* pDestVar = destVarGroup.GetVariable(k);
                IFuncVariable* pSrcVar = srcVarGroup.FindVariable(pDestVar->name.c_str());
                CopyVariable(pSrcVar, pDestVar);
            }
        }
    }

    void CopyOptics(IOpticsElementBasePtr pSrcOptics, IOpticsElementBasePtr pDestOptics, bool bReculsiveCopy)
    {
        if (pSrcOptics->GetType() == pDestOptics->GetType())
        {
            CopyOpticsAboutSameOpticsType(pSrcOptics, pDestOptics);
        }
        else
        {
            CopyOpticsAboutDifferentOpticsType(pSrcOptics, pDestOptics);
        }

        if (bReculsiveCopy)
        {
            pDestOptics->RemoveAll();
            for (int i = 0, iChildCount(pSrcOptics->GetElementCount()); i < iChildCount; ++i)
            {
                IOpticsElementBasePtr pSrcChildOptics = pSrcOptics->GetElementAt(i);
                IOpticsElementBasePtr pNewOptics = gEnv->pOpticsManager->Create(pSrcChildOptics->GetType());
                if (pNewOptics == NULL)
                {
                    continue;
                }
                pNewOptics->SetName(pSrcChildOptics->GetName().c_str());
                CopyOptics(pSrcChildOptics, pNewOptics, bReculsiveCopy);
                pDestOptics->AddElement(pNewOptics);
            }
        }
    }

    CEntityObject* GetSelectedLightEntity()
    {
        CBaseObject* pSelectedObj = GetIEditor()->GetSelectedObject();
        if (pSelectedObj == NULL)
        {
            return NULL;
        }
        if (!qobject_cast<CEntityObject*>(pSelectedObj))
        {
            return NULL;
        }
        CEntityObject* pEntity = (CEntityObject*)pSelectedObj;
        if (!pEntity->IsLight())
        {
            return NULL;
        }
        return pEntity;
    }

    void GetSelectedLightEntities(std::vector<CEntityObject*>& outLightEntities)
    {
        CSelectionGroup* pSelectionGroup = GetIEditor()->GetSelection();
        if (pSelectionGroup == NULL)
        {
            return;
        }
        int nSelectionCount = pSelectionGroup->GetCount();
        outLightEntities.reserve(nSelectionCount);
        for (int i = 0; i < nSelectionCount; ++i)
        {
            CBaseObject* pSelectedObj = pSelectionGroup->GetObject(i);
            if (!qobject_cast<CEntityObject*>(pSelectedObj))
            {
                continue;
            }
            CEntityObject* pEntity = (CEntityObject*)pSelectedObj;
            if (!pEntity->IsLight())
            {
                continue;
            }
            outLightEntities.push_back(pEntity);
        }
    }

    IOpticsElementBasePtr GetSelectedLightOptics()
    {
        CEntityObject* pEntity = GetSelectedLightEntity();
        if (pEntity == NULL)
        {
            return NULL;
        }
        IOpticsElementBasePtr pEntityOptics = pEntity->GetOpticsElement();
        if (pEntityOptics == NULL)
        {
            return NULL;
        }
        return pEntityOptics;
    }

    IOpticsElementBasePtr FindOptics(IOpticsElementBasePtr pStartOptics, const QString& name)
    {
        if (pStartOptics == NULL)
        {
            return NULL;
        }

        if (!QString::compare(pStartOptics->GetName().c_str(), name))
        {
            return pStartOptics;
        }

        for (int i = 0, iElementCount(pStartOptics->GetElementCount()); i < iElementCount; ++i)
        {
            IOpticsElementBasePtr pFoundOptics = FindOptics(pStartOptics->GetElementAt(i), name);
            if (pFoundOptics)
            {
                return pFoundOptics;
            }
        }

        return NULL;
    }

    void RemoveOptics(IOpticsElementBasePtr pOptics)
    {
        if (pOptics == NULL)
        {
            return;
        }

        IOpticsElementBasePtr pParent = pOptics->GetParent();
        if (pParent == NULL)
        {
            return;
        }

        for (int i = 0, iElementCount(pParent->GetElementCount()); i < iElementCount; ++i)
        {
            if (pOptics == pParent->GetElementAt(i))
            {
                pParent->Remove(i);
                break;
            }
        }
    }

    void ChangeOpticsRootName(IOpticsElementBasePtr pOptics, const QString& newRootName)
    {
        if (pOptics == NULL)
        {
            return;
        }

        QString opticsName = pOptics->GetName().c_str();
        QString opticsRootName;

        int nPos = opticsName.indexOf(".");
        if (nPos == -1)
        {
            opticsRootName = opticsName;
        }
        else
        {
            opticsRootName = opticsName.left(nPos);
        }

        opticsName.replace(opticsRootName, newRootName);
        pOptics->SetName(opticsName.toUtf8().data());

        for (int i = 0, iElementSize(pOptics->GetElementCount()); i < iElementSize; ++i)
        {
            ChangeOpticsRootName(pOptics->GetElementAt(i), newRootName);
        }
    }

    void UpdateClipboard(const QString& type, const QString& groupName, bool bPasteAtSameLevel, std::vector<SClipboardData>& dataList)
    {
        XmlNodeRef rootNode = CreateXMLFromClipboardData(type, groupName, bPasteAtSameLevel, dataList);
        CClipboard clipboard(nullptr);
        clipboard.Put(rootNode);
    }

    XmlNodeRef CreateXMLFromClipboardData(const QString& type, const QString& groupName, bool bPasteAtSameLevel, std::vector<SClipboardData>& dataList)
    {
        XmlNodeRef rootNode = gEnv->pSystem->CreateXmlNode();
        rootNode->setTag("FlareDB");
        rootNode->setAttr("Type", type.toUtf8().data());
        rootNode->setAttr("GroupName", groupName.toUtf8().data());
        rootNode->setAttr("PasteAtSameLevel", bPasteAtSameLevel);

        for (int i = 0, iDataSize(dataList.size()); i < iDataSize; ++i)
        {
            XmlNodeRef xmlNode = gEnv->pSystem->CreateXmlNode();
            xmlNode->setTag("Data");
            dataList[i].FillXmlNode(xmlNode);
            rootNode->addChild(xmlNode);
        }

        return rootNode;
    }

    void UpdateOpticsName(IOpticsElementBasePtr pOptics)
    {
        if (pOptics == NULL)
        {
            return;
        }

        IOpticsElementBasePtr pParent = pOptics->GetParent();
        if (pParent)
        {
            QString oldName = pOptics->GetName().c_str();
            QString parentName = pParent->GetName().c_str();
            QString updatedName = parentName + QString(".") + GetShortName(oldName);
            pOptics->SetName(updatedName.toUtf8().data());
        }

        for (int i = 0, iElementCount(pOptics->GetElementCount()); i < iElementCount; ++i)
        {
            UpdateOpticsName(pOptics->GetElementAt(i));
        }
    }

    QString ReplaceLastName(const QString& fullName, const QString& shortName)
    {
        int nPos = fullName.lastIndexOf('.');
        if (nPos == -1)
        {
            return shortName;
        }
        QString newName = fullName.left(nPos + 1);
        newName += shortName;
        return newName;
    }

    IFuncVariable* GetFuncVariable(IOpticsElementBasePtr pOptics, int nFuncKey)
    {
        if (pOptics == NULL)
        {
            return NULL;
        }

        int nGroupIndex = (nFuncKey & 0xFFFF0000) >> 16;
        int nVarIndex = (nFuncKey & 0x0000FFFF);

        AZStd::vector<FuncVariableGroup> paramGroups = pOptics->GetEditorParamGroups();

        if (nGroupIndex >= paramGroups.size())
        {
            return NULL;
        }

        FuncVariableGroup* pGroup = &paramGroups[nGroupIndex];
        if (pGroup == NULL)
        {
            return NULL;
        }

        if (nVarIndex >= pGroup->GetVariableCount())
        {
            return NULL;
        }

        return pGroup->GetVariable(nVarIndex);
    }

    void GetLightEntityObjects(std::vector<CEntityObject*>& outEntityLights)
    {
        std::vector<CBaseObject*> pEntityObjects;
        GetIEditor()->GetObjectManager()->FindObjectsOfType(&CEntityObject::staticMetaObject, pEntityObjects);
        for (int i = 0, iObjectSize(pEntityObjects.size()); i < iObjectSize; ++i)
        {
            CEntityObject* pEntity = (CEntityObject*)pEntityObjects[i];
            if (pEntity == NULL)
            {
                continue;
            }
            if (pEntity->CheckFlags(OBJFLAG_DELETED))
            {
                continue;
            }
            if (!pEntity->IsLight())
            {
                continue;
            }
            outEntityLights.push_back(pEntity);
        }
    }

    void OutputOpticsDebug(IOpticsElementBasePtr pOptics)
    {
        QString opticsName;
        opticsName = QStringLiteral("Optics Name : %1\n").arg(pOptics->GetName().c_str());
        OutputDebugString(opticsName.toUtf8().data());

        AZStd::vector<FuncVariableGroup> groupArray(pOptics->GetEditorParamGroups());
        for (int i = 0, iGroupCount(groupArray.size()); i < iGroupCount; ++i)
        {
            FuncVariableGroup* pGroup = &groupArray[i];

            QString groupStr;
            groupStr = QStringLiteral("\tGroup : %1\n").arg(pGroup->GetName());
            OutputDebugString(groupStr.toUtf8().data());

            for (int k = 0, iParamCount(pGroup->GetVariableCount()); k < iParamCount; ++k)
            {
                IFuncVariable* pVar = pGroup->GetVariable(k);
                if (pVar == NULL)
                {
                    continue;
                }
                QString str;
                if (pVar->paramType == e_FLOAT)
                {
                    str = QStringLiteral("\t\t%1 : %2\n").arg(pVar->name.c_str()).arg(pVar->GetFloat());
                }
                else if (pVar->paramType == e_INT)
                {
                    str = QStringLiteral("\t\t%1 : %2\n").arg(pVar->name.c_str()).arg(pVar->GetInt());
                }
                else if (pVar->paramType == e_BOOL)
                {
                    str = QStringLiteral("\t\t%1 : %2\n").arg(pVar->name.c_str(), pVar->GetBool() ? "TRUE" : "FALSE");
                }
                else if (pVar->paramType == e_VEC2)
                {
                    str = QStringLiteral("\t\t%1 : %2,%3\n").arg(pVar->name.c_str()).arg(pVar->GetVec2().x).arg(pVar->GetVec2().y);
                }
                else if (pVar->paramType == e_VEC3)
                {
                    str = QStringLiteral("\t\t%1 : %2,%3,%4\n").arg(pVar->name.c_str()).arg(pVar->GetVec3().x).arg(pVar->GetVec3().y).arg(pVar->GetVec3().z);
                }
                else if (pVar->paramType == e_VEC4)
                {
                    str = QStringLiteral("\t\t%1 : %2,%3,%4,%5\n").arg(pVar->name.c_str()).arg(pVar->GetVec4().x).arg(pVar->GetVec4().y).arg(pVar->GetVec4().z);
                }
                else if (pVar->paramType == e_COLOR)
                {
                    str = QStringLiteral("\t\t%1 : %2,%3,%4,%5\n").arg(pVar->name.c_str()).arg(pVar->GetColorF().r).arg(pVar->GetColorF().g).arg(pVar->GetColorF().b).arg(pVar->GetColorF().a);
                }
                else if (pVar->paramType == e_TEXTURE2D || pVar->paramType == e_TEXTURE3D || pVar->paramType == e_TEXTURE_CUBE)
                {
                    if (pVar->GetTexture())
                    {
                        str = QStringLiteral("\t\t%1 : %2\n").arg(pVar->name.c_str(), pVar->GetTexture()->GetName());
                    }
                    else
                    {
                        str = QStringLiteral("\t\t%1 : NULL\n").arg(pVar->name.c_str());
                    }
                }
                OutputDebugString(str.toUtf8().data());
            }
        }

        for (int i = 0, iChildCount(pOptics->GetElementCount()); i < iChildCount; ++i)
        {
            OutputOpticsDebug(pOptics->GetElementAt(i));
        }
    }

    void FillParams(XmlNodeRef& paramNode, IOpticsElementBase* pOptics)
    {
        if (pOptics == NULL)
        {
            return;
        }
        AZStd::vector<FuncVariableGroup> groupArray(pOptics->GetEditorParamGroups());
        for (int i = 0, iGroupCount(paramNode->getChildCount()); i < iGroupCount; ++i)
        {
            XmlNodeRef groupNode = paramNode->getChild(i);
            if (groupNode == NULL)
            {
                continue;
            }

            int nGroupIndex(0);
            if (!FindGroup(groupNode->getTag(), pOptics, nGroupIndex))
            {
                continue;
            }

            FuncVariableGroup* pGroup = &groupArray[nGroupIndex];
            for (int k = 0, iParamCount(pGroup->GetVariableCount()); k < iParamCount; ++k)
            {
                IFuncVariable* pVar = pGroup->GetVariable(k);
                if (pVar == NULL)
                {
                    continue;
                }

                if (pVar->paramType == e_FLOAT)
                {
                    float fValue(0);
                    if (groupNode->getAttr(pVar->name.c_str(), fValue))
                    {
                        pVar->InvokeSetter((void*)&fValue);
                    }
                }
                else if (pVar->paramType == e_INT)
                {
                    int nValue(0);
                    if (groupNode->getAttr(pVar->name.c_str(), nValue))
                    {
                        pVar->InvokeSetter((void*)&nValue);
                    }
                }
                else if (pVar->paramType == e_BOOL)
                {
                    bool bValue(false);
                    if (groupNode->getAttr(pVar->name.c_str(), bValue))
                    {
                        pVar->InvokeSetter((void*)&bValue);
                    }
                }
                else if (pVar->paramType == e_VEC2)
                {
                    Vec2 vec2Value;
                    if (groupNode->getAttr(pVar->name.c_str(), vec2Value))
                    {
                        pVar->InvokeSetter((void*)&vec2Value);
                    }
                }
                else if (pVar->paramType == e_VEC3)
                {
                    Vec3 vec3Value;
                    if (groupNode->getAttr(pVar->name.c_str(), vec3Value))
                    {
                        pVar->InvokeSetter((void*)&vec3Value);
                    }
                }
                else if (pVar->paramType == e_VEC4)
                {
                    const char* strVec4Value;
                    if (groupNode->getAttr(pVar->name.c_str(), &strVec4Value))
                    {
                        Vec4 vec4Value;
                        ExtractVec4FromString(strVec4Value, vec4Value);
                        pVar->InvokeSetter((void*)&vec4Value);
                    }
                }
                else if (pVar->paramType == e_COLOR)
                {
                    Vec3 vec3Value;
                    if (!groupNode->getAttr(pVar->name.c_str(), vec3Value))
                    {
                        continue;
                    }
                    string alphaStr = string(pVar->name.c_str()) + ".alpha";
                    int alpha;
                    if (!groupNode->getAttr(alphaStr.c_str(), alpha))
                    {
                        continue;
                    }
                    ColorF color;
                    color.r = vec3Value.x;
                    color.g = vec3Value.y;
                    color.b = vec3Value.z;
                    color.a = (float)alpha / 255.0f;
                    pVar->InvokeSetter((void*)&color);
                }
                else if (pVar->paramType == e_MATRIX33)
                {
                    //reserved
                }
                else if (pVar->paramType == e_TEXTURE2D || pVar->paramType == e_TEXTURE3D || pVar->paramType == e_TEXTURE_CUBE)
                {
                    const char* textureName;
                    if (!groupNode->getAttr(pVar->name.c_str(), &textureName))
                    {
                        continue;
                    }
                    ITexture* pTexture = NULL;
                    if (textureName && strlen(textureName) > 0)
                    {
                        pTexture = gEnv->pRenderer->EF_LoadTexture(textureName);
                    }
                    pVar->InvokeSetter((void*)pTexture);
                    if (pTexture)
                    {
                        pTexture->Release();
                    }
                }
            }
        }
    }

    void GetExpandedItemNames(CBaseLibraryItem* item, const QString& groupName, const QString& itemName, QString& outNameWithGroup, QString& outFullName)
    {
        QString name;
        if (!groupName.isEmpty())
        {
            name = groupName + ".";
        }
        name += itemName;
        QString fullName = name;
        if (item->GetLibrary())
        {
            fullName = item->GetLibrary()->GetName() + "." + name;
        }
        outNameWithGroup = name;
        outFullName = fullName;
    }

    QModelIndex GetTreeItemByHitTest(QTreeView& treeCtrl)
    {
        return treeCtrl.indexAt(treeCtrl.mapFromGlobal(QCursor::pos()));
    }

    int FindOpticsIndexUnderParentOptics(IOpticsElementBasePtr pOptics, IOpticsElementBasePtr pParentOptics)
    {
        for (int i = 0, iElementSize(pParentOptics->GetElementCount()); i < iElementSize; ++i)
        {
            if (pParentOptics->GetElementAt(i) == pOptics)
            {
                return i + 1;
            }
        }
        return -1;
    }
}
