/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

#include <AssetBuilderInfo.h>
#include <AssetBuilderApplication.h>

namespace AssetBuilder
{
    ExternalModuleAssetBuilderInfo::ExternalModuleAssetBuilderInfo(const QString& modulePath)
        : m_builderName(modulePath)
        , m_entity(nullptr)
        , m_componentDescriptorList()
        , m_initializeModuleFunction(nullptr)
        , m_moduleRegisterDescriptorsFunction(nullptr)
        , m_moduleAddComponentsFunction(nullptr)
        , m_uninitializeModuleFunction(nullptr)
        , m_modulePath(modulePath)
        , m_library(modulePath)
    {
        Load();
    }

    ExternalModuleAssetBuilderInfo::~ExternalModuleAssetBuilderInfo()
    {
        Unload();
    }

    const QString& ExternalModuleAssetBuilderInfo::GetName() const
    {
        return m_builderName;
    }

    //! Sanity check for the module's status
    bool ExternalModuleAssetBuilderInfo::IsLoaded() const
    {
        return m_library.isLoaded();
    }

    void ExternalModuleAssetBuilderInfo::Initialize()
    {
        AZ_Error("AssetBuilder", IsLoaded(), "External module %s not loaded.", m_builderName.toUtf8().data());

        m_initializeModuleFunction(AZ::Environment::GetInstance());

        m_moduleRegisterDescriptorsFunction();

        AZStd::string entityName = AZStd::string::format("%s Entity", GetName().toUtf8().data());
        m_entity = aznew AZ::Entity(entityName.c_str());

        m_moduleAddComponentsFunction(m_entity);

        AZ_TracePrintf("AssetBuilder", "Init Entity %s\n", GetName().toUtf8().data());
        m_entity->Init();

        //Activate all the components
        m_entity->Activate();
    }


    void ExternalModuleAssetBuilderInfo::UnInitialize()
    {
        AZ_Error("AssetBuilder", IsLoaded(), "External module %s not loaded.", m_builderName.toUtf8().data());

        AZ_TracePrintf("AssetBuilder", "Uninitializing builder: %s\n", m_modulePath.toUtf8().data());

        if (m_entity)
        {
            m_entity->Deactivate();
            delete m_entity;
            m_entity = nullptr;
        }

        for (AZ::ComponentDescriptor* componentDesc : m_componentDescriptorList)
        {
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::UnregisterComponentDescriptor, componentDesc);
            componentDesc->ReleaseDescriptor(); // this kills the descriptor.
        }

        m_componentDescriptorList.clear();

        m_registeredBuilderDescriptorIDs.clear();

        m_uninitializeModuleFunction();
    }

    AssetBuilderType ExternalModuleAssetBuilderInfo::GetAssetBuilderType()
    {
        QStringList missingFunctionsList;
        ResolveModuleFunction<QFunctionPointer>("IsAssetBuilder", missingFunctionsList);
        InitializeModuleFunction initializeModuleAddress = ResolveModuleFunction<InitializeModuleFunction>("InitializeModule", missingFunctionsList);
        ModuleRegisterDescriptorsFunction moduleRegisterDescriptorsAddress = ResolveModuleFunction<ModuleRegisterDescriptorsFunction>("ModuleRegisterDescriptors", missingFunctionsList);
        ModuleAddComponentsFunction moduleAddComponentsAddress = ResolveModuleFunction<ModuleAddComponentsFunction>("ModuleAddComponents", missingFunctionsList);
        UninitializeModuleFunction uninitializeModuleAddress = ResolveModuleFunction<UninitializeModuleFunction>("UninitializeModule", missingFunctionsList);


        if (missingFunctionsList.empty())
        {
            // a valid builder
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
            AZ_Warning("AssetBuilder", false, "External module %s already loaded.", m_builderName.toUtf8().data());
            return AssetBuilderType::None;
        }

        m_library.setFileName(m_modulePath);
        if (!m_library.load())
        {
            AZ_TracePrintf("AssetBuilder", "Unable to load builder : %s\n", GetName().toUtf8().data());
            return AssetBuilderType::Invalid;
        }

        return GetAssetBuilderType();
    }

    void ExternalModuleAssetBuilderInfo::Unload()
    {
        if (IsLoaded())
        {
            m_library.unload();
        }

        m_initializeModuleFunction = nullptr;
        m_moduleRegisterDescriptorsFunction = nullptr;
        m_moduleAddComponentsFunction = nullptr;
        m_uninitializeModuleFunction = nullptr;
    }

    void ExternalModuleAssetBuilderInfo::RegisterBuilderDesc(const AZ::Uuid& builderDescID)
    {
        if (m_registeredBuilderDescriptorIDs.find(builderDescID) != m_registeredBuilderDescriptorIDs.end())
        {
            AZ_Warning(AssetBuilderSDK::InfoWindow,
                false,
                "Builder description id '%s' already registered to external builder module %s",
                builderDescID.ToString<AZStd::string>().c_str(),
                m_builderName.toUtf8().data());
            return;
        }
        m_registeredBuilderDescriptorIDs.insert(builderDescID);
    }

    void ExternalModuleAssetBuilderInfo::RegisterComponentDesc(AZ::ComponentDescriptor* descriptor)
    {
        m_componentDescriptorList.push_back(descriptor);
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::RegisterComponentDescriptor, descriptor);
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
