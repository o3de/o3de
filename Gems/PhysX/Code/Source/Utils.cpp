/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/EBus/Results.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/SimdMath.h>
#include <AzCore/Math/MathStringConversions.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/SimulatedBodies/StaticRigidBody.h>
#include <AzFramework/Physics/HeightfieldProviderBus.h>

#include <PhysX/ColliderShapeBus.h>
#include <PhysX/EditorColliderComponentRequestBus.h>
#include <PhysX/SystemComponentBus.h>
#include <PhysX/MeshAsset.h>
#include <PhysX/PhysXLocks.h>
#include <PhysX/Utils.h>
#include <Source/SystemComponent.h>
#include <Source/Collision.h>
#include <Source/Pipeline/MeshAssetHandler.h>
#include <Source/Shape.h>
#include <Source/StaticRigidBodyComponent.h>
#include <Source/RigidBodyStatic.h>
#include <Source/Utils.h>
#include <PhysX/Material/PhysXMaterialConfiguration.h>
#include <PhysX/PhysXLocks.h>
#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>
#include <PhysX/MathConversion.h>

namespace PhysX
{
    namespace Utils
    {
        physx::PxBase* CreateNativeMeshObjectFromCookedData(const AZStd::vector<AZ::u8>& cookedData,
            Physics::CookedMeshShapeConfiguration::MeshType meshType)
        {
            // PxDefaultMemoryInputData only accepts a non-const U8* pointer however keeps it as const U8* inside.
            // Hence we do const_cast here but it's safe to assume the data won't be modifed.
            physx::PxDefaultMemoryInputData inpStream(
                const_cast<physx::PxU8*>(cookedData.data()),
                static_cast<physx::PxU32>(cookedData.size()));

            if (meshType == Physics::CookedMeshShapeConfiguration::MeshType::Convex)
            {
                return PxGetPhysics().createConvexMesh(inpStream);
            }
            else
            {
                return PxGetPhysics().createTriangleMesh(inpStream);
            }
        }

        AZStd::pair<uint8_t, uint8_t> GetPhysXMaterialIndicesFromHeightfieldSamples(
            const AZStd::vector<Physics::HeightMaterialPoint>& samples,
            const size_t col, const size_t row, 
            const size_t numCols, const size_t numRows)
        {
            uint8_t materialIndex0 = 0;
            uint8_t materialIndex1 = 0;

            const bool lastRowIndex = (row == (numRows - 1));
            const bool lastColumnIndex = (col == (numCols - 1));

            // In PhysX, the material indices refer to the quad down and to the right of the sample.
            // If we're in the last row or last column, there aren't any quads down or to the right,
            // so just clear these out.
            if (lastRowIndex || lastColumnIndex)
            {
                return { materialIndex0, materialIndex1 };
            }
            
            auto GetIndex = [numCols](size_t col, size_t row)
            {
                return (row * numCols) + col;
            };

            // Our source data is providing one material index per vertex, but PhysX wants one material index
            // per triangle.  The heuristic that we'll go with for selecting the material index is to choose
            // the material for the vertex that's not on the diagonal of each triangle.
            // Ex:  A *---* B
            //        | / |      For this, we'll use A for index0 and D for index1.
            //      C *---* D
            //
            // Ex:  A *---* B
            //        | \ |      For this, we'll use C for index0 and B for index1.
            //      C *---* D
            //
            // This is a pretty arbitrary choice, so the heuristic might need to be revisited over time if this
            // causes incorrect or unpredictable physics material mappings.

            const Physics::HeightMaterialPoint& currentSample = samples[GetIndex(col, row)];

            switch (currentSample.m_quadMeshType)
            {
            case Physics::QuadMeshType::SubdivideUpperLeftToBottomRight:
                materialIndex0 = samples[GetIndex(col, row + 1)].m_materialIndex;
                materialIndex1 = samples[GetIndex(col + 1, row)].m_materialIndex;
                break;
            case Physics::QuadMeshType::SubdivideBottomLeftToUpperRight:
                materialIndex0 = currentSample.m_materialIndex;
                materialIndex1 = samples[GetIndex(col + 1, row + 1)].m_materialIndex;
                break;
            case Physics::QuadMeshType::Hole:
                materialIndex0 = physx::PxHeightFieldMaterial::eHOLE;
                materialIndex1 = physx::PxHeightFieldMaterial::eHOLE;
                break;
            default:
                AZ_Assert(false, "Unhandled case in GetPhysXMaterialIndicesFromHeightfieldSamples");
                break;
            }
            
            return { materialIndex0, materialIndex1 };
        }

        //! Convert a subset of a heightfield shape configuration to a vector of PhysX Heightfield samples.
        AZStd::vector<physx::PxHeightFieldSample> ConvertHeightfieldSamples(
            const Physics::HeightfieldShapeConfiguration& heightfield,
            const size_t startCol, const size_t startRow, 
            const size_t numColsToUpdate, const size_t numRowsToUpdate)

        {
            const size_t numCols = heightfield.GetNumColumnVertices();
            const size_t numRows = heightfield.GetNumRowVertices();

            AZ_Assert(startRow < numRows, "Invalid starting row (%d vs %d total rows)", startRow, numRows);
            AZ_Assert(startCol < numCols, "Invalid starting columm (%d vs %d total columns)", startCol, numCols);
            AZ_Assert((startRow + numRowsToUpdate) <= numRows, "Invalid row selection");
            AZ_Assert((startCol + numColsToUpdate) <= numCols, "Invalid column selection");

            const AZStd::vector<Physics::HeightMaterialPoint>& samples = heightfield.GetSamples();
            AZ_Assert(samples.size() == numRows * numCols, "Heightfield configuration has invalid heightfield sample size.");

            if (samples.empty() || (numRowsToUpdate == 0) || (numColsToUpdate == 0))
            {
                return {};
            }

            const float minHeightBounds = heightfield.GetMinHeightBounds();
            const float maxHeightBounds = heightfield.GetMaxHeightBounds();
            const float halfBounds{ (maxHeightBounds - minHeightBounds) / 2.0f };

            // We're making the assumption right now that the min/max bounds are centered around 0.
            // If we ever want to allow off-center bounds, we'll need to fix up the float-to-int16 height math below
            // to account for it.
            AZ_Assert(
                AZ::IsClose(-halfBounds, minHeightBounds) && AZ::IsClose(halfBounds, maxHeightBounds),
                "Min/Max height bounds aren't centered around 0, the height conversions below will be incorrect.");

            AZ_Assert(
                maxHeightBounds >= minHeightBounds,
                "Max height bounds is less than min height bounds, the height conversions below will be incorrect.");

            // To convert our floating-point heights to fixed-point representation inside of an int16, we need a scale factor
            // for the conversion.  The scale factor is used to map the most important bits of our floating-point height to the
            // full 16-bit range.
            // Note that the scaleFactor choice here affects overall precision.  For each bit that the integer part of our max
            // height uses, that's one less bit for the fractional part.
            const float scaleFactor = (maxHeightBounds <= minHeightBounds) ? 1.0f : AZStd::numeric_limits<int16_t>::max() / halfBounds;

            [[maybe_unused]] constexpr uint8_t physxMaximumMaterialIndex = 0x7f;

            AZStd::vector<physx::PxHeightFieldSample> physxSamples(numRowsToUpdate * numColsToUpdate);

            for (size_t row = 0; row < numRowsToUpdate; row++)
            {
                for (size_t col = 0; col < numColsToUpdate; col++)
                {
                    const size_t sampleIndex = ((row + startRow) * numCols) + (col + startCol);
                    const size_t pxSampleIndex = (row * numColsToUpdate) + col;

                    const Physics::HeightMaterialPoint& currentSample = samples[sampleIndex];
                    physx::PxHeightFieldSample& currentPhysxSample = physxSamples[pxSampleIndex];
                    AZ_Assert(currentSample.m_materialIndex < physxMaximumMaterialIndex, "MaterialIndex must be less than 128");
                    currentPhysxSample.height = azlossy_cast<physx::PxI16>(
                        AZ::GetClamp(currentSample.m_height, minHeightBounds, maxHeightBounds) * scaleFactor);

                    auto [materialIndex0, materialIndex1] =
                        GetPhysXMaterialIndicesFromHeightfieldSamples(samples, col + startCol, row + startRow, numCols, numRows);
                    currentPhysxSample.materialIndex0 = materialIndex0;
                    currentPhysxSample.materialIndex1 = materialIndex1;

                    if (currentSample.m_quadMeshType == Physics::QuadMeshType::SubdivideUpperLeftToBottomRight)
                    {
                        // Set the tesselation flag to say that we need to go from UL to BR
                        currentPhysxSample.setTessFlag();
                    }
                }
            }

            return physxSamples;
        }

        void CreatePxGeometryFromHeightfield(
            Physics::HeightfieldShapeConfiguration& heightfieldConfig, physx::PxGeometryHolder& pxGeometry)
        {
            const AZ::Vector2& gridSpacing = heightfieldConfig.GetGridResolution();

            const size_t numCols = heightfieldConfig.GetNumColumnVertices();
            const size_t numRows = heightfieldConfig.GetNumRowVertices();

            const float rowScale = gridSpacing.GetX();
            const float colScale = gridSpacing.GetY();

            const float minHeightBounds = heightfieldConfig.GetMinHeightBounds();
            const float maxHeightBounds = heightfieldConfig.GetMaxHeightBounds();
            const float halfBounds{ (maxHeightBounds - minHeightBounds) / 2.0f };

            // We're making the assumption right now that the min/max bounds are centered around 0.
            // If we ever want to allow off-center bounds, we'll need to fix up the float-to-int16 height math below
            // to account for it.
            AZ_Assert(
                AZ::IsClose(-halfBounds, minHeightBounds) && AZ::IsClose(halfBounds, maxHeightBounds),
                "Min/Max height bounds aren't centered around 0, the height conversions below will be incorrect.");

            AZ_Assert(
                maxHeightBounds >= minHeightBounds,
                "Max height bounds is less than min height bounds, the height conversions below will be incorrect.");

            // To convert our floating-point heights to fixed-point representation inside of an int16, we need a scale factor
            // for the conversion.  The scale factor is used to map the most important bits of our floating-point height to the
            // full 16-bit range.
            // Note that the scaleFactor choice here affects overall precision.  For each bit that the integer part of our max
            // height uses, that's one less bit for the fractional part.
            const float scaleFactor = (maxHeightBounds <= minHeightBounds) ? 1.0f : AZStd::numeric_limits<int16_t>::max() / halfBounds;
            const float heightScale{ 1.0f / scaleFactor };

            if (physx::PxHeightField* cachedHeightfield
                = static_cast<physx::PxHeightField*>(heightfieldConfig.GetCachedNativeHeightfield()))
            {
                physx::PxHeightFieldGeometry hfGeom(cachedHeightfield, physx::PxMeshGeometryFlags(), heightScale, rowScale, colScale);
                pxGeometry.storeAny(hfGeom);
                return;
            }

            AZStd::vector<physx::PxHeightFieldSample> physxSamples = ConvertHeightfieldSamples(heightfieldConfig, 0, 0, numCols, numRows);

            physx::PxHeightField* heightfield = nullptr;

            if (!physxSamples.empty())
            {
                SystemRequestsBus::BroadcastResult(heightfield, &SystemRequests::CreateHeightField, physxSamples.data(), numCols, numRows);
            }
            if (heightfield)
            {
                heightfieldConfig.SetCachedNativeHeightfield(heightfield);

                physx::PxHeightFieldGeometry hfGeom(heightfield, physx::PxMeshGeometryFlags(), heightScale, rowScale, colScale);

                pxGeometry.storeAny(hfGeom);
            }
        }

