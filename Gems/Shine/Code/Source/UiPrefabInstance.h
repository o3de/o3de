/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/IO/Path/Path.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! A UiPrefabInstance represents one instantiation of a .uiprefab in a canvas.
//! It stores the source prefab path, an entity ID mapping (base -> instance),
//! and per-instance field-level overrides as JSON Patches (RFC 6902).
//!
//! This replaces the old SliceReference + SliceInstance + DataPatch system.
class UiPrefabInstance
{
public:
    AZ_CLASS_ALLOCATOR(UiPrefabInstance, AZ::SystemAllocator);
    AZ_RTTI(UiPrefabInstance, "{B3F7E2A1-8C4D-4E9F-A1D2-7F5B3C8E9A01}");

    UiPrefabInstance() = default;
    virtual ~UiPrefabInstance() = default;

    static void Reflect(AZ::ReflectContext* context);

    //! Relative path to the source .uiprefab file (e.g. "UI/Prefabs/Button.uiprefab")
    AZStd::string m_sourcePath;

    //! Unique identifier for this instance
    AZ::Uuid m_instanceId = AZ::Uuid::CreateNull();

    //! Maps base entity IDs (from the .uiprefab) to instance entity IDs (in this canvas).
    //! This is how we track which instance entity corresponds to which base entity.
    AZStd::unordered_map<AZ::EntityId, AZ::EntityId> m_entityIdMap;

    //! JSON Patch (RFC 6902) overrides as a JSON string.
    //! Empty string means no overrides (exact copy of the base prefab).
    //! Example: [{"op":"replace","path":"/Entities/Entity_[123]/Components/.../Text","value":"OK"}]
    AZStd::string m_patchesJson;
};
