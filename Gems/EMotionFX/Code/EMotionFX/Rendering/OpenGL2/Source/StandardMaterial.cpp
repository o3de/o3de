/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector2.h>
#include "StandardMaterial.h"
#include "GraphicsManager.h"
#include "glactor.h"
#include <EMotionFX/Source/TransformData.h>
#include <MCore/Source/AzCoreConversions.h>


namespace RenderGL
{
    // constructor
    StandardMaterial::StandardMaterial(GLActor* actor)
        : Material(actor)
    {
        m_material           = nullptr;
        m_activeShader       = nullptr;
        m_attributesUpdated  = true;

        m_diffuseMap     = GetGraphicsManager()->GetTextureCache()->GetWhiteTexture();
        m_specularMap    = GetGraphicsManager()->GetTextureCache()->GetWhiteTexture();
        m_normalMap      = GetGraphicsManager()->GetTextureCache()->GetDefaultNormalTexture();

        SetAttribute(LIGHTING,  true);
        SetAttribute(SKINNING,  false);
        SetAttribute(SHADOWS,   false);
        SetAttribute(TEXTURING, true);
    }


    // destructor
    StandardMaterial::~StandardMaterial()
    {
    }


    // activate the material
    void StandardMaterial::Activate(uint32 flags)
    {
        // update shader attributes
        UpdateShader();

        // check if the shader is valid and return in case it's not
        if (m_activeShader == nullptr)
        {
            return;
        }

        // get the graphics manager shortcut
        RenderGL::GraphicsManager* gfx = GetGraphicsManager();

        if (flags & GLOBAL)
        {
            m_activeShader->Activate();

            // vertex attributes
            uint32 stride = m_attributes[SKINNING] ? sizeof(SkinnedVertex) : sizeof(StandardVertex);

            static char* structStart  = reinterpret_cast<char*>(reinterpret_cast<SkinnedVertex*>(static_cast<char*>(0)));
            static size_t offsetOfNormal = static_cast<size_t>((reinterpret_cast<char*>(&static_cast<SkinnedVertex*>(0)->m_normal)) - structStart);
            static size_t offsetOfTangent  = static_cast<size_t>((reinterpret_cast<char*>(&static_cast<SkinnedVertex*>(0)->m_tangent)) - structStart);
            static size_t offsetOfUV       = static_cast<size_t>((reinterpret_cast<char*>(&static_cast<SkinnedVertex*>(0)->m_uv)) - structStart);
            static size_t offsetOfWeights  = static_cast<size_t>((reinterpret_cast<char*>(&static_cast<SkinnedVertex*>(0)->m_weights)) - structStart);
            static size_t offsetOfBoneIndices = static_cast<size_t>((reinterpret_cast<char*>(&static_cast<SkinnedVertex*>(0)->m_boneIndices)) - structStart);

            m_activeShader->SetAttribute("inPosition", 4, GL_FLOAT, stride, 0);
            m_activeShader->SetAttribute("inNormal",  4, GL_FLOAT, stride, offsetOfNormal);
            m_activeShader->SetAttribute("inTangent", 4, GL_FLOAT, stride, offsetOfTangent);
            m_activeShader->SetAttribute("inUV", 2, GL_FLOAT, stride, offsetOfUV);

            // vertex weights & indices
            if (m_attributes[SKINNING])
            {
                m_activeShader->SetAttribute("inWeights", 4, GL_FLOAT, stride, offsetOfWeights);
                m_activeShader->SetAttribute("inIndices", 4, GL_FLOAT, stride, offsetOfBoneIndices);
            }

            // set the view projection matrix
            MCommon::Camera* camera = GetGraphicsManager()->GetCamera();
            m_activeShader->SetUniform("matViewProj", camera->GetViewProjMatrix());
            m_activeShader->SetUniform("matView", camera->GetViewMatrix());

            {
                AZ::Vector3 mainLightDir(0.0f, -1.0f, 0.0f);
                mainLightDir = AZ::Matrix3x3::CreateRotationX(MCore::Math::DegreesToRadians(gfx->GetMainLightAngleB())) * AZ::Matrix3x3::CreateRotationZ(MCore::Math::DegreesToRadians(gfx->GetMainLightAngleA())) * mainLightDir;
                mainLightDir.Normalize();
                m_activeShader->SetUniform("mainLightDir", mainLightDir);
                m_activeShader->SetUniform("skyColor", m_actor->GetSkyColor() * gfx->GetMainLightIntensity());
                m_activeShader->SetUniform("groundColor", m_actor->GetGroundColor());
                m_activeShader->SetUniform("eyePoint", camera->GetPosition());

                AZ::Vector3 rimLightDir = MCore::GetUp(camera->GetViewMatrix());
                rimLightDir = AZ::Matrix3x3::CreateRotationZ(MCore::Math::DegreesToRadians(gfx->GetRimAngle())) * rimLightDir;
                rimLightDir.Normalize();
                m_activeShader->SetUniform("rimLightDir", rimLightDir);

                m_activeShader->SetUniform("rimLightFactor", gfx->GetRimIntensity());
                m_activeShader->SetUniform("rimWidth",       gfx->GetRimWidth());
                m_activeShader->SetUniform("rimLightColor",  gfx->GetRimColor());
            }
        }

        // Local settings
        if (flags & LOCAL)
        {
            EMotionFX::StandardMaterial* stdMaterial = (m_material->GetType() == EMotionFX::StandardMaterial::TYPE_ID) ? static_cast<EMotionFX::StandardMaterial*>(m_material) : nullptr;

            if (m_diffuseMap == nullptr || m_diffuseMap == gfx->GetTextureCache()->GetWhiteTexture() && stdMaterial)
            {
                m_activeShader->SetUniform("diffuseColor", stdMaterial->GetDiffuse());
            }
            else
            {
                m_activeShader->SetUniform("diffuseColor", MCore::RGBAColor(1.0f, 1.0f, 1.0f, 1.0f));
            }

            {
                if (stdMaterial)
                {
                    MCore::RGBAColor specularColor = stdMaterial->GetSpecular() * (stdMaterial->GetShineStrength() * gfx->GetMainLightIntensity() * gfx->GetSpecularIntensity());
                    m_activeShader->SetUniform("specularPower", stdMaterial->GetShine());
                    m_activeShader->SetUniform("lightSpecular", specularColor);
                }
                else
                {
                    MCore::RGBAColor specularColor = MCore::RGBAColor(1.0f, 1.0f, 1.0f) * (1.0f * gfx->GetMainLightIntensity() * gfx->GetSpecularIntensity());
                    m_activeShader->SetUniform("specularPower", 25.0f);
                    m_activeShader->SetUniform("lightSpecular", specularColor);
                }

                m_activeShader->SetUniform("normalMap", m_normalMap);
            }

            {
                m_activeShader->SetUniform("diffuseMap", m_diffuseMap);
                m_activeShader->SetUniform("specularMap", m_specularMap);
            }
        }


        // update the advanced rendering settings
        m_activeShader->SetUniform("glowThreshold",      gfx->GetBloomThreshold());
        m_activeShader->SetUniform("focalPlaneDepth",    gfx->GetDOFFocalDistance());
        m_activeShader->SetUniform("nearPlaneDepth",     gfx->GetDOFNear());
        m_activeShader->SetUniform("farPlaneDepth",      gfx->GetDOFFar());
        m_activeShader->SetUniform("blurCutoff",         1.0f);
    }


