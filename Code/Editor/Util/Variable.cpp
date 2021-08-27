/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "Variable.h"
#include "UIEnumsDatabase.h"

#include "UsedResources.h"              // for CUsedResources

//////////////////////////////////////////////////////////////////////////
CVarBlock* CVarBlock::Clone(bool bRecursive) const
{
    CVarBlock* vb = new CVarBlock;
    for (Variables::const_iterator it = m_vars.begin(); it != m_vars.end(); ++it)
    {
        IVariable* var = *it;
        vb->AddVariable(var->Clone(bRecursive));
    }
    return vb;
}

//////////////////////////////////////////////////////////////////////////
void CVarBlock::CopyValues(const CVarBlock* fromVarBlock)
{
    // Copy all variables.
    int numSrc = fromVarBlock->GetNumVariables();
    int numTrg = GetNumVariables();
    for (int i = 0; i < numSrc && i < numTrg; i++)
    {
        GetVariable(i)->CopyValue(fromVarBlock->GetVariable(i));
    }
}

//////////////////////////////////////////////////////////////////////////
void CVarBlock::CopyValuesByName(CVarBlock* fromVarBlock)
{
    // Copy values using saving and loading to/from xml.
    XmlNodeRef node = XmlHelpers::CreateXmlNode("Temp");
    fromVarBlock->Serialize(node, false);
    Serialize(node, true);
}

