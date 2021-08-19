/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "ConfigGroup.h"

namespace Config
{
    CConfigGroup::CConfigGroup()
    {
    }

    CConfigGroup::~CConfigGroup()
    {
        for (TConfigVariables::const_iterator it = m_vars.begin();
             it != m_vars.end(); ++it)
        {
            delete (*it);
        }
    }

    void CConfigGroup::AddVar(IConfigVar* var)
    {
        m_vars.push_back(var);
    }

    uint32 CConfigGroup::GetVarCount()
    {
        return static_cast<uint32>(m_vars.size());
    }

    IConfigVar* CConfigGroup::GetVar(const char* szName)
    {
        for (TConfigVariables::const_iterator it = m_vars.begin();
             it != m_vars.end(); ++it)
        {
            IConfigVar* var = (*it);
            if (0 == _stricmp(szName, var->GetName().c_str()))
            {
                return var;
            }
        }

        return nullptr;
    }

    const IConfigVar* CConfigGroup::GetVar(const char* szName) const
    {
        for (TConfigVariables::const_iterator it = m_vars.begin();
             it != m_vars.end(); ++it)
        {
            IConfigVar* var = (*it);
            if (0 == _stricmp(szName, var->GetName().c_str()))
            {
                return var;
            }
        }

        return nullptr;
    }

    IConfigVar* CConfigGroup::GetVar(uint index)
    {
        if (index < m_vars.size())
        {
            return m_vars[index];
        }

        return nullptr;
    }

    const IConfigVar* CConfigGroup::GetVar(uint index) const
    {
        if (index < m_vars.size())
        {
            return m_vars[index];
        }

        return nullptr;
    }

    void CConfigGroup::SaveToXML(XmlNodeRef node)
    {
        // save only values that don't have default values
        for (TConfigVariables::const_iterator it = m_vars.begin();
             it != m_vars.end(); ++it)
        {
            IConfigVar* var = (*it);
            if (!var->IsFlagSet(IConfigVar::eFlag_DoNotSave))
            {
                if (!var->IsDefault())
                {
                    const char* szName = var->GetName().c_str();

                    switch (var->GetType())
                    {
                    case IConfigVar::eType_BOOL:
                    {
                        bool currentValue = false;
                        var->Get(&currentValue);
                        node->setAttr(szName, currentValue);
                        break;
                    }

                    case IConfigVar::eType_INT:
                    {
                        int currentValue = 0;
                        var->Get(&currentValue);
                        node->setAttr(szName, currentValue);
                        break;
                    }

                    case IConfigVar::eType_FLOAT:
                    {
                        float currentValue = 0;
                        var->Get(&currentValue);
                        node->setAttr(szName, currentValue);
                        break;
                    }

                    case IConfigVar::eType_STRING:
                    {
                        AZStd::string currentValue;
                        var->Get(&currentValue);
                        node->setAttr(szName, currentValue.c_str());
                        break;
                    }
                    }
                }
            }
        }
    }

    void CConfigGroup::LoadFromXML(XmlNodeRef node)
    {
        // save only values that don't have default values
        for (TConfigVariables::const_iterator it = m_vars.begin();
             it != m_vars.end(); ++it)
        {
            IConfigVar* var = (*it);
            if (!var->IsFlagSet(IConfigVar::eFlag_DoNotSave))
            {
                const char* szName = var->GetName().c_str();

                switch (var->GetType())
                {
                case IConfigVar::eType_BOOL:
                {
                    bool currentValue = false;
                    var->GetDefault(&currentValue);
                    if (node->getAttr(szName, currentValue))
                    {
                        var->Set(&currentValue);
                    }
                    break;
                }

                case IConfigVar::eType_INT:
                {
                    int currentValue = 0;
                    var->GetDefault(&currentValue);
                    if (node->getAttr(szName, currentValue))
                    {
                        var->Set(&currentValue);
                    }
                    break;
                }

                case IConfigVar::eType_FLOAT:
                {
                    float currentValue = 0;
                    var->GetDefault(&currentValue);
                    if (node->getAttr(szName, currentValue))
                    {
                        var->Set(&currentValue);
                    }
                    break;
                }

                case IConfigVar::eType_STRING:
                {
                    AZStd::string currentValue;
                    var->GetDefault(&currentValue);
                    QString readValue(currentValue.c_str());
                    if (node->getAttr(szName, readValue))
                    {
                        currentValue = readValue.toUtf8().data();
                        var->Set(&currentValue);
                    }
                    break;
                }
                }
            }
        }
    }
}
