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

#ifndef CRYINCLUDE_EDITORCOMMON_SERIALIZATION_DECORATORS_IGIZMOSINK_H
#define CRYINCLUDE_EDITORCOMMON_SERIALIZATION_DECORATORS_IGIZMOSINK_H
#pragma once


namespace Serialization {
    struct LocalPosition;
    struct LocalFrame;
    struct LocalOrientation;

    struct GizmoFlags
    {
        bool visible;
        bool selected;

        GizmoFlags()
            : visible(true)
            , selected(false) {}
    };

    struct IGizmoSink
    {
        virtual ~IGizmoSink() = default;
        virtual int CurrentGizmoIndex() const = 0;
        virtual int Write(const LocalPosition&, const GizmoFlags& flags, const void* handle) = 0;
        virtual int Write(const LocalOrientation&, const GizmoFlags& flags, const void* handle) = 0;
        virtual int Write(const LocalFrame&, const GizmoFlags& flags, const void* handle) = 0;
        virtual void SkipRead() = 0;
        virtual bool Read(LocalPosition* position, GizmoFlags* flags, const void* handle) = 0;
        virtual bool Read(LocalOrientation* position, GizmoFlags* flags, const void* handle) = 0;
        virtual bool Read(LocalFrame* position, GizmoFlags* flags, const void* handle) = 0;
        virtual void Reset(const void* handle) = 0;
    };
}

#endif // CRYINCLUDE_EDITORCOMMON_SERIALIZATION_DECORATORS_IGIZMOSINK_H