//////////////////////////////////////////////////////////////////////////
void CVarBlock::OnSetValues()
{
    for (Variables::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
    {
        IVariable* var = *it;
        var->OnSetValue(true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVarBlock::SetRecreateSplines()
{
    for (Variables::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
    {
        IVariable* var = *it;
        var->SetFlagRecursive(IVariable::UI_CREATE_SPLINE);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVarBlock::AddVariable(IVariable* var)
{
    //assert( !strstr(var->GetName(), " ") ); // spaces not allowed because of serialization
    m_vars.push_back(var);
}

//////////////////////////////////////////////////////////////////////////
void CVarBlock::AddVariable(IVariable* pVar, const char* varName, unsigned char dataType)
{
    if (varName)
    {
        pVar->SetName(varName);
    }
    pVar->SetDataType(dataType);
    AddVariable(pVar);
}

//////////////////////////////////////////////////////////////////////////
void CVarBlock::AddVariable(CVariableBase& var, const char* varName, unsigned char dataType)
{
    if (varName)
    {
        var.SetName(varName);
    }
    var.SetDataType(dataType);
    AddVariable(&var);
}

//////////////////////////////////////////////////////////////////////////
bool CVarBlock::DeleteVariable(IVariable* var, bool bRecursive)
{
    bool found = stl::find_and_erase(m_vars, var);

    if (!found && bRecursive)
    {
        for (Variables::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
        {
            if ((*it)->DeleteVariable(var, bRecursive))
            {
                return true;
            }
        }
    }

    return found;
}

//////////////////////////////////////////////////////////////////////////
bool CVarBlock::IsContainsVariable(IVariable* pVar, bool bRecursive) const
{
    for (Variables::const_iterator it = m_vars.begin(); it != m_vars.end(); ++it)
    {
        if (*it == pVar)
        {
            return true;
        }
    }

    // If not found search childs.
    if (bRecursive)
    {
        // Search all top level variables.
        for (Variables::const_iterator it = m_vars.begin(); it != m_vars.end(); ++it)
        {
            if ((*it)->IsContainsVariable(pVar))
            {
                return true;
            }
        }
    }

    return false;
}

namespace
{
    IVariable* FindVariable(const char* name, bool bRecursive, bool bHumanName, const std::vector<IVariablePtr>& vars)
    {
        // Search all top level variables.
        for (std::vector<IVariablePtr>::const_iterator it = vars.begin(); it != vars.end(); ++it)
        {
            IVariable* var = *it;
            if (bHumanName && QString::compare(var->GetHumanName(), name, Qt::CaseInsensitive) == 0)
            {
                return var;
            }
            else if (!bHumanName && QString::compare(var->GetName(), name) == 0)
            {
                return var;
            }
        }

        // If not found search childs.
        if (bRecursive)
        {
            // Search all top level variables.
            for (std::vector<IVariablePtr>::const_iterator it = vars.begin(); it != vars.end(); ++it)
            {
                IVariable* var = *it;
                IVariable* found = var->FindVariable(name, bRecursive, bHumanName);
                if (found)
                {
                    return found;
                }
            }
        }

        return nullptr;
    }
}

//////////////////////////////////////////////////////////////////////////
IVariable* CVarBlock::FindVariable(const char* name, bool bRecursive, bool bHumanName) const
{
    return ::FindVariable(name, bRecursive, bHumanName, m_vars);
}

//////////////////////////////////////////////////////////////////////////
IVariable* CVariableArray::FindVariable(const char* name, bool bRecursive, bool bHumanName) const
{
    return ::FindVariable(name, bRecursive, bHumanName, m_vars);
}

//////////////////////////////////////////////////////////////////////////
void CVarBlock::Serialize(XmlNodeRef vbNode, bool load)
{
    if (load)
    {
        // Loading.
        QString name;
        for (Variables::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
        {
            IVariable* var = *it;
            if (var->GetNumVariables())
            {
                XmlNodeRef child = vbNode->findChild(var->GetName().toUtf8().data());
                if (child)
                {
                    var->Serialize(child, load);
                }
            }
            else
            {
                var->Serialize(vbNode, load);
            }
        }
    }
    else
    {
        // Saving.
        for (Variables::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
        {
            IVariable* var = *it;
            if (var->GetNumVariables())
            {
                XmlNodeRef child = vbNode->newChild(var->GetName().toUtf8().data());
                var->Serialize(child, load);
            }
            else
            {
                var->Serialize(vbNode, load);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVarBlock::ReserveNumVariables(int numVars)
{
    m_vars.reserve(numVars);
}

//////////////////////////////////////////////////////////////////////////
void CVarBlock::WireVar(IVariable* src, IVariable* trg, bool bWire)
{
    if (bWire)
    {
        src->Wire(trg);
    }
    else
    {
        src->Unwire(trg);
    }
    int numSrcVars = src->GetNumVariables();
    if (numSrcVars > 0)
    {
        int numTrgVars = trg->GetNumVariables();
        for (int i = 0; i < numSrcVars && i < numTrgVars; i++)
        {
            WireVar(src->GetVariable(i), trg->GetVariable(i), bWire);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVarBlock::Wire(CVarBlock* toVarBlock)
{
    Variables::iterator tit = toVarBlock->m_vars.begin();
    Variables::iterator sit = m_vars.begin();
    for (; sit != m_vars.end() && tit != toVarBlock->m_vars.end(); ++sit, ++tit)
    {
        IVariable* src = *sit;
        IVariable* trg = *tit;
        WireVar(src, trg, true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVarBlock::Unwire(CVarBlock* toVarBlock)
{
    Variables::iterator tit = toVarBlock->m_vars.begin();
    Variables::iterator sit = m_vars.begin();
    for (; sit != m_vars.end() && tit != toVarBlock->m_vars.end(); ++sit, ++tit)
    {
        IVariable* src = *sit;
        IVariable* trg = *tit;
        WireVar(src, trg, false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVarBlock::AddOnSetCallback(IVariable::OnSetCallback* func)
{
    for (Variables::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
    {
        IVariable* var = *it;
        SetCallbackToVar(func, var, true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVarBlock::RemoveOnSetCallback(IVariable::OnSetCallback* func)
{
    for (Variables::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
    {
        IVariable* var = *it;
        SetCallbackToVar(func, var, false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVarBlock::SetCallbackToVar(IVariable::OnSetCallback* func, IVariable* pVar, bool bAdd)
{
    if (bAdd)
    {
        pVar->AddOnSetCallback(func);
    }
    else
    {
        pVar->RemoveOnSetCallback(func);
    }
    int numVars = pVar->GetNumVariables();
    if (numVars > 0)
    {
        for (int i = 0; i < numVars; i++)
        {
            SetCallbackToVar(func, pVar->GetVariable(i), bAdd);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVarBlock::GatherUsedResources(CUsedResources& resources)
{
    for (int i = 0; i < GetNumVariables(); i++)
    {
        IVariable* pVar = GetVariable(i);
        GatherUsedResourcesInVar(pVar, resources);
    }
}
//////////////////////////////////////////////////////////////////////////
void CVarBlock::EnableUpdateCallbacks(bool boEnable)
{
    for (int i = 0; i < GetNumVariables(); i++)
    {
        IVariable* pVar = GetVariable(i);
        pVar->EnableUpdateCallbacks(boEnable);
    }
}
//////////////////////////////////////////////////////////////////////////
void CVarBlock::GatherUsedResourcesInVar(IVariable* pVar, CUsedResources& resources)
{
    int type = pVar->GetDataType();
    if (type == IVariable::DT_TEXTURE)
    {
        // this is file.
        QString filename;
        pVar->Get(filename);
        if (!filename.isEmpty())
        {
            resources.Add(filename.toUtf8().data());
        }
    }

    for (int i = 0; i < pVar->GetNumVariables(); i++)
    {
        GatherUsedResourcesInVar(pVar->GetVariable(i), resources);
    }
}


//////////////////////////////////////////////////////////////////////////
inline bool CompareNames(const IVariable* pVar1, const IVariable* pVar2)
{
    return (QString::compare(pVar1->GetHumanName(), pVar2->GetHumanName(), Qt::CaseInsensitive) < 0);
}


//////////////////////////////////////////////////////////////////////////
void CVarBlock::Sort()
{
    std::sort(m_vars.begin(), m_vars.end(), CompareNames);
}


//////////////////////////////////////////////////////////////////////////
CVarObject::CVarObject()
{}

//////////////////////////////////////////////////////////////////////////
CVarObject::~CVarObject()
{}

//////////////////////////////////////////////////////////////////////////
void CVarObject::AddVariable(CVariableBase& var, const QString& varName, VarOnSetCallback* cb, unsigned char dataType)
{
    if (!m_vars)
    {
        m_vars = new CVarBlock;
    }
    var.AddRef(); // Variables are local and must not be released by CVarBlock.
    var.SetName(varName);
    var.SetDataType(dataType);
    if (cb)
    {
        var.AddOnSetCallback(cb);
    }
    m_vars->AddVariable(&var);
}

//////////////////////////////////////////////////////////////////////////
void CVarObject::AddVariable(CVariableBase& var, const QString& varName, const QString& varHumanName, VarOnSetCallback* cb, unsigned char dataType)
{
    if (!m_vars)
    {
        m_vars = new CVarBlock;
    }
    var.AddRef(); // Variables are local and must not be released by CVarBlock.
    var.SetName(varName);
    var.SetHumanName(varHumanName);
    var.SetDataType(dataType);
    if (cb)
    {
        var.AddOnSetCallback(cb);
    }
    m_vars->AddVariable(&var);
}

//////////////////////////////////////////////////////////////////////////
void CVarObject::AddVariable(CVariableArray& table, CVariableBase& var, const QString& varName, const QString& varHumanName, VarOnSetCallback* cb, unsigned char dataType)
{
    if (!m_vars)
    {
        m_vars = new CVarBlock;
    }
    var.AddRef(); // Variables are local and must not be released by CVarBlock.
    var.SetName(varName);
    var.SetHumanName(varHumanName);
    var.SetDataType(dataType);
    if (cb)
    {
        var.AddOnSetCallback(cb);
    }
    table.AddVariable(&var);
}

//////////////////////////////////////////////////////////////////////////
void CVarObject::RemoveVariable(IVariable* var)
{
    if (m_vars != nullptr)
    {
        m_vars->DeleteVariable(var);
    }
}
//////////////////////////////////////////////////////////////////////////
void CVarObject::EnableUpdateCallbacks(bool boEnable)
{
    if (m_vars != nullptr)
    {
        m_vars->EnableUpdateCallbacks(boEnable);
    }
}
//////////////////////////////////////////////////////////////////////////
void CVarObject::OnSetValues()
{
    if (m_vars != nullptr)
    {
        m_vars->OnSetValues();
    }
}
//////////////////////////////////////////////////////////////////////////
void CVarObject::ReserveNumVariables(int numVars)
{
    if (m_vars != nullptr)
    {
        m_vars->ReserveNumVariables(numVars);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVarObject::CopyVariableValues(CVarObject* sourceObject)
{
    // Check if compatible types.
    assert(metaObject() == sourceObject->metaObject());
    if (m_vars != nullptr && sourceObject->m_vars != nullptr)
    {
        m_vars->CopyValues(sourceObject->m_vars);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVarObject::Serialize(XmlNodeRef node, bool load)
{
    if (m_vars)
    {
        m_vars->Serialize(node, load);
    }
}


CVarGlobalEnumList::CVarGlobalEnumList(CUIEnumsDatabase_SEnum* pEnum)
    : m_pEnum(pEnum)
{
}

CVarGlobalEnumList::CVarGlobalEnumList(const QString& enumName)
{
    m_pEnum = GetIEditor()->GetUIEnumsDatabase()->FindEnum(enumName);
}

//! Get the name of specified value in enumeration.
QString CVarGlobalEnumList::GetItemName(uint index)
{
    if (!m_pEnum || index >= static_cast<uint>(m_pEnum->strings.size()))
    {
        return QString();
    }
    return m_pEnum->strings[index];
}

QString CVarGlobalEnumList::NameToValue(const QString& name)
{
    if (m_pEnum)
    {
        return m_pEnum->NameToValue(name);
    }
    else
    {
        return name;
    }
}

QString CVarGlobalEnumList::ValueToName(const QString& value)
{
    if (m_pEnum)
    {
        return m_pEnum->ValueToName(value);
    }
    else
    {
        return value;
    }
}

