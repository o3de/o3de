// AMD AMDUtils code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "stdafx.h"
#include "Camera.h"

Camera::Camera()
{
    m_View = XMMatrixIdentity();
    m_eyePos = XMVectorSet(0, 0, 0, 0);
    m_distance = -1;
}

//--------------------------------------------------------------------------------------
//
// OnCreate 
//
//--------------------------------------------------------------------------------------
void Camera::SetFov(float fovV, uint32_t width, uint32_t height, float nearPlane, float farPlane)
{
    m_aspectRatio = width *1.f / height;

    m_near = nearPlane;
    m_far = farPlane;

    m_fovV = fovV;
    m_fovH = std::min<float>((m_fovV * width) / height, XM_PI / 2.0f);
    m_fovV = m_fovH * height / width;

    float halfWidth = (float)width / 2.0f;
    float halfHeight = (float)height / 2.0f;
    m_Viewport = XMMATRIX(
        halfWidth, 0.0f, 0.0f, 0.0f,
        0.0f, -halfHeight, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        halfWidth, halfHeight, 0.0f, 1.0f);
    
    if (fovV==0)
        m_Proj = XMMatrixOrthographicRH(height/40.0f, height/40.0f, nearPlane, farPlane);
    else
        m_Proj = XMMatrixPerspectiveFovRH(fovV, m_aspectRatio, nearPlane, farPlane);
}

void Camera::SetMatrix(XMMATRIX cameraMatrix)
{
    m_eyePos = cameraMatrix.r[3];
    m_View = XMMatrixInverse(nullptr, cameraMatrix);
}

//--------------------------------------------------------------------------------------
//
// LookAt, use this functions before calling update functions
//
//--------------------------------------------------------------------------------------
void Camera::LookAt(XMVECTOR eyePos, XMVECTOR lookAt)
{
    m_eyePos = eyePos;
    m_View = LookAtRH(eyePos, lookAt);
    m_distance = XMVectorGetX(XMVector3Length(lookAt - eyePos));

    XMMATRIX mInvView = XMMatrixInverse( nullptr, m_View );

    XMFLOAT3 zBasis;
    XMStoreFloat3( &zBasis, mInvView.r[2] );

    m_yaw = atan2f( zBasis.x, zBasis.z );
    float fLen = sqrtf( zBasis.z * zBasis.z + zBasis.x * zBasis.x );
    m_pitch = atan2f( zBasis.y, fLen );
}

void Camera::LookAt(float yaw, float pitch, float distance, XMVECTOR at )
{   
    LookAt(at + PolarToVector(yaw, pitch)*distance, at);
}

//--------------------------------------------------------------------------------------
//
// UpdateCamera
//
//--------------------------------------------------------------------------------------
void Camera::UpdateCameraWASD(float yaw, float pitch, const bool keyDown[256], double deltaTime)
{   
    m_eyePos += XMVector4Transform(MoveWASD(keyDown) * m_speed * (float)deltaTime, XMMatrixTranspose(m_View));
    XMVECTOR dir = PolarToVector(yaw, pitch) * m_distance;   
    LookAt(m_eyePos, m_eyePos - dir);
}

void Camera::UpdateCameraPolar(float yaw, float pitch, float x, float y, float distance)
{    
    m_eyePos += GetSide() * x * distance / 10.0f;
    m_eyePos += GetUp() * y * distance / 10.0f;

    XMVECTOR dir = GetDirection();    
    XMVECTOR pol = PolarToVector(yaw, pitch);

    XMVECTOR at = m_eyePos - dir * m_distance;   
    XMVECTOR m_eyePos = at + pol * distance;

    LookAt(m_eyePos, at);
}

//--------------------------------------------------------------------------------------
//
// SetProjectionJitter
//
//--------------------------------------------------------------------------------------
void Camera::SetProjectionJitter(float jitterX, float jitterY)
{
    XMFLOAT4X4 Proj;
    XMStoreFloat4x4(&Proj, m_Proj);
    Proj.m[2][0] = jitterX;
    Proj.m[2][1] = jitterY;
    m_Proj = XMLoadFloat4x4(&Proj);
}

//--------------------------------------------------------------------------------------
//
// UpdatePreviousMatrices
//
//--------------------------------------------------------------------------------------
void Camera::UpdatePreviousMatrices()
{
    m_PrevView = m_View;
}

//--------------------------------------------------------------------------------------
//
// Get a vector pointing in the direction of yaw and pitch
//
//--------------------------------------------------------------------------------------
XMVECTOR PolarToVector(float yaw, float pitch)
{
    return XMVectorSet(sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch), 0);    
}

XMMATRIX LookAtRH(XMVECTOR eyePos, XMVECTOR lookAt)
{
    return XMMatrixLookAtRH(eyePos, lookAt, XMVectorSet(0, 1, 0, 0));
}

XMVECTOR MoveWASD(const bool keyDown[256])
{ 
    float x = 0, y = 0, z = 0;

    if (keyDown['W'])
    {
        z = -1;
    }
    if (keyDown['S'])
    {
        z = 1;
    }
    if (keyDown['A'])
    {
        x = -1;
    }
    if (keyDown['D'])
    {
        x = 1;
    }
    if (keyDown['E'])
    {
        y = 1;
    }
    if (keyDown['Q'])
    {
        y = -1;
    }

    float scale = keyDown[VK_SHIFT] ? 5.0f : 1.0f;

    return XMVectorSet(x, y, z, 0.0f) * scale;
}

