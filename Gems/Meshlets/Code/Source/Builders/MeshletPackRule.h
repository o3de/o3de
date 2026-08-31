/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::Meshlets::Builders
{
    //! Scenemanifest rule. When present on an FBX's IMeshGroup, the SceneAPI
    //! export module produces a sibling .azmeshletpack. UI surfaces this rule
    //! under "Add Modifier" → "Meshlet Pack Rule" in the Asset Editor.
    class MeshletPackRule : public AZ::SceneAPI::DataTypes::IRule
    {
    public:
        AZ_RTTI(MeshletPackRule, "{8C3D9B27-2F4E-4B6A-9C5D-1E8F4A6B3D70}",
                AZ::SceneAPI::DataTypes::IRule);
        AZ_CLASS_ALLOCATOR(MeshletPackRule, AZ::SystemAllocator);

        MeshletPackRule();

        static void Reflect(ReflectContext* context);

        AZ::u16 GetMaxVerticesPerCluster() const  { return m_maxVerticesPerCluster; }
        AZ::u16 GetMaxTrianglesPerCluster() const { return m_maxTrianglesPerCluster; }
        float   GetConeWeight() const             { return m_coneWeight; }
        const AZStd::vector<AZStd::string>& GetMeshFilter() const { return m_meshFilter; }

        //! Reset all fields to the documented sane defaults. Called by
        //! MeshletPackRuleBehavior::InitializeObject when the rule is first added
        //! to a mesh group via the Scene Settings "Add Modifier" dropdown, so a
        //! freshly-added rule has the same budgets as the constructor.
        void SetDefaults();

    private:
        //! Cluster budgets. Larger clusters => fewer clusters => fewer per-cluster
        //! draw commands and better post-transform vertex-cache reuse (the dominant
        //! cost when many meshlet instances are on screen). Defaults raised from the
        //! old 64/64; the builder clamps to meshopt limits (verts<=255, tris<=512 &
        //! multiple-of-4).
        AZ::u16 m_maxVerticesPerCluster  = 128;
        AZ::u16 m_maxTrianglesPerCluster = 256;
        float   m_coneWeight             = 0.5f;
        AZStd::vector<AZStd::string> m_meshFilter; //!< Empty / "*" = all.
    };

} // namespace AZ::Meshlets::Builders
