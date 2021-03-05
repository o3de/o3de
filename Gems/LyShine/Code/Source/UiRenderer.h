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

#include <IRenderer.h>

#ifndef _RELEASE
#include <AzCore/std/containers/unordered_set.h>
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Implementation of IUiRenderInterface
//
class UiRenderer
{
public: // member functions

    //! Constructor, constructed by the LyShine class
    UiRenderer();
    ~UiRenderer();

    //! Start the rendering of the frame for LyShine
    void BeginUiFrameRender();

    //! End the rendering of the frame for LyShine
    void EndUiFrameRender();

    //! Start the rendering of a UI canvas
    void BeginCanvasRender();

    //! End the rendering of a UI canvas
    void EndCanvasRender();

    //! Get the current base state
    int GetBaseState();

    //! Set the base state
    void SetBaseState(int state);

    //! Get the current stencil test reference value
    uint32 GetStencilRef();

    //! Set the stencil test reference value
    void SetStencilRef(uint32);

    //! Increment the current stencil reference value
    void IncrementStencilRef();

    //! Decrement the current stencil reference value
    void DecrementStencilRef();

    //! Set the current texture
    void SetTexture(ITexture* texture, int texUnit, bool clamp);

#ifndef _RELEASE
    //! Setup to record debug texture data before rendering
    void DebugSetRecordingOptionForTextureData(int recordingOption);

    //! Display debug texture data after rendering
    void DebugDisplayTextureData(int recordingOption);
#endif

private:

    AZ_DISABLE_COPY_MOVE(UiRenderer);

    //! Bind the global white texture for all the texture units we use
    void BindNullTexture();
    
protected: // attributes

    int                 m_baseState;
    uint32              m_stencilRef;
    IRenderer*          m_renderer = nullptr;

#ifndef _RELEASE
    int                 m_debugTextureDataRecordLevel = 0;
    AZStd::unordered_set<ITexture*> m_texturesUsedInFrame;
#endif
};
