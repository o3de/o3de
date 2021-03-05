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

#if defined(USE_NV_API)
void CDeviceTexture::DisableMgpuSync()
{
    if (gRenDev->m_bVendorLibInitialized && (gRenDev->GetFeatures() & RFT_HW_NVIDIA))
    {
        NVDX_ObjectHandle handle = (NVDX_ObjectHandle&)m_handleMGPU;
        if (handle == NULL)
        {
            NvAPI_Status status = NvAPI_D3D_GetObjectHandleForResource(&gcpRendD3D->GetDevice(), m_pD3DTexture, &handle);
            assert(status == NVAPI_OK);
            m_handleMGPU = handle;
        }
        //if (nWidth >= 4 && nHeight >= 4)
        {
            NvU32 value = 1;
            // disable driver watching this texture - it will never be synced unless requested by the application
            NvAPI_Status status = NvAPI_D3D_SetResourceHint(&gcpRendD3D->GetDevice(), handle, NVAPI_D3D_SRH_CATEGORY_SLI, NVAPI_D3D_SRH_SLI_APP_CONTROLLED_INTERFRAME_CONTENT_SYNC, &value);
            assert(status == NVAPI_OK);
        }
    }
}

void CDeviceTexture::MgpuResourceUpdate(bool bUpdating)
{
    if (gcpRendD3D->m_bVendorLibInitialized && (gRenDev->GetFeatures() & RFT_HW_NVIDIA))
    {
        ID3D11Device* pDevice = &gcpRendD3D->GetDevice();
        NVDX_ObjectHandle handle = (NVDX_ObjectHandle&)m_handleMGPU;
        if (handle == NULL)
        {
            NvAPI_Status status = NvAPI_D3D_GetObjectHandleForResource(pDevice, m_pD3DTexture, &handle);
            assert(status == NVAPI_OK);
            m_handleMGPU = handle;
        }
        if (handle)
        {
            NvAPI_Status status = NVAPI_OK;
            if (bUpdating)
            {
                status = NvAPI_D3D_BeginResourceRendering(pDevice, handle, NVAPI_D3D_RR_FLAG_FORCE_DISCARD_CONTENT);
            }
            else
            {
                status = NvAPI_D3D_EndResourceRendering(pDevice, handle, 0);
            }

            // TODO: Verify that NVAPI_WAS_STILL_DRAWING is valid here
            assert(status == NVAPI_OK || status == NVAPI_WAS_STILL_DRAWING);
        }
    }
}
#endif
