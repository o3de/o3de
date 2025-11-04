/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }

        namespace DataTypes
        {
            class IMeshVertexUVData;
            enum class UVsGenerationMethod
            {
                LeaveSceneDataAsIs = 0, //! don't do anything to the scene
                SphericalProjection = 1 //! generate UVs using simple spherical positional projection
            };
        }

        namespace SceneData
        {
            //! The UVsRule class contains the settings for one particular instance of the "Generate UVs" modifier
            //! on one particular mesh group in the scene.
            class SCENE_DATA_CLASS UVsRule
                : public DataTypes::IRule
            {
            public:
                AZ_RTTI(UVsRule, "{79FB186C-E9B2-4569-9172-84B85DF81DB9}", DataTypes::IRule);
                AZ_CLASS_ALLOCATOR(UVsRule, AZ::SystemAllocator)

                SCENE_DATA_API UVsRule();
                SCENE_DATA_API ~UVsRule() override = default;

                SCENE_DATA_API AZ::SceneAPI::DataTypes::UVsGenerationMethod GetGenerationMethod() const;
                SCENE_DATA_API bool GetReplaceExisting() const;

                static void Reflect(ReflectContext* context);

                // it can be useful to have a different default for when there is no rule ("do nothing" for example)
                // versus if the user actually clicks a button or something to cause a rule to exist now, ie, actually do something
                // useful.

                //! Return the default method for UV Generation when a Generate UVs rule is attached as a modifier to a mesh group.
                SCENE_DATA_API static AZ::SceneAPI::DataTypes::UVsGenerationMethod GetDefaultGenerationMethodWhenAddingNewRule();

                //! Return the default method for when there is no Generate UVs rule attached to the mesh group.
                //! this should probably be left as "do nothing" unless you want to auto-generate UVs for everything without UVs
                SCENE_DATA_API static AZ::SceneAPI::DataTypes::UVsGenerationMethod GetDefaultGenerationMethodWithNoRule();

            protected:
                AZ::SceneAPI::DataTypes::UVsGenerationMethod m_generationMethod = AZ::SceneAPI::DataTypes::UVsGenerationMethod::LeaveSceneDataAsIs;
                bool m_replaceExisting = false;

            };
        } // SceneData
    } // SceneAPI
} // AZ
