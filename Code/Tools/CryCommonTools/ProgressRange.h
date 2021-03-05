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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_PROGRESSRANGE_H
#define CRYINCLUDE_CRYCOMMONTOOLS_PROGRESSRANGE_H
#pragma once


class ProgressRange
{
public:
    template <typename T>
    ProgressRange(T* object, void (T::* setter)(float progress))
        : m_target(new MethodTarget<T>(object, setter))
        , m_progress(0.0f)
        , m_start(0.0f)
        , m_scale(1.0f)
    {
        m_target->Set(m_start);
    }

    ProgressRange(ProgressRange& parent, float scale)
        : m_target(new ParentRangeTarget(parent))
        , m_progress(0.0f)
        , m_start(parent.m_progress)
        , m_scale(scale)
    {
        m_target->Set(m_start);
    }

    ~ProgressRange()
    {
        m_target->Set(m_start + m_scale);
        delete m_target;
    }

    void SetProgress(float progress)
    {
        assert(progress > -0.01f && progress < 1.1f);
        m_progress = progress;
        m_target->Set(m_start + m_scale * progress);
    }

private:
    struct ITarget
    {
        virtual ~ITarget() {}
        virtual void Set(float progress) = 0;
    };

    struct ParentRangeTarget
        : public ITarget
    {
        ParentRangeTarget(ProgressRange& range)
            : range(range) {}
        virtual void Set(float progress) {range.SetProgress(progress); }
        ProgressRange& range;
    };

    template <typename T>
    struct MethodTarget
        : public ITarget
    {
        typedef void (T::* Setter)(float progress);
        MethodTarget(T* object, Setter setter)
            : object(object)
            , setter(setter) {}
        virtual void Set(float progress) {(object->*setter)(progress); }
        T* object;
        Setter setter;
    };

    ITarget* m_target;
    float m_progress;
    float m_start;
    float m_scale;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_PROGRESSRANGE_H
