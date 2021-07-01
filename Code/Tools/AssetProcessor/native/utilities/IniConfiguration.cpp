/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "native/utilities/IniConfiguration.h"
#include "native/utilities/assetUtils.h"

namespace
{
    // singleton Pattern
    IniConfiguration* s_iniConfigurationSingleton = nullptr;
}

IniConfiguration::IniConfiguration(QObject* pParent)
    : QObject(pParent)
    , m_listeningPort(0)
{
    AZ_Assert(s_iniConfigurationSingleton == nullptr, "Duplicate singleton installation detected.");
    s_iniConfigurationSingleton = this;
}

IniConfiguration::~IniConfiguration()
{
    AZ_Assert(s_iniConfigurationSingleton == this, "There should always be a single singleton!");
    s_iniConfigurationSingleton = nullptr;
}

const IniConfiguration* IniConfiguration::Get()
{
    return s_iniConfigurationSingleton;
}

void IniConfiguration::parseCommandLine(QStringList args)
{
    for (QString arg : args)
    {
        if (arg.startsWith("--port="))
        {
            bool converted = false;
            quint16 port = arg.replace("--port=", "").toUShort(&converted);
            m_listeningPort = converted ? port : m_listeningPort;
        }
    }
}

void IniConfiguration::readINIConfigFile(QDir dir)
{
    m_userConfigFilePath = dir.filePath("AssetProcessorConfiguration.ini");
    // if AssetProcessorProxyInformation.ini file exists then delete it
    // we used to store proxy info in this file
    if (QFile::exists(m_userConfigFilePath))
    {
        QFile::remove(m_userConfigFilePath);
    }
    
    m_listeningPort = AssetUtilities::ReadListeningPortFromSettingsRegistry();
}

quint16 IniConfiguration::listeningPort() const
{
    return m_listeningPort;
}

void IniConfiguration::SetListeningPort(quint16 port)
{
    m_listeningPort = port;
}

