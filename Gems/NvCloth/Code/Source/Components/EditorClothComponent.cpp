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
#include <AzFramework/Translation/TranslationDef.h>

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
                    QT_TRANSLATE_NOOP("NvCloth", "Cloth"),
                    QT_TRANSLATE_NOOP("NvCloth", "The mesh node behaves like a piece of cloth."))
                     ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Cloth.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Cloth.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/physx/cloth/")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->UIElement(AZ::Edit::UIHandlers::CheckBox,
                        QT_TRANSLATE_NOOP("NvCloth", "Simulate in editor"),
                        QT_TRANSLATE_NOOP("NvCloth", "Enables cloth simulation in editor when set."))
                        ->Attribute(AZ::Edit::Attributes::CheckboxDefaultValue, &EditorClothComponent::IsSimulatedInEditor)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorClothComponent::OnSimulatedInEditorToggled)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorClothComponent::m_config)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorClothComponent::OnConfigurationChanged)
                    ;

                editContext->Class<ClothConfiguration>(
                    QT_TRANSLATE_NOOP("NvCloth", "Cloth Configuration"),
                    QT_TRANSLATE_NOOP("NvCloth", "Configuration for cloth simulation."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    // Mesh Node
                    ->DataElement(Editor::MeshNodeSelector, &ClothConfiguration::m_meshNode,
                        QT_TRANSLATE_NOOP("NvCloth", "Mesh node"),
                        QT_TRANSLATE_NOOP("NvCloth", "List of mesh nodes with cloth simulation data. These are the nodes selected inside Cloth Modifiers in Scene Settings."))
                        ->Attribute(AZ::Edit::UIHandlers::EntityId, &ClothConfiguration::m_entityId)
                        ->Attribute(AZ::Edit::Attributes::StringList, &ClothConfiguration::m_meshNodeList)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    // Mass and Gravity
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_mass,
                        QT_TRANSLATE_NOOP("NvCloth", "Mass"),
                        QT_TRANSLATE_NOOP("NvCloth", "Mass scale applied to all particles."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_useCustomGravity,
                        QT_TRANSLATE_NOOP("NvCloth", "Custom Gravity"),
                        QT_TRANSLATE_NOOP("NvCloth", "When enabled it allows to set a custom gravity value for this cloth."))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_customGravity,
                        QT_TRANSLATE_NOOP("NvCloth", "Gravity"),
                        QT_TRANSLATE_NOOP("NvCloth", "Gravity applied to particles."))
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ClothConfiguration::IsUsingWorldBusGravity)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_gravityScale,
                        QT_TRANSLATE_NOOP("NvCloth", "Gravity Scale"),
                        QT_TRANSLATE_NOOP("NvCloth", "Use this parameter to scale the gravity applied to particles."))

                    // Global stiffness frequency
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_stiffnessFrequency,
                        QT_TRANSLATE_NOOP("NvCloth", "Stiffness frequency"),
                        QT_TRANSLATE_NOOP("NvCloth", "Stiffness exponent per second applied to damping, damping dragging, wind dragging, wind lifting, self collision stiffness, fabric stiffness, fabric compression, fabric stretch and tether constraint stiffness."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    
                    // Motion Constraints
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("NvCloth", "Motion constraints"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_motionConstraintsMaxDistance,
                        QT_TRANSLATE_NOOP("NvCloth", "Max Distance"),
                        QT_TRANSLATE_NOOP("NvCloth", "Maximum distance for motion constraints to limit particles movement during simulation."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, Internal::AttributeSuffixMetersUnit)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_motionConstraintsScale,
                        QT_TRANSLATE_NOOP("NvCloth", "Scale"),
                        QT_TRANSLATE_NOOP("NvCloth", "Scale value applied to all motion constraints."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_motionConstraintsBias,
                        QT_TRANSLATE_NOOP("NvCloth", "Bias"),
                        QT_TRANSLATE_NOOP("NvCloth", "Bias value added to all motion constraints."))
                        ->Attribute(AZ::Edit::Attributes::Suffix, Internal::AttributeSuffixMetersUnit)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_motionConstraintsStiffness,
                        QT_TRANSLATE_NOOP("NvCloth", "Stiffness"),
                        QT_TRANSLATE_NOOP("NvCloth", "Stiffness for motion constraints."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    
                    // Backstop
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("NvCloth", "Backstop"))
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ClothConfiguration::m_hasBackstopData)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_backstopRadius,
                        QT_TRANSLATE_NOOP("NvCloth", "Radius"),
                        QT_TRANSLATE_NOOP("NvCloth", "Maximum radius that will prevent the associated cloth particle from moving into that area."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, Internal::AttributeSuffixMetersUnit)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ClothConfiguration::m_hasBackstopData)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_backstopBackOffset,
                        QT_TRANSLATE_NOOP("NvCloth", "Back offset"),
                        QT_TRANSLATE_NOOP("NvCloth", "Maximum offset for backstop spheres behind the cloth."))
                        ->Attribute(AZ::Edit::Attributes::Suffix, Internal::AttributeSuffixMetersUnit)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ClothConfiguration::m_hasBackstopData)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_backstopFrontOffset,
                        QT_TRANSLATE_NOOP("NvCloth", "Front offset"),
                        QT_TRANSLATE_NOOP("NvCloth", "Maximum offset for backstop spheres in front of the cloth."))
                        ->Attribute(AZ::Edit::Attributes::Suffix, Internal::AttributeSuffixMetersUnit)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ClothConfiguration::m_hasBackstopData)

                    // Damping
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("NvCloth", "Damping"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_damping,
                        QT_TRANSLATE_NOOP("NvCloth", "Damping"),
                        QT_TRANSLATE_NOOP("NvCloth", "Damping of particle velocity.\n"
                        "0: Velocity is unaffected\n"
                        "1: Velocity is zeroed"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_linearDrag,
                        QT_TRANSLATE_NOOP("NvCloth", "Linear drag"),
                        QT_TRANSLATE_NOOP("NvCloth", "Portion of velocity applied to particles.\n"
                        "0: Particles is unaffected\n"
                        "1: Damped global particle velocity"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_angularDrag,
                        QT_TRANSLATE_NOOP("NvCloth", "Angular drag"),
                        QT_TRANSLATE_NOOP("NvCloth", "Portion of angular velocity applied to turning particles.\n"
                        "0: Particles is unaffected\n"
                        "1: Damped global particle angular velocity"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)

                    // Inertia
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("NvCloth", "Inertia"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_linearInteria,
                        QT_TRANSLATE_NOOP("NvCloth", "Linear"),
                        QT_TRANSLATE_NOOP("NvCloth", "Portion of acceleration applied to particles.\n"
                        "0: Particles are unaffected\n"
                        "1: Physically correct"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_angularInteria,
                        QT_TRANSLATE_NOOP("NvCloth", "Angular"),
                        QT_TRANSLATE_NOOP("NvCloth", "Portion of angular acceleration applied to turning particles.\n"
                        "0: Particles are unaffected\n"
                        "1: Physically correct"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_centrifugalInertia,
                        QT_TRANSLATE_NOOP("NvCloth", "Centrifugal"),
                        QT_TRANSLATE_NOOP("NvCloth", "Portion of angular velocity applied to turning particles.\n"
                        "0: Particles are unaffected\n"
                        "1: Physically correct"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)

                    // Wind
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("NvCloth", "Wind"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_useCustomWindVelocity,
                        QT_TRANSLATE_NOOP("NvCloth", "Enable local wind velocity"),
                        QT_TRANSLATE_NOOP("NvCloth", "When enabled it allows to set a custom wind velocity value for this cloth, otherwise using wind velocity from Physics::WindBus.\n"
                        "Wind is disabled when both air coefficients are zero."))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_windVelocity,
                        QT_TRANSLATE_NOOP("NvCloth", "Local velocity"),
                        QT_TRANSLATE_NOOP("NvCloth", "Wind in global coordinates acting on cloth's triangles. Disabled when both air coefficients are zero.\n"
                        "NOTE: A combination of high values in wind properties can cause unstable results."))
                        ->Attribute(AZ::Edit::Attributes::Min, -50.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 50.0f)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ClothConfiguration::IsUsingWindBus )
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_airDragCoefficient,
                        QT_TRANSLATE_NOOP("NvCloth", "Air drag coefficient"),
                        QT_TRANSLATE_NOOP("NvCloth", "Amount of air dragging.\n"
                        "NOTE: A combination of high values in wind properties can cause unstable results."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_airLiftCoefficient,
                        QT_TRANSLATE_NOOP("NvCloth", "Air lift coefficient"),
                        QT_TRANSLATE_NOOP("NvCloth", "Amount of air lifting.\n"
                        "NOTE: A combination of high values in wind properties can cause unstable results."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_fluidDensity,
                        QT_TRANSLATE_NOOP("NvCloth", "Air Density"),
                        QT_TRANSLATE_NOOP("NvCloth", "Density of air used for air drag and lift calculations.\n"
                        "NOTE: A combination of high values in wind properties can cause unstable results."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01f)

                    // Collision
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("NvCloth", "Collision"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_collisionFriction,
                        QT_TRANSLATE_NOOP("NvCloth", "Friction"),
                        QT_TRANSLATE_NOOP("NvCloth", "Amount of friction with colliders.\n"
                        "0: No friction\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_collisionMassScale,
                        QT_TRANSLATE_NOOP("NvCloth", "Mass scale"),
                        QT_TRANSLATE_NOOP("NvCloth", "Controls how quickly mass is increased during collisions.\n"
                        "0: No mass scaling\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_continuousCollisionDetection,
                        QT_TRANSLATE_NOOP("NvCloth", "Continuous detection"),
                        QT_TRANSLATE_NOOP("NvCloth", "Continuous collision detection improves collision by computing time of impact between cloth particles and colliders."
                        "The increase in quality comes with a cost in performance, it's recommended to use only when required."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_collisionAffectsStaticParticles,
                        QT_TRANSLATE_NOOP("NvCloth", "Affects static particles"),
                        QT_TRANSLATE_NOOP("NvCloth", "When enabled colliders will move static particles (inverse mass 0)."))

                    // Self collision
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("NvCloth", "Self collision"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_selfCollisionDistance,
                        QT_TRANSLATE_NOOP("NvCloth", "Distance"),
                        QT_TRANSLATE_NOOP("NvCloth", "Meters that particles need to be separated from each other.\n"
                        "0: No self collision\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_selfCollisionStiffness,
                        QT_TRANSLATE_NOOP("NvCloth", "Stiffness"),
                        QT_TRANSLATE_NOOP("NvCloth", "Stiffness for the self collision constraints.\n"
                        "0: No self collision\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)

                    // Fabric stiffness
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("NvCloth", "Fabric stiffness"))
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_horizontalStiffness,
                        QT_TRANSLATE_NOOP("NvCloth", "Horizontal"),
                        QT_TRANSLATE_NOOP("NvCloth", "Stiffness value for horizontal constraints.\n"
                        "0: no horizontal constraints\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_horizontalStiffnessMultiplier,
                        QT_TRANSLATE_NOOP("NvCloth", "Horizontal multiplier"),
                        QT_TRANSLATE_NOOP("NvCloth", "Scale value for horizontal fabric compression and stretch limits.\n"
                        "0: No horizontal compression and stretch limits applied\n"
                        "1: Fully apply horizontal compression and stretch limits\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_verticalStiffness,
                        QT_TRANSLATE_NOOP("NvCloth", "Vertical"),
                        QT_TRANSLATE_NOOP("NvCloth", "Stiffness value for vertical constraints.\n"
                        "0: no vertical constraints\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_verticalStiffnessMultiplier,
                        QT_TRANSLATE_NOOP("NvCloth", "Vertical multiplier"),
                        QT_TRANSLATE_NOOP("NvCloth", "Scale value for vertical fabric compression and stretch limits.\n"
                        "0: No vertical compression and stretch limits applied\n"
                        "1: Fully apply vertical compression and stretch limits\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_bendingStiffness,
                        QT_TRANSLATE_NOOP("NvCloth", "Bending"),
                        QT_TRANSLATE_NOOP("NvCloth", "Stiffness value for bending constraints.\n"
                        "0: no bending constraints\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_bendingStiffnessMultiplier,
                        QT_TRANSLATE_NOOP("NvCloth", "Bending multiplier"),
                        QT_TRANSLATE_NOOP("NvCloth", "Scale value for bending fabric compression and stretch limits.\n"
                        "0: No bending compression and stretch limits applied\n"
                        "1: Fully apply bending compression and stretch limits\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_shearingStiffness,
                        QT_TRANSLATE_NOOP("NvCloth", "Shearing"),
                        QT_TRANSLATE_NOOP("NvCloth", "Stiffness value for shearing constraints.\n"
                        "0: no shearing constraints\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_shearingStiffnessMultiplier,
                        QT_TRANSLATE_NOOP("NvCloth", "Shearing multiplier"),
                        QT_TRANSLATE_NOOP("NvCloth", "Scale value for shearing fabric compression and stretch limits.\n"
                        "0: No shearing compression and stretch limits applied\n"
                        "1: Fully apply shearing compression and stretch limits\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)

                    // Fabric compression
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("NvCloth", "Fabric compression"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_horizontalCompressionLimit,
                        QT_TRANSLATE_NOOP("NvCloth", "Horizontal limit"),
                        QT_TRANSLATE_NOOP("NvCloth", "Compression limit for horizontal constraints. It's affected by fabric horizontal stiffness multiplier.\n"
                        "0: No compression\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_verticalCompressionLimit,
                        QT_TRANSLATE_NOOP("NvCloth", "Vertical limit"),
                        QT_TRANSLATE_NOOP("NvCloth", "Compression limit for vertical constraints. It's affected by fabric vertical stiffness multiplier.\n"
                        "0: No compression\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_bendingCompressionLimit,
                        QT_TRANSLATE_NOOP("NvCloth", "Bending limit"),
                        QT_TRANSLATE_NOOP("NvCloth", "Compression limit for bending constraints. It's affected by fabric bending stiffness multiplier.\n"
                        "0: No compression\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_shearingCompressionLimit,
                        QT_TRANSLATE_NOOP("NvCloth", "Shearing limit"),
                        QT_TRANSLATE_NOOP("NvCloth", "Compression limit for shearing constraints. It's affected by fabric shearing stiffness multiplier.\n"
                        "0: No compression\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    // Fabric stretch
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("NvCloth", "Fabric stretch"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_horizontalStretchLimit,
                        QT_TRANSLATE_NOOP("NvCloth", "Horizontal limit"),
                        QT_TRANSLATE_NOOP("NvCloth", "Stretch limit for horizontal constraints. It's affected by fabric horizontal stiffness multiplier."
                        "Reduce stiffness of tether constraints (or increase its scale) to allow cloth to stretch.\n"
                        "0: No stretching\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_verticalStretchLimit,
                        QT_TRANSLATE_NOOP("NvCloth", "Vertical limit"),
                        QT_TRANSLATE_NOOP("NvCloth", "Stretch limit for vertical constraints. It's affected by fabric vertical stiffness multiplier."
                        "Reduce stiffness of tether constraints (or increase its scale) to allow cloth to stretch.\n"
                        "0: No stretching\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_bendingStretchLimit,
                        QT_TRANSLATE_NOOP("NvCloth", "Bending limit"),
                        QT_TRANSLATE_NOOP("NvCloth", "Stretch limit for bending constraints. It's affected by fabric bending stiffness multiplier."
                        "Reduce stiffness of tether constraints (or increase its scale) to allow cloth to stretch.\n"
                        "0: No stretching\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_shearingStretchLimit,
                        QT_TRANSLATE_NOOP("NvCloth", "Shearing limit"),
                        QT_TRANSLATE_NOOP("NvCloth", "Stretch limit for shearing constraints. It's affected by fabric shearing stiffness multiplier."
                        "Reduce stiffness of tether constraints (or increase its scale) to allow cloth to stretch.\n"
                        "0: No stretching\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    // Tether constraints
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("NvCloth", "Tether constraints"))
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClothConfiguration::m_tetherConstraintStiffness,
                        QT_TRANSLATE_NOOP("NvCloth", "Stiffness"),
                        QT_TRANSLATE_NOOP("NvCloth", "Stiffness for tether constraints. Tether constraints are generated when the inverse mass data of the cloth (selected in the cloth modifier) has static particles.\n"
                        "0: No tether constraints applied\n"
                        "1: Makes the constraints behave springy\n"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_tetherConstraintScale,
                        QT_TRANSLATE_NOOP("NvCloth", "Scale"),
                        QT_TRANSLATE_NOOP("NvCloth", "Tether constraint scale"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    // Quality
                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("NvCloth", "Quality"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_solverFrequency,
                        QT_TRANSLATE_NOOP("NvCloth", "Solver frequency"),
                        QT_TRANSLATE_NOOP("NvCloth", "Target solver iterations per second. At least 1 iteration per frame will be solved regardless of the value set."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_accelerationFilterIterations,
                        QT_TRANSLATE_NOOP("NvCloth", "Acceleration filter iterations"),
                        QT_TRANSLATE_NOOP("NvCloth", "Number of iterations to average delta time factor used for gravity and external acceleration."))
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_removeStaticTriangles,
                        QT_TRANSLATE_NOOP("NvCloth", "Remove static triangles"),
                        QT_TRANSLATE_NOOP("NvCloth", "Removing static triangles improves performance by not taking into account triangles whose particles are all static.\n"
                        "The removed static particles will not be present for collision or self collision during simulation."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ClothConfiguration::m_updateNormalsOfStaticParticles,
                        QT_TRANSLATE_NOOP("NvCloth", "Update normals of static particles"),
                        QT_TRANSLATE_NOOP("NvCloth", "When enabled the normals of static particles will be updated according with the movement of the simulated mesh.\n"
                        "When disabled the static particles will keep the same normals as the original mesh."))
                    ;
            }
        }
    }

    EditorClothComponent::EditorClothComponent()
    {
        m_meshNodeList = { {Internal::StatusMessageNoAsset} };
        m_config.m_meshNodeList = m_meshNodeList;
    }

    EditorClothComponent::~EditorClothComponent() = default;

    void EditorClothComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ClothMeshService"));
    }

    void EditorClothComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("MeshService"));
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

        m_config.m_entityId = GetEntityId();
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

        UpdateConfigMeshNodeData();

        // Refresh UI
        InvalidatePropertyDisplay(AzToolsFramework::Refresh_EntireTree);
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

        UpdateConfigMeshNodeData();

        // Refresh UI
        InvalidatePropertyDisplay(AzToolsFramework::Refresh_EntireTree);
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

    void EditorClothComponent::UpdateConfigMeshNodeData()
    {
        // Update our config mesh node data based on changes to the associated Mesh component
        // This gets called after updating our internal data on OnModelReady and OnModelPreDestroy
        m_config.m_meshNodeList = m_meshNodeList;
        auto meshNodeIt = m_meshNodesWithBackstopData.find(m_config.m_meshNode);
        m_config.m_hasBackstopData = (meshNodeIt != m_meshNodesWithBackstopData.end());
    }
} // namespace NvCloth
