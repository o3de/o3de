/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Editor/PropertyTypes.h>

#include <Components/EditorClothComponent.h>
#include <Components/ClothComponent.h>
#include <Components/ClothComponentMesh/ClothComponentMesh.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>

#include <Utils/AssetHelper.h>

namespace NvCloth
{
    namespace Internal
    {
        extern const char* const StatusMessageSelectNode = "Select a node";
        extern const char* const StatusMessageNoAsset = "<No asset>";
        extern const char* const StatusMessageNoClothNodes = "<No cloth modifiers>";

        const char* const AttributeSuffixMetersUnit = " m";
    }

    void EditorClothComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorClothComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Field("Configuration", &EditorClothComponent::m_config)
                ->Version(0)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorClothComponent>(
                    "Cloth", "The mesh node behaves like a piece of cloth.")
                     ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Cloth.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Cloth.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/physx/cloth/")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->UIElement(AZ::Edit::UIHandlers::CheckBox, "Simulate in editor",
                        "Enables cloth simulation in editor when set.")
                        ->Attribute(AZ::Edit::Attributes::CheckboxDefaultValue, &EditorClothComponent::IsSimulatedInEditor)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorClothComponent::OnSimulatedInEditorToggled)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorClothComponent::m_config)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorClothComponent::OnConfigurationChanged)
                    ;

                editContext->Class<ClothConfiguration>("Cloth Configuration", "Configuration for cloth simulation.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    // Mesh Node
                    ->DataElement(Editor::MeshNodeSelector, &ClothConfiguration::m_meshNode, "Mesh node", 
                        "List of mesh nodes with cloth simulation data. These are the nodes selected inside Cloth Modifiers in Scene Settings.")
                        ->Attribute(AZ::Edit::UIHandlers::EntityId, &ClothConfiguration::GetEntityId)
                        ->Attribute(AZ::Edit::Attributes::StringList, &ClothConfiguration::PopulateMeshNodeList)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    // Mass and Gravity
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_mass, "Mass",
                        "Mass scale applied to all particles.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_useCustomGravity, "Custom Gravity", 
                        "When enabled it allows to set a custom gravity value for this cloth.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_customGravity, "Gravity", 
                        "Gravity applied to particles.")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ClothConfiguration::IsUsingWorldBusGravity)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_gravityScale, "Gravity Scale", 
                        "Use this parameter to scale the gravity applied to particles.")

                    // Global stiffness frequency
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_stiffnessFrequency, "Stiffness frequency",
                        "Stiffness exponent per second applied to damping, damping dragging, wind dragging, wind lifting, self collision stiffness, fabric stiffness, fabric compression, fabric stretch and tether constraint stiffness.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    
                    // Motion Constraints
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Motion constraints")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_motionConstraintsMaxDistance, "Max Distance",
                        "Maximum distance for motion constraints to limit particles movement during simulation.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, Internal::AttributeSuffixMetersUnit)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_motionConstraintsScale, "Scale",
                        "Scale value applied to all motion constraints.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_motionConstraintsBias, "Bias",
                        "Bias value added to all motion constraints.")
                        ->Attribute(AZ::Edit::Attributes::Suffix, Internal::AttributeSuffixMetersUnit)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_motionConstraintsStiffness, "Stiffness",
                        "Stiffness for motion constraints.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    
                    // Backstop
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Backstop")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ClothConfiguration::HasBackstopData)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_backstopRadius, "Radius",
                        "Maximum radius that will prevent the associated cloth particle from moving into that area.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, Internal::AttributeSuffixMetersUnit)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ClothConfiguration::HasBackstopData)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_backstopBackOffset, "Back offset",
                        "Maximum offset for backstop spheres behind the cloth.")
                        ->Attribute(AZ::Edit::Attributes::Suffix, Internal::AttributeSuffixMetersUnit)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ClothConfiguration::HasBackstopData)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_backstopFrontOffset, "Front offset",
                        "Maximum offset for backstop spheres in front of the cloth.")
                        ->Attribute(AZ::Edit::Attributes::Suffix, Internal::AttributeSuffixMetersUnit)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ClothConfiguration::HasBackstopData)

                    // Damping
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Damping")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_damping, "Damping",
                        "Damping of particle velocity.\n"
                        "0: Velocity is unaffected\n"
                        "1: Velocity is zeroed")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_linearDrag, "Linear drag",
                        "Portion of velocity applied to particles.\n"
                        "0: Particles is unaffected\n"
                        "1: Damped global particle velocity")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_angularDrag, "Angular drag",
                        "Portion of angular velocity applied to turning particles.\n"
                        "0: Particles is unaffected\n"
                        "1: Damped global particle angular velocity")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)

                    // Inertia
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Inertia")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_linearInteria, "Linear",
                        "Portion of acceleration applied to particles.\n"
                        "0: Particles are unaffected\n"
                        "1: Physically correct")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_angularInteria, "Angular",
                        "Portion of angular acceleration applied to turning particles.\n"
                        "0: Particles are unaffected\n"
                        "1: Physically correct")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_centrifugalInertia, "Centrifugal",
                        "Portion of angular velocity applied to turning particles.\n"
                        "0: Particles are unaffected\n"
                        "1: Physically correct")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)

                    // Wind
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Wind")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_useCustomWindVelocity, "Enable local wind velocity",
                        "When enabled it allows to set a custom wind velocity value for this cloth, otherwise using wind velocity from Physics::WindBus.\n"
                        "Wind is disabled when both air coefficients are zero.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_windVelocity, "Local velocity", 
                        "Wind in global coordinates acting on cloth's triangles. Disabled when both air coefficients are zero.\n"
                        "NOTE: A combination of high values in wind properties can cause unstable results.")
                        ->Attribute(AZ::Edit::Attributes::Min, -50.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 50.0f)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ClothConfiguration::IsUsingWindBus )
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_airDragCoefficient, "Air drag coefficient",
                        "Amount of air dragging.\n"
                        "NOTE: A combination of high values in wind properties can cause unstable results.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_airLiftCoefficient, "Air lift coefficient",
                        "Amount of air lifting.\n"
                        "NOTE: A combination of high values in wind properties can cause unstable results.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_fluidDensity, "Air Density", 
                        "Density of air used for air drag and lift calculations.\n"
                        "NOTE: A combination of high values in wind properties can cause unstable results.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01f)

                    // Collision
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Collision")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_collisionFriction, "Friction", 
                        "Amount of friction with colliders.\n"
                        "0: No friction\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_collisionMassScale, "Mass scale", 
                        "Controls how quickly mass is increased during collisions.\n"
                        "0: No mass scaling\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_continuousCollisionDetection, "Continuous detection", 
                        "Continuous collision detection improves collision by computing time of impact between cloth particles and colliders."
                        "The increase in quality comes with a cost in performance, it's recommended to use only when required.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_collisionAffectsStaticParticles, "Affects static particles",
                        "When enabled colliders will move static particles (inverse mass 0).")

                    // Self collision
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Self collision")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_selfCollisionDistance, "Distance", 
                        "Meters that particles need to be separated from each other.\n"
                        "0: No self collision\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_selfCollisionStiffness, "Stiffness",
                        "Stiffness for the self collision constraints.\n"
                        "0: No self collision\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)

                    // Fabric stiffness
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Fabric stiffness")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_horizontalStiffness, "Horizontal", 
                        "Stiffness value for horizontal constraints.\n"
                        "0: no horizontal constraints\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_horizontalStiffnessMultiplier, "Horizontal multiplier",
                        "Scale value for horizontal fabric compression and stretch limits.\n"
                        "0: No horizontal compression and stretch limits applied\n"
                        "1: Fully apply horizontal compression and stretch limits\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_verticalStiffness, "Vertical",
                        "Stiffness value for vertical constraints.\n"
                        "0: no vertical constraints\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_verticalStiffnessMultiplier, "Vertical multiplier",
                        "Scale value for vertical fabric compression and stretch limits.\n"
                        "0: No vertical compression and stretch limits applied\n"
                        "1: Fully apply vertical compression and stretch limits\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_bendingStiffness, "Bending",
                        "Stiffness value for bending constraints.\n"
                        "0: no bending constraints\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_bendingStiffnessMultiplier, "Bending multiplier",
                        "Scale value for bending fabric compression and stretch limits.\n"
                        "0: No bending compression and stretch limits applied\n"
                        "1: Fully apply bending compression and stretch limits\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_shearingStiffness, "Shearing",
                        "Stiffness value for shearing constraints.\n"
                        "0: no shearing constraints\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_shearingStiffnessMultiplier, "Shearing multiplier",
                        "Scale value for shearing fabric compression and stretch limits.\n"
                        "0: No shearing compression and stretch limits applied\n"
                        "1: Fully apply shearing compression and stretch limits\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)

                    // Fabric compression
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Fabric compression")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_horizontalCompressionLimit, "Horizontal limit", 
                        "Compression limit for horizontal constraints. It's affected by fabric horizontal stiffness multiplier.\n"
                        "0: No compression\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_verticalCompressionLimit, "Vertical limit",
                        "Compression limit for vertical constraints. It's affected by fabric vertical stiffness multiplier.\n"
                        "0: No compression\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_bendingCompressionLimit, "Bending limit",
                        "Compression limit for bending constraints. It's affected by fabric bending stiffness multiplier.\n"
                        "0: No compression\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_shearingCompressionLimit, "Shearing limit",
                        "Compression limit for shearing constraints. It's affected by fabric shearing stiffness multiplier.\n"
                        "0: No compression\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    // Fabric stretch
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Fabric stretch")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_horizontalStretchLimit, "Horizontal limit",
                        "Stretch limit for horizontal constraints. It's affected by fabric horizontal stiffness multiplier."
                        "Reduce stiffness of tether constraints (or increase its scale) to allow cloth to stretch.\n"
                        "0: No stretching\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_verticalStretchLimit, "Vertical limit",
                        "Stretch limit for vertical constraints. It's affected by fabric vertical stiffness multiplier."
                        "Reduce stiffness of tether constraints (or increase its scale) to allow cloth to stretch.\n"
                        "0: No stretching\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_bendingStretchLimit, "Bending limit",
                        "Stretch limit for bending constraints. It's affected by fabric bending stiffness multiplier."
                        "Reduce stiffness of tether constraints (or increase its scale) to allow cloth to stretch.\n"
                        "0: No stretching\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_shearingStretchLimit, "Shearing limit",
                        "Stretch limit for shearing constraints. It's affected by fabric shearing stiffness multiplier."
                        "Reduce stiffness of tether constraints (or increase its scale) to allow cloth to stretch.\n"
                        "0: No stretching\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    // Tether constraints
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Tether constraints")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_tetherConstraintStiffness, "Stiffness",
                        "Stiffness for tether constraints. Tether constraints are generated when the inverse mass data of the cloth (selected in the cloth modifier) has static particles.\n"
                        "0: No tether constraints applied\n"
                        "1: Makes the constraints behave springy\n")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_tetherConstraintScale, "Scale",
                        "Tether constraint scale")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    // Quality
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Quality")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_solverFrequency, "Solver frequency", 
                        "Target solver iterations per second. At least 1 iteration per frame will be solved regardless of the value set.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_accelerationFilterIterations, "Acceleration filter iterations", 
                        "Number of iterations to average delta time factor used for gravity and external acceleration.")
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_removeStaticTriangles, "Remove static triangles",
                        "Removing static triangles improves performance by not taking into account triangles whose particles are all static.\n"
                        "The removed static particles will not be present for collision or self collision during simulation.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_updateNormalsOfStaticParticles, "Update normals of static particles",
                        "When enabled the normals of static particles will be updated according with the movement of the simulated mesh.\n"
                        "When disabled the static particles will keep the same normals as the original mesh.")
                    ;
            }
        }
    }

    EditorClothComponent::EditorClothComponent()
    {
        m_meshNodeList = { {Internal::StatusMessageNoAsset} };
        m_config.m_populateMeshNodeListCallback = [this]()
            {
                return m_meshNodeList;
            };
        m_config.m_hasBackstopDataCallback = [this]()
            {
                auto meshNodeIt = m_meshNodesWithBackstopData.find(m_config.m_meshNode);
                return meshNodeIt != m_meshNodesWithBackstopData.end();
            };
        m_config.m_getEntityIdCallback = [this]()
            {
                return GetEntityId();
            };
    }

    EditorClothComponent::~EditorClothComponent() = default;

    void EditorClothComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ClothMeshService", 0x6ffcbca5));
    }

    void EditorClothComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("MeshService", 0x71d8a455));
    }

    void EditorClothComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    const MeshNodeList& EditorClothComponent::GetMeshNodeList() const
    {
        return m_meshNodeList;
    }

    const AZStd::unordered_set<AZStd::string>& EditorClothComponent::GetMeshNodesWithBackstopData() const
    {
        return m_meshNodesWithBackstopData;
    }

    void EditorClothComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<ClothComponent>(m_config);
    }

    void EditorClothComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();

        AZ::Render::MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void EditorClothComponent::Deactivate()
    {
        AZ::Render::MeshComponentNotificationBus::Handler::BusDisconnect();

        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        OnModelPreDestroy();
    }

    void EditorClothComponent::OnModelReady(
        const AZ::Data::Asset<AZ::RPI::ModelAsset>& asset,
        [[maybe_unused]] const AZ::Data::Instance<AZ::RPI::Model>& model)
    {
        if (!asset.IsReady())
        {
            return;
        }

        m_meshNodeList.clear();
        m_meshNodesWithBackstopData.clear();

        AZStd::unique_ptr<AssetHelper> assetHelper = AssetHelper::CreateAssetHelper(GetEntityId());
        if (assetHelper)
        {
            // Gather cloth mesh node list
            assetHelper->GatherClothMeshNodes(m_meshNodeList);

            for (const auto& meshNode : m_meshNodeList)
            {
                if (ContainsBackstopData(assetHelper.get(), meshNode))
                {
                    m_meshNodesWithBackstopData.insert(meshNode);
                }
            }
        }

        if (m_meshNodeList.empty())
        {
            m_meshNodeList.emplace_back(Internal::StatusMessageNoClothNodes);
            m_config.m_meshNode = Internal::StatusMessageNoClothNodes;
        }
        else
        {
            bool foundNode = AZStd::find(m_meshNodeList.cbegin(), m_meshNodeList.cend(), m_config.m_meshNode) != m_meshNodeList.cend();

            if (!foundNode && !m_lastKnownMeshNode.empty())
            {
                // Check the if the mesh node previously selected is still part of the mesh list
                // to keep using it and avoid the user to select it again in the combo box.
                foundNode = AZStd::find(m_meshNodeList.cbegin(), m_meshNodeList.cend(), m_lastKnownMeshNode) != m_meshNodeList.cend();
                if (foundNode)
                {
                    m_config.m_meshNode = m_lastKnownMeshNode;
                }
            }

            // If the mesh node is not in the list then add and use an option
            // that tells the user to select the node.
            if (!foundNode)
            {
                m_meshNodeList.insert(m_meshNodeList.begin(), Internal::StatusMessageSelectNode);
                m_config.m_meshNode = Internal::StatusMessageSelectNode;
            }
        }

        m_lastKnownMeshNode = "";

        if (m_simulateInEditor)
        {
            m_clothComponentMesh = AZStd::make_unique<ClothComponentMesh>(GetEntityId(), m_config);
        }

        // Refresh UI
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
            AzToolsFramework::Refresh_EntireTree);
    }

    void EditorClothComponent::OnModelPreDestroy()
    {
        if (m_config.m_meshNode != Internal::StatusMessageSelectNode &&
            m_config.m_meshNode != Internal::StatusMessageNoAsset &&
            m_config.m_meshNode != Internal::StatusMessageNoClothNodes)
        {
            m_lastKnownMeshNode = m_config.m_meshNode;
        }

        m_meshNodeList = { {Internal::StatusMessageNoAsset} };
        m_config.m_meshNode = Internal::StatusMessageNoAsset;

        m_clothComponentMesh.reset();

        m_meshNodesWithBackstopData.clear();

        // Refresh UI
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
            AzToolsFramework::Refresh_EntireTree);
    }

    bool EditorClothComponent::IsSimulatedInEditor() const
    {
        return m_simulateInEditor;
    }

    AZ::u32 EditorClothComponent::OnSimulatedInEditorToggled()
    {
        m_simulateInEditor = !m_simulateInEditor;

        m_clothComponentMesh = AZStd::make_unique<ClothComponentMesh>(GetEntityId(), m_config);

        if (!m_simulateInEditor)
        {
            // Since the instance was just created this will restore the model
            // to its original position before cloth simulation.
            m_clothComponentMesh->CopyRenderDataToModel();

            m_clothComponentMesh.reset();
        }

        return AZ::Edit::PropertyRefreshLevels::None;
    }

    void EditorClothComponent::OnConfigurationChanged()
    {
        if (m_clothComponentMesh)
        {
            m_clothComponentMesh->UpdateConfiguration(GetEntityId(), m_config);
        }
    }

    bool EditorClothComponent::ContainsBackstopData(AssetHelper* assetHelper, const AZStd::string& meshNode) const
    {
        if (!assetHelper)
        {
            return false;
        }

        // Obtain cloth mesh info
        MeshNodeInfo meshNodeInfo;
        MeshClothInfo meshClothInfo;
        bool clothInfoObtained = assetHelper->ObtainClothMeshNodeInfo(meshNode,
            meshNodeInfo, meshClothInfo);
        if (!clothInfoObtained)
        {
            return false;
        }

        return AZStd::any_of(
            meshClothInfo.m_backstopData.cbegin(),
            meshClothInfo.m_backstopData.cend(),
            [](const AZ::Vector2& backstop)
            {
                const float backstopRadius = backstop.GetY();
                return backstopRadius > 0.0f;
            });
    }
} // namespace NvCloth
