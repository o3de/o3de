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

#pragma once


class Camera
{
public:
    Camera();
    void SetMatrix(XMMATRIX cameraMatrix);
    void LookAt(XMVECTOR eyePos, XMVECTOR lookAt);
    void LookAt(float yaw, float pitch, float distance, XMVECTOR at);
    void SetFov(float fov, uint32_t width, uint32_t height, float nearPlane, float farPlane);
    void UpdateCameraPolar(float yaw, float pitch, float x, float y, float distance);
    void UpdateCameraWASD(float yaw, float pitch, const bool keyDown[256], double deltaTime);

    XMMATRIX GetView() const { return m_View; }
    XMMATRIX GetPrevView() const { return m_PrevView; }
    XMMATRIX GetViewport() const { return m_Viewport; }
    XMVECTOR GetPosition() const { return m_eyePos;   }

    XMVECTOR GetDirection() const { return XMVectorSetW(XMVector4Transform(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), XMMatrixTranspose(m_View)), 0); }
    XMVECTOR GetUp() const       { return XMVectorSetW(XMVector4Transform(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), XMMatrixTranspose(m_View)), 0); }
    XMVECTOR GetSide() const     { return XMVectorSetW(XMVector4Transform(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), XMMatrixTranspose(m_View)), 0); }
    XMMATRIX GetProjection() const { return m_Proj; }

    float GetFovH() const { return m_fovH; }
    float GetFovV() const { return m_fovV; }

    float GetNearPlane() const { return m_near; }
    float GetFarPlane() const { return m_far; }

    float GetYaw() const { return m_yaw; }
    float GetPitch() const { return m_pitch; }
    float GetDistance() const { return m_distance; }

    void SetSpeed( float speed ) { m_speed = speed; }
    void SetProjectionJitter(float jitterX, float jitterY);
    void UpdatePreviousMatrices();

private:
    XMMATRIX            m_View;
    XMMATRIX            m_Proj;
    XMMATRIX            m_PrevView;
    XMMATRIX            m_Viewport;
    XMVECTOR            m_eyePos;
    float               m_distance;
    float               m_fovV, m_fovH;
    float               m_near, m_far;
    float               m_aspectRatio;

    float               m_speed = 1.0f;
    float               m_yaw = 0.0f;
    float               m_pitch = 0.0f;
};

XMVECTOR PolarToVector(float roll, float pitch);
XMMATRIX LookAtRH(XMVECTOR eyePos, XMVECTOR lookAt);
XMVECTOR MoveWASD(const bool keyDown[256]);