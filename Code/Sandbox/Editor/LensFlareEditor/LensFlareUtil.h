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

#ifndef CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREUTIL_H
#define CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREUTIL_H
#pragma once

#include <QTreeView>
#include <QModelIndex>

#include "Util/Variable.h"

#include "IFlares.h"
class CEntityObject;

class QTreeView;

#define LENSFLARE_ELEMENT_TREE "LensFlareElementTree"
#define LENSFLARE_ITEM_TREE "LensFlareItemTree"
#define FLARECLIPBOARDTYPE_COPY "Copy"
#define FLARECLIPBOARDTYPE_CUT "Cut"

namespace LensFlareUtil
{
    inline bool IsElement(EFlareType type)
    {
        return type != eFT__Base__ && type != eFT_Root && type != eFT_Group;
    }
    inline bool IsGroup(EFlareType type)
    {
        return type == eFT_Root || type == eFT_Group;
    }
    inline bool IsValidFlare(EFlareType type)
    {
        return type >= eFT__Base__ && type < eFT_Max;
    }

    inline void AddVariable(CVariableBase& varArray, CVariableBase& var, const char* varName, const char* humanVarName, const char* description, char dataType = IVariable::DT_SIMPLE)
    {
        if (varName)
        {
            var.SetName(varName);
        }
        if (humanVarName)
        {
            var.SetHumanName(humanVarName);
        }
        if (description)
        {
            var.SetDescription(description);
        }
        var.SetDataType(dataType);
        varArray.AddVariable(&var);
    }

    inline void AddVariable(CVarBlock* vars, CVariableBase& var, const char* varName, const char* humanVarName, const char* description, unsigned char dataType = IVariable::DT_SIMPLE)
    {
        if (varName)
        {
            var.SetName(varName);
        }
        if (humanVarName)
        {
            var.SetHumanName(humanVarName);
        }
        if (description)
        {
            var.SetDescription(description);
        }
        var.SetDataType(dataType);
        vars->AddVariable(&var);
    }

    inline bool GetFlareType(const QString& typeName, EFlareType& outType)
    {
        FlareInfoArray::Props array = FlareInfoArray::Get();
        for (size_t i = 0; i < array.size; ++i)
        {
            if (typeName == array.p[i].name)
            {
                outType = array.p[i].type;
                return true;
            }
        }
        return false;
    }

    inline bool GetFlareTypeName(QString& outTypeName, IOpticsElementBasePtr pOptics)
    {
        if (pOptics == NULL)
        {
            return false;
        }

        const FlareInfoArray::Props array = FlareInfoArray::Get();
        for (size_t i = 0; i < array.size; ++i)
        {
            if (array.p[i].type == pOptics->GetType())
            {
                outTypeName = array.p[i].name;
                return true;
            }
        }

        return false;
    }

    inline QString GetShortName(const QString& name)
    {
        int nPos = name.lastIndexOf('.');
        if (nPos == -1)
        {
            return name;
        }
        return name.right(name.length() - nPos - 1);
    }

    inline QString GetGroupNameFromName(const QString& name)
    {
        int nPos = name.lastIndexOf('.');
        if (nPos == -1)
        {
            return name;
        }
        return name.left(nPos);
    }

    inline QString GetGroupNameFromFullName(const QString& fullItemName)
    {
        int nLastDotPos = fullItemName.lastIndexOf('.');
        if (nLastDotPos == -1)
        {
            return fullItemName;
        }

        QString name = GetGroupNameFromName(fullItemName);

        int nFirstDotPos = name.indexOf('.');
        if (nFirstDotPos == -1)
        {
            return name;
        }

        return name.right(name.length() - nFirstDotPos - 1);
    }

    inline bool HaveParameterLowBoundary(const QString& paramName)
    {
        if (!QString::compare(paramName, "Noiseseed", Qt::CaseInsensitive) || !QString::compare(paramName, "translation", Qt::CaseInsensitive) || !QString::compare(paramName, "position", Qt::CaseInsensitive) || !QString::compare(paramName, "rotation", Qt::CaseInsensitive))
        {
            return false;
        }
        return true;
    }

    inline int MakeFuncKey(int nGroupIndex, int nVarIndex)
    {
        return (nGroupIndex << 16) | nVarIndex;
    }

    template<class T>
    inline void BoundaryProcess(T& fValue)
    {
        if (fValue < 0)
        {
            fValue = 0;
        }
    }

    inline void BoundaryProcess(Vec2& v)
    {
        if (v.x < 0)
        {
            v.x = 0;
        }
        if (v.y < 0)
        {
            v.y = 0;
        }
    }

    inline void BoundaryProcess(Vec3& v)
    {
        if (v.x < 0)
        {
            v.x = 0;
        }
        if (v.y < 0)
        {
            v.y = 0;
        }
        if (v.z < 0)
        {
            v.z = 0;
        }
    }

    inline void BoundaryProcess(Vec4& v)
    {
        if (v.x < 0)
        {
            v.x = 0;
        }
        if (v.y < 0)
        {
            v.y = 0;
        }
        if (v.z < 0)
        {
            v.z = 0;
        }
        if (v.w < 0)
        {
            v.w = 0;
        }
    }

    inline bool ExtractVec4FromString(const string& buffer, Vec4& outVec4)
    {
        int nCount(0);
        string copiedBuffer(buffer);
        int commaPos(0);
        while (nCount < 4 || commaPos != -1)
        {
            commaPos = copiedBuffer.find(",");
            string strV;
            if (commaPos == -1)
            {
                strV = copiedBuffer;
            }
            else
            {
                strV = copiedBuffer.Left(commaPos);
                strV += ",";
            }
            copiedBuffer.replace(0, commaPos + 1, "");
            outVec4[nCount++] = (float)atof(strV.c_str());
        }
        return nCount == 4;
    }

    void FillParams(XmlNodeRef& paramNode, IOpticsElementBase* pOptics);

    inline bool FindGroup(const char* groupName, IOpticsElementBase* pOptics, int& nOutGroupIndex)
    {
        if (groupName == NULL)
        {
            return false;
        }

        AZStd::vector<FuncVariableGroup> groupArray = pOptics->GetEditorParamGroups();
        for (int i = 0, iCount(groupArray.size()); i < iCount; ++i)
        {
            if (!_stricmp(groupArray[i].GetName(), groupName))
            {
                nOutGroupIndex = i;
                return true;
            }
        }
        return false;
    }

    struct SClipboardData
    {
        SClipboardData(){}
        ~SClipboardData(){}
        SClipboardData(const QString& from, const QString& lensFlareFullPath, const QString& lensOpticsPath)
            : m_From(from)
            , m_LensFlareFullPath(lensFlareFullPath)
            , m_LensOpticsPath(lensOpticsPath)
        {
        }

        void FillXmlNode(XmlNodeRef node)
        {
            if (node == NULL)
            {
                return;
            }
            node->setAttr("From", m_From.toUtf8().data());
            node->setAttr("FlareFullPath", m_LensFlareFullPath.toUtf8().data());
            node->setAttr("OpticsPath", m_LensOpticsPath.toUtf8().data());
        }

        void FillThisFromXmlNode(XmlNodeRef node)
        {
            if (node == NULL)
            {
                return;
            }
            node->getAttr("From", m_From);
            node->getAttr("FlareFullPath", m_LensFlareFullPath);
            node->getAttr("OpticsPath", m_LensOpticsPath);
        }

        QString m_From;
        QString m_LensFlareFullPath;
        QString m_LensOpticsPath;
    };

    struct SDragAndDropInfo
    {
        SDragAndDropInfo()
        {
            m_XMLContents = NULL;
            m_bDragging = false;
        }
        ~SDragAndDropInfo()
        {
            Reset();
        }
        void Reset()
        {
            m_XMLContents = NULL;
            m_bDragging = false;
        }
        bool m_bDragging;
        XmlNodeRef m_XMLContents;
    };

    inline SDragAndDropInfo& GetDragDropInfo()
    {
        static SDragAndDropInfo dragDropInfo;
        return dragDropInfo;
    }

    int FindOpticsIndexUnderParentOptics(IOpticsElementBasePtr pOptics, IOpticsElementBasePtr pParentOptics);
    bool IsPointInWindow(const QWidget* pWindow, const QPoint& screenPos);
    QModelIndex GetTreeItemByHitTest(QTreeView& treeCtrl);
    QPoint GetDragCursorPos(QWidget* pWnd, const QPoint& point);
    void GetExpandedItemNames(CBaseLibraryItem* item, const QString& groupName, const QString& itemName, QString& outNameWithGroup, QString& outFullName);
    void GetSelectedLightEntities(std::vector<CEntityObject*>& outLightEntities);
    void GetLightEntityObjects(std::vector<CEntityObject*>& outEntityLights);
    IFuncVariable* GetFuncVariable(IOpticsElementBasePtr pOptics, int nFuncKey);
    QString ReplaceLastName(const QString& fullName, const QString& shortName);
    void UpdateOpticsName(IOpticsElementBasePtr pOptics);
    XmlNodeRef CreateXMLFromClipboardData(const QString& type, const QString& groupName, bool bPasteAtSameLevel, std::vector<SClipboardData>& dataList);
    void UpdateClipboard(const QString& type, const QString& groupName, bool bPasteAtSameLevel, std::vector<SClipboardData>& dataList);
    void ChangeOpticsRootName(IOpticsElementBasePtr pOptics, const QString& newRootName);
    void RemoveOptics(IOpticsElementBasePtr pOptics);
    IOpticsElementBasePtr FindOptics(IOpticsElementBasePtr pStartOptics, const QString& name);
    CEntityObject* GetSelectedLightEntity();
    IOpticsElementBasePtr GetSelectedLightOptics();
    IOpticsElementBasePtr CreateOptics(const XmlNodeRef& xmlNode);
    IOpticsElementBasePtr CreateOptics(IOpticsElementBasePtr pOptics, bool bForceTypeToGroup = false);
    bool FillOpticsFromXML(IOpticsElementBasePtr pOptics, const XmlNodeRef& xmlNode);
    bool CreateXmlData(IOpticsElementBasePtr pOptics, XmlNodeRef& pOutNode);
    void SetVariablesTemplateFromOptics(IOpticsElementBasePtr pOptics, CVarBlockPtr& pRootVar, std::vector<IVariable::OnSetCallback>& funcs);
    void SetVariablesTemplateFromOptics(IOpticsElementBasePtr pOptics, CVarBlockPtr& pRootVar);
    void CopyOptics(IOpticsElementBasePtr pSrcOptics, IOpticsElementBasePtr pDestOptics, bool bReculsiveCopy = true);
    void OutputOpticsDebug(IOpticsElementBasePtr pOptics);
};
#endif // CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREUTIL_H
