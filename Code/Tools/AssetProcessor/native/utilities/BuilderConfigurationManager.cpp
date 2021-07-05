/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BuilderConfigurationManager.h"


#include <QFile>
#include <QSettings>

namespace AssetProcessor
{

    BuilderConfigurationManager::BuilderConfigurationManager()
    {
        BuilderConfigurationRequestBus::Handler::BusConnect();
    }

    BuilderConfigurationManager::~BuilderConfigurationManager()
    {
        BuilderConfigurationRequestBus::Handler::BusDisconnect();
    }
 
    bool BuilderConfigurationManager::LoadConfiguration(const AZStd::string& configFile) 
    { 
        if (!QFile::exists(configFile.c_str()))
        {
            AZ_Warning("BuilderConfiguration", false, "Couldn't load builder configuration file at %s", configFile.c_str());
            return false;
        }

        QSettings loader(configFile.c_str(), QSettings::IniFormat);

        QStringList groups = loader.childGroups();
        for (QString group : groups)
        {
            const char JobGroupKey[] = "Job ";
            if (group.startsWith(JobGroupKey, Qt::CaseInsensitive))
            {
                loader.beginGroup(group);
                AZStd::string jobName = group.mid(static_cast<int>(strlen(JobGroupKey))).toStdString().c_str(); // Job name is after "job "

                ParamMap thisMap;

                for (const auto& thisKey : loader.allKeys())
                {
                    thisMap[thisKey.toUtf8().data()] = loader.value(thisKey);
                }

                m_jobSettings[jobName] = thisMap;
                loader.endGroup();

            }

            const char BuilderGroupKey[] = "Builder ";
            if (group.startsWith(BuilderGroupKey, Qt::CaseInsensitive))
            {
                loader.beginGroup(group);
                AZStd::string builderName = group.mid(static_cast<int>(strlen(BuilderGroupKey))).toStdString().c_str(); // Builder name is after "Builder"

                ParamMap thisMap;

                for (const auto& thisKey : loader.allKeys())
                {
                    thisMap[AZStd::string(thisKey.toUtf8().data())] = QVariant(loader.value(thisKey));
                }

                m_builderSettings[builderName] = thisMap;
                loader.endGroup();
            }
        }

        m_loaded = true;
        return true;

    }

    bool BuilderConfigurationManager::UpdateJobDescriptor(const AZStd::string& jobKey, AssetBuilderSDK::JobDescriptor& jobDesc)
    {
        auto jobEntry = m_jobSettings.find(jobKey);
        if (jobEntry == m_jobSettings.end())
        {
            return false;
        }

        const auto& paramMap = jobEntry->second;

        auto thisParam = paramMap.find("fingerprint");
        if (thisParam != paramMap.end())
        {
            jobDesc.m_additionalFingerprintInfo = thisParam->second.toString().toStdString().c_str();
        }
        thisParam = paramMap.find("checkServer");
        if (thisParam != paramMap.end())
        {
            jobDesc.m_checkServer = thisParam->second.toBool();
        }
        thisParam = paramMap.find("critical");
        if (thisParam != paramMap.end())
        {
            jobDesc.m_critical = thisParam->second.toBool();;
        }
        thisParam = paramMap.find("priority");
        if (thisParam != paramMap.end())
        {
            jobDesc.m_priority = thisParam->second.toInt();
        }
        thisParam = paramMap.find("checkExclusiveLock");
        if (thisParam != paramMap.end())
        {
            jobDesc.m_checkExclusiveLock = thisParam->second.toBool();
        }
        thisParam = paramMap.find("params");
        if (thisParam != paramMap.end())
        {
            QString patternString = thisParam->second.type() == QVariant::StringList ? thisParam->second.toStringList().join(",") : thisParam->second.toString();
            if (patternString.length())
            {
                jobDesc.m_jobParameters.clear();
                QStringList paramList = patternString.split(",");
                for (const auto& stringParam : paramList)
                {
                    QStringList paramVals = stringParam.split("=");
                    jobDesc.m_jobParameters[AZ_CRC(paramVals[0].toUtf8().data())] = paramVals.size() > 1 ? paramVals[1].toUtf8().data() : "";
                }
            }
        }
        return true;
    }

    bool BuilderConfigurationManager::UpdateBuilderDescriptor(const AZStd::string& builderName, AssetBuilderSDK::AssetBuilderDesc& builderDesc)
    {
        auto builderEntry = m_builderSettings.find(builderName);
        if (builderEntry == m_builderSettings.end())
        {
            return false;
        }

        const auto& paramMap = builderEntry->second;
        auto paramEntry = paramMap.find("fingerprint");
        if (paramEntry != paramMap.end())
        {
            builderDesc.m_analysisFingerprint = paramEntry->second.toString().toStdString().c_str();
        }
        paramEntry = paramMap.find("version");
        if (paramEntry != paramMap.end())
        {
            builderDesc.m_version = paramEntry->second.toInt();
        }
        paramEntry = paramMap.find("flags");
        if (paramEntry != paramMap.end())
        {
            builderDesc.m_flags = static_cast<AZ::u8>(paramEntry->second.toInt());
        }
        paramEntry = builderEntry->second.find("patterns");
        if (paramEntry != paramMap.end())
        {
            QString patternString = paramEntry->second.type() == QVariant::StringList ? paramEntry->second.toStringList().join(",") : paramEntry->second.toString();
            if (patternString.length())
            {
                builderDesc.m_patterns.clear();
                QStringList paramList = patternString.split(",");
                for (const auto& thisParam : paramList)
                {
                    AssetBuilderSDK::AssetBuilderPattern thisPattern;
                    QStringList paramVals = thisParam.split("=");
                    thisPattern.m_pattern = paramVals[0].toUtf8().data();
                    if (paramVals.length() > 1 && (paramVals[1] == "1" || paramVals[1].compare("regex", Qt::CaseInsensitive)))
                    {
                        thisPattern.m_type = AssetBuilderSDK::AssetBuilderPattern::Regex;
                    }
                    else
                    {
                        thisPattern.m_type = AssetBuilderSDK::AssetBuilderPattern::Wildcard;
                    }
                    builderDesc.m_patterns.emplace_back(AZStd::move(thisPattern));
                }
            }
        }
        return true;
    }
} // namespace AssetProcessor
