// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#pragma once

/*
 * AssetCommon.h provides AZ::Data::AssetData, the base class for all O3DE data assets.
 * Deriving from AssetData makes this class loadable by the Asset Manager and referenceable
 * via AZ::Data::Asset<T> handles elsewhere in code.
 *
 * RTTI.h provides AZ_RTTI, which is required on any AssetData subclass so that the
 * serialization system and asset pipeline can identify the type at runtime. Without
 * AZ_RTTI the Asset Manager cannot cast asset pointers to the correct type.
 *
 * SerializeContext.h and EditContext.h are needed by Reflect() to register this asset's
 * fields for serialization and for display in the Asset Editor window.
 */
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace ${GemName}
{
    /*
     * ${SanitizedCppName}Asset - A data asset type for ${GemName}.
     *
     * This class represents a loadable asset managed by the O3DE Asset Manager.
     * Instances are created by the asset pipeline when it processes asset source files
     * with the file extension registered in ${GemName}DataAssetSystemComponent.
     *
     * Inheritance chain:
     *   ${SanitizedCppName}Asset -> AZ::Data::AssetData
     *
     * AZ_RTTI is mandatory on all AssetData subclasses. It enables:
     *   - Runtime type identification and safe downcasting via azrtti_cast<>
     *   - Serialization system type lookup by UUID
     *   - Asset Manager type routing
     * The UUID must be unique across the entire project.
     *
     * AZ_CLASS_ALLOCATOR declares which memory allocator to use for instances of this class.
     * AZ::SystemAllocator is the general-purpose heap allocator and is appropriate for most assets.
     *
     * To reference this asset type from a component, use:
     *   AZ::Data::Asset<${SanitizedCppName}Asset> m_myAsset;
     * and reflect it with SerializeContext so it appears as an asset picker in the Editor.
     *
     * IMPORTANT: This asset type must be registered with a GenericAssetHandler in
     * ${GemName}DataAssetSystemComponent::Activate() before the Asset Manager will
     * recognize and load files of this type.
     */
    class ${SanitizedCppName}Asset
        : public AZ::Data::AssetData
    {
    public:
        /*
         * AZ_RTTI declares runtime type information for this class.
         * The string name and UUID uniquely identify this asset type in the engine.
         * The second base-class argument chains RTTI through AZ::Data::AssetData.
         */
        AZ_RTTI(${SanitizedCppName}Asset, "{${Random_Uuid}}", AZ::Data::AssetData);

        /*
         * AZ_CLASS_ALLOCATOR ties this class to a specific memory allocator.
         * AZ::SystemAllocator is the standard choice. Always declare this on AssetData
         * subclasses to ensure the Asset Manager can allocate and free instances correctly.
         */
        AZ_CLASS_ALLOCATOR(${SanitizedCppName}Asset, AZ::SystemAllocator);

        /*
         * Reflect registers this asset type with the serialization and edit contexts.
         * It must be called from ${GemName}DataAssetSystemComponent::Reflect() so that
         * the engine knows the layout of this asset's data before loading any files.
         *
         * At minimum, reflect at least one member variable. Assets with no reflected fields
         * will not appear in the Asset Editor and cannot be saved with meaningful data.
         */
        static void Reflect(AZ::ReflectContext* context);

        /*
         * Replace this placeholder with the actual data fields for your asset.
         * Every field you want editable in the Asset Editor or loadable from file
         * must be reflected in Reflect() using ->Field("fieldName", &Class::member).
         *
         * Example fields:
         *   float m_speed = 1.0f;
         *   AZStd::string m_displayName;
         *   AZStd::vector<int> m_valueTable;
         */
        bool m_singleVariable = true;
    };
} // namespace ${GemName}