        void RefreshHeightfieldShape(
            AzPhysics::Scene* physicsScene,
            Physics::Shape* heightfieldShape,
            Physics::HeightfieldShapeConfiguration& heightfield,
            const size_t startCol, const size_t startRow,
            const size_t numColsToUpdate, const size_t numRowsToUpdate)
        {
            AZ_PROFILE_FUNCTION(Physics);

            auto* pxScene = static_cast<physx::PxScene*>(physicsScene->GetNativePointer());
            AZ_Assert(pxScene, "Attempting to reference a null physics scene");

            auto* pxShape = static_cast<physx::PxShape*>(heightfieldShape->GetNativePointer());
            AZ_Assert(pxShape, "Attempting to refresh a null heightfield shape");

            physx::PxHeightField* pxHeightfield = static_cast<physx::PxHeightField*>(heightfield.GetCachedNativeHeightfield());
            AZ_Assert(pxHeightfield, "Attempting to refresh a null heightfield");

            // Convert the generic heightfield samples in the heigthfield shape to PhysX heightfield samples.
            // This can be done outside the scene lock because we aren't modifying anything yet.
            AZStd::vector<physx::PxHeightFieldSample> physxSamples =
                ConvertHeightfieldSamples(heightfield, startCol, startRow, numColsToUpdate, numRowsToUpdate);

            // Create a descriptor for the subregion that we're updating.
            physx::PxHeightFieldDesc desc;
            desc.format = physx::PxHeightFieldFormat::eS16_TM;
            desc.nbColumns = static_cast<physx::PxU32>(numColsToUpdate);
            desc.nbRows = static_cast<physx::PxU32>(numRowsToUpdate);
            desc.samples.data = physxSamples.data();
            desc.samples.stride = sizeof(physx::PxHeightFieldSample);

            // Modify the heightfield samples
            constexpr bool shrinkBounds = false;
            pxHeightfield->modifySamples(static_cast<physx::PxI32>(startCol), static_cast<physx::PxI32>(startRow), desc, shrinkBounds);

            // Lock the scene and modify the heightfield shape in the scene.
            // (If only the heightfield is modified, the shape won't get refreshed with the new data)
            {
                PHYSX_SCENE_WRITE_LOCK(pxScene);

                physx::PxHeightFieldGeometry hfGeom;
                pxShape->getHeightFieldGeometry(hfGeom);
                hfGeom.heightField = pxHeightfield;
                pxShape->setGeometry(hfGeom);
            }
        }

        bool CreatePxGeometryFromConfig(const Physics::ShapeConfiguration& shapeConfiguration, physx::PxGeometryHolder& pxGeometry)
        {
            if (!shapeConfiguration.m_scale.IsGreaterThan(AZ::Vector3::CreateZero()))
            {
                AZ_Error("PhysX Utils", false, "Negative or zero values are invalid for shape configuration scale values %s",
                    AZStd::to_string(shapeConfiguration.m_scale).c_str());
                return false;
            }

            auto shapeType = shapeConfiguration.GetShapeType();

            switch (shapeType)
            {
            case Physics::ShapeType::Sphere:
            {
                const Physics::SphereShapeConfiguration& sphereConfig = static_cast<const Physics::SphereShapeConfiguration&>(shapeConfiguration);
                if (sphereConfig.m_radius <= 0.0f)
                {
                    AZ_Error("PhysX Utils", false, "Invalid radius value: %f", sphereConfig.m_radius);
                    return false;
                }
                pxGeometry.storeAny(physx::PxSphereGeometry(sphereConfig.m_radius * shapeConfiguration.m_scale.GetMaxElement()));
                break;
            }
            case Physics::ShapeType::Box:
            {
                const Physics::BoxShapeConfiguration& boxConfig = static_cast<const Physics::BoxShapeConfiguration&>(shapeConfiguration);
                if (!boxConfig.m_dimensions.IsGreaterThan(AZ::Vector3::CreateZero()))
                {
                    AZ_Error("PhysX Utils", false, "Negative or zero values are invalid for box dimensions %s",
                        AZStd::to_string(boxConfig.m_dimensions).c_str());
                    return false;
                }
                pxGeometry.storeAny(physx::PxBoxGeometry(PxMathConvert(boxConfig.m_dimensions * 0.5f * shapeConfiguration.m_scale)));
                break;
            }
            case Physics::ShapeType::Capsule:
            {
                const Physics::CapsuleShapeConfiguration& capsuleConfig = static_cast<const Physics::CapsuleShapeConfiguration&>(shapeConfiguration);
                float height = capsuleConfig.m_height * capsuleConfig.m_scale.GetZ();
                float radius = capsuleConfig.m_radius * AZ::GetMax(capsuleConfig.m_scale.GetX(), capsuleConfig.m_scale.GetY());

                if (height <= 0.0f || radius <= 0.0f)
                {
                    AZ_Error("PhysX Utils", false, "Negative or zero values are invalid for capsule dimensions (height: %f, radius: %f)",
                        capsuleConfig.m_height, capsuleConfig.m_radius);
                    return false;
                }

                float halfHeight = 0.5f * height - radius;
                if (halfHeight <= 0.0f)
                {
                    AZ_Warning("PhysX", halfHeight < 0.0f, "Height must exceed twice the radius in capsule configuration (height: %f, radius: %f)",
                        capsuleConfig.m_height, capsuleConfig.m_radius);
                    halfHeight = std::numeric_limits<float>::epsilon();
                }
                pxGeometry.storeAny(physx::PxCapsuleGeometry(radius, halfHeight));
                break;
            }
            case Physics::ShapeType::Native:
            {
                const Physics::NativeShapeConfiguration& nativeShapeConfig = static_cast<const Physics::NativeShapeConfiguration&>(shapeConfiguration);
                AZ::Vector3 scale = nativeShapeConfig.m_nativeShapeScale * nativeShapeConfig.m_scale;
                physx::PxBase* meshData = reinterpret_cast<physx::PxBase*>(nativeShapeConfig.m_nativeShapePtr);
                return MeshDataToPxGeometry(meshData, pxGeometry, scale);
            }
            case Physics::ShapeType::CookedMesh:
            {
                const Physics::CookedMeshShapeConfiguration& constCookedMeshShapeConfig =
                    static_cast<const Physics::CookedMeshShapeConfiguration&>(shapeConfiguration);

                // We are deliberately removing the const off of the ShapeConfiguration here because we're going to change the cached
                // native mesh pointer that gets stored in the configuration.
                Physics::CookedMeshShapeConfiguration& cookedMeshShapeConfig =
                    const_cast<Physics::CookedMeshShapeConfiguration&>(constCookedMeshShapeConfig);

                physx::PxBase* nativeMeshObject = nullptr;

                // Use the cached mesh object if it is there, otherwise create one and save in the shape configuration
                if (cookedMeshShapeConfig.GetCachedNativeMesh())
                {
                    nativeMeshObject = static_cast<physx::PxBase*>(cookedMeshShapeConfig.GetCachedNativeMesh());
                }
                else
                {
                    nativeMeshObject = CreateNativeMeshObjectFromCookedData(
                        cookedMeshShapeConfig.GetCookedMeshData(),
                        cookedMeshShapeConfig.GetMeshType());

                    if (nativeMeshObject)
                    {
                        cookedMeshShapeConfig.SetCachedNativeMesh(nativeMeshObject);
                    }
                    else
                    {
                        AZ_Warning("PhysX Rigid Body", false,
                            "Unable to create a mesh object from the CookedMeshShapeConfiguration buffer. "
                            "Please check if the data was cooked correctly.");
                        return false;
                    }
                }

                return MeshDataToPxGeometry(nativeMeshObject, pxGeometry, cookedMeshShapeConfig.m_scale);
            }
            case Physics::ShapeType::PhysicsAsset:
            {
                AZ_Assert(false,
                    "CreatePxGeometryFromConfig: Cannot pass PhysicsAsset configuration since it is a collection of shapes. "
                    "Please iterate over m_colliderShapes in the asset and call this function for each of them.");
                return false;
            }
            case Physics::ShapeType::Heightfield:
            {
                const Physics::HeightfieldShapeConfiguration& constHeightfieldConfig =
                    static_cast<const Physics::HeightfieldShapeConfiguration&>(shapeConfiguration);

                // We are deliberately removing the const off of the ShapeConfiguration here because we're going to change the cached
                // native heightfield pointer that gets stored in the configuration.
                Physics::HeightfieldShapeConfiguration& heightfieldConfig =
                    const_cast<Physics::HeightfieldShapeConfiguration&>(constHeightfieldConfig);

                CreatePxGeometryFromHeightfield(heightfieldConfig, pxGeometry);
                break;
            }
            default:
                AZ_Warning("PhysX Rigid Body", false, "Shape not supported in PhysX. Shape Type: %d", shapeType);
                return false;
            }

            return true;
        }

