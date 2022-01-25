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
        for (IConfigVar* var : m_vars)
        {
            delete var;
        }
    }

    void CConfigGroup::AddVar(IConfigVar* var)
    {
        m_vars.push_back(var);
    }

    AZ::u32 CConfigGroup::GetVarCount()
    {
        return aznumeric_cast<AZ::u32>(m_vars.size());
    }

    IConfigVar* CConfigGroup::GetVar(const char* szName)
    {
        for (IConfigVar* var : m_vars)
        {
            if (0 == _stricmp(szName, var->GetName().c_str()))
            {
                return var;
            }
        }

        return nullptr;
    }

    const IConfigVar* CConfigGroup::GetVar(const char* szName) const
    {
        for (const IConfigVar* var : m_vars)
        {
            if (0 == _stricmp(szName, var->GetName().c_str()))
            {
                return var;
            }

        }

        return nullptr;
    }

    IConfigVar* CConfigGroup::GetVar(AZ::u32 index)
    {
        if (index < m_vars.size())
        {
            return m_vars[index];
        }

        return nullptr;
    }

    const IConfigVar* CConfigGroup::GetVar(AZ::u32 index) const
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
        for (const IConfigVar* var : m_vars)
        {
            if (var->IsFlagSet(IConfigVar::eFlag_DoNotSave) || var->IsDefault())
            {
                continue;
            }

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

    void CConfigGroup::LoadFromXML(XmlNodeRef node)
    {
        // load values that are save-able
        for (IConfigVar* var : m_vars)
        {
            if (var->IsFlagSet(IConfigVar::eFlag_DoNotSave))
            {
                continue;
            }
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
