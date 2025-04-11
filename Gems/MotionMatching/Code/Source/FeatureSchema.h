/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

#include <Feature.h>

namespace EMotionFX::MotionMatching
{
    //! The set of features involved in the motion matching search.
    //! The schema represents the order of the features as well as their settings while the feature matrix stores the actual feature data.
    class EMFX_API FeatureSchema
    {
    public:
        AZ_RTTI(FeatureSchema, "{E34F6BFE-73DB-4DED-AAB9-09FBC5113236}")
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~FeatureSchema();

        void AddFeature(Feature* feature);
        void Clear();
        
        size_t GetNumFeatures() const;
        Feature* GetFeature(size_t index) const;
        const AZStd::vector<Feature*>& GetFeatures() const;

        Feature* FindFeatureById(const AZ::TypeId& featureId) const;

        AZStd::vector<AZStd::string> CollectColumnNames() const;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        static Feature* CreateFeatureByType(const AZ::TypeId& typeId);

        AZStd::vector<Feature*> m_features; //< Ordered set of features (Owns the feature objects).
        AZStd::unordered_map<AZ::TypeId, Feature*> m_featuresById; //< Hash-map for fast access to the features by ID. (Weak ownership)
    };
} // namespace EMotionFX::MotionMatching
