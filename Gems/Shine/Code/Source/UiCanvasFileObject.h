/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Shine/Bus/Tools/UiSystemToolsBus.h>
#include <AzCore/Script/ScriptAsset.h>

#include "UiPrefabInstance.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Wrapper class for canvas file. Supports v3 format (flat ChildEntities) and
//! v4 format (ChildEntities + PrefabInstances with JSON Patch overrides).
class UiCanvasFileObject
    : public UiSystemToolsInterface::CanvasAssetHandle
{
public:
    ~UiCanvasFileObject() override;
    AZ_CLASS_ALLOCATOR(UiCanvasFileObject, AZ::SystemAllocator);
    AZ_RTTI(UiCanvasFileObject, "{1F02632F-F113-49B1-85AD-8CD0FA78B8AA}");

    // Load canvas from stream with an optional asset filter. No asset references are ignored by default
    static UiCanvasFileObject* LoadCanvasFromStream(AZ::IO::GenericStream& stream, const AZ::ObjectStream::FilterDescriptor& filterDesc = AZ::ObjectStream::FilterDescriptor());
    static void SaveCanvasToStream(AZ::IO::GenericStream& stream, UiCanvasFileObject* canvasFileObject);

    static AZ::Entity* LoadCanvasEntitiesFromStream(
        AZ::IO::GenericStream& stream,
        AZStd::vector<AZ::Entity*>& childEntities,
        AZStd::vector<UiPrefabInstance>& prefabInstances);

    static void Reflect(AZ::ReflectContext* context);

public: // data

    AZ::Entity* m_canvasEntity = nullptr;

    //! Entities that belong directly to this canvas (not from any prefab reference)
    AZStd::vector<AZ::Entity*> m_childEntities;

    //! Prefab instances referenced by this canvas. Each stores a source .uiprefab path,
    //! entity ID mapping, and JSON Patch overrides. Entities are instantiated at load time.
    AZStd::vector<UiPrefabInstance> m_prefabInstances;
};