        physx::PxShape* CreatePxShapeFromConfig(const Physics::ColliderConfiguration& colliderConfiguration,
            const Physics::ShapeConfiguration& shapeConfiguration, AzPhysics::CollisionGroup& assignedCollisionGroup)
        {
            physx::PxGeometryHolder pxGeomHolder;
            if (!Utils::CreatePxGeometryFromConfig(shapeConfiguration, pxGeomHolder))
            {
                return nullptr;
            }

            AZStd::vector<AZStd::shared_ptr<Material>> materials = Material::FindOrCreateMaterials(colliderConfiguration.m_materialSlots);
            AZStd::vector<const physx::PxMaterial*> pxMaterials(materials.size(), nullptr);
            for (size_t materialIndex = 0; materialIndex < materials.size(); ++materialIndex)
            {
                pxMaterials[materialIndex] = materials[materialIndex]->GetPxMaterial();
            }

            physx::PxShape* shape = PxGetPhysics().createShape(
                pxGeomHolder.any(),
                const_cast<physx::PxMaterial**>(pxMaterials.data()),
                static_cast<physx::PxU16>(pxMaterials.size()),
                colliderConfiguration.m_isExclusive);
            if (!shape)
            {
                AZ_Error("PhysX Rigid Body", false, "Failed to create shape.");
                return nullptr;
            }

            AzPhysics::CollisionGroup collisionGroup;
            Physics::CollisionRequestBus::BroadcastResult(collisionGroup, &Physics::CollisionRequests::GetCollisionGroupById, colliderConfiguration.m_collisionGroupId);

            physx::PxFilterData filterData = PhysX::Collision::CreateFilterData(colliderConfiguration.m_collisionLayer, collisionGroup);
            shape->setSimulationFilterData(filterData);
            shape->setQueryFilterData(filterData);

            // Do custom logic for specific shape types
            if (pxGeomHolder.getType() == physx::PxGeometryType::eCAPSULE)
            {
                // PhysX capsules are oriented around x by default.
                physx::PxQuat pxQuat(AZ::Constants::HalfPi, physx::PxVec3(0.0f, 1.0f, 0.0f));
                shape->setLocalPose(physx::PxTransform(pxQuat));
            }
            else if (pxGeomHolder.getType() == physx::PxGeometryType::eHEIGHTFIELD)
            {
                const Physics::HeightfieldShapeConfiguration& heightfieldConfig =
                    static_cast<const Physics::HeightfieldShapeConfiguration&>(shapeConfiguration);

                // PhysX heightfields have the origin at the corner, not the center, so add an offset to the passed-in transform
                // to account for this difference.
                const AZ::Vector2 gridSpacing = heightfieldConfig.GetGridResolution();
                AZ::Vector3 offset(
                    -(gridSpacing.GetX() * heightfieldConfig.GetNumColumnSquares() / 2.0f),
                    -(gridSpacing.GetY() * heightfieldConfig.GetNumRowSquares() / 2.0f),
                    0.0f);

                // PhysX heightfields are always defined to have the height in the Y direction, not the Z direction, so we need
                // to provide additional rotations to make it Z-up.
                physx::PxQuat pxQuat = PxMathConvert(
                    AZ::Quaternion::CreateFromEulerAnglesRadians(AZ::Vector3(AZ::Constants::HalfPi, AZ::Constants::HalfPi, 0.0f)));
                physx::PxTransform pxHeightfieldTransform = physx::PxTransform(PxMathConvert(offset), pxQuat);
                shape->setLocalPose(pxHeightfieldTransform);
            }

            // Handle a possible misconfiguration when a shape is set to be both simulated & trigger. This is illegal in PhysX.
            shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, colliderConfiguration.m_isSimulated && !colliderConfiguration.m_isTrigger);
            shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, colliderConfiguration.m_isInSceneQueries);
            shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, colliderConfiguration.m_isTrigger);

            shape->setRestOffset(colliderConfiguration.m_restOffset);
            shape->setContactOffset(colliderConfiguration.m_contactOffset);

            physx::PxTransform pxShapeTransform = PxMathConvert(colliderConfiguration.m_position, colliderConfiguration.m_rotation);
            shape->setLocalPose(pxShapeTransform * shape->getLocalPose());

            assignedCollisionGroup = collisionGroup;
            return shape;
        }

        AzPhysics::Scene* GetDefaultScene()
        {
            AzPhysics::SceneHandle sceneHandle;
            Physics::DefaultWorldBus::BroadcastResult(sceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);

            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                if (auto* scene = physicsSystem->GetScene(sceneHandle))
                {
                    return scene;
                }
            }

            return nullptr;
        }

        AZStd::optional<Physics::CookedMeshShapeConfiguration> CreatePxCookedMeshConfiguration(const AZStd::vector<AZ::Vector3>& points, const AZ::Vector3& scale)
        {
            Physics::CookedMeshShapeConfiguration shapeConfig;

            AZStd::vector<AZ::u8> cookedData;
            bool cookingResult = false;
            Physics::SystemRequestBus::BroadcastResult(cookingResult, &Physics::SystemRequests::CookConvexMeshToMemory,
                points.data(), aznumeric_cast<AZ::u32>(points.size()), cookedData);
            shapeConfig.SetCookedMeshData(cookedData.data(), cookedData.size(),
                Physics::CookedMeshShapeConfiguration::MeshType::Convex);
            shapeConfig.m_scale = scale;

            if (!cookingResult)
            {
                AZ_Error("PhysX", false, "PhysX cooking of mesh data failed");
                return {};
            }

            return shapeConfig;
        }

        bool IsPrimitiveShape(const Physics::ShapeConfiguration& shapeConfig)
        {
            const Physics::ShapeType shapeType = shapeConfig.GetShapeType();
            return
                shapeType == Physics::ShapeType::Box ||
                shapeType == Physics::ShapeType::Capsule ||
                shapeType == Physics::ShapeType::Sphere;
        }

        AZStd::optional<Physics::CookedMeshShapeConfiguration> CreateConvexFromPrimitive(
            const Physics::ColliderConfiguration& colliderConfig,
            const Physics::ShapeConfiguration& primitiveShapeConfig, AZ::u8 subdivisionLevel,
            const AZ::Vector3& scale)
        {
            AZ::u8 subdivisionLevelClamped = AZ::GetClamp(subdivisionLevel, MinCapsuleSubdivisionLevel, MaxCapsuleSubdivisionLevel);

            auto applyColliderOffset = [&colliderConfig](const AZ::Vector3 point) {
                return colliderConfig.m_rotation.TransformVector(point) + colliderConfig.m_position;
            };

            auto shapeType = primitiveShapeConfig.GetShapeType();
            switch (shapeType)
            {
            case Physics::ShapeType::Box:
            {
                auto boxConfig = static_cast<const Physics::BoxShapeConfiguration&>(primitiveShapeConfig);
                AZStd::vector<AZ::Vector3> points;
                points.reserve(8);
                const float x = 0.5f * boxConfig.m_dimensions.GetX();
                const float y = 0.5f * boxConfig.m_dimensions.GetY();
                const float z = 0.5f * boxConfig.m_dimensions.GetZ();
                points.push_back(applyColliderOffset(AZ::Vector3(-x, -y, -z)));
                points.push_back(applyColliderOffset(AZ::Vector3(-x, -y, +z)));
                points.push_back(applyColliderOffset(AZ::Vector3(-x, +y, -z)));
                points.push_back(applyColliderOffset(AZ::Vector3(-x, +y, +z)));
                points.push_back(applyColliderOffset(AZ::Vector3(+x, -y, -z)));
                points.push_back(applyColliderOffset(AZ::Vector3(+x, -y, +z)));
                points.push_back(applyColliderOffset(AZ::Vector3(+x, +y, -z)));
                points.push_back(applyColliderOffset(AZ::Vector3(+x, +y, +z)));
                return CreatePxCookedMeshConfiguration(points, scale);
            }
            break;
            case Physics::ShapeType::Capsule:
            {
                auto capsuleConfig = static_cast<const Physics::CapsuleShapeConfiguration&>(primitiveShapeConfig);
                const AZ::u8 numLayers = subdivisionLevelClamped;
                const AZ::u8 numPerLayer = 4 * subdivisionLevelClamped;
                AZStd::vector<AZ::Vector3> points;
                points.reserve(2 * numLayers * numPerLayer + 2);
                points.push_back(applyColliderOffset(AZ::Vector3::CreateAxisZ(0.5f * capsuleConfig.m_height)));
                points.push_back(applyColliderOffset(AZ::Vector3::CreateAxisZ(-0.5f * capsuleConfig.m_height)));
                for (AZ::u8 layerIndex = 0; layerIndex < numLayers; layerIndex++)
                {
                    const float theta = (layerIndex + 1) * AZ::Constants::HalfPi / aznumeric_cast<float>(numLayers);
                    const float layerRadius = capsuleConfig.m_radius * AZ::Sin(theta);
                    const float layerHeight = 0.5f * capsuleConfig.m_height + capsuleConfig.m_radius * (AZ::Cos(theta) - 1.0f);
                    for (AZ::u8 radialIndex = 0; radialIndex < numPerLayer; radialIndex++)
                    {
                        const float phi = radialIndex * AZ::Constants::TwoPi / aznumeric_cast<float>(numPerLayer);
                        points.push_back(applyColliderOffset(AZ::Vector3(
                            layerRadius * AZ::Cos(phi), layerRadius * AZ::Sin(phi), layerHeight)));
                        points.push_back(applyColliderOffset(AZ::Vector3(
                            layerRadius * AZ::Cos(phi), layerRadius * AZ::Sin(phi), -layerHeight)));
                    }
                }
                return CreatePxCookedMeshConfiguration(points, scale);
            }
            break;
            case Physics::ShapeType::Sphere:
            {
                auto sphereConfig = static_cast<const Physics::SphereShapeConfiguration&>(primitiveShapeConfig);
                const AZ::u8 numLayers = 2 * subdivisionLevelClamped;
                const AZ::u8 numPerLayer = 4 * subdivisionLevelClamped;
                AZStd::vector<AZ::Vector3> points;
                points.reserve((numLayers - 1) * numPerLayer + 2);
                points.push_back(applyColliderOffset(AZ::Vector3::CreateAxisZ(sphereConfig.m_radius)));
                points.push_back(applyColliderOffset(AZ::Vector3::CreateAxisZ(-sphereConfig.m_radius)));

                for (AZ::u8 layerIndex = 1; layerIndex < numLayers; layerIndex++)
                {
                    const float theta = layerIndex * AZ::Constants::Pi / aznumeric_cast<float>(numLayers);
                    const float layerRadius = sphereConfig.m_radius * AZ::Sin(theta);
                    const float layerHeight = sphereConfig.m_radius * AZ::Cos(theta);
                    for (AZ::u8 radialIndex = 0; radialIndex < numPerLayer; radialIndex++)
                    {
                        const float phi = radialIndex * AZ::Constants::TwoPi / aznumeric_cast<float>(numPerLayer);
                        points.push_back(applyColliderOffset(AZ::Vector3(
                            layerRadius * AZ::Cos(phi), layerRadius * AZ::Sin(phi), layerHeight)));
                    }
                }
                return CreatePxCookedMeshConfiguration(points, scale);
            }
            break;
            case Physics::ShapeType::CookedMesh:
                return static_cast<const Physics::CookedMeshShapeConfiguration&>(primitiveShapeConfig);
            default:
                AZ_Error("PhysX Utils", false, "CreateConvexFromPrimitive was called with a non-primitive shape configuration.");
                return {};
            }
        }

        // Returns a point list of the frustum extents based on the supplied frustum parameters.
        AZStd::optional<AZStd::vector<AZ::Vector3>> CreatePointsAtFrustumExtents(float height, float bottomRadius, float topRadius, AZ::u8 subdivisions)
        {
            AZStd::vector<AZ::Vector3> points;

            if (height <= 0.0f)
            {
                AZ_Error("PhysX", false, "Frustum height %f must be greater than 0.", height);
                return {};
            }

            if (bottomRadius < 0.0f)
            {
                AZ_Error("PhysX", false, "Frustum bottom radius %f must be greater or equal to 0.", bottomRadius);
                return {};
            }
            else if (topRadius < 0.0f)
            {
                AZ_Error("PhysX", false, "Frustum top radius %f must be greater or equal to 0.", topRadius);
                return {};
            }
            else if (bottomRadius == 0.0f && topRadius == 0.0f)
            {
                AZ_Error("PhysX", false, "Either frustum bottom radius or top radius must be greater than to 0.");
                return {};
            }

            if (subdivisions < MinFrustumSubdivisions || subdivisions > MaxFrustumSubdivisions)
            {
                AZ_Error("PhysX", false, "Frustum subdivision count %u is not in [%u, %u] range", subdivisions, MinFrustumSubdivisions, MaxFrustumSubdivisions);
                return {};
            }

            points.reserve(subdivisions * 2);
            const float halfHeight = height * 0.5f;
            const double step = AZ::Constants::TwoPi / aznumeric_cast<double>(subdivisions);

            for (double rad = 0; rad < AZ::Constants::TwoPi; rad += step)
            {
                float x = aznumeric_cast<float>(std::cos(rad));
                float y = aznumeric_cast<float>(std::sin(rad));

                points.emplace_back(x * topRadius, y * topRadius, +halfHeight);
                points.emplace_back(x * bottomRadius, y * bottomRadius, -halfHeight);
            }

            return points;
        }

        AZStd::string ConvexCookingResultToString(physx::PxConvexMeshCookingResult::Enum convexCookingResultCode)
        {
            static const AZStd::string resultToString[] = { "eSUCCESS", "eZERO_AREA_TEST_FAILED", "ePOLYGONS_LIMIT_REACHED", "eFAILURE" };
            AZ_PUSH_DISABLE_WARNING(, "-Wtautological-constant-out-of-range-compare")
                if (AZ_ARRAY_SIZE(resultToString) > convexCookingResultCode)
                    AZ_POP_DISABLE_WARNING
                {
                    return resultToString[convexCookingResultCode];
                }
                else
                {
                    AZ_Error("PhysX", false, "Unknown convex cooking result code: %i", convexCookingResultCode);
                    return "";
                }
        }

        AZStd::string TriMeshCookingResultToString(physx::PxTriangleMeshCookingResult::Enum triangleCookingResultCode)
        {
            static const AZStd::string resultToString[] = { "eSUCCESS", "eLARGE_TRIANGLE", "eFAILURE" };
            AZ_PUSH_DISABLE_WARNING(, "-Wtautological-constant-out-of-range-compare")
                if (AZ_ARRAY_SIZE(resultToString) > triangleCookingResultCode)
                    AZ_POP_DISABLE_WARNING
                {
                    return resultToString[triangleCookingResultCode];
                }
                else
                {
                    AZ_Error("PhysX", false, "Unknown trimesh cooking result code: %i", triangleCookingResultCode);
                    return "";
                }
        }

        bool WriteCookedMeshToFile(const AZStd::string& filePath, const AZStd::vector<AZ::u8>& physxData,
            Physics::CookedMeshShapeConfiguration::MeshType meshType)
        {
            Pipeline::MeshAssetData assetData;

            AZStd::shared_ptr<Pipeline::AssetColliderConfiguration> colliderConfig;
            AZStd::shared_ptr<Physics::CookedMeshShapeConfiguration> shapeConfig = AZStd::make_shared<Physics::CookedMeshShapeConfiguration>();

            shapeConfig->SetCookedMeshData(physxData.data(), physxData.size(), meshType);

            assetData.m_colliderShapes.emplace_back(colliderConfig, shapeConfig);

            return Utils::WriteCookedMeshToFile(filePath, assetData);
        }

        bool WriteCookedMeshToFile(const AZStd::string& filePath, const Pipeline::MeshAssetData& assetData)
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            return AZ::Utils::SaveObjectToFile(filePath, AZ::DataStream::ST_BINARY, &assetData, serializeContext);
        }

        bool CookConvexToPxOutputStream(const AZ::Vector3* vertices, AZ::u32 vertexCount, physx::PxOutputStream& stream)
        {
            physx::PxCooking* cooking = nullptr;
            SystemRequestsBus::BroadcastResult(cooking, &SystemRequests::GetCooking);

            physx::PxConvexMeshDesc convexDesc;
            convexDesc.points.count = vertexCount;
            convexDesc.points.stride = sizeof(AZ::Vector3);
            convexDesc.points.data = vertices;
            convexDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

            physx::PxConvexMeshCookingResult::Enum resultCode = physx::PxConvexMeshCookingResult::eSUCCESS;

            bool result = cooking->cookConvexMesh(convexDesc, stream, &resultCode);

            AZ_Error("PhysX", result,
                "CookConvexToPxOutputStream: Failed to cook convex mesh. Please check the data is correct. Error: %s",
                Utils::ConvexCookingResultToString(resultCode).c_str());

            return result;
        }

        bool CookTriangleMeshToToPxOutputStream(const AZ::Vector3* vertices, AZ::u32 vertexCount,
            const AZ::u32* indices, AZ::u32 indexCount, physx::PxOutputStream& stream)
        {
            physx::PxCooking* cooking = nullptr;
            SystemRequestsBus::BroadcastResult(cooking, &SystemRequests::GetCooking);

            // Validate indices size
            AZ_Error("PhysX", indexCount % 3 == 0, "Number of indices must be a multiple of 3.");

            physx::PxTriangleMeshDesc meshDesc;
            meshDesc.points.count = vertexCount;
            meshDesc.points.stride = sizeof(AZ::Vector3);
            meshDesc.points.data = vertices;

            meshDesc.triangles.count = indexCount / 3;
            meshDesc.triangles.stride = sizeof(AZ::u32) * 3;
            meshDesc.triangles.data = indices;

            physx::PxTriangleMeshCookingResult::Enum resultCode = physx::PxTriangleMeshCookingResult::eSUCCESS;

            bool result = cooking->cookTriangleMesh(meshDesc, stream, &resultCode);

            AZ_Error("PhysX", result,
                "CookTriangleMeshToToPxOutputStream: Failed to cook triangle mesh. Please check the data is correct. Error: %s.",
                Utils::TriMeshCookingResultToString(resultCode).c_str());

            return result;
        }

        bool MeshDataToPxGeometry(physx::PxBase* meshData, physx::PxGeometryHolder& pxGeometry, const AZ::Vector3& scale)
        {
            if (meshData)
            {
                if (meshData->is<physx::PxTriangleMesh>())
                {
                    pxGeometry.storeAny(physx::PxTriangleMeshGeometry(reinterpret_cast<physx::PxTriangleMesh*>(meshData), physx::PxMeshScale(PxMathConvert(scale))));
                }
                else
                {
                    pxGeometry.storeAny(physx::PxConvexMeshGeometry(reinterpret_cast<physx::PxConvexMesh*>(meshData), physx::PxMeshScale(PxMathConvert(scale))));
                }

                return true;
            }
            else
            {
                AZ_Error("PhysXUtils::MeshDataToPxGeometry", false, "Mesh data is null.");
                return false;
            }
        }

        bool ReadFile(const AZStd::string& path, AZStd::vector<uint8_t>& buffer)
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (!fileIO)
            {
                AZ_Warning("PhysXUtils::ReadFile", false, "No File System");
                return false;
            }

            // Open file
            AZ::IO::HandleType file;
            if (!fileIO->Open(path.c_str(), AZ::IO::OpenMode::ModeRead, file))
            {
                AZ_Warning("PhysXUtils::ReadFile", false, "Failed to open file:%s", path.c_str());
                return false;
            }

            // Get file size, we want to read the whole thing in one go
            AZ::u64 fileSize;
            if (!fileIO->Size(file, fileSize))
            {
                AZ_Warning("PhysXUtils::ReadFile", false, "Failed to read file size:%s", path.c_str());
                fileIO->Close(file);
                return false;
            }

            if (fileSize <= 0)
            {
                AZ_Warning("PhysXUtils::ReadFile", false, "File is empty:%s", path.c_str());
                fileIO->Close(file);
                return false;
            }

            buffer.resize(fileSize);

            AZ::u64 bytesRead = 0;
            bool failOnFewerThanSizeBytesRead = false;
            if (!fileIO->Read(file, &buffer[0], fileSize, failOnFewerThanSizeBytesRead, &bytesRead))
            {
                AZ_Warning("PhysXUtils::ReadFile", false, "Failed to read file:%s", path.c_str());
                fileIO->Close(file);
                return false;
            }

            fileIO->Close(file);

            return true;
        }

        AZStd::string ReplaceAll(AZStd::string str, const AZStd::string& fromString, const AZStd::string& toString) {
            size_t positionBegin = 0;
            while ((positionBegin = str.find(fromString, positionBegin)) != AZStd::string::npos)
            {
                str.replace(positionBegin, fromString.length(), toString);
                positionBegin += toString.length();
            }
            return str;
        }

        static AZStd::string FormatEntityNames(
            const AZStd::vector<AZ::EntityId>& entityIds, const char* message)
        {
            AZStd::string messageOutput = message;
            messageOutput += "\n";
            for (const auto& entityId : entityIds)
            {
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
                if (entity)
                {
                    messageOutput += entity->GetName() + "\n";
                }
            }

            AZStd::string percentageSymbol("%");
            AZStd::string percentageReplace(
                "%%"); // Replacing % with %% serves to escape the % character when printing out the entity names in printf style.
            messageOutput = ReplaceAll(messageOutput, percentageSymbol, percentageReplace);
            return messageOutput;
        }

        void PrintEntityNames(const AZStd::vector<AZ::EntityId>& entityIds, [[maybe_unused]] const char* category, const char* message)
        {
            const AZStd::string messageOutput = FormatEntityNames(entityIds, message);
            AZ_Printf(category, messageOutput.c_str());
        }

        void WarnEntityNames(const AZStd::vector<AZ::EntityId>& entityIds, [[maybe_unused]] const char* category, const char* message)
        {
            const AZStd::string messageOutput = FormatEntityNames(entityIds, message);
            AZ_Warning(category, false, messageOutput.c_str());
        }

        AZ::Transform GetColliderLocalTransform(const AZ::Vector3& colliderRelativePosition,
            const AZ::Quaternion& colliderRelativeRotation)
        {
            return AZ::Transform::CreateFromQuaternionAndTranslation(colliderRelativeRotation, colliderRelativePosition);
        }

        AZ::Transform GetColliderLocalTransform(const AZ::EntityComponentIdPair& idPair)
        {
            AZ::Quaternion colliderRotation = AZ::Quaternion::CreateIdentity();
            PhysX::EditorColliderComponentRequestBus::EventResult(colliderRotation, idPair, &PhysX::EditorColliderComponentRequests::GetColliderRotation);

            AZ::Vector3 colliderOffset = AZ::Vector3::CreateZero();
            PhysX::EditorColliderComponentRequestBus::EventResult(colliderOffset, idPair, &PhysX::EditorColliderComponentRequests::GetColliderOffset);

            return AZ::Transform::CreateFromQuaternionAndTranslation(colliderRotation, colliderOffset);
        }

        AZ::Transform GetColliderWorldTransform(const AZ::Transform& worldTransform,
            const AZ::Vector3& colliderRelativePosition,
            const AZ::Quaternion& colliderRelativeRotation)
        {
            return worldTransform * GetColliderLocalTransform(colliderRelativePosition, colliderRelativeRotation);
        }

        void ColliderPointsLocalToWorld(AZStd::vector<AZ::Vector3>& pointsInOut,
            const AZ::Transform& worldTransform,
            const AZ::Vector3& colliderRelativePosition,
            const AZ::Quaternion& colliderRelativeRotation,
            const AZ::Vector3& nonUniformScale)
        {
            for (AZ::Vector3& point : pointsInOut)
            {
                point = worldTransform.TransformPoint(nonUniformScale *
                    GetColliderLocalTransform(colliderRelativePosition, colliderRelativeRotation).TransformPoint(point));
            }
        }

        AZ::Aabb GetPxGeometryAabb(const physx::PxGeometryHolder& geometryHolder,
            const AZ::Transform& worldTransform,
            const ::Physics::ColliderConfiguration& colliderConfiguration
        )
        {
            const float boundsInflationFactor = 1.0f;
            AZ::Transform overallTransformNoScale = GetColliderWorldTransform(worldTransform,
                colliderConfiguration.m_position, colliderConfiguration.m_rotation);
            overallTransformNoScale.ExtractUniformScale();
            const physx::PxBounds3 bounds = physx::PxGeometryQuery::getWorldBounds(geometryHolder.any(),
                PxMathConvert(overallTransformNoScale),
                boundsInflationFactor);
            return PxMathConvert(bounds);
        }

        AZ::Aabb GetColliderAabb(const AZ::Transform& worldTransform,
            bool hasNonUniformScale,
            AZ::u8 subdivisionLevel,
            const ::Physics::ShapeConfiguration& shapeConfiguration,
            const ::Physics::ColliderConfiguration& colliderConfiguration)
        {
            const AZ::Aabb worldPosAabb = AZ::Aabb::CreateFromPoint(worldTransform.GetTranslation());
            physx::PxGeometryHolder geometryHolder;
            bool isAssetShape = shapeConfiguration.GetShapeType() == Physics::ShapeType::PhysicsAsset;

            if (!isAssetShape)
            {
                if (!hasNonUniformScale)
                {
                    if (CreatePxGeometryFromConfig(shapeConfiguration, geometryHolder))
                    {
                        return GetPxGeometryAabb(geometryHolder, worldTransform, colliderConfiguration);
                    }
                }
                else
                {
                    auto convexPrimitive = Utils::CreateConvexFromPrimitive(colliderConfiguration, shapeConfiguration, subdivisionLevel, shapeConfiguration.m_scale);
                    if (convexPrimitive.has_value())
                    {
                        if (CreatePxGeometryFromConfig(convexPrimitive.value(), geometryHolder))
                        {
                            Physics::ColliderConfiguration colliderConfigurationNoOffset = colliderConfiguration;
                            colliderConfigurationNoOffset.m_rotation = AZ::Quaternion::CreateIdentity();
                            colliderConfigurationNoOffset.m_position = AZ::Vector3::CreateZero();
                            return GetPxGeometryAabb(geometryHolder, worldTransform, colliderConfigurationNoOffset);
                        }
                    }
                }
                return worldPosAabb;
            }
            else
            {
                const Physics::PhysicsAssetShapeConfiguration& physicsAssetConfig =
                    static_cast<const Physics::PhysicsAssetShapeConfiguration&>(shapeConfiguration);

                if (!physicsAssetConfig.m_asset.IsReady())
                {
                    return worldPosAabb;
                }

                AzPhysics::ShapeColliderPairList colliderShapes;
                GetColliderShapeConfigsFromAsset(physicsAssetConfig,
                    colliderConfiguration,
                    hasNonUniformScale,
                    subdivisionLevel,
                    colliderShapes);

                if (colliderShapes.empty())
                {
                    return worldPosAabb;
                }

                AZ::Aabb aabb = AZ::Aabb::CreateNull();
                for (const auto& colliderShape : colliderShapes)
                {
                    if (colliderShape.second &&
                        CreatePxGeometryFromConfig(*colliderShape.second, geometryHolder))
                    {
                        aabb.AddAabb(
                            GetPxGeometryAabb(geometryHolder, worldTransform, *colliderShape.first)
                        );
                    }
                    else
                    {
                        return worldPosAabb;
                    }
                }
                return aabb;
            }
        }

        bool TriggerColliderExists(AZ::EntityId entityId)
        {
            AZ::EBusLogicalResult<bool, AZStd::logical_or<bool>> response(false);
            PhysX::ColliderShapeRequestBus::EventResult(response,
                entityId,
                &PhysX::ColliderShapeRequestBus::Events::IsTrigger);
            return response.value;
        }

        void GetColliderShapeConfigsFromAsset(const Physics::PhysicsAssetShapeConfiguration& assetConfiguration,
            const Physics::ColliderConfiguration& originalColliderConfiguration, bool hasNonUniformScale,
            AZ::u8 subdivisionLevel, AzPhysics::ShapeColliderPairList& resultingColliderShapes)
        {
            if (!assetConfiguration.m_asset.IsReady())
            {
                AZ_Error("PhysX", false, "GetColliderShapesFromAsset: Asset %s is not ready."
                    "Please make sure the calling code connects to the AssetBus and "
                    "creates the collider shapes only when OnAssetReady or OnAssetReload is invoked.",
                    assetConfiguration.m_asset.GetHint().c_str());
                return;
            }

            const Pipeline::MeshAsset* asset = assetConfiguration.m_asset.GetAs<Pipeline::MeshAsset>();

            if (!asset)
            {
                AZ_Error("PhysX", false, "GetColliderShapesFromAsset: Mesh Asset %s is null."
                    "Please check the file is in the correct format. Try to delete it and get AssetProcessor re-create it. "
                    "The data is loaded in Pipeline::MeshAssetHandler::LoadAssetData()",
                    assetConfiguration.m_asset.GetHint().c_str());
                return;
            }

            const Pipeline::MeshAssetData& assetData = asset->m_assetData;
            const Pipeline::MeshAssetData::ShapeConfigurationList& shapeConfigList = assetData.m_colliderShapes;

            resultingColliderShapes.reserve(resultingColliderShapes.size() + shapeConfigList.size());

            for (size_t shapeIndex = 0; shapeIndex < shapeConfigList.size(); shapeIndex++)
            {
                const Pipeline::MeshAssetData::ShapeConfigurationPair& shapeConfigPair = shapeConfigList[shapeIndex];

                AZStd::shared_ptr<Physics::ColliderConfiguration> thisColliderConfiguration =
                    AZStd::make_shared<Physics::ColliderConfiguration>(originalColliderConfiguration);

                AZ::u16 shapeMaterialIndex = assetData.m_materialIndexPerShape[shapeIndex];

                // Triangle meshes have material indices cooked in the data.
                if (shapeMaterialIndex != Pipeline::MeshAssetData::TriangleMeshMaterialIndex)
                {
                    // Clear the materials that came in from the component collider configuration
                    thisColliderConfiguration->m_materialSlots.SetSlots(Physics::MaterialDefaultSlot::Default);

                    // Set the material that is relevant for this specific shape
                    thisColliderConfiguration->m_materialSlots.SetMaterialAsset(
                        0,
                        originalColliderConfiguration.m_materialSlots.GetMaterialAsset(shapeMaterialIndex));
                }

                // Here we use the collider configuration data saved in the asset to update the one coming from the component
                if (const Pipeline::AssetColliderConfiguration* optionalColliderData = shapeConfigPair.first.get())
                {
                    optionalColliderData->UpdateColliderConfiguration(*thisColliderConfiguration);
                }

                // Update the scale with the data from the asset configuration
                AZStd::shared_ptr<Physics::ShapeConfiguration> thisShapeConfiguration = shapeConfigPair.second;
                thisShapeConfiguration->m_scale = assetConfiguration.m_scale * assetConfiguration.m_assetScale;

                // If the shape is a primitive and there is non-uniform scale, replace it with a convex approximation
                if (hasNonUniformScale && Utils::IsPrimitiveShape(*thisShapeConfiguration))
                {
                    auto scaledPrimitive = Utils::CreateConvexFromPrimitive(*thisColliderConfiguration,
                        *thisShapeConfiguration, subdivisionLevel, thisShapeConfiguration->m_scale);
                    if (scaledPrimitive.has_value())
                    {
                        thisShapeConfiguration = AZStd::make_shared<Physics::CookedMeshShapeConfiguration>(scaledPrimitive.value());
                        physx::PxGeometryHolder pxGeometryHolder;
                        CreatePxGeometryFromConfig(*thisShapeConfiguration, pxGeometryHolder);
                        thisColliderConfiguration->m_rotation = AZ::Quaternion::CreateIdentity();
                        thisColliderConfiguration->m_position = AZ::Vector3::CreateZero();
                        resultingColliderShapes.emplace_back(thisColliderConfiguration, thisShapeConfiguration);
                    }
                }
                else
                {
                    resultingColliderShapes.emplace_back(thisColliderConfiguration, thisShapeConfiguration);
                }
            }
        }

        void GetShapesFromAsset(const Physics::PhysicsAssetShapeConfiguration& assetConfiguration,
            const Physics::ColliderConfiguration& originalColliderConfiguration, bool hasNonUniformScale,
            AZ::u8 subdivisionLevel, AZStd::vector<AZStd::shared_ptr<Physics::Shape>>& resultingShapes)
        {
            AzPhysics::ShapeColliderPairList resultingColliderShapeConfigs;
            GetColliderShapeConfigsFromAsset(assetConfiguration, originalColliderConfiguration,
                hasNonUniformScale, subdivisionLevel, resultingColliderShapeConfigs);

            resultingShapes.reserve(resultingShapes.size() + resultingColliderShapeConfigs.size());

            for (const AzPhysics::ShapeColliderPair& shapeConfigPair : resultingColliderShapeConfigs)
            {
                // Scale the collider offset
                shapeConfigPair.first->m_position *= shapeConfigPair.second->m_scale;

                AZStd::shared_ptr<Physics::Shape> shape;
                Physics::SystemRequestBus::BroadcastResult(shape, &Physics::SystemRequests::CreateShape,
                    *shapeConfigPair.first, *shapeConfigPair.second);

                if (shape)
                {
                    resultingShapes.emplace_back(shape);
                }
            }
        }

        float GetTransformScale(AZ::EntityId entityId)
        {
            float transformScale = 1.0f;
            AZ::TransformBus::EventResult(transformScale, entityId, &AZ::TransformBus::Events::GetWorldUniformScale);
            return transformScale;
        }

        AZ::Vector3 GetNonUniformScale(AZ::EntityId entityId)
        {
            AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
            AZ::NonUniformScaleRequestBus::EventResult(nonUniformScale, entityId, &AZ::NonUniformScaleRequests::GetScale);
            return nonUniformScale;
        }

        AZ::Vector3 GetOverallScale(AZ::EntityId entityId)
        {
            return GetTransformScale(entityId) * GetNonUniformScale(entityId);
        }

        const AZ::Vector3& Sanitize(const AZ::Vector3& input, const AZ::Vector3& defaultValue)
        {
            if (!input.IsFinite())
            {
                AZ_Error("PhysX", false, "Invalid Vector3 was passed to PhysX.");
                return defaultValue;
            }
            return input;
        }

        namespace Geometry
        {
            PointList GenerateBoxPoints(const AZ::Vector3& min, const AZ::Vector3& max)
            {
                PointList pointList;

                auto size = max - min;

                const auto minSamples = 2.f;
                const auto maxSamples = 8.f;
                const auto desiredSampleDelta = 2.f;

                // How many sample in each axis
                int numSamples[] =
                {
                    static_cast<int>(AZ::GetClamp(size.GetX() / desiredSampleDelta, minSamples, maxSamples)),
                    static_cast<int>(AZ::GetClamp(size.GetY() / desiredSampleDelta, minSamples, maxSamples)),
                    static_cast<int>(AZ::GetClamp(size.GetZ() / desiredSampleDelta, minSamples, maxSamples))
                };

                float sampleDelta[] =
                {
                    size.GetX() / static_cast<float>(numSamples[0] - 1),
                    size.GetY() / static_cast<float>(numSamples[1] - 1),
                    size.GetZ() / static_cast<float>(numSamples[2] - 1),
                };

                for (auto i = 0; i < numSamples[0]; ++i)
                {
                    for (auto j = 0; j < numSamples[1]; ++j)
                    {
                        for (auto k = 0; k < numSamples[2]; ++k)
                        {
                            pointList.emplace_back(
                                min.GetX() + i * sampleDelta[0],
                                min.GetY() + j * sampleDelta[1],
                                min.GetZ() + k * sampleDelta[2]
                            );
                        }
                    }
                }

                return pointList;
            }

            PointList GenerateSpherePoints(float radius)
            {
                PointList points;

                int nSamples = static_cast<int>(radius * 5);
                nSamples = AZ::GetClamp(nSamples, 5, 512);

                // Draw arrows using Fibonacci sphere
                float offset = 2.f / nSamples;
                float increment = AZ::Constants::Pi * (3.f - sqrt(5.f));
                for (int i = 0; i < nSamples; ++i)
                {
                    float phi = ((i + 1) % nSamples) * increment;
                    float y = ((i * offset) - 1) + (offset / 2.f);
                    float r = aznumeric_cast<float>(sqrt(1 - pow(y, 2)));
                    float x = cos(phi) * r;
                    float z = sin(phi) * r;
                    points.emplace_back(x * radius, y * radius, z * radius);
                }
                return points;
            }

            PointList GenerateCylinderPoints(float height, float radius)
            {
                PointList points;
                AZ::Vector3 base(0.f, 0.f, -height * 0.5f);
                AZ::Vector3 radiusVector(radius, 0.f, 0.f);

                const auto sides = AZ::GetClamp(radius, 3.f, 8.f);
                const auto segments = AZ::GetClamp(height * 0.5f, 2.f, 8.f);
                const auto angleDelta = AZ::Quaternion::CreateRotationZ(AZ::Constants::TwoPi / sides);
                const auto segmentDelta = height / (segments - 1);
                for (auto segment = 0; segment < segments; ++segment)
                {
                    for (auto side = 0; side < sides; ++side)
                    {
                        auto point = base + radiusVector;
                        points.emplace_back(point);
                        radiusVector = angleDelta.TransformVector(radiusVector);
                    }
                    base += AZ::Vector3(0, 0, segmentDelta);
                }
                return points;
            }

            void GetBoxGeometry(const physx::PxBoxGeometry& geometry, AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices)
            {
                constexpr size_t numVertices = 8;
                vertices.reserve(numVertices);

                vertices.push_back(AZ::Vector3(-geometry.halfExtents.x, -geometry.halfExtents.y, -geometry.halfExtents.z));
                vertices.push_back(AZ::Vector3(geometry.halfExtents.x, -geometry.halfExtents.y, -geometry.halfExtents.z));
                vertices.push_back(AZ::Vector3(geometry.halfExtents.x, geometry.halfExtents.y, -geometry.halfExtents.z));
                vertices.push_back(AZ::Vector3(-geometry.halfExtents.x, geometry.halfExtents.y, -geometry.halfExtents.z));

                vertices.push_back(AZ::Vector3(-geometry.halfExtents.x, -geometry.halfExtents.y, geometry.halfExtents.z));
                vertices.push_back(AZ::Vector3(geometry.halfExtents.x, -geometry.halfExtents.y, geometry.halfExtents.z));
                vertices.push_back(AZ::Vector3(geometry.halfExtents.x, geometry.halfExtents.y, geometry.halfExtents.z));
                vertices.push_back(AZ::Vector3(-geometry.halfExtents.x, geometry.halfExtents.y, geometry.halfExtents.z));

                constexpr size_t numIndices = 36;
                static const AZ::u32 boxIndices[numIndices] =
                {
                    2, 1, 0,
                    0, 3, 2,
                    3, 0, 7,
                    0, 4, 7,
                    0, 1, 5,
                    0, 5, 4,
                    1, 2, 5,
                    6, 5, 2,
                    7, 2, 3,
                    7, 6, 2,
                    7, 4, 5,
                    7, 5, 6
                };
                indices.reserve(numIndices);
                for (int i = 0; i < numIndices; ++i)
                {
                    indices.push_back(boxIndices[i]);
                }
            }

            void GetCapsuleGeometry(const physx::PxCapsuleGeometry& geometry, AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::u32 stacks, const AZ::u32 slices)
            {
                const AZ::Vector3 base(0.0, 0.0, -geometry.halfHeight);
                const AZ::Vector3 top(0.0, 0.0, geometry.halfHeight);
                const float radius = geometry.radius;

                // topStack refers to the top row of vertices starting at 0
                // get an even number so our caps reach all the way out to sphere radius
                const AZ::u32 topStack = stacks % 2 ? stacks + 1 : stacks;
                const AZ::u32 midStack = topStack / 2;

                vertices.reserve(slices * topStack + 2);
                indices.reserve((slices - 1) * topStack * 6);

                const float thetaFactor = 1.f / aznumeric_cast<float>(topStack) * AZ::Constants::Pi;
                const float phiFactor = 1.f / aznumeric_cast<float>(slices - 1) * AZ::Constants::TwoPi;

                // bottom cap
                vertices.push_back(base + AZ::Vector3(0.f, 0.f, -radius));
                for (size_t stack = 1; stack <= midStack; ++stack)
                {
                    for (size_t i = 0; i < slices; ++i)
                    {
                        float theta(aznumeric_cast<float>(stack) * thetaFactor);
                        float phi(aznumeric_cast<float>(i) * phiFactor);

                        float sinTheta, cosTheta;
                        AZ::SinCos(theta, sinTheta, cosTheta);

                        float sinPhi, cosPhi;
                        AZ::SinCos(phi, sinPhi, cosPhi);

                        vertices.push_back(base + AZ::Vector3(sinTheta * cosPhi * radius, sinTheta * sinPhi * radius, -cosTheta * radius));
                    }
                }

                // top cap
                for (size_t stack = midStack; stack < topStack; ++stack)
                {
                    for (size_t i = 0; i < slices; ++i)
                    {
                        float theta(aznumeric_cast<float>(stack) * thetaFactor);
                        float phi(aznumeric_cast<float>(i) * phiFactor);

                        float sinTheta, cosTheta;
                        AZ::SinCos(theta, sinTheta, cosTheta);

                        float sinPhi, cosPhi;
                        AZ::SinCos(phi, sinPhi, cosPhi);

                        vertices.push_back(top + AZ::Vector3(sinTheta * cosPhi * radius, sinTheta * sinPhi * radius, -cosTheta * radius));
                    }
                }
                vertices.push_back(top + AZ::Vector3(0.f, 0.f, radius));

                const AZ::u32 lastVertex = aznumeric_cast<AZ::u32>(vertices.size()) - 1;
                const AZ::u32 topRow = aznumeric_cast<AZ::u32>(vertices.size()) - slices - 1;

                // top and bottom segment indices
                for (AZ::u32 i = 0; i < slices - 1; ++i)
                {
                    // bottom (add one to account for single bottom vertex)
                    indices.push_back(0);
                    indices.push_back(i + 2);
                    indices.push_back(i + 1);

                    //top (topRow accounts for the added bottom vertex)
                    indices.push_back(topRow + i + 0);
                    indices.push_back(topRow + i + 1);
                    indices.push_back(lastVertex);
                }

                // there are stacks + 1 stacks because we stretched the middle for the cylinder section,
                // but we already built the top and bottom stack so there are stacks + 1 - 2 to build
                // add 1 to each vertex index because there is a single bottom vertex for the bottom cap
                for (AZ::u32 j = 0; j < stacks - 1; ++j)
                {
                    for (AZ::u32 i = 0; i < slices - 1; ++i)
                    {
                        indices.push_back(j * slices + i + 2);
                        indices.push_back((j + 1) * slices + i + 2);
                        indices.push_back((j + 1) * slices + i + 1);
                        indices.push_back(j * slices + i + 1);
                        indices.push_back(j * slices + i + 2);
                        indices.push_back((j + 1) * slices + i + 1);
                    }
                }
            }

            void GetConvexMeshGeometry(const physx::PxConvexMeshGeometry& geometry, AZStd::vector<AZ::Vector3>& vertices, [[maybe_unused]] AZStd::vector<AZ::u32>& indices)
            {
                const physx::PxConvexMesh* convexMesh = geometry.convexMesh;
                const physx::PxU8* pxIndices = convexMesh->getIndexBuffer();
                const physx::PxVec3* pxVertices = convexMesh->getVertices();
                const AZ::u32 numPolys = convexMesh->getNbPolygons();

                physx::PxHullPolygon poly;
                for (AZ::u32 polygonIndex = 0; polygonIndex < numPolys; ++polygonIndex)
                {
                    if (convexMesh->getPolygonData(polygonIndex, poly))
                    {
                        constexpr AZ::u32 index1 = 0;
                        AZ::u32 index2 = 1;
                        AZ::u32 index3 = 2;

                        const AZ::Vector3 a = PxMathConvert(geometry.scale.transform(pxVertices[pxIndices[poly.mIndexBase + index1]]));
                        const AZ::u32 triangleCount = poly.mNbVerts - 2;

                        for (AZ::u32 triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
                        {
                            AZ_Assert(index3 < poly.mNbVerts, "Implementation error: attempted to index outside range of polygon vertices.");

                            const AZ::Vector3 b = PxMathConvert(geometry.scale.transform(pxVertices[pxIndices[poly.mIndexBase + index2]]));
                            const AZ::Vector3 c = PxMathConvert(geometry.scale.transform(pxVertices[pxIndices[poly.mIndexBase + index3]]));

                            vertices.push_back(a);
                            vertices.push_back(b);
                            vertices.push_back(c);

                            index2 = index3++;
                        }
                    }
                }
            }

            void GetHeightFieldGeometry(const physx::PxHeightFieldGeometry& geometry, AZStd::vector<AZ::Vector3>& vertices,
                [[maybe_unused]] AZStd::vector<AZ::u32>& indices, const AZ::Aabb* optionalBounds)
            {
                int minX = 0;
                int minY = 0;

                // rows map to y and columns to x see EditorTerrainComponent
                int maxX = geometry.heightField->getNbColumns() - 1;
                int maxY = geometry.heightField->getNbRows() - 1;

                if (optionalBounds)
                {
                    // convert the provided bounds to heightfield sample grid positions
                    const AZ::Aabb bounds = *optionalBounds;
                    const float inverseRowScale = 1.f / geometry.rowScale;
                    const float inverseColumnScale = 1.f / geometry.columnScale;

                    minX = AZStd::max(minX, static_cast<int>(floor(bounds.GetMin().GetX() * inverseColumnScale)));
                    minY = AZStd::max(minY, static_cast<int>(floor(bounds.GetMin().GetY() * inverseRowScale)));
                    maxX = AZStd::min(maxX, static_cast<int>(ceil(bounds.GetMax().GetX() * inverseColumnScale)));
                    maxY = AZStd::min(maxY, static_cast<int>(ceil(bounds.GetMax().GetY() * inverseRowScale)));

                    // Make sure min values don't exceed the max 
                    minX = AZStd::min(minX, maxX);
                    minY = AZStd::min(minY, maxY);
                }

                // num quads * 2 triangles per quad * 3 vertices per triangle
                const size_t numVertices = (maxY - minY) * (maxX - minX) * 2 * 3;
                vertices.reserve(numVertices);

                for (int y = minY; y < maxY; ++y)
                {
                    for (int x = minX; x < maxX; ++x)
                    {
                        const physx::PxHeightFieldSample& pxSample = geometry.heightField->getSample(y, x);

                        if (pxSample.materialIndex0 == physx::PxHeightFieldMaterial::eHOLE ||
                            pxSample.materialIndex1 == physx::PxHeightFieldMaterial::eHOLE)
                        {
                            // skip terrain geometry marked as eHOLE, this feature is often used for tunnels
                            continue;
                        }

                        float height = aznumeric_cast<float>(pxSample.height) * geometry.heightScale;

                        const AZ::Vector3 v0(aznumeric_cast<float>(x) * geometry.rowScale, aznumeric_cast<float>(y) * geometry.columnScale, height);

                        height = aznumeric_cast<float>(geometry.heightField->getSample(y + 1, x).height) * geometry.heightScale;
                        const AZ::Vector3 v1(aznumeric_cast<float>(x) * geometry.rowScale, aznumeric_cast<float>(y + 1) * geometry.columnScale, height);

                        height = aznumeric_cast<float>(geometry.heightField->getSample(y, x + 1).height) * geometry.heightScale;
                        const AZ::Vector3 v2(aznumeric_cast<float>(x + 1) * geometry.rowScale, aznumeric_cast<float>(y) * geometry.columnScale, height);

                        height = aznumeric_cast<float>(geometry.heightField->getSample(y + 1, x + 1).height) * geometry.heightScale;
                        const AZ::Vector3 v3(aznumeric_cast<float>(x + 1) * geometry.rowScale, aznumeric_cast<float>(y + 1) * geometry.columnScale, height);

                        vertices.push_back(v0);
                        vertices.push_back(v1);
                        vertices.push_back(v2);

                        vertices.push_back(v1);
                        vertices.push_back(v3);
                        vertices.push_back(v2);
                    }
                }
            }

            void GetSphereGeometry(const physx::PxSphereGeometry& geometry, AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::u32 stacks, const AZ::u32 slices)
            {
                const float radius = geometry.radius;
                const size_t vertexCount = slices * (stacks - 2) + 2;
                vertices.reserve(vertexCount);

                vertices.push_back(AZ::Vector3(0.f, radius, 0.f));
                vertices.push_back(AZ::Vector3(0.f, -radius, 0.f));

                for (size_t j = 1; j < stacks - 1; ++j)
                {
                    for (size_t i = 0; i < slices; ++i)
                    {
                        float theta = (j / (float)(stacks - 1)) * AZ::Constants::Pi;
                        float phi = (i / (float)(slices - 1)) * AZ::Constants::TwoPi;

                        float sinTheta, cosTheta;
                        AZ::SinCos(theta, sinTheta, cosTheta);

                        float sinPhi, cosPhi;
                        AZ::SinCos(phi, sinPhi, cosPhi);

                        vertices.push_back(AZ::Vector3(sinTheta * cosPhi * radius, cosTheta * radius, -sinTheta * sinPhi * radius));
                    }
                }

                const size_t indexCount = (slices - 1) * (stacks - 2) * 6;
                indices.reserve(indexCount);

                for (AZ::u32 i = 0; i < slices - 1; ++i)
                {
                    indices.push_back(0);
                    indices.push_back(i + 2);
                    indices.push_back(i + 3);

                    indices.push_back((stacks - 3) * slices + i + 3);
                    indices.push_back((stacks - 3) * slices + i + 2);
                    indices.push_back(1);
                }

                for (AZ::u32 j = 0; j < stacks - 3; ++j)
                {
                    for (AZ::u32 i = 0; i < slices - 1; ++i)
                    {
                        indices.push_back((j + 1) * slices + i + 3);
                        indices.push_back(j * slices + i + 3);
                        indices.push_back((j + 1) * slices + i + 2);
                        indices.push_back(j * slices + i + 3);
                        indices.push_back(j * slices + i + 2);
                        indices.push_back((j + 1) * slices + i + 2);
                    }
                }
            }

            void GetTriangleMeshGeometry(const physx::PxTriangleMeshGeometry& geometry, AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices)
            {
                const physx::PxTriangleMesh* triangleMesh = geometry.triangleMesh;
                const physx::PxVec3* meshVertices = triangleMesh->getVertices();
                const AZ::u32 vertCount = triangleMesh->getNbVertices();
                const AZ::u32 triangleCount = triangleMesh->getNbTriangles();

                vertices.reserve(vertCount);
                indices.reserve(triangleCount * 3);

                for (AZ::u32 vertIndex = 0; vertIndex < vertCount; ++vertIndex)
                {
                    vertices.push_back(PxMathConvert(geometry.scale.transform(meshVertices[vertIndex])));
                }

                physx::PxTriangleMeshFlags triangleMeshFlags = triangleMesh->getTriangleMeshFlags();
                if (triangleMeshFlags.isSet(physx::PxTriangleMeshFlag::Enum::e16_BIT_INDICES))
                {
                    const physx::PxU16* triangles = static_cast<const physx::PxU16*>(triangleMesh->getTriangles());
                    for (AZ::u32 triangleIndex = 0; triangleIndex < triangleCount * 3; triangleIndex += 3)
                    {
                        indices.push_back(triangles[triangleIndex]);
                        indices.push_back(triangles[triangleIndex + 1]);
                        indices.push_back(triangles[triangleIndex + 2]);
                    }
                }
                else
                {
                    const physx::PxU32* triangles = static_cast<const physx::PxU32*>(triangleMesh->getTriangles());
                    for (AZ::u32 triangleIndex = 0; triangleIndex < triangleCount * 3; triangleIndex += 3)
                    {
                        indices.push_back(triangles[triangleIndex]);
                        indices.push_back(triangles[triangleIndex + 1]);
                        indices.push_back(triangles[triangleIndex + 2]);
                    }
                }
            }
        } // namespace Geometry

        AZ::Transform GetEntityWorldTransformWithScale(AZ::EntityId entityId)
        {
            AZ::Transform worldTransformWithoutScale = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldTransformWithoutScale
                , entityId
                , &AZ::TransformInterface::GetWorldTM);
            return worldTransformWithoutScale;
        }

        AZ::Transform GetEntityWorldTransformWithoutScale(AZ::EntityId entityId)
        {
            AZ::Transform worldTransformWithoutScale = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldTransformWithoutScale
                , entityId
                , &AZ::TransformInterface::GetWorldTM);
            worldTransformWithoutScale.ExtractUniformScale();
            return worldTransformWithoutScale;
        }

        AZ::Transform ComputeJointLocalTransform(const AZ::Transform& jointWorldTransform,
            const AZ::Transform& entityWorldTransform)
        {
            AZ::Transform jointWorldTransformWithoutScale = jointWorldTransform;
            jointWorldTransformWithoutScale.ExtractUniformScale();

            AZ::Transform entityWorldTransformWithoutScale = entityWorldTransform;
            entityWorldTransformWithoutScale.ExtractUniformScale();
            AZ::Transform entityWorldTransformInverse = entityWorldTransformWithoutScale.GetInverse();

            return entityWorldTransformInverse * jointWorldTransformWithoutScale;
        }

        AZ::Transform ComputeJointWorldTransform(const AZ::Transform& jointLocalTransform,
            const AZ::Transform& entityWorldTransform)
        {
            AZ::Transform jointLocalTransformWithoutScale = jointLocalTransform;
            jointLocalTransformWithoutScale.ExtractUniformScale();

            AZ::Transform entityWorldTransformWithoutScale = entityWorldTransform;
            entityWorldTransformWithoutScale.ExtractUniformScale();

            return entityWorldTransformWithoutScale * jointLocalTransformWithoutScale;
        }
        
        Physics::HeightfieldShapeConfiguration CreateBaseHeightfieldShapeConfiguration(AZ::EntityId entityId)
        {
            Physics::HeightfieldShapeConfiguration configuration;

            AZ::Vector2 gridSpacing(1.0f);
            Physics::HeightfieldProviderRequestsBus::EventResult(
                gridSpacing, entityId, &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSpacing);

            configuration.SetGridResolution(gridSpacing);

            size_t numRows = 0;
            size_t numColumns = 0;
            Physics::HeightfieldProviderRequestsBus::Event(
                entityId, &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, numColumns, numRows);

            // The heightfield needs to be at least 2 x 2 vertices to define a single heightfield square.
            if ((numRows >= 2) && (numColumns >= 2))
            {
                configuration.SetNumRowVertices(numRows);
                configuration.SetNumColumnVertices(numColumns);
            }

            float minHeightBounds = 0.0f;
            float maxHeightBounds = 0.0f;
            Physics::HeightfieldProviderRequestsBus::Event(
                entityId, &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldHeightBounds, minHeightBounds, maxHeightBounds);

            configuration.SetMinHeightBounds(minHeightBounds);
            configuration.SetMaxHeightBounds(maxHeightBounds);

            return configuration;
        }

        Physics::HeightfieldShapeConfiguration CreateHeightfieldShapeConfiguration(AZ::EntityId entityId)
        {
            Physics::HeightfieldShapeConfiguration configuration = CreateBaseHeightfieldShapeConfiguration(entityId);

            AZStd::vector<Physics::HeightMaterialPoint> samples;
            Physics::HeightfieldProviderRequestsBus::EventResult(
                samples, entityId, &Physics::HeightfieldProviderRequestsBus::Events::GetHeightsAndMaterials);

            configuration.SetSamples(samples);

            return configuration;
        }

        void SetMaterialsFromPhysicsAssetShape(const Physics::ShapeConfiguration& shapeConfiguration, Physics::MaterialSlots& materialSlots)
        {
            if (shapeConfiguration.GetShapeType() != Physics::ShapeType::PhysicsAsset)
            {
                return;
            }

            const Physics::PhysicsAssetShapeConfiguration& assetConfiguration =
                static_cast<const Physics::PhysicsAssetShapeConfiguration&>(shapeConfiguration);

            if (!assetConfiguration.m_asset.GetId().IsValid())
            {
                // Set the default selection if there's no physics asset.
                materialSlots.SetSlots(Physics::MaterialDefaultSlot::Default);
                return;
            }

            if (!assetConfiguration.m_asset.IsReady())
            {
                // The asset is valid but is still loading,
                // Do not set the empty slots in this case to avoid the entity being in invalid state
                return;
            }

            Pipeline::MeshAsset* meshAsset = assetConfiguration.m_asset.GetAs<Pipeline::MeshAsset>();
            if (!meshAsset)
            {
                materialSlots.SetSlots(Physics::MaterialDefaultSlot::Default);
                AZ_Warning("Physics", false, "Invalid mesh asset in physics asset shape configuration.");
                return;
            }

            // If it has to use the materials assets from the mesh.
            if (assetConfiguration.m_useMaterialsFromAsset)
            {
                // Copy slots entirely, which also include the material assets assigned to them.
                materialSlots = meshAsset->m_assetData.m_materialSlots;
            }
            else
            {
                // Set only the slots, but do not set the material assets.
                materialSlots.SetSlots(meshAsset->m_assetData.m_materialSlots.GetSlotsNames());
            }
        }

        void SetMaterialsFromHeightfieldProvider(const AZ::EntityId& heightfieldProviderId, Physics::MaterialSlots& materialSlots)
        {
            AZStd::vector<AZ::Data::Asset<Physics::MaterialAsset>> materialList;
            Physics::HeightfieldProviderRequestsBus::EventResult(
                materialList, heightfieldProviderId, &Physics::HeightfieldProviderRequestsBus::Events::GetMaterialList);

            materialSlots.SetSlots({ materialList.size(), "" }); // Nameless slots, their names are not shown in the heightfield component.

            for (size_t slotIndex = 0; slotIndex < materialList.size(); ++slotIndex)
            {
                materialSlots.SetMaterialAsset(slotIndex, materialList[slotIndex]);
            }
        }
    } // namespace Utils

    namespace ReflectionUtils
    {
        // Forwards invocation of CalculateNetForce in a force region to script canvas.
        class ForceRegionBusBehaviorHandler
            : public ForceRegionNotificationBus::Handler
            , public AZ::BehaviorEBusHandler
        {
        public:
            AZ_EBUS_BEHAVIOR_BINDER(ForceRegionBusBehaviorHandler, "{EB6C0F7A-0BDA-4052-84C0-33C05E3FF739}", AZ::SystemAllocator
                , OnCalculateNetForce
            );

            static void Reflect(AZ::ReflectContext* context);

            /// Callback invoked when net force exerted on object is computed by a force region.
            void OnCalculateNetForce(AZ::EntityId forceRegionEntityId
                , AZ::EntityId targetEntityId
                , const AZ::Vector3& netForceDirection
                , float netForceMagnitude) override;
        };

        void ReflectPhysXOnlyApi(AZ::ReflectContext* context)
        {
            PhysXSystemConfiguration::Reflect(context);
            Debug::DebugConfiguration::Reflect(context);

            ForceRegionBusBehaviorHandler::Reflect(context);

            D6JointLimitConfiguration::Reflect(context);
            JointGenericProperties::Reflect(context);
            JointLimitProperties::Reflect(context);
            FixedJointConfiguration::Reflect(context);
            BallJointConfiguration::Reflect(context);
            HingeJointConfiguration::Reflect(context);
            PrismaticJointConfiguration::Reflect(context);

            MaterialConfiguration::Reflect(context);
        }

        void ForceRegionBusBehaviorHandler::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<PhysX::ForceRegionNotificationBus>("ForceRegionNotificationBus")
                    ->Attribute(AZ::Script::Attributes::Module, "physics")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Handler<ForceRegionBusBehaviorHandler>()
                    ;
            }
        }

        void ForceRegionBusBehaviorHandler::OnCalculateNetForce(AZ::EntityId forceRegionEntityId
            , AZ::EntityId targetEntityId
            , const AZ::Vector3& netForceDirection
            , float netForceMagnitude)
        {
            Call(FN_OnCalculateNetForce
                , forceRegionEntityId
                , targetEntityId
                , netForceDirection
                , netForceMagnitude);
        }
    } // namespace ReflectionUtils

    namespace PxActorFactories
    {
        constexpr auto PxActorDestructor = [](physx::PxActor* actor)
        {
            if (!actor)
            {
                return;
            }

            if (auto* userData = Utils::GetUserData(actor))
            {
                userData->Invalidate();
            }

            actor->release();
        };

        AZStd::shared_ptr<physx::PxRigidDynamic> CreatePxRigidBody(const AzPhysics::RigidBodyConfiguration& configuration)
        {
            physx::PxTransform pxTransform(PxMathConvert(configuration.m_position),
                PxMathConvert(configuration.m_orientation).getNormalized());

            auto rigidDynamic = AZStd::shared_ptr<physx::PxRigidDynamic>(
                PxGetPhysics().createRigidDynamic(pxTransform),
                PxActorDestructor);

            if (!rigidDynamic)
            {
                AZ_Error("PhysX Rigid Body", false, "Failed to create PhysX rigid actor. Name: %s", configuration.m_debugName.c_str());
                return nullptr;
            }

            rigidDynamic->setMass(configuration.m_mass);
            rigidDynamic->setSleepThreshold(configuration.m_sleepMinEnergy);
            rigidDynamic->setLinearVelocity(PxMathConvert(configuration.m_initialLinearVelocity));
            rigidDynamic->setAngularVelocity(PxMathConvert(configuration.m_initialAngularVelocity));
            rigidDynamic->setLinearDamping(configuration.m_linearDamping);
            rigidDynamic->setAngularDamping(configuration.m_angularDamping);
            rigidDynamic->setCMassLocalPose(physx::PxTransform(PxMathConvert(configuration.m_centerOfMassOffset)));
            rigidDynamic->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, configuration.m_kinematic);
            rigidDynamic->setMaxAngularVelocity(configuration.m_maxAngularVelocity);

            // Set axis locks.
            rigidDynamic->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_X, configuration.m_lockLinearX);
            rigidDynamic->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, configuration.m_lockLinearY);
            rigidDynamic->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, configuration.m_lockLinearZ);
            rigidDynamic->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, configuration.m_lockAngularX);
            rigidDynamic->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, configuration.m_lockAngularY);
            rigidDynamic->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, configuration.m_lockAngularZ);

            return rigidDynamic;
        }

        AZStd::shared_ptr<physx::PxRigidStatic> CreatePxStaticRigidBody(const AzPhysics::StaticRigidBodyConfiguration& configuration)
        {
            physx::PxTransform pxTransform(PxMathConvert(configuration.m_position),
                PxMathConvert(configuration.m_orientation).getNormalized());

            auto rigidStatic = AZStd::shared_ptr<physx::PxRigidStatic>(
                PxGetPhysics().createRigidStatic(pxTransform),
                PxActorDestructor);

            if (!rigidStatic)
            {
                AZ_Error("PhysX Static Rigid Body", false, "Failed to create PhysX static rigid actor. Name: %s", configuration.m_debugName.c_str());
                return nullptr;
            }

            return rigidStatic;
        }
    } // namespace PxActorFactories

    namespace StaticRigidBodyUtils
    {
        bool EntityHasComponentsUsingService(const AZ::Entity& entity, AZ::Crc32 service)
        {
            const AZ::Entity::ComponentArrayType& components = entity.GetComponents();

            return AZStd::any_of(components.begin(), components.end(),
                [service](const AZ::Component* component) -> bool
                {
                    AZ::ComponentDescriptor* componentDescriptor = nullptr;
                    AZ::ComponentDescriptorBus::EventResult(
                        componentDescriptor, azrtti_typeid(component), &AZ::ComponentDescriptorBus::Events::GetDescriptor);

                    AZ::ComponentDescriptor::DependencyArrayType services;
                    componentDescriptor->GetDependentServices(services, nullptr);

                    return AZStd::find(services.begin(), services.end(), service) != services.end();
                }
            );
        }

        bool CanCreateRuntimeComponent(const AZ::Entity& editorEntity)
        {
            // Allow to create runtime StaticRigidBodyComponent if there are no components
            // using 'PhysXColliderService' attached to entity.
            const AZ::Crc32 physxColliderServiceId = AZ_CRC_CE("PhysicsColliderService");

            return !EntityHasComponentsUsingService(editorEntity, physxColliderServiceId);
        }

        bool TryCreateRuntimeComponent(const AZ::Entity& editorEntity, AZ::Entity& gameEntity)
        {
            // Only allow single StaticRigidBodyComponent per entity
            const auto* staticRigidBody = gameEntity.FindComponent<StaticRigidBodyComponent>();
            if (staticRigidBody)
            {
                return false;
            }

            if (CanCreateRuntimeComponent(editorEntity))
            {
                gameEntity.CreateComponent<StaticRigidBodyComponent>();
                return true;
            }

            return false;
        }
    } // namespace StaticRigidBodyUtils
} // namespace PhysX
