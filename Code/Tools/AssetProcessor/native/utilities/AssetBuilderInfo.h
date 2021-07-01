/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef UTILITIES_ASSETBUILDERINFO_H
#define UTILITIES_ASSETBUILDERINFO_H
#pragma once

#include <functional>
#include <QVector>
#include <QLibrary>

#include <AzCore/std/base.h>
#include <AzCore/std/containers/set.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <native/assetprocessor.h>
#include <native/utilities/PlatformConfiguration.h>
#include <native/resourcecompiler/RCBuilder.h>
#include <AssetBuilder/AssetBuilderInfo.h>

class FolderWatchCallbackEx;
class QCoreApplication;

namespace AssetProcessor
{
    using AssetBuilder::AssetBuilderType;
    enum ASSET_BUILDER_TYPE
    {
        INVALID, VALID, NONE
    };

    // A string like "AssetBuilder.exe" which names the executable of the asset builder.
    extern const char* const s_assetBuilderRelativePath;

    //! Class to manage external module builders for the asset processor
    class ExternalModuleAssetBuilderInfo
    {
    public:
        ExternalModuleAssetBuilderInfo(const QString& modulePath);
        virtual ~ExternalModuleAssetBuilderInfo() = default;

        const QString& GetName() const;
        QString GetModuleFullPath() const;


        //! Perform a load of the external module, this is required before initialize.
        AssetBuilderType Load();

        //! Sanity check for the module's status
        bool IsLoaded() const;

        //! Perform the module initialization for the external builder
        void Initialize();

        //! Perform the necessary process of uninitializing an external builder
        void UnInitialize();

        //! Check to see if the builder has the required functions defined.
        AssetBuilderType GetAssetBuilderType();
        ASSET_BUILDER_TYPE IsAssetBuilder();

        //! Register a builder descriptor ID to track as part of this builders lifecycle management
        void RegisterBuilderDesc(const AZ::Uuid& builderDesc);

        //! Register a component descriptor to track as part of this builders lifecycle management
        void RegisterComponentDesc(AZ::ComponentDescriptor* descriptor);
    protected:
        AZStd::set<AZ::Uuid>    m_registeredBuilderDescriptorIDs;

        typedef void(* InitializeModuleFunction)(AZ::EnvironmentInstance sharedEnvironment);
        typedef void(* ModuleRegisterDescriptorsFunction)(void);
        typedef void(* ModuleAddComponentsFunction)(AZ::Entity* entity);
        typedef void(* UninitializeModuleFunction)(void);

        template<typename T>
        T ResolveModuleFunction(const char* functionName, QStringList& missingFunctionsList);

        InitializeModuleFunction m_initializeModuleFunction;
        ModuleRegisterDescriptorsFunction m_moduleRegisterDescriptorsFunction;
        ModuleAddComponentsFunction m_moduleAddComponentsFunction;
        UninitializeModuleFunction m_uninitializeModuleFunction;
        AZStd::vector<AZ::ComponentDescriptor*> m_componentDescriptorList;
        AZ::Entity* m_entity = nullptr;

        QString m_builderName;
        QLibrary m_library;
    };


    //!This EBUS is used to send information from an internal builder to the AssetProcessor
    class AssetBuilderRegistrationBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        typedef AZStd::recursive_mutex MutexType;

        virtual ~AssetBuilderRegistrationBusTraits() {}

        virtual void UnRegisterBuilderDescriptor(const AZ::Uuid& /*builderId*/) {}
    };

    typedef AZ::EBus<AssetBuilderRegistrationBusTraits> AssetBuilderRegistrationBus;
} // AssetProcessor
#endif //UTILITIES_ASSETBUILDERINFO_H
