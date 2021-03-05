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
#pragma once
#include "DevBuffer.h"

#ifndef NULL_RENDERER
#include "DriverD3D.h"
#endif

template <typename T>
class CTypedConstantBuffer
{
    T m_hostBuffer;
    AzRHI::ConstantBufferPtr m_constantBuffer;

public:
    CTypedConstantBuffer()
        : m_constantBuffer(nullptr) {}
    CTypedConstantBuffer(const CTypedConstantBuffer<T>& cb)
        : m_constantBuffer(nullptr) { m_hostBuffer = cb.m_hostBuffer; }
    CTypedConstantBuffer(AzRHI::ConstantBufferPtr incb)
        : m_constantBuffer(incb) {}

    bool IsDeviceBufferAllocated() { return m_constantBuffer != nullptr; }
    AzRHI::ConstantBufferPtr GetDeviceConstantBuffer()
    {
        if (!m_constantBuffer)
        {
            CreateDeviceBuffer();
        }
        return m_constantBuffer;
    }

    void CreateDeviceBuffer()
    {
#ifndef NULL_RENDERER
        int size = sizeof(T);
        AzRHI::ConstantBuffer* cb = gcpRendD3D->m_DevBufMan.CreateConstantBuffer(
            "TypedConstantBuffer",
            size,
            AzRHI::ConstantBufferUsage::Dynamic); // TODO: this could be a good candidate for dynamic=false
        m_constantBuffer.attach(cb);
        CopyToDevice();
#endif
    }

    T& GetData()
    {
        return m_hostBuffer;
    }

    void CopyToDevice()
    {
        m_constantBuffer->UpdateBuffer(&m_hostBuffer, sizeof(m_hostBuffer));
    }

    T* operator->() { return &m_hostBuffer; }
    T& operator=(const T& hostData)
    {
        return m_hostBuffer = hostData;
    }
};
