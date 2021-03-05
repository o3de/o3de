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
#pragma once
#ifndef __CCRYDX12ASYNCHRONOUS__
#define __CCRYDX12ASYNCHRONOUS__

#include "DX12/Device/CCryDX12DeviceChild.hpp"

template <typename T>
class CCryDX12Asynchronous
    : public CCryDX12DeviceChild<T>
{
public:
    DX12_OBJECT(CCryDX12Asynchronous, CCryDX12DeviceChild<T>);

    virtual ~CCryDX12Asynchronous()
    {
    }

    #pragma region /* ID3D11Asynchronous implementation */

    virtual UINT STDMETHODCALLTYPE GetDataSize()
    {
        return 0;
    }

    #pragma endregion

protected:
    CCryDX12Asynchronous()
        : Super(nullptr, nullptr)
    {
    }

private:
};


#endif // __CCRYDX12ASYNCHRONOUS__
