/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/*
 * The compiled-in preset library.
 *
 * Kept in its own file because it is data, not logic - the code that consumes it lives in
 * EntityPresets.cpp and should stay readable without scrolling past seventy table entries.
 *
 * TODO: most of this table names components that AzToolsFramework has no business knowing about -
 * Light, PostFX Layer and Mesh belong to Atom, not to the framework. They are only reachable here
 * because a preset names a component by its display string rather than by type, so there is no
 * link-time dependency, but the layering is still wrong. These should move out to Presets/*.json
 * inside the gems that own the components, which EntityPresets::FromGems() already reads, leaving
 * this file holding only presets built from components the framework itself provides.
 */

#include <AzToolsFramework/Entity/EntityPresets/EntityPresets.h>

namespace AzToolsFramework
{
    namespace EntityPresets
    {
        namespace
        {
            //! Property paths as the Edit Context spells them. Named because they are long, easy
            //! to mistype, and repeated across many presets.
            constexpr const char* LightTypePath = "Controller|Configuration|Light type";
            constexpr const char* LayerCategoryPath = "Controller|Configuration|Layer Category";
            constexpr const char* ModelAssetPath = "Controller|Configuration|Model Asset";

            //! Light type enum values, in the order the Light component reflects them.
            enum LightType : AZ::s64
            {
                LightSphere = 1,
                LightSpotDisk = 2,
                LightCapsule = 3,
                LightQuad = 4,
                LightPolygon = 5,
                LightSimplePoint = 6,
                LightSimpleSpot = 7
            };

            //! PostFX layer categories. The values are sort keys, not indices - a PostFX layer
            //! blends against others by category, and the large numbers space the tiers apart.
            constexpr AZ::s64 PostFxCategoryVolume = 5000000;

            PropertyValue IntValue(const AZ::s64 value)
            {
                PropertyValue property;
                property.m_type = PropertyValue::Type::Int;
                property.m_int = value;
                return property;
            }

            PropertyValue AssetValue(const char* productPath)
            {
                PropertyValue property;
                property.m_type = PropertyValue::Type::AssetPath;
                property.m_string = productPath;
                return property;
            }

            //! A preset that is just "an entity with these components on it, all left at default".
            Preset Simple(const char* name, const char* category, const AZStd::vector<const char*>& componentNames)
            {
                Preset preset;
                preset.m_name = name;
                preset.m_category = category;
                preset.m_builtIn = true;

                for (const char* componentName : componentNames)
                {
                    ComponentSpec component;
                    component.m_componentName = componentName;
                    preset.m_components.push_back(AZStd::move(component));
                }

                return preset;
            }

            //! A preset with a single component and a single property set on it - which covers
            //! every light and every PostFX volume in the table.
            Preset Configured(
                const char* name,
                const char* category,
                const char* componentName,
                const char* propertyPath,
                const PropertyValue& value)
            {
                Preset preset = Simple(name, category, { componentName });
                preset.m_components.front().m_properties.push_back(PropertyAssignment{ propertyPath, value });
                return preset;
            }

            //! An engine-shipped model on a Mesh component. The path is the *relative product*
            //! path in the asset cache, resolved to an AssetId at creation time, so a preset does
            //! not care where the engine or project lives on disk.
            Preset Mesh(const char* name, const char* productPath)
            {
                return Configured(name, "Meshes", "Mesh", ModelAssetPath, AssetValue(productPath));
            }

