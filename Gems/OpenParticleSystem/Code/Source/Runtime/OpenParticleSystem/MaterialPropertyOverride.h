/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyValue.h>

#include <AzCore/Name/Name.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/unordered_map.h>

namespace OpenParticle
{
    //! Per-emitter material property overrides.
    //!
    //! Every emitter owns its own AZ::RPI::Material instance (see EmitterInstance::Setup), which means two
    //! emitters can reference the same .material asset and still hold different property values. This map
    //! stores the values that differ from the assigned material asset, keyed by the full material property
    //! id (for example "baseColor.color" or "baseColor.textureMap").
    //!
    //! This mirrors AZ::Render::MaterialAssignment::m_propertyOverrides used by the mesh material component,
    //! but is redeclared here so the runtime gem does not have to depend on AtomLyIntegration_CommonFeatures.
    using MaterialPropertyOverrideMap = AZStd::unordered_map<AZ::Name, AZStd::any>;

    //! Registers the generic types needed to serialize a MaterialPropertyOverrideMap.
    //! Safe to call more than once; SerializeContext ignores duplicate generic type registrations.
    void ReflectMaterialPropertyOverrides(AZ::ReflectContext* context);

    //! Converts a stored AZStd::any into a MaterialPropertyValue of the type the property actually expects.
    //! Numeric and enum properties need coercing because the editor and script layers are loose about which
    //! concrete numeric type they hand back. Everything else falls through to MaterialPropertyValue::FromAny,
    //! which already knows how to turn asset ids and image assets into image property values.
    AZ::RPI::MaterialPropertyValue ConvertMaterialPropertyValue(
        const AZ::RPI::MaterialPropertyDescriptor* propertyDescriptor, const AZStd::any& value);

    //! Applies every override in the map to the given material instance and compiles it.
    //! Returns true if the instance is up to date afterwards (nothing to compile, or the compile succeeded).
    //! A false return generally means the material SRG was still in use this frame and the caller should
    //! try again next tick.
    bool ApplyMaterialPropertyOverrides(
        const AZ::Data::Instance<AZ::RPI::Material>& material, const MaterialPropertyOverrideMap& overrides);

    //! Drops overrides whose property id does not exist in the given material asset.
    //! Used when an emitter is pointed at a different material so stale ids do not linger in the source data.
    void PruneMaterialPropertyOverrides(
        const AZ::Data::Asset<AZ::RPI::MaterialAsset>& materialAsset, MaterialPropertyOverrideMap& overrides);
} // namespace OpenParticle
