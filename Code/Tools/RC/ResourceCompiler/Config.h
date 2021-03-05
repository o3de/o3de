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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_CONFIG_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_CONFIG_H
#pragma once

#include "IConfig.h"
#include "StlUtils.h"
#include "StringHelpers.h"

class CPropertyVars;

/** Implementation of IConfig interface.
*/
class Config
    : public IConfig
{
public:
    Config();
    virtual ~Config();

    void SetConfigKeyRegistry(IConfigKeyRegistry* pConfigKeyRegistry);
    IConfigKeyRegistry* GetConfigKeyRegistry() const;

    // interface IConfigSink ------------------------------------------

    virtual void SetKeyValue(EConfigPriority ePri, const char* key, const char* value);

    // interface IConfig ----------------------------------------------

    virtual void Release() { delete this; };
    virtual const Config* GetInternalRepresentation() const;
    virtual bool HasKeyRegistered(const char* key) const;
    virtual bool HasKeyMatchingWildcards(const char* wildcards) const;
    virtual bool GetKeyValue(const char* key, const char*& value, int ePriMask) const;
    virtual int GetSum(const char* key) const;
    virtual void GetUnknownKeys(std::vector<string>& unknownKeys) const;
    virtual void AddConfig(const IConfig* inpConfig);
    virtual void Clear();
    virtual uint32 ClearPriorityUsage(int ePriMask);
    virtual uint32 CountPriorityUsage(int ePriMask) const;
    virtual void CopyToConfig(EConfigPriority ePri, IConfigSink* pDestConfig) const;
    virtual void CopyToPropertyVars(CPropertyVars& properties) const;

private: // ---------------------------------------------------------------------

    struct MapKey
    {
        string m_sKeyName;
        EConfigPriority m_eKeyPri;

        bool operator<(const MapKey& b) const
        {
            // sort by m_sKeyName
            const int cmp = StringHelpers::CompareIgnoreCase(m_sKeyName, b.m_sKeyName);
            if (cmp != 0)
            {
                return (cmp < 0);
            }

            // sort by m_eKeyPri (higher priority goes first)
            return (m_eKeyPri > b.m_eKeyPri);
        }
    };

    typedef std::map<MapKey, string> Map;

    Map m_map;
    IConfigKeyRegistry* m_pConfigKeyRegistry;   // used to verify key registration
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_CONFIG_H
