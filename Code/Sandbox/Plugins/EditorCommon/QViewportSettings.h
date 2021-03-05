/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITORCOMMON_QVIEWPORTSETTINGS_H
#define CRYINCLUDE_EDITORCOMMON_QVIEWPORTSETTINGS_H
#pragma once

#include <Cry_Math.h>
#include <Cry_Color.h>
#include "EditorCommonAPI.h"
#include "Serialization.h"
#include <MathConversion.h>

namespace Serialization
{
    class IArchive;
};

enum ECameraTransformRestraint
{
    eCameraTransformRestraint_Rotation   = 0x01,
    eCameraTransformRestraint_Panning    = 0x02,
    eCameraTransformRestraint_Zoom       = 0x04
};

struct SViewportState
{
    QuatT cameraTarget;
    QuatT cameraParentFrame;
    QuatT gridOrigin;
    Vec3 gridCellOffset;
    QuatT lastCameraTarget;
    QuatT lastCameraParentFrame;

    Vec3 orbitTarget;
    float orbitRadius;


    SViewportState()
        : cameraParentFrame(IDENTITY)
        , gridOrigin(IDENTITY)
        , gridCellOffset(0)
        , lastCameraTarget(IDENTITY)
        , lastCameraParentFrame(IDENTITY)
        , orbitTarget(ZERO)
    {
        // This eye position is similar to Maya's initial camera position
        AZ::Transform transform = AZ::Transform::CreateLookAt(
            AZ::Vector3(-3.5f, 3.625f, 2.635f), // Eye position
            AZ::Vector3(LYVec3ToAZVec3(orbitTarget))
        );
        cameraTarget = AZTransformToLYQuatT(transform);
        orbitRadius = cameraTarget.t.GetLength();

        lastCameraTarget = cameraTarget;
    }
};

struct SViewportRenderingSettings
{
    bool wireframe;
    bool sunlight;          // Add setting for time of day feature - Vera, Confetti
    bool fps;

    SViewportRenderingSettings()
        : wireframe(false)
        , fps(true)
        , sunlight(false)
    {
    }

    void Serialize(Serialization::IArchive& ar)
    {
        ar(wireframe, "wireframe", "Wireframe");
        ar(fps, "fps", "Framerate");
        ar(sunlight, "sunlight", "Sunlight");
    }
};

struct SViewportCameraSettings
{
    bool showViewportOrientation;

    float fov;
    float nearClip;
    float smoothPos;
    float smoothRot;

    float moveSpeed;
    float rotationSpeed;
    float zoomSpeed;
    float fastMoveMultiplier;
    float slowMoveMultiplier;

    int transformRestraint;

    SViewportCameraSettings()
        : showViewportOrientation(true)
        , fov(60)
        , nearClip(0.01f)
        , smoothPos(0.07f)
        , smoothRot(0.05f)
        , moveSpeed(0.7f)
        , rotationSpeed(2.0f)
        , zoomSpeed(0.1f)
        , fastMoveMultiplier(3.0f)
        , slowMoveMultiplier(0.1f)
        , transformRestraint(0)
    {
    }

    void Serialize(Serialization::IArchive& ar)
    {
        ar(showViewportOrientation, "showViewportOrientation", "Show Viewport Orientation");
        ar(Serialization::Range(fov, 20.0f, 120.0f), "fov", "FOV");
        ar(Serialization::Range(nearClip, 0.01f, 0.5f), "nearClip", "Near Clip");
        ar(Serialization::Range(moveSpeed, 0.1f, 3.0f), "moveSpeed", "Move Speed");
        ar(transformRestraint, "TransformRestraint", "Transform Restraint");
        ar.Doc("Relative to the scene size");
        ar(Serialization::Range(rotationSpeed, 0.1f, 4.0f), "rotationSpeed", "Rotation Speed");
        ar.Doc("Degrees per 1000 px");
        if (ar.OpenBlock("movementSmoothing", "+Movement Smoothing"))
        {
            ar(smoothPos, "smoothPos", "Position");
            ar(smoothRot, "smoothRot", "Rotation");
            ar.CloseBlock();
        }
    }
};


