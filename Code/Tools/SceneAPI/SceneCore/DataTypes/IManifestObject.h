#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <AzCore/std/containers/vector.h>

#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace Containers
        {
            class SceneManifest;
        }
        namespace DataTypes
        {
            class SCENE_CORE_CLASS IManifestObject
            {
            public:
                SCENE_CORE_API virtual AZ::TypeId RTTI_GetType() const;
                SCENE_CORE_API virtual const char* RTTI_GetTypeName() const;
                SCENE_CORE_API virtual bool RTTI_IsTypeOf(const AZ::TypeId & typeId) const;
                SCENE_CORE_API virtual void RTTI_EnumTypes(AZ::RTTI_EnumCallback cb, void* userData);
                SCENE_CORE_API virtual const void* RTTI_AddressOf(const AZ::TypeId& id) const;
                SCENE_CORE_API virtual void* RTTI_AddressOf(const AZ::TypeId& id);
                SCENE_CORE_API static bool RTTI_IsContainType(const AZ::TypeId& id);
                SCENE_CORE_API static void RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData);
                SCENE_CORE_API static AZ::TypeId RTTI_Type();
                SCENE_CORE_API static const char* RTTI_TypeName();

                SCENE_CORE_API static void Reflect(AZ::ReflectContext* context);

                virtual ~IManifestObject() = 0;
                virtual void OnUserAdded() {};
                virtual void OnUserRemoved() const {};
                // Some manifest objects cause other manifest objects to be created.
                // When those manifest objects are removed, the added manifest objects should be removed, too.
                virtual void GetManifestObjectsToRemoveOnRemoved(
                    AZStd::vector<const IManifestObject*>& /*toRemove*/, const Containers::SceneManifest& /*manifest*/) const {}
            };

            inline IManifestObject::~IManifestObject()
            {
            }

            SCENE_CORE_API AZ::TypeNameString GetO3deTypeName(AZ::Adl, AZStd::type_identity<IManifestObject>);
            SCENE_CORE_API AZ::TypeId GetO3deTypeId(AZ::Adl, AZStd::type_identity<IManifestObject>);
        }  //namespace DataTypes
    }  //namespace SceneAPI
}  //namespace AZ
