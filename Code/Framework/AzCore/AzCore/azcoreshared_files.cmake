#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Asset/AssetInternal/WeakAsset.h
    Asset/AssetCommon.cpp
    Asset/AssetCommon.h
    Asset/AssetContainer.cpp
    Asset/AssetContainer.h
    Asset/AssetDataStream.cpp
    Asset/AssetDataStream.h
    Asset/AssetJsonSerializer.cpp
    Asset/AssetJsonSerializer.h
    Asset/AssetManager_private.h
    Asset/AssetManager.cpp
    Asset/AssetManager.h
    Asset/AssetManagerBus.cpp
    Asset/AssetManagerBus.h
    Asset/AssetManagerComponent.cpp
    Asset/AssetManagerComponent.h
    Asset/AssetSerializer.cpp
    Asset/AssetSerializer.h
    Asset/AssetTypeInfoBus.cpp
    Asset/AssetTypeInfoBus.h

    Module/EBusInstantiations.cpp
    Module/AZStdInstantiations.cpp

    Settings/CommandLine.cpp
    Settings/CommandLine.h
    Settings/CommandLineParser.cpp
    Settings/CommandLineParser.h
    Settings/ConfigParser.cpp
    Settings/ConfigParser.h
    Settings/ConfigurableStack.cpp
    Settings/ConfigurableStack.inl
    Settings/ConfigurableStack.h
    Settings/SettingsRegistry.cpp
    Settings/SettingsRegistry.h
    Settings/SettingsRegistryConsoleUtils.cpp
    Settings/SettingsRegistryConsoleUtils.h
    Settings/SettingsRegistryImpl.cpp
    Settings/SettingsRegistryImpl.h
    Settings/SettingsRegistryMergeUtils.cpp
    Settings/SettingsRegistryMergeUtils.h
    Settings/SettingsRegistryOriginTracker.cpp
    Settings/SettingsRegistryOriginTracker.h
    Settings/SettingsRegistryScriptUtils.cpp
    Settings/SettingsRegistryScriptUtils.h
    Settings/SettingsRegistryVisitorUtils.cpp
    Settings/SettingsRegistryVisitorUtils.h
    Settings/TextParser.cpp
    Settings/TextParser.h
)