    // deactivate the standard material
    void StandardMaterial::Deactivate()
    {
        // check if the shader is valid and return in case it's not
        if (m_activeShader == nullptr)
        {
            return;
        }

        // deactivate the active shader
        m_activeShader->Deactivate();
    }


    // initialize the standard material
    bool StandardMaterial::Init(EMotionFX::Material* material)
    {
        initializeOpenGLFunctions();
        m_material = material;

        if (material->GetType() == EMotionFX::StandardMaterial::TYPE_ID)
        {
            EMotionFX::StandardMaterial* stdMaterial = static_cast<EMotionFX::StandardMaterial*>(material);

            // get the number of material layers and iterate through them
            const size_t numLayers = stdMaterial->GetNumLayers();
            for (size_t i = 0; i < numLayers; ++i)
            {
                EMotionFX::StandardMaterialLayer* layer = stdMaterial->GetLayer(i);
                switch (layer->GetType())
                {
                case EMotionFX::StandardMaterialLayer::LAYERTYPE_DIFFUSE:
                {
                    m_diffuseMap   = LoadTexture(layer->GetFileName());
                    if (m_diffuseMap  == nullptr)
                    {
                        m_diffuseMap = GetGraphicsManager()->GetTextureCache()->GetWhiteTexture();
                    }
                } break;
                case EMotionFX::StandardMaterialLayer::LAYERTYPE_SHINESTRENGTH:
                {
                    m_specularMap  = LoadTexture(layer->GetFileName());
                    if (m_specularMap == nullptr)
                    {
                        m_specularMap = GetGraphicsManager()->GetTextureCache()->GetWhiteTexture();
                    }
                } break;
                case EMotionFX::StandardMaterialLayer::LAYERTYPE_BUMP:
                {
                    m_normalMap    = LoadTexture(layer->GetFileName());
                    if (m_normalMap   == nullptr)
                    {
                        m_normalMap = GetGraphicsManager()->GetTextureCache()->GetDefaultNormalTexture();
                    }
                } break;
                case EMotionFX::StandardMaterialLayer::LAYERTYPE_NORMALMAP:
                {
                    m_normalMap    = LoadTexture(layer->GetFileName());
                    if (m_normalMap   == nullptr)
                    {
                        m_normalMap = GetGraphicsManager()->GetTextureCache()->GetDefaultNormalTexture();
                    }
                } break;
                }
            }
        }

        return true;
    }


