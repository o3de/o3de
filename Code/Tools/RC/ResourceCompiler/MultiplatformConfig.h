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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_MULTIPLATFORMCONFIG_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_MULTIPLATFORMCONFIG_H
#pragma once

#include "Config.h"
#include "IMultiplatformConfig.h"


class MultiplatformConfig
    : public IMultiplatformConfig
{
public:
    MultiplatformConfig()
        : m_platformCount(0)
    {
    }

    virtual void init(int platformCount, int activePlatform, IConfigKeyRegistry* pConfigKeyRegistry)
    {
        if (platformCount < 0 ||
            platformCount > kMaxPlatformCount ||
            unsigned(activePlatform) >= platformCount ||
            pConfigKeyRegistry == 0)
        {
            assert(0);
            m_platformCount = 0;
            return;
        }

        m_platformCount = platformCount;
        m_activePlatform = activePlatform;

        for (int i = 0; i < platformCount; ++i)
        {
            m_config[i].SetConfigKeyRegistry(pConfigKeyRegistry);
        }
    }

    virtual int getPlatformCount() const
    {
        return m_platformCount;
    }

    virtual int getActivePlatform() const
    {
        return m_activePlatform;
    }

    virtual const IConfig& getConfig(int platform) const
    {
        assert(unsigned(platform) < unsigned(m_platformCount));
        return m_config[platform];
    }

    virtual IConfig& getConfig(int platform)
    {
        assert(unsigned(platform) < unsigned(m_platformCount));
        return m_config[platform];
    }

    virtual const IConfig& getConfig() const
    {
        return getConfig(m_activePlatform);
    }

    virtual IConfig& getConfig()
    {
        return getConfig(m_activePlatform);
    }

    virtual void setKeyValue(EConfigPriority ePri, const char* key, const char* value)
    {
        for (int i = 0; i < m_platformCount; ++i)
        {
            m_config[i].SetKeyValue(ePri, key, value);
        }
    }

    virtual void setActivePlatform(int platformIndex)
    {
        m_activePlatform = platformIndex;
    }
private:
    enum
    {
        kMaxPlatformCount = 20
    };
    int m_platformCount;
    int m_activePlatform;
    Config m_config[kMaxPlatformCount];
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_MULTIPLATFORMCONFIG_H
