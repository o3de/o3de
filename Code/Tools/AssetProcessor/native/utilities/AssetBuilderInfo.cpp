/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/utilities/AssetBuilderInfo.h>


#include <AzCore/Component/Entity.h>


namespace AssetProcessor
{
#ifdef AZ_PLATFORM_WINDOWS
    const char* const s_assetBuilderRelativePath = "AssetBuilder.exe";
#else
    const char* const s_assetBuilderRelativePath = "AssetBuilder";
#endif

    ExternalModuleAssetBuilderInfo::ExternalModuleAssetBuilderInfo(const QString& modulePath)
        : m_builderName(modulePath)
        , m_entity(nullptr)
        , m_componentDescriptorList()
        , m_initializeModuleFunction(nullptr)
        , m_moduleRegisterDescriptorsFunction(nullptr)
        , m_moduleAddComponentsFunction(nullptr)
        , m_uninitializeModuleFunction(nullptr)
        , m_library(modulePath)
    {
    }

    const QString& ExternalModuleAssetBuilderInfo::GetName() const
    {
        return m_builderName;
    }

    QString ExternalModuleAssetBuilderInfo::GetModuleFullPath() const
    {
        return m_library.fileName();
    }

    //! Sanity check for the module's status
    bool ExternalModuleAssetBuilderInfo::IsLoaded() const
    {
        return m_library.isLoaded();
    }

    void ExternalModuleAssetBuilderInfo::Initialize()
    {
        AZ_Error(AssetProcessor::ConsoleChannel, IsLoaded(), "External module %s not loaded.", GetName().toUtf8().data());

        if (GetAssetBuilderType() == AssetBuilderType::Valid)
        {
            m_initializeModuleFunction(AZ::Environment::GetInstance());
            m_moduleRegisterDescriptorsFunction();

            AZStd::string entityName = AZStd::string::format("%s Entity", GetName().toUtf8().data());
            m_entity = aznew AZ::Entity(entityName.c_str());

            m_moduleAddComponentsFunction(m_entity);

            AZ_TracePrintf(AssetProcessor::DebugChannel, "Init Entity %s", GetName().toUtf8().data());
            m_entity->Init();

            //Activate all the components
            m_entity->Activate();
        }
    }


    void ExternalModuleAssetBuilderInfo::UnInitialize()
    {
        AZ_Error(AssetProcessor::ConsoleChannel, IsLoaded(), "External module %s not loaded.", GetName().toUtf8().data());

        AZ_TracePrintf(AssetProcessor::DebugChannel, "Uninitializing builder: %s\n", GetModuleFullPath().toUtf8().data());

        if (m_entity)
        {
            m_entity->Deactivate();
            delete m_entity;
            m_entity = nullptr;
        }

        for (AZ::ComponentDescriptor* componentDesc : m_componentDescriptorList)
        {
            componentDesc->ReleaseDescriptor();
        }
        m_componentDescriptorList.clear();

        for (const AZ::Uuid& builderDescID : m_registeredBuilderDescriptorIDs)
        {
            AssetBuilderRegistrationBus::Broadcast(&AssetBuilderRegistrationBusTraits::UnRegisterBuilderDescriptor, builderDescID);
        }
        m_registeredBuilderDescriptorIDs.clear();

        m_uninitializeModuleFunction();

        if (IsLoaded())
        {
            m_library.unload();
        }
    }

    AssetBuilderType ExternalModuleAssetBuilderInfo::GetAssetBuilderType()
    {
        QStringList missingFunctionsList;
        ResolveModuleFunction<QFunctionPointer>("IsAssetBuilder", missingFunctionsList);
        InitializeModuleFunction initializeModuleAddress = ResolveModuleFunction<InitializeModuleFunction>("InitializeModule", missingFunctionsList);
        ModuleRegisterDescriptorsFunction moduleRegisterDescriptorsAddress = ResolveModuleFunction<ModuleRegisterDescriptorsFunction>("ModuleRegisterDescriptors", missingFunctionsList);
        ModuleAddComponentsFunction moduleAddComponentsAddress = ResolveModuleFunction<ModuleAddComponentsFunction>("ModuleAddComponents", missingFunctionsList);
        UninitializeModuleFunction uninitializeModuleAddress = ResolveModuleFunction<UninitializeModuleFunction>("UninitializeModule", missingFunctionsList);

        if (missingFunctionsList.size() == 0)
        {
            //if we are here then it is a builder
            m_initializeModuleFunction = initializeModuleAddress;
            m_moduleRegisterDescriptorsFunction = moduleRegisterDescriptorsAddress;
            m_moduleAddComponentsFunction = moduleAddComponentsAddress;
            m_uninitializeModuleFunction = uninitializeModuleAddress;
            return AssetBuilderType::Valid;
        }
        else if (missingFunctionsList.size() > 0 && missingFunctionsList.contains("IsAssetBuilder"))
        {
            // This DLL is not a builder and should be ignored.
            return AssetBuilderType::None;
        }
        else
        {
            // This is supposed to be a builder but is invalid
            QString errorMessage = QString("Builder library %1 is missing one or more exported functions: %2").arg(QString(GetName()), missingFunctionsList.join(','));
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "One or more builder functions is missing in the library: %s\n", errorMessage.toUtf8().data());
            return AssetBuilderType::Invalid;
        }
    }


    AssetBuilderType ExternalModuleAssetBuilderInfo::Load()
    {
        if (IsLoaded())
        {
            // This builder is already loaded - ignore the duplicate
            AZ_Warning(AssetProcessor::ConsoleChannel, false, "External module %s already loaded.", GetName().toUtf8().data());
            return AssetBuilderType::None;
        }

        if (!m_library.load())
        {
            //  Invalid builder - unable to load
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Unable to load builder : %s\n", GetName().toUtf8().data());
            return AssetBuilderType::Invalid;
        }

        return GetAssetBuilderType();
    }

    void ExternalModuleAssetBuilderInfo::RegisterBuilderDesc(const AZ::Uuid& builderDescID)
    {
        if (m_registeredBuilderDescriptorIDs.find(builderDescID) != m_registeredBuilderDescriptorIDs.end())
        {
            AZ_Warning(AssetBuilderSDK::InfoWindow,
                false,
                "Builder description id '%s' already registered to external builder module %s",
                builderDescID.ToString<AZStd::string>().c_str(),
                GetName().toUtf8().data());
            return;
        }
        m_registeredBuilderDescriptorIDs.insert(builderDescID);
    }

    void ExternalModuleAssetBuilderInfo::RegisterComponentDesc(AZ::ComponentDescriptor* descriptor)
    {
        m_componentDescriptorList.push_back(descriptor);
    }

    template<typename T>
    T ExternalModuleAssetBuilderInfo::ResolveModuleFunction(const char* functionName, QStringList& missingFunctionsList)
    {
        T functionAddr = reinterpret_cast<T>(m_library.resolve(functionName));
        if (!functionAddr)
        {
            missingFunctionsList.append(QString(functionName));
        }
        return functionAddr;
    }
}


