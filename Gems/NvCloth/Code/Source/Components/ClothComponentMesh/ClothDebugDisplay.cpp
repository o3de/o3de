/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>

#include <AzFramework/Viewport/ViewportColors.h>

#include <Components/ClothComponentMesh/ClothDebugDisplay.h>
#include <Components/ClothComponentMesh/ActorClothColliders.h>
#include <Components/ClothComponentMesh/ClothConstraints.h>
#include <Components/ClothComponentMesh/ClothComponentMesh.h>

#include <NvCloth/ICloth.h>

namespace NvCloth
{
    AZ_CVAR(int32_t, cloth_DebugDraw, 0, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Draw cloth wireframe mesh:\n"
        " 0 - Disabled\n"
        " 1 - Cloth wireframe and particle weights");

    AZ_CVAR(int32_t, cloth_DebugDrawNormals, 0, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Draw cloth normals:\n"
        " 0 - Disabled\n"
        " 1 - Cloth normals\n"
        " 2 - Cloth normals, tangents and bitangents");

    AZ_CVAR(int32_t, cloth_DebugDrawColliders, 0, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Draw cloth colliders:\n"
        " 0 - Disabled\n"
        " 1 - Cloth colliders");

    AZ_CVAR(int32_t, cloth_DebugDrawMotionConstraints, 0, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Draw cloth motion constraints:\n"
        " 0 - Disabled\n"
        " 1 - Cloth motion constraints");

    AZ_CVAR(int32_t, cloth_DebugDrawBackstop, 0, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Draw cloth backstop:\n"
        " 0 - Disabled\n"
        " 1 - Cloth backstop");

    ClothDebugDisplay::ClothDebugDisplay(ClothComponentMesh* clothComponentMesh)
        : m_clothComponentMesh(clothComponentMesh)
    {
        AZ_Assert(m_clothComponentMesh, "Invalid cloth component mesh");
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(m_clothComponentMesh->m_entityId);
    }