            AZStd::vector<Preset> BuildBuiltInPresets()
            {
                AZStd::vector<Preset> presets;

                // ── Lights ────────────────────────────────────────────────────────────
                presets.push_back(Configured("Point Light (Sphere)", "Lights", "Light", LightTypePath, IntValue(LightSphere)));
                presets.push_back(Configured("Point Light (Simple)", "Lights", "Light", LightTypePath, IntValue(LightSimplePoint)));
                presets.push_back(Configured("Spot Light (Disk)", "Lights", "Light", LightTypePath, IntValue(LightSpotDisk)));
                presets.push_back(Configured("Spot Light (Simple)", "Lights", "Light", LightTypePath, IntValue(LightSimpleSpot)));
                presets.push_back(Configured("Capsule Light", "Lights", "Light", LightTypePath, IntValue(LightCapsule)));
                presets.push_back(Configured("Quad Light", "Lights", "Light", LightTypePath, IntValue(LightQuad)));
                presets.push_back(Configured("Polygon Light", "Lights", "Light", LightTypePath, IntValue(LightPolygon)));
                presets.push_back(Simple("Directional Light", "Lights", { "Directional Light" }));

                // ── Environment ───────────────────────────────────────────────────────
                presets.push_back(Simple("Global Skylight (IBL)", "Environment", { "Global Skylight (IBL)" }));
                presets.push_back(Simple("HDRi Skybox", "Environment", { "HDRi Skybox" }));
                presets.push_back(Simple("Sky Atmosphere", "Environment", { "Sky Atmosphere" }));
                presets.push_back(Simple("Physical Sky", "Environment", { "Physical Sky" }));
                presets.push_back(Simple("Reflection Probe", "Environment", { "Reflection Probe" }));
                presets.push_back(Simple("Decal", "Environment", { "Decal" }));

                // ── Shapes ────────────────────────────────────────────────────────────
                presets.push_back(Simple("Box Shape", "Shapes", { "Box Shape" }));
                presets.push_back(Simple("Sphere Shape", "Shapes", { "Sphere Shape" }));
                presets.push_back(Simple("Capsule Shape", "Shapes", { "Capsule Shape" }));
                presets.push_back(Simple("Cylinder Shape", "Shapes", { "Cylinder Shape" }));
                presets.push_back(Simple("Tube Shape", "Shapes", { "Tube Shape" }));
                presets.push_back(Simple("Disk Shape", "Shapes", { "Disk Shape" }));
                presets.push_back(Simple("Polygon Prism Shape", "Shapes", { "Polygon Prism Shape" }));
                presets.push_back(Simple("Compound Shape", "Shapes", { "Compound Shape" }));

                // ── Physics ───────────────────────────────────────────────────────────
                presets.push_back(Simple("Static Rigid Body", "Physics", { "PhysX Static Rigid Body" }));
                presets.push_back(Simple("Character Controller", "Physics", { "PhysX Character Controller" }));

                // ── Rendering ─────────────────────────────────────────────────────────
                presets.push_back(Simple("Actor", "Rendering", { "Actor" }));
                presets.push_back(Simple("White Box", "Rendering", { "White Box" }));
                presets.push_back(Simple("Camera", "Rendering", { "Camera" }));
                presets.push_back(Simple("Camera Rig", "Rendering", { "Camera", "Camera Rig" }));

                // ── Meshes ────────────────────────────────────────────────────────────
                presets.push_back(Simple("Mesh (Empty)", "Meshes", { "Mesh" }));
                presets.push_back(Mesh("Ground Plane 512x512m", "objects/groundplane/groundplane_512x512m.fbx.azmodel"));
                presets.push_back(Mesh("Ground Plane 4x4m", "objects/shaderball/ground_plane_4x4m.fbx.azmodel"));
                presets.push_back(Mesh("Box 1x1", "objects/_primitives/_box_1x1.fbx.azmodel"));
                presets.push_back(Mesh("Sphere 1x1", "objects/_primitives/_sphere_1x1.fbx.azmodel"));
                presets.push_back(Mesh("Cylinder 1x1", "objects/_primitives/_cylinder_1x1.fbx.azmodel"));
                presets.push_back(Mesh("Cube", "materialeditor/viewportmodels/cube.fbx.azmodel"));
                presets.push_back(Mesh("Cone", "materialeditor/viewportmodels/cone.fbx.azmodel"));
                presets.push_back(Mesh("Cylinder", "materialeditor/viewportmodels/cylinder.fbx.azmodel"));
                presets.push_back(Mesh("Torus", "materialeditor/viewportmodels/torus.fbx.azmodel"));
                presets.push_back(Mesh("Beveled Cube", "materialeditor/viewportmodels/beveledcube.fbx.azmodel"));
                presets.push_back(Mesh("Beveled Cone", "materialeditor/viewportmodels/beveledcone.fbx.azmodel"));
                presets.push_back(Mesh("Beveled Cylinder", "materialeditor/viewportmodels/beveledcylinder.fbx.azmodel"));
                presets.push_back(Mesh("Plane 1x1", "materialeditor/viewportmodels/plane_1x1.fbx.azmodel"));
                presets.push_back(Mesh("Plane 3x3", "materialeditor/viewportmodels/plane_3x3.fbx.azmodel"));
                presets.push_back(Mesh("Platonic Sphere", "materialeditor/viewportmodels/platonicsphere.fbx.azmodel"));
                presets.push_back(Mesh("Polar Sphere", "materialeditor/viewportmodels/polarsphere.fbx.azmodel"));
                presets.push_back(Mesh("Quad Sphere", "materialeditor/viewportmodels/quadsphere.fbx.azmodel"));
                presets.push_back(Mesh("Shaderball", "materialeditor/viewportmodels/shaderball.fbx.azmodel"));
                presets.push_back(Mesh("Shaderball 1m", "objects/shaderball/shaderball_default_1m.fbx.azmodel"));
                presets.push_back(Mesh("Caduceus", "materialeditor/viewportmodels/caduceus.fbx.azmodel"));
                presets.push_back(Mesh("Hermanubis", "materialeditor/viewportmodels/hermanubis.fbx.azmodel"));

                // ── PostFX ────────────────────────────────────────────────────────────
                //
                // The effects all need a PostFX Layer alongside them - the layer is what places
                // the effect in the blend order; the effect component on its own does nothing.
                presets.push_back(Simple("PostFX Layer", "PostFX", { "PostFX Layer" }));
                presets.push_back(Configured(
                    "PostFX Volume", "PostFX", "PostFX Layer", LayerCategoryPath, IntValue(PostFxCategoryVolume)));

                presets.push_back(Simple("Exposure Control", "PostFX", { "PostFX Layer", "Exposure Control" }));

                {
                    // A volume-scoped exposure override: layer marks it as a volume, the shape
                    // weight modifier is what fades it in as the camera enters the shape.
                    Preset preset = Simple(
                        "Exposure Volume", "PostFX",
                        { "PostFX Layer", "Exposure Control", "PostFX Shape Weight Modifier" });
                    preset.m_components.front().m_properties.push_back(
                        PropertyAssignment{ LayerCategoryPath, IntValue(PostFxCategoryVolume) });
                    presets.push_back(AZStd::move(preset));
                }

                presets.push_back(Simple("Bloom", "PostFX", { "PostFX Layer", "Bloom" }));
                presets.push_back(Simple("SSAO", "PostFX", { "PostFX Layer", "SSAO" }));
                presets.push_back(Simple("Chromatic Aberration", "PostFX", { "PostFX Layer", "Chromatic Aberration" }));
                presets.push_back(Simple("Vignette", "PostFX", { "PostFX Layer", "Vignette" }));
                presets.push_back(Simple("HDR Color Grading", "PostFX", { "PostFX Layer", "HDR Color Grading" }));

                // ── Audio ─────────────────────────────────────────────────────────────
                presets.push_back(Simple("Audio Trigger", "Audio", { "Audio Trigger" }));
                presets.push_back(Simple("Audio Listener", "Audio", { "Audio Listener" }));
                presets.push_back(Simple("Audio Environment", "Audio", { "Audio Environment" }));

                // ── Scripting ─────────────────────────────────────────────────────────
                presets.push_back(Simple("Script Canvas", "Scripting", { "Script Canvas" }));
                presets.push_back(Simple("Lua Script", "Scripting", { "Lua Script" }));

                // ── Vegetation ────────────────────────────────────────────────────────
                presets.push_back(Simple("Vegetation Spawner", "Vegetation", { "Vegetation Layer Spawner" }));

                return presets;
            }
        } // namespace

        const AZStd::vector<Preset>& BuiltIn()
        {
            static const AZStd::vector<Preset> presets = BuildBuiltInPresets();
            return presets;
        }
    } // namespace EntityPresets
} // namespace AzToolsFramework
