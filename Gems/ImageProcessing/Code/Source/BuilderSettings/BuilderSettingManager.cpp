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

#include "ImageProcessing_precompiled.h"

#include "BuilderSettingManager.h"
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QSettings>

#include <ImageProcessing/PixelFormats.h>
#include <BuilderSettings/CubemapSettings.h>
#include <BuilderSettings/TextureSettings.h>
#include <Processing/PixelFormatInfo.h>
#include <Processing/ImageToProcess.h>
#include <Converters/Cubemap.h>
#include <ImageLoader/ImageLoaders.h>
#include <ImageProcessing_Traits_Platform.h>

#include <AzCore/Math/Sha1.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Application/Application.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace ImageProcessing
{

#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
    namespace ImageProcess##PrivateName\
    {\
        bool DoesSupport(AZStd::string);\
    }
AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif //AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS

    const char* BuilderSettingManager::s_environmentVariableName = "ImageBuilderSettingManager";
    AZ::EnvironmentVariable<BuilderSettingManager*> BuilderSettingManager::s_globalInstance = nullptr;
    AZStd::mutex BuilderSettingManager::s_instanceMutex;
    const PlatformName BuilderSettingManager::s_defaultPlatform = AZ_TRAIT_IMAGEPROCESSING_DEFAULT_PLATFORM;
    
    AZ::Outcome<AZStd::vector<AZStd::string>, AZStd::string> GetPlatformNamesFromRC(AZStd::string& filePath)
    {
        QFile inputFile(filePath.c_str());

        if (inputFile.exists() == false)
        {
            return AZ::Failure(AZStd::string::format("'%s' does not exist", filePath.c_str()));
        }

        AZStd::vector<AZStd::string> all_platforms;
        if (inputFile.open(QIODevice::ReadOnly))
        {
            QTextStream in(&inputFile);
            while (!in.atEnd())
            {
                if (in.readLine() == "[_platform]")
                {
                    QString name_line = in.readLine();
                    if (name_line.contains("name="))
                    {
                        QString name_value = name_line.split("=")[1];
                        // Remove name alias after ','
                        AZStd::string az_name_value = name_value.split(",")[0].toUtf8().constData();
                        all_platforms.push_back(az_name_value);
                    }
                }
            }
            inputFile.close();
        }
        return AZ::Success(all_platforms);
    }

    StringOutcome ParseKeyToData(QString settingKey, const QSettings& rcINI, PresetSettings& presetSettings)
    {
        // We may be parsing platform-specific settings denoted by a colon and a platform (ex. mintexturesize:ios=35).
        // We always extract the actual setting name to determine the setting we are parsing. We must reference the key
        // as a whole to properly index into the rcINI file content.
        QString key = settingKey.split(":")[0];

        // There is no standard way to map an enum to string--
        // Also, can't seem to make this static because it allocates something that the destructor doesn't catch.
        const AZStd::map<QString, ColorSpace> colorSpaceMap{
            { "linear"    , ColorSpace::linear },
            { "sRGB"      , ColorSpace::sRGB },
            { "auto"      , ColorSpace::autoSelect }
        }; 

        const AZStd::map<QString, RGBWeight> rgbWeightMap{
            { "uniform"    , RGBWeight::uniform },
            { "luminance"      , RGBWeight::luminance },
            { "ciexyz"      , RGBWeight::ciexyz }
        };
        
        //cm_ftype: gaussian, cone, disc, cosine, cosine_power, ggx
        const AZStd::map<QString, CubemapFilterType> cubemapFilterTypeMap{
            { "cone"    , CubemapFilterType::cone },
            { "gaussian"    , CubemapFilterType::gaussian },
            { "ggx"    , CubemapFilterType::ggx },
            { "cosine"      , CubemapFilterType::cosine },
            { "cosine_power"      , CubemapFilterType::cosine_power }
        };

        // To avoid abusing '#define', we use lambda instead.
        auto INI_VALUE = [&rcINI, &settingKey]() { return rcINI.value(settingKey); };
        auto INI_VALUE_QSTRING = [&rcINI, &settingKey]() { return rcINI.value(settingKey).toString(); };

        /************************************************************************/
        /* GENERAL PRESET SETTINGS                                              */
        /************************************************************************/
        if (key == "rgbweights")
        {
            auto rgbWeightStr = INI_VALUE_QSTRING().toUtf8();
            auto rgbWeightIter = rgbWeightMap.find(rgbWeightStr);
            if (rgbWeightIter != rgbWeightMap.end())
            {
                presetSettings.m_rgbWeight = rgbWeightIter->second;
            }
            else
            {
                return AZ::Failure(AZStd::string("Unmapped rgbweights enum detected."));
            }
        }
        else if (key == "powof2")
        {
            presetSettings.m_isPowerOf2 = INI_VALUE().toBool();
        }
        else if (key == "discardalpha")
        {
            presetSettings.m_discardAlpha = INI_VALUE().toBool();
        }
        else if (key == "reduce")
        {
            int reduce = INI_VALUE().toInt();
            if (reduce > 0)
            {
                presetSettings.m_sizeReduceLevel = reduce;
            }
        }
        else if (key == "ser")
        {
            presetSettings.m_suppressEngineReduce = INI_VALUE().toBool();
        }
        else if (key == "colorchart")
        {
            presetSettings.m_isColorChart = INI_VALUE().toBool();
        }
        else if (key == "highpass")
        {
            presetSettings.m_highPassMip = INI_VALUE().toInt();
        }
        else if (key == "glossfromnormals")
        {
            presetSettings.m_glossFromNormals = INI_VALUE().toBool();
        }
        else if (key == "glosslegacydist")
        {
            presetSettings.m_isLegacyGloss = INI_VALUE().toBool();
        }
        else if (key == "swizzle")
        {
            presetSettings.m_swizzle = INI_VALUE_QSTRING().toUtf8().constData();
        }
        else if (key == "mipnormalize")
        {
            presetSettings.m_isMipRenormalize = INI_VALUE().toBool();
        }
        else if (key == "numstreamablemips")
        {
            presetSettings.m_numStreamableMips = INI_VALUE().toInt();
        }
        else if (key == "colorspace")
        {
            // By default, RC.ini contains data written in non-standard INI format inherited from CryEngine.
            // We need to parse in the value as string list.
            // Example:
            //
            //      [MyValues]
            //      colorspace=src,dst
            //
            QVariant paramValue = rcINI.value(key, QString());
            if (paramValue.type() != QVariant::StringList)
            {
                return AZ::Failure(AZStd::string("Expect ColorSpace parameter to be a string list!"));
            }
            QStringList stringValueList = paramValue.toStringList(); // The order of values for this key is... (SRC, DST)

            if (stringValueList.size() != 2)
            {
                return AZ::Failure(AZStd::string("Expect ColorSpace parameter list size to be 2!"));
            }

            auto srcColorSpaceIter = colorSpaceMap.find(stringValueList[0]);
            if (srcColorSpaceIter != colorSpaceMap.end())
            {
                presetSettings.m_srcColorSpace = srcColorSpaceIter->second;
            }
            else
            {
                return AZ::Failure(AZStd::string("Unmapped ColorSpace enum detected."));
            }

            auto dstColorSpaceIter = colorSpaceMap.find(stringValueList[1]);
            if (dstColorSpaceIter != colorSpaceMap.end())
            {
                presetSettings.m_destColorSpace = dstColorSpaceIter->second;
            }
            else
            {
                return AZ::Failure(AZStd::string("Unmapped ColorSpace enum detected."));
            }
        }
        else if (key == "filemasks")
        {
            QVariant iniVariant = rcINI.value(settingKey);
            QStringList stringValueList = iniVariant.toStringList();
            for (QString value : stringValueList)
            {
                //remove stars. For example: "*_ddna*" => "_ddna"
                QString suffix = value.mid(1, value.length()-2);
                QByteArray suffixByteArray = suffix.toUtf8();
                presetSettings.m_fileMasks.emplace_back(suffixByteArray.constData());
            }
        }
        else if (key == "pixelformat")
        {
            auto pixelFormatQBytes = INI_VALUE_QSTRING().toUtf8();
            const char* pixelFormatString = pixelFormatQBytes.constData();

            EPixelFormat pixelFormatEnum = CPixelFormats::GetInstance().FindPixelFormatByLegacyName(pixelFormatString);
            if (pixelFormatEnum == EPixelFormat::ePixelFormat_Unknown)
            {
                return AZ::Failure(AZStd::string::format("Unsupported ePixelFormat detected: %s", pixelFormatString));
            }

            presetSettings.m_pixelFormat = pixelFormatEnum;
            presetSettings.m_pixelFormatName = CPixelFormats::GetInstance().GetPixelFormatInfo(pixelFormatEnum)->szName;
        }
        else if (key == "pixelformatalpha")
        {
            auto pixelFormatQBytes = INI_VALUE_QSTRING().toUtf8();
            const char* pixelFormatString = pixelFormatQBytes.constData();

            EPixelFormat pixelFormatEnum = CPixelFormats::GetInstance().FindPixelFormatByLegacyName(pixelFormatString);
            if (pixelFormatEnum == EPixelFormat::ePixelFormat_Unknown)
            {
                return AZ::Failure(AZStd::string::format("Unsupported ePixelFormat detected: %s", pixelFormatString));
            }

            presetSettings.m_pixelFormatAlpha = pixelFormatEnum;
            presetSettings.m_pixelFormatAlphaName = CPixelFormats::GetInstance().GetPixelFormatInfo(pixelFormatEnum)->szName;
        }
        else if (key == "maxtexturesize")
        {
            bool isOk = false;
            auto maxTextureSize = INI_VALUE().toUInt(&isOk);
            if (isOk)
            {
                presetSettings.m_maxTextureSize = maxTextureSize;
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Invalid number for key 'maxtexturesize' for [%s]", presetSettings.m_name.c_str()));
            }
        }
        else if (key == "mintexturesize")
        {
            bool isOk = false;
            auto minTextureSize = INI_VALUE().toUInt(&isOk);
            if (isOk)
            {
                presetSettings.m_minTextureSize = minTextureSize;
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Invalid number for key 'mintexturesize' for [%s]", presetSettings.m_name.c_str()));
            }
        }
        /************************************************************************/
        /* CUBEMAP PRESET SETTINGS                                              */
        /************************************************************************/
        else if (key == "cm")
        {
            if (presetSettings.m_cubemapSetting == nullptr && INI_VALUE().toBool())
            {
                presetSettings.m_cubemapSetting = AZStd::make_unique<CubemapSettings>();
            }
            else
            {
                return AZ::Failure(AZStd::string("Multiple CubeMap settings detected. Reduce to a single settings entry."));
            }
        }
        else if (key == "cm_ftype")
        {
            if (presetSettings.m_cubemapSetting == nullptr)
            {
                if (rcINI.value("cm").toBool())
                {
                    presetSettings.m_cubemapSetting = AZStd::make_unique<CubemapSettings>();
                }
            }

            if (presetSettings.m_cubemapSetting)
            {
                auto filterTypeStr = INI_VALUE_QSTRING().toUtf8();
                auto filterTypeIter = cubemapFilterTypeMap.find(filterTypeStr);
                if (filterTypeIter != cubemapFilterTypeMap.end())
                {
                    presetSettings.m_cubemapSetting->m_filter = filterTypeIter->second;
                }
                else
                {
                    return AZ::Failure(AZStd::string("Unmapped cubemap filter type enum detected."));
                }
            }
        }
        else if (key == "cm_fangle")
        {
            if (presetSettings.m_cubemapSetting == nullptr)
            {
                if (rcINI.value("cm").toBool())
                {
                    presetSettings.m_cubemapSetting = AZStd::make_unique<CubemapSettings>();
                    presetSettings.m_cubemapSetting->m_angle = INI_VALUE().toFloat();
                }
            }
            else
            {
                presetSettings.m_cubemapSetting->m_angle = INI_VALUE().toFloat();
            }
        }
        else if (key == "cm_fmipangle")
        {
            if (presetSettings.m_cubemapSetting == nullptr)
            {
                if (rcINI.value("cm").toBool())
                {
                    presetSettings.m_cubemapSetting = AZStd::make_unique<CubemapSettings>();
                    presetSettings.m_cubemapSetting->m_mipAngle = INI_VALUE().toFloat();
                }
            }
            else
            {
                presetSettings.m_cubemapSetting->m_mipAngle = INI_VALUE().toFloat();
            }
        }
        else if (key == "cm_fmipslope")
        {
            if (presetSettings.m_cubemapSetting == nullptr)
            {
                if (rcINI.value("cm").toBool())
                {
                    presetSettings.m_cubemapSetting = AZStd::make_unique<CubemapSettings>();
                    presetSettings.m_cubemapSetting->m_mipSlope = INI_VALUE().toFloat();
                }
            }
            else
            {
                presetSettings.m_cubemapSetting->m_mipSlope = INI_VALUE().toFloat();
            }
        }
        else if (key == "cm_edgefixup")
        {
            if (presetSettings.m_cubemapSetting == nullptr)
            {
                if (rcINI.value("cm").toBool())
                {
                    presetSettings.m_cubemapSetting = AZStd::make_unique<CubemapSettings>();
                    presetSettings.m_cubemapSetting->m_edgeFixup = INI_VALUE().toFloat();
                }
            }
            else
            {
                presetSettings.m_cubemapSetting->m_edgeFixup = INI_VALUE().toFloat();
            }
        }
        else if (key == "cm_diff")
        {
            if (presetSettings.m_cubemapSetting == nullptr)
            {
                if (rcINI.value("cm").toBool())
                {
                    presetSettings.m_cubemapSetting = AZStd::make_unique<CubemapSettings>();
                    presetSettings.m_cubemapSetting->m_generateDiff = INI_VALUE().toBool();
                }
            }
            else
            {
                presetSettings.m_cubemapSetting->m_generateDiff = INI_VALUE().toBool();
            }
        }
        else if (key == "cm_diffpreset")
        {
            QByteArray presetNameByteArray = INI_VALUE().toString().toUtf8();
            AZ::Uuid presetID = BuilderSettingManager::Instance()->GetPresetIdFromName(presetNameByteArray.constData());
            if (presetID.IsNull())
            {
                return STRING_OUTCOME_ERROR(AZStd::string::format("Parsing error [cm_diffpreset]. Unable to find UUID for preset: %s", presetNameByteArray.constData()));
            }            
            
            if (presetSettings.m_cubemapSetting == nullptr)
            {
                if (rcINI.value("cm").toBool())
                {
                    presetSettings.m_cubemapSetting = AZStd::make_unique<CubemapSettings>();
                    presetSettings.m_cubemapSetting->m_diffuseGenPreset = presetID;
                }
            }
            else
            {
                presetSettings.m_cubemapSetting->m_diffuseGenPreset = presetID;
            }
        }
        /************************************************************************/
        /* MIPMAP PRESET SETTINGS                                               */
        /************************************************************************/
        else if (key == "mipmaps")
        {
            // We convey whether 'mipmaps' is enabled/available by whether the pointer is valid or empty.
            if (presetSettings.m_mipmapSetting == nullptr && INI_VALUE().toBool())
            {
                presetSettings.m_mipmapSetting = AZStd::make_unique<MipmapSettings>();
            }
        }
        else if (key == "mipgentype")
        {
            // We must handle parsing settings of missing/disabled parent settings ("mipmaps")
            if (rcINI.value("mipmaps") == QVariant())
            {
                return AZ::Failure(AZStd::string("'mipgentype' specified, but dependent 'mipmaps' setting is missing in rc.ini."));
            }

            // If we are missing the mipmap settings (possibly yet to be parsed)...
            if (presetSettings.m_mipmapSetting == nullptr)
            {
                if (rcINI.value("mipmaps").toBool())
                {
                    presetSettings.m_mipmapSetting = AZStd::make_unique<MipmapSettings>();
                }
                else
                {
                    return AZ::Failure(AZStd::string::format(
                        "Cannot assign 'mipgentype' because current Preset [%s] has 'mipmaps' disabled.", presetSettings.m_name.c_str()));
                }
            }

            presetSettings.m_mipmapSetting->m_type = MipGenType::blackmanHarris;
            if ("average" == INI_VALUE_QSTRING().toUtf8())
            {
                presetSettings.m_mipmapSetting->m_type = MipGenType::box;
            }
        }
        else
        {
            QByteArray keyByteArray = key.toUtf8();
            return STRING_OUTCOME_WARNING(AZStd::string::format("Unsupported key parsed from RC.ini: %s", keyByteArray.constData()));
        }

        return STRING_OUTCOME_SUCCESS;
    }

    void LoadPresetAliasFromRC(QSettings& rcINI, AZStd::map <PresetName, PresetName>& presetAliases)
    {
        rcINI.beginGroup("_presetAliases");

        for (QString legacyPresetString : rcINI.childKeys())
        {
            QString modernPresetString = rcINI.value(legacyPresetString).toString();

            // We must store this intermediary data in order to convert to a c-string.
            QByteArray legacyPresetUtf8 = legacyPresetString.toUtf8();
            QByteArray modernPresetUtf8 = modernPresetString.toUtf8();

            PresetName legacyPresetName(legacyPresetUtf8.constData());
            PresetName modernPresetName(modernPresetUtf8.constData());

            presetAliases.insert(AZStd::pair<PresetName, PresetName>(legacyPresetName, modernPresetName));
        }

        rcINI.endGroup();
    }

    void BuilderSettingManager::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<BuilderSettingManager>()
                ->Version(1)
                ->Field("BuildSettings", &BuilderSettingManager::m_builderSettings)
                ->Field("PresetAliases", &BuilderSettingManager::m_presetAliases)
                ->Field("DefaultPresetsByFileMask", &BuilderSettingManager::m_defaultPresetByFileMask)
                ->Field("DefaultPreset", &BuilderSettingManager::m_defaultPreset)
                ->Field("DefaultPresetAlpha", &BuilderSettingManager::m_defaultPresetAlpha)
                ->Field("DefaultPresetNonePOT", &BuilderSettingManager::m_defaultPresetNonePOT);
        }
    }

    BuilderSettingManager* BuilderSettingManager::Instance()
    {
        AZStd::lock_guard<AZStd::mutex> lock(s_instanceMutex);

        if (!s_globalInstance)
        {
            s_globalInstance = AZ::Environment::FindVariable<BuilderSettingManager*>(s_environmentVariableName);
        }
        AZ_Assert(s_globalInstance, "BuilderSettingManager not created!");

        return s_globalInstance.Get();
    }

    void BuilderSettingManager::CreateInstance()
    {
        AZStd::lock_guard<AZStd::mutex> lock(s_instanceMutex);

        if (s_globalInstance)
        {
            AZ_Assert(false, "BuilderSettingManager already created!");
            return;
        }
        
        if (!s_globalInstance)
        {
            s_globalInstance = AZ::Environment::CreateVariable<BuilderSettingManager*>(s_environmentVariableName);
        }
        if (!s_globalInstance.Get())
        {
            s_globalInstance.Set(aznew BuilderSettingManager());
        }
    }

    void BuilderSettingManager::DestroyInstance()
    {
        AZStd::lock_guard<AZStd::mutex> lock(s_instanceMutex);
        AZ_Assert(s_globalInstance, "Invalid call to DestroyInstance - no instance exists.");
        AZ_Assert(s_globalInstance.Get(), "You can only call DestroyInstance if you have called CreateInstance.");

        delete s_globalInstance.Get();
        s_globalInstance.Reset();
    }
    
    const PresetSettings* BuilderSettingManager::GetPreset(const AZ::Uuid presetId, const PlatformName& platform)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_presetMapLock);
        PlatformName platformName = platform;
        if (platformName.empty())
        {
            platformName = BuilderSettingManager::s_defaultPlatform;
        }
        
        if (m_builderSettings.find(platformName) != m_builderSettings.end())
        {
            const BuilderSettings& platformBuilderSetting = m_builderSettings[platformName];
            auto settingsIter = platformBuilderSetting.m_presets.find(presetId);
            if (settingsIter != platformBuilderSetting.m_presets.end())
            {
                return &settingsIter->second;
            }
            else
            {
                AZ_Error("Image Processing", false, "Cannot find preset settings on platform [%s] for preset id: %s", platformName.c_str(), presetId.ToString<AZStd::string>().c_str());
            }
        }
        else
        {
            AZ_Error("Image Processing", false, "Cannot find platform [%s]", platformName.c_str());
        }

        return nullptr;
    }


    const BuilderSettings* BuilderSettingManager::GetBuilderSetting(const PlatformName& platform)
    {
        if (m_builderSettings.find(platform) != m_builderSettings.end())
        {
            return &m_builderSettings[platform];
        }
        return nullptr;
    }

    const PlatformNameList BuilderSettingManager::GetPlatformList()
    {
        PlatformNameList platforms;

        for(auto& builderSetting : m_builderSettings)
        {
            if (builderSetting.second.m_enablePlatform)
            {
                platforms.push_back(builderSetting.first);
            }
        }

        return platforms;
    }

    const AZStd::map <FileMask, AZStd::set<PresetName>>& BuilderSettingManager::GetPresetFilterMap()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_presetMapLock);
        return m_presetFilterMap;
    }

    const AZ::Uuid BuilderSettingManager::GetPresetIdFromName(const PresetName& presetName)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_presetMapLock);

        // Each preset shares the same UUID across platforms, therefore, it's safe to pick a random
        // platform to search for the preset UUID. We'll use PC, in this case.
        const PlatformName defaultPlatform = BuilderSettingManager::s_defaultPlatform;
        if (m_builderSettings.find(defaultPlatform) != m_builderSettings.end())
        {
            auto presets = m_builderSettings[defaultPlatform].m_presets;

            for (auto curIter : presets)
            {
                if (curIter.second.m_name == presetName)
                {
                    return curIter.first;
                }
            }
        }

        return AZ::Uuid::CreateNull();
    }

    const PresetName BuilderSettingManager::GetPresetNameFromId(const AZ::Uuid& presetId)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_presetMapLock);
        const PlatformName defaultPlatform = BuilderSettingManager::s_defaultPlatform;
        if (m_builderSettings.find(defaultPlatform) != m_builderSettings.end())
        {
            auto& presetMap = m_builderSettings[defaultPlatform].m_presets;
            auto presetIter = presetMap.find(presetId);
            if (presetIter != presetMap.end())
            {
                return presetIter->second.m_name;
            }
        }

        return "Unknown";
    }

    void BuilderSettingManager::ClearSettings()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_presetMapLock);
        m_presetFilterMap.clear();
        m_presetAliases.clear();
        m_builderSettings.clear();
    }

    StringOutcome BuilderSettingManager::LoadBuilderSettings()
    {
        StringOutcome outcome = STRING_OUTCOME_ERROR("");
        AZ::IO::FileIOBase* ioInstance = AZ::IO::FileIOBase::GetInstance();
        // Construct the project setting path that is used by the tool for loading setting file
        const char* gameFolderPath = nullptr;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(gameFolderPath, &AzToolsFramework::AssetSystemRequestBus::Events::GetAbsoluteDevGameFolderPath);
        AZStd::string projectSettingPath = "";
        
        if (gameFolderPath)
        {   
            AzFramework::StringFunc::Path::Join(gameFolderPath, "Config/ImageBuilder/ImageBuilderPresets.settings", projectSettingPath);
            outcome = LoadBuilderSettings(projectSettingPath);
        }

        if (!outcome.IsSuccess())
        {
            AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Failed to read project specific preset setting at [%s], will use default setting file.\n", projectSettingPath.c_str());
            // Construct the default setting path
            const char* engineRoot = nullptr;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);

            if (engineRoot)
            {
                AZStd::string defaultSettingPath = "";
                AzFramework::StringFunc::Path::Join(engineRoot, "Gems/ImageProcessing/Code/Source/ImageBuilderDefaultPresets.settings", defaultSettingPath);
                outcome = LoadBuilderSettings(defaultSettingPath);
            }
        }

        return outcome;
    }

    StringOutcome BuilderSettingManager::LoadBuilderSettings(AZStd::string filepath, AZ::SerializeContext* context)
    {
        // Ensure filepath exists.
        AZ::IO::FileIOBase* fileReader = AZ::IO::FileIOBase::GetInstance();
        if(false == fileReader->Exists(filepath.c_str()))
        {
            return AZ::Failure(AZStd::string::format("Build settings file not found: %s", filepath.c_str()));
        }

        AZ::IO::HandleType settingsFileHandle;
        m_builderSettingsFileVersion = 0;
        if (fileReader->Open(filepath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, settingsFileHandle) == AZ::IO::ResultCode::Success)
        {
            // Read the contents of the file and hash it. The first u32 of the result of this will be used as a version 
            // number. Any changes to the builder settings file will then cause all textures to be reconverted to pick
            // up that change.
            AZStd::string settingsFileBuffer;
            AZ::u64 settingsFileSize = 0;
            fileReader->Size(settingsFileHandle, settingsFileSize);
            settingsFileBuffer.resize_no_construct(settingsFileSize);
            fileReader->Read(settingsFileHandle, settingsFileBuffer.data(), settingsFileSize);
            fileReader->Close(settingsFileHandle);

            AZ::Sha1 block;
            AZ::u32 hashDigest[5];
            block.ProcessBytes(settingsFileBuffer.data(), settingsFileSize);
            block.GetDigest(hashDigest);
            m_builderSettingsFileVersion = hashDigest[0];
        }

        auto loadedSettingsPtr = AZStd::unique_ptr<BuilderSettingManager>(AZ::Utils::LoadObjectFromFile<BuilderSettingManager>(filepath, context));

        // Ensure file is loaded.
        if (!loadedSettingsPtr)
        {
            return AZ::Failure(AZStd::string::format("Failed to read from file: %s", filepath.c_str()));
        }

        m_presetMapLock.lock();
        // Normally, we would perform a deep-copy from the loaded settings onto 'this' via assignment operator overload. However, we have deleted our 
        // assignment operator overloads, because we intend for this class to be a singleton. Instead, we will manually perform a deep-copy 
        // in this function since it's the only time we require something akin to assignment operation.
        m_builderSettings = loadedSettingsPtr->m_builderSettings;
        m_presetAliases = loadedSettingsPtr->m_presetAliases;
        m_defaultPresetByFileMask = loadedSettingsPtr->m_defaultPresetByFileMask;
        m_defaultPreset = loadedSettingsPtr->m_defaultPreset;
        m_defaultPresetAlpha = loadedSettingsPtr->m_defaultPresetAlpha;
        m_defaultPresetNonePOT = loadedSettingsPtr->m_defaultPresetNonePOT;

        m_presetFilterMap.clear();

        //enable builder settings for enabled restricted platforms. These settings should be disabled by default in the setting file
#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
        for (auto& buildSetting : m_builderSettings)\
        {\
            if (ImageProcess##PrivateName::DoesSupport(buildSetting.first))\
            {\
                buildSetting.second.m_enablePlatform = true;\
                break;\
            }\
        }
        AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif //AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS

        //convert pixel format string to enum for each preset
        for (auto& buildSetting : m_builderSettings)
        {
            for (auto& preset : buildSetting.second.m_presets)
            {
                preset.second.m_pixelFormat = CPixelFormats::GetInstance().FindPixelFormatByName(preset.second.m_pixelFormatName.c_str());
                preset.second.m_pixelFormatAlpha = CPixelFormats::GetInstance().FindPixelFormatByName(preset.second.m_pixelFormatAlphaName.c_str());
            }
        }
        m_presetMapLock.unlock();

        RegenerateMappings();

        return AZ::Success(AZStd::string());
    }

    StringOutcome BuilderSettingManager::WriteBuilderSettings(AZStd::string filepath, AZ::SerializeContext* context)
    {
        if( false == AZ::Utils::SaveObjectToFile<BuilderSettingManager>(filepath, AZ::DataStream::StreamType::ST_XML, this, context))
        {
            return STRING_OUTCOME_ERROR(AZStd::string::format("Failed to write file: %s", filepath.c_str()));
        }
        return STRING_OUTCOME_SUCCESS;
    }

    const PresetName BuilderSettingManager::TranslateLegacyPresetName(const PresetName& legacyName)
    {
        auto iterResult = m_presetAliases.find(legacyName);

        if (iterResult == m_presetAliases.end())
        {
            return legacyName;
        }

        return iterResult->second;
    }

    StringOutcome BuilderSettingManager::LoadBuilderSettingsFromRC(AZStd::string& filePath)
    {
        //Clear previous settings first
        ClearSettings();

        // Find all the platforms
        auto outcome = GetPlatformNamesFromRC(filePath);
        AZ_ENSURE_STRING_OUTCOME(outcome);
        PlatformNameVector all_platforms = outcome.TakeValue();

        m_presetMapLock.lock();
        // Register all the platforms with empty settings.
        for (const AZStd::string& platformName : all_platforms)
        {
            auto newEntry = AZStd::pair<AZStd::string, BuilderSettings>(platformName, BuilderSettings());
            m_builderSettings.insert(newEntry);
        }

        // Open settings for parsing
        QSettings set(filePath.c_str(), QSettings::IniFormat);

        // Load the preset alias mapping
        LoadPresetAliasFromRC(set, m_presetAliases);

        QStringList childGroups = set.childGroups();
        QStringList exemptGroups;
        exemptGroups.append("_platform");
        exemptGroups.append("_presetAliases");
        
        // We must generate new preset settings and UUID's prior to parsing the rest of the data, because
        // some preset settings make references to other presets within RC.ini. We must make sure all presets
        // are identified, before making references to them via their UUID.
        for (QString groupName : childGroups)
        {
            auto newPresetUuid = AZ::Uuid::CreateRandom();
            PresetSettings newPresetSetting;
            newPresetSetting.m_name = groupName.toUtf8().constData();
            newPresetSetting.m_uuid = newPresetUuid;
            for (const PlatformName& platform : all_platforms)
            {
                m_builderSettings[platform].m_presets.insert(AZStd::make_pair(newPresetUuid, newPresetSetting));
            }
        }
        m_presetMapLock.unlock();

        // Apply preset settings from file to the existing presets (process each platform).
        for(QString groupName : childGroups)
        {
            // Only process Presets from here on out.
            if (exemptGroups.contains(groupName))
            {
                continue;
            }

            auto outcome2 = ProcessPreset(groupName, set, all_platforms);
            AZ_ENSURE_STRING_OUTCOME(outcome2);
        }

        RegenerateMappings();
        
        // The original rc.ini doesn't have the information below. Included here for GetSuggestedPreset() to work properly.
        m_defaultPresetByFileMask["_diff"] = GetPresetIdFromName("Albedo");
        m_defaultPresetByFileMask["_spec"] = GetPresetIdFromName("Reflectance");
        m_defaultPresetByFileMask["_refl"] = GetPresetIdFromName("Reflectance");
        m_defaultPresetByFileMask["_ddn"] = GetPresetIdFromName("Normals");
        m_defaultPresetByFileMask["_ddna"] = GetPresetIdFromName("NormalsWithSmoothness");
        m_defaultPresetByFileMask["_cch"] = GetPresetIdFromName("ColorChart");
        m_defaultPresetByFileMask["_cm"] = GetPresetIdFromName("EnvironmentProbeHDR");

        m_defaultPreset = GetPresetIdFromName("Albedo");
        m_defaultPresetAlpha = GetPresetIdFromName("AlbedoWithGenericAlpha");
        m_defaultPresetNonePOT = GetPresetIdFromName("ReferenceImage");

        return STRING_OUTCOME_SUCCESS;
    }

    AZ::u32 BuilderSettingManager::BuilderSettingsVersion() const
    {
        return m_builderSettingsFileVersion;
    }

    void BuilderSettingManager::RegenerateMappings()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_presetMapLock);

        AZStd::string noFilter = AZStd::string();

        m_presetFilterMap.clear();
     
        for (auto& builderSettingIter : m_builderSettings)
        {
            const BuilderSettings& builderSetting = builderSettingIter.second;
            for (auto& presetIter : builderSetting.m_presets)
            {
                //Put into no filter preset list
                m_presetFilterMap[noFilter].insert(presetIter.second.m_name);

                //Put into file mask preset list if any
                for (const PlatformName& filemask : presetIter.second.m_fileMasks)
                {
                    m_presetFilterMap[filemask].insert(presetIter.second.m_name);
                }
            }
        }
    }

    StringOutcome BuilderSettingManager::ProcessPreset(QString& preset, QSettings& rcINI, PlatformNameVector& all_platforms)
    {
        // Early-out check. We should have the preset available before processing.
        QByteArray presetByteArray = preset.toUtf8();
        AZ::Uuid parsingPresetUuid = GetPresetIdFromName(presetByteArray.constData());
        if (parsingPresetUuid.IsNull())
        {
            return STRING_OUTCOME_ERROR(AZStd::string::format("Unable to find UUID for preset: %s", presetByteArray.constData()));
        }

        rcINI.beginGroup(preset);
        QStringList groupKeys = rcINI.allKeys();

        // Build a list for common & platform-specific settings
        QStringList commonPresetSettingKeys;
        QStringList platformSpecificPresetSettingKeys;
        for (QString key : groupKeys)
        {
            if (key.contains(":"))
            {
                platformSpecificPresetSettingKeys.append(key);
            }
            else
            {
                commonPresetSettingKeys.append(key);
            }
        }

        // Parse the common settings (retain preset name & uuid)
        PresetSettings commonPresetSettings;
        commonPresetSettings.m_name = presetByteArray.constData();
        commonPresetSettings.m_uuid = parsingPresetUuid;
        for (QString settingKey : commonPresetSettingKeys)
        {
            AZ_ENSURE_STRING_OUTCOME(ParseKeyToData(settingKey, rcINI, commonPresetSettings));
        }

        // When loading a preset, the UUID is the same per-preset, regardless of the target-platform.
        for (AZStd::string& platformId : all_platforms)
        {
            // Begin platform-specific settings loading with a copy of common Preset settings.
            PresetSettings currentPlatformPresetSetting = commonPresetSettings;

            // Obtain platform-specific settings
            QString platformFilter = QString(":%1").arg(platformId.c_str()); // Example results-> ":ios", ":osx", ":es3"
            QStringList currentPlatformSettings = platformSpecificPresetSettingKeys.filter(platformFilter);

            // Overwrite values for platform-specific settings...
            for (QString platformSetting : currentPlatformSettings)
            {
                AZ_ENSURE_STRING_OUTCOME(ParseKeyToData(platformSetting, rcINI, currentPlatformPresetSetting));
            }

            // Assign the overridden platform preset settings to the BuilderSettingManager.
            m_builderSettings[platformId].m_presets[parsingPresetUuid] = currentPlatformPresetSetting;
        }

        rcINI.endGroup();
        return STRING_OUTCOME_SUCCESS;
    }

    void BuilderSettingManager::MetafilePathFromImagePath(const AZStd::string& imagePath, AZStd::string& metafilePath)
    {
        // Determine if we have a meta file (legacy or modern).
        AZ::IO::LocalFileIO fileIO;

        AZStd::string modernMetaFilepath = imagePath + TextureSettings::modernExtensionName;
        if (fileIO.Exists(modernMetaFilepath.c_str()))
        {
            metafilePath = modernMetaFilepath;
            return;
        }

        AZStd::string legacyMetaFilepath = imagePath + TextureSettings::legacyExtensionName;
        if (fileIO.Exists(legacyMetaFilepath.c_str()))
        {
            metafilePath = legacyMetaFilepath;
            return;
        }

        // We found neither.
        metafilePath = AZStd::string();
    }

    AZStd::string GetFileMask(const AZStd::string& imageFilePath)
    {
        //get file name 
        AZStd::string fileName;
        QString lowerFileName = imageFilePath.c_str();
        lowerFileName = lowerFileName.toLower();
        AzFramework::StringFunc::Path::GetFileName(lowerFileName.toUtf8().constData(), fileName);

        //get the substring from last '_'
        size_t lastUnderScore = fileName.find_last_of('_');
        if (lastUnderScore != AZStd::string::npos)
        {
            return fileName.substr(lastUnderScore);
        }

        return AZStd::string();
    }
    
    AZ::Uuid BuilderSettingManager::GetSuggestedPreset(const AZStd::string& imageFilePath, IImageObjectPtr imageFromFile)
    {
        //load the image to get its size for later use
        IImageObjectPtr image = imageFromFile;
        //if the input image is empty we will try to load it from the path
        if (imageFromFile == nullptr)
        {
            image = IImageObjectPtr(LoadImageFromFile(imageFilePath));
        }

        if (image == nullptr)
        {
            AZ_Error("Image Processing", image, "Cannot load image file [%s]. Invalid image format or corrupt data. Note that \"Indexed Color\" is not currently supported for .tga files.", imageFilePath.c_str());
            return AZ::Uuid::CreateNull();
        }

        //get file mask of this image file
        AZStd::string fileMask = GetFileMask(imageFilePath);

        AZ::Uuid outPreset = AZ::Uuid::CreateNull();

        if ("_diff" == fileMask && image->GetAlphaContent() != EAlphaContent::eAlphaContent_Absent)
        {
            outPreset = m_defaultPresetAlpha;
        }
        else
        {
            //check default presets for some file masks
            if (m_defaultPresetByFileMask.find(fileMask) != m_defaultPresetByFileMask.end())
            {
                outPreset = m_defaultPresetByFileMask[fileMask];
            }
        }

        //use the preset filter map to find 
        if (outPreset.IsNull() && !fileMask.empty())
        {
            auto& presetFilterMap = GetPresetFilterMap();
            if (presetFilterMap.find(fileMask) != presetFilterMap.end())
            {
                AZStd::string presetName = *(presetFilterMap.find(fileMask)->second.begin());
                outPreset = GetPresetIdFromName(presetName);
            }
        }

        const PresetSettings* presetInfo = nullptr;
        
        if (!outPreset.IsNull())
        {
            presetInfo = GetPreset(outPreset);

            //special case for cubemap
            if (presetInfo && presetInfo->m_cubemapSetting)
            {
                if (CubemapLayout::GetCubemapLayoutInfo(image) == nullptr)
                {
                    outPreset = AZ::Uuid::CreateNull();
                }
            }
        }
        
        if (outPreset.IsNull())
        {
            if (!image->HasPowerOfTwoSizes())
            {
                // The resource compiler used the non power of 2 preset if the width or the height is not power of 2
                // even if it would have been possible to compress the image, so this behavior is matched here.
                return m_defaultPresetNonePOT;
            }
            else if (image->GetAlphaContent() == EAlphaContent::eAlphaContent_Absent)
            {
                outPreset = m_defaultPreset;
            }
            else
            {
                outPreset = m_defaultPresetAlpha;
            }
        }

        //get the pixel format for selected preset
        presetInfo = GetPreset(outPreset);

        if (presetInfo)
        {
            //valid whether image size work with pixel format
            if (CPixelFormats::GetInstance().IsImageSizeValid(presetInfo->m_pixelFormat,
                image->GetWidth(0), image->GetHeight(0), false))
            {
                return outPreset;
            }
        }

        //uncompressed one which could be used for almost everything
        return m_defaultPresetNonePOT;
    }

    bool BuilderSettingManager::DoesSupportPlatform(const AZStd::string& platformId)
    {
        bool rv = m_builderSettings.find(platformId) != m_builderSettings.end();
        return rv;
    }

} // namespace ImageProcessing