    ClothDebugDisplay::~ClothDebugDisplay()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
    }

    bool ClothDebugDisplay::IsDebugDrawEnabled() const
    {
        return cloth_DebugDraw > 0
            || cloth_DebugDrawNormals > 0
            || cloth_DebugDrawColliders > 0
            || cloth_DebugDrawMotionConstraints > 0
            || cloth_DebugDrawBackstop > 0;
    }

    void ClothDebugDisplay::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_UNUSED(viewportInfo);

        if (!IsDebugDrawEnabled() || !m_clothComponentMesh->m_cloth)
        {
            return;
        }

        AZ::Transform entityTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(entityTransform, m_clothComponentMesh->m_entityId, &AZ::TransformInterface::GetWorldTM);
        debugDisplay.PushMatrix(entityTransform);

        if (cloth_DebugDraw > 0)
        {
            DisplayParticles(debugDisplay);
            DisplayWireCloth(debugDisplay);
        }

        if (cloth_DebugDrawNormals > 0)
        {
            bool showTangents = (cloth_DebugDrawNormals > 1);
            DisplayNormals(debugDisplay, showTangents);
        }

        if (cloth_DebugDrawColliders > 0)
        {
            DisplayColliders(debugDisplay);
        }

        if (cloth_DebugDrawMotionConstraints > 0)
        {
            DisplayMotionConstraints(debugDisplay);
        }

        if (cloth_DebugDrawBackstop > 0)
        {
            DisplaySeparationConstraints(debugDisplay);
        }

        debugDisplay.PopMatrix();
    }

    void ClothDebugDisplay::DisplayParticles(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const float particleAlpha = 1.0f;
        const float particleRadius = 0.007f;

        const auto& clothRenderParticles = m_clothComponentMesh->m_cloth->GetParticles();

        for(const auto& particle : clothRenderParticles)
        {
            const AZ::Vector4 color = AZ::Vector4::CreateFromVector3AndFloat(AZ::Vector3(particle.GetW()), particleAlpha);
            debugDisplay.SetColor(color);
            debugDisplay.DrawBall(particle.GetAsVector3(), particleRadius, false/*drawShaded*/);
        }
    }

    void ClothDebugDisplay::DisplayWireCloth(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const float lineAlpha = 1.0f;

        const auto& clothIndices = m_clothComponentMesh->m_cloth->GetInitialIndices();
        const auto& clothRenderParticles = m_clothComponentMesh->m_cloth->GetParticles();

        const size_t numIndices = clothIndices.size();
        if (numIndices % 3 != 0)
        {
            AZ_Warning("ClothDebugDisplay", false, 
                "Cloth indices contains a list of triangles but its count (%zu) is not a multiple of 3.", numIndices);
            return;
        }

        for (size_t index = 0; index < numIndices; index += 3)
        {
            const SimIndexType& vertexIndex0 = clothIndices[index + 0];
            const SimIndexType& vertexIndex1 = clothIndices[index + 1];
            const SimIndexType& vertexIndex2 = clothIndices[index + 2];
            const SimParticleFormat& particle0 = clothRenderParticles[vertexIndex0];
            const SimParticleFormat& particle1 = clothRenderParticles[vertexIndex1];
            const SimParticleFormat& particle2 = clothRenderParticles[vertexIndex2];
            const AZ::Vector3 position0 = particle0.GetAsVector3();
            const AZ::Vector3 position1 = particle1.GetAsVector3();
            const AZ::Vector3 position2 = particle2.GetAsVector3();
            const AZ::Vector4 color0 = AZ::Vector4::CreateFromVector3AndFloat(AZ::Vector3(particle0.GetW()), lineAlpha);
            const AZ::Vector4 color1 = AZ::Vector4::CreateFromVector3AndFloat(AZ::Vector3(particle1.GetW()), lineAlpha);
            const AZ::Vector4 color2 = AZ::Vector4::CreateFromVector3AndFloat(AZ::Vector3(particle2.GetW()), lineAlpha);
            debugDisplay.DrawLine(position0, position1, color0, color1);
            debugDisplay.DrawLine(position1, position2, color1, color2);
            debugDisplay.DrawLine(position2, position0, color2, color0);
        }
    }

    void ClothDebugDisplay::DisplayNormals(AzFramework::DebugDisplayRequests& debugDisplay, bool showTangents)
    {
        const auto& clothRenderData = m_clothComponentMesh->GetRenderData();

        const auto& clothRenderParticles = clothRenderData.m_particles;
        const auto& clothRenderTangents = clothRenderData.m_tangents;
        const auto& clothRenderBitangents = clothRenderData.m_bitangents;
        const auto& clothRenderNormals = clothRenderData.m_normals;

        if (clothRenderParticles.size() != clothRenderNormals.size())
        {
            AZ_Warning("ClothDebugDisplay", false,
                "Number of cloth particles (%zu) doesn't match with the number of normals (%zu).", 
                clothRenderParticles.size(), clothRenderNormals.size());
            return;
        }

        const float normalLength = 0.05f;
        const float tangentLength = 0.05f;
        const float bitangentLength = 0.05f;
        const AZ::Vector4 colorNormal = AZ::Colors::Blue.GetAsVector4();
        const AZ::Vector4 colorTangent = AZ::Colors::Red.GetAsVector4();
        const AZ::Vector4 colorBitangent = AZ::Colors::Green.GetAsVector4();

        for (size_t i = 0; i < clothRenderParticles.size(); ++i)
        {
            if (m_clothComponentMesh->m_meshRemappedVertices[i] < 0)
            {
                // Removed particle
                continue;
            }

            const AZ::Vector3 position = clothRenderParticles[i].GetAsVector3();

            debugDisplay.DrawLine(position, position + normalLength * clothRenderNormals[i], colorNormal, colorNormal);

            if (showTangents)
            {
                debugDisplay.DrawLine(position, position + tangentLength * clothRenderTangents[i], colorTangent, colorTangent);
                debugDisplay.DrawLine(position, position + bitangentLength * clothRenderBitangents[i], colorBitangent, colorBitangent);
            }
        }
    }

    void ClothDebugDisplay::DisplayColliders(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (!m_clothComponentMesh->m_actorClothColliders)
        {
            return;
        }

        for (const SphereCollider& collider : m_clothComponentMesh->m_actorClothColliders->GetSphereColliders())
        {
            DrawSphere(debugDisplay, collider.m_radius, collider.m_currentModelSpaceTransform.GetTranslation(), AzFramework::ViewportColors::DeselectedColor);
        }

        for (const CapsuleCollider& collider : m_clothComponentMesh->m_actorClothColliders->GetCapsuleColliders())
        {
            DrawCapsule(debugDisplay, collider.m_radius, collider.m_height, collider.m_currentModelSpaceTransform, AzFramework::ViewportColors::DeselectedColor);
        }
    }

    void ClothDebugDisplay::DisplayMotionConstraints(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const AZ::Vector4 particleColor = AZ::Colors::Green.GetAsVector4();
        const AZ::Vector4 staticPraticleColor = AZ::Colors::Black.GetAsVector4();
        const AZ::Vector4 lineColor = AZ::Colors::Magenta.GetAsVector4();
        const float ballsize = 0.008f;

        for (const auto& constraint : m_clothComponentMesh->m_motionConstraints)
        {
            const AZ::Vector3 position = constraint.GetAsVector3();
            const float radius = constraint.GetW();

            debugDisplay.SetColor((radius > 0.0f) ? particleColor : staticPraticleColor);
            debugDisplay.DrawBall(position, ballsize, false/*drawShaded*/);
            debugDisplay.DrawLine(position, position + AZ::Vector3::CreateAxisY(radius), lineColor, lineColor);
        }
    }

    void ClothDebugDisplay::DisplaySeparationConstraints(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (m_clothComponentMesh->m_separationConstraints.empty())
        {
            return;
        }

        const AZ::Vector4 sphereColor = AZ::Colors::Red.GetAsVector4();
        const AZ::Vector4 lineColor = AZ::Colors::Aqua.GetAsVector4();

        const auto& particles = m_clothComponentMesh->m_cloth->GetParticles();

        for (size_t i = 0; i < particles.size(); ++i)
        {
            const AZ::Vector3 position = m_clothComponentMesh->m_separationConstraints[i].GetAsVector3();
            const float radius = m_clothComponentMesh->m_separationConstraints[i].GetW();

            DrawSphere(debugDisplay, radius, position, sphereColor);

            debugDisplay.DrawLine(position, particles[i].GetAsVector3(), lineColor, lineColor);
        }
    }

    void ClothDebugDisplay::DrawSphere(
        AzFramework::DebugDisplayRequests& debugDisplay,
        float radius,
        const AZ::Vector3& position,
        const AZ::Color& color)
    {
        debugDisplay.SetColor(color);
        debugDisplay.DrawBall(position, radius, false/*drawShaded*/);
        debugDisplay.SetColor(AzFramework::ViewportColors::WireColor);
        debugDisplay.DrawWireSphere(position, radius);
    }

    void ClothDebugDisplay::DrawCapsule(
        AzFramework::DebugDisplayRequests& debugDisplay,
        float radius, float height,
        const AZ::Transform& transform,
        [[maybe_unused]] const AZ::Color& color)
    {
        const float heightStraightSection = AZStd::max(AZ::Constants::FloatEpsilon, height - 2.0f * radius);

        debugDisplay.SetColor(AzFramework::ViewportColors::WireColor);
        debugDisplay.DrawWireCapsule(transform.GetTranslation(), transform.GetBasisZ(), radius, heightStraightSection);
    }
} // namespace NvCloth