    //
    void StandardMaterial::SetAttribute(EAttribute attribute, bool enabled)
    {
        if (m_attributes[attribute] != enabled)
        {
            m_attributes[attribute] = enabled;
            m_attributesUpdated = true;
        }
    }


    // render the actor instance using the standard material
    void StandardMaterial::Render(EMotionFX::ActorInstance* actorInstance, const Primitive* primitive)
    {
        // check if the shader is valid and return in case it's not
        if (m_activeShader == nullptr)
        {
            return;
        }

        // change depth buffer settings
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);

        const EMotionFX::TransformData* transformData = actorInstance->GetTransformData();

        if (m_attributes[SKINNING])
        {
            const AZ::Matrix3x4* skinningMatrices = transformData->GetSkinningMatrices();

            // multiple each transform by its inverse bind pose
            const size_t numBones = primitive->m_boneNodeIndices.size();
            for (size_t i = 0; i < numBones; ++i)
            {
                const size_t nodeNr = primitive->m_boneNodeIndices[i];
                const AZ::Matrix3x4& skinTransform = skinningMatrices[nodeNr];
                m_boneMatrices[i] = AZ::Matrix4x4::CreateFromMatrix3x4(skinTransform);
            }

            m_activeShader->SetUniform("matBones", m_boneMatrices, aznumeric_caster(numBones));
        }

        const MCommon::Camera*    camera         = GetGraphicsManager()->GetCamera();
        const AZ::Transform       worldTransform = actorInstance->GetWorldSpaceTransform().ToAZTransform();
        const AZ::Matrix4x4       world          = AZ::Matrix4x4::CreateFromTransform(worldTransform);
        const AZ::Matrix4x4       worldView      = camera->GetViewMatrix() * world;
        const AZ::Matrix4x4       worldViewProj  = camera->GetViewProjMatrix() * world;
        const AZ::Matrix4x4       worldIT        = world.GetInverseFull().GetTranspose();

        m_activeShader->SetUniform("matWorld", world);
        m_activeShader->SetUniform("matWorldIT", worldIT);
        m_activeShader->SetUniform("matWorldView", worldView);
        m_activeShader->SetUniform("matWorldViewProj", worldViewProj);

        // render the primitive
        glDrawElementsBaseVertex(GL_TRIANGLES, primitive->m_numTriangles * 3, GL_UNSIGNED_INT, (GLvoid*)(primitive->m_indexOffset * sizeof(uint32)), primitive->m_vertexOffset);
    }


    // update the shader
    void StandardMaterial::UpdateShader()
    {
        // check if any attibutes have changed and skip directly if not
        if (m_attributesUpdated == false)
        {
            return;
        }

        // reset the active shader
        m_activeShader = nullptr;

        // get the number of shaders and iterate through them
        for (GLSLShader* shader : m_shaders)
        {
            if (shader == nullptr)
            {
                continue;
            }

            // check the shader for each define
            bool match = true;
            for (uint32 n = 0; n < NUM_ATTRIBUTES; ++n)
            {
                if (m_attributes[n])
                {
                    if (shader->CheckIfIsDefined(AttributeToString((EAttribute)n)) == false)
                    {
                        match = false;
                        break;
                    }
                }
                else
                {
                    if (shader->CheckIfIsDefined(AttributeToString((EAttribute)n)))
                    {
                        match = false;
                        break;
                    }
                }
            }

            // in case we have found a matching shader update the active shader
            if (match)
            {
                m_activeShader = shader;
                break;
            }
        }

        // if we didn't find a matching shader, compile it new
        if (m_activeShader == nullptr)
        {
            // if this function gets called at runtime something is wrong, go bug hunting!

            // construct an array of string attributes
            AZStd::vector<AZStd::string> defines;
            for (uint32 n = 0; n < NUM_ATTRIBUTES; ++n)
            {
                if (m_attributes[n])
                {
                    defines.emplace_back(AttributeToString((EAttribute)n));
                }
            }

            // compile shader and add it to the list of shaders
            m_activeShader = GetGraphicsManager()->LoadShader("StandardMaterial_VS.glsl", "StandardMaterial_PS.glsl", defines);
            m_shaders.emplace_back(m_activeShader);
        }

        m_attributesUpdated = false;
    }
}