struct SViewportGridSettings
{
    bool showGrid;
    bool circular;
    ColorB mainColor;
    ColorB middleColor;
    int alphaFalloff;
    float spacing;
    uint16 count;
    uint16 interCount;
    bool origin;
    ColorB originColor;

    SViewportGridSettings()
        : showGrid(true)
        , circular(true)
        , mainColor(255, 255, 255, 50)
        , middleColor(255, 255, 255, 10)
        , alphaFalloff(100)
        , spacing(1.0f)
        , count(10)
        , interCount(10)
        , origin(false)
        , originColor(10, 10, 10, 255)
    {
    }

    void Serialize(Serialization::IArchive& ar)
    {
        ar(showGrid, "showGrid", "Show Grid");
        if (showGrid)
        {
            ar(circular, "circular", 0);
        }
        ar(mainColor, "mainColor", "Main Color");
        ar(middleColor, "middleColor", "Middle Color");
        ar(Serialization::Range(alphaFalloff, 0, 100), "alphaFalloff", 0);
        ar(spacing, "spacing", "Spacing");
        ar(count, "count", "Main Lines");
        ar(interCount, "interCount", "Middle Lines");
        ar(origin, "origin", "Origin");
        ar(originColor, "originColor", origin ? "Origin Color" : 0);
    }
};

struct SViewportLightingSettings
{
    f32 m_brightness;
    ColorB m_ambientColor;

    bool m_useLightRotation;
    f32 m_lightMultiplier;
    f32 m_lightSpecMultiplier;

    ColorB m_directionalLightColor;

    SViewportLightingSettings()
    {
        m_brightness = 1.0f;
        m_ambientColor = ColorB(128, 128, 128, 255);

        m_useLightRotation = 0;
        m_lightMultiplier = 3.0;
        m_lightSpecMultiplier = 2.0f;

        m_directionalLightColor = ColorB(255, 255, 255, 255);
    }

    void Serialize(Serialization::IArchive& ar)
    {
        ar(Serialization::Range(m_brightness, 0.0f, 200.0f), "brightness", "Brightness");
        ar(m_ambientColor, "ambientColor", "Ambient Color");

        ar(m_useLightRotation, "rotatelight", "Rotate Light");
        ar(m_lightMultiplier, "lightMultiplier", "Light Multiplier");
        ar(m_lightSpecMultiplier, "lightSpecMultiplier", "Light Spec Multiplier");

        ar(m_directionalLightColor, "directionalLightColor", "Directional Light Color");
    }
};

struct SViewportBackgroundSettings
{
    bool useGradient;
    ColorB topColor;
    ColorB bottomColor;

    SViewportBackgroundSettings()
        : useGradient(true)
        , topColor(128, 128, 128, 255)
        , bottomColor(32, 32, 32, 255)
    {
    }

    void Serialize(Serialization::IArchive& ar)
    {
        ar(useGradient, "useGradient", "Use Gradient");
        if (useGradient)
        {
            ar(topColor, "topColor", "Top Color");
            ar(bottomColor, "bottomColor", "Bottom Color");
        }
        else
        {
            ar(topColor, "topColor", "Color");
        }
    }
};

struct SViewportSettings
{
    SViewportRenderingSettings rendering;
    SViewportCameraSettings camera;
    SViewportGridSettings grid;
    SViewportLightingSettings lighting;
    SViewportBackgroundSettings background;

    void Serialize(Serialization::IArchive& ar)
    {
        ar(rendering, "debug", "Debug");
        ar(camera, "camera", "Camera");
        ar(grid, "grid", "Grid");
        ar(lighting, "lighting", "Lighting");
        ar(background, "background", "Background");
    }
};

#endif // CRYINCLUDE_EDITORCOMMON_QVIEWPORTSETTINGS_H
