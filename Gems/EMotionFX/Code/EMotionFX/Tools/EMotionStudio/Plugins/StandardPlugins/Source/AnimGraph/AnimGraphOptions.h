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

#include <AzCore/std/functional.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <EMotionStudio/EMStudioSDK/Source/PluginOptions.h>
#include <qglobal.h>

QT_FORWARD_DECLARE_CLASS(QSettings);

namespace EMStudio
{
    class AnimGraphPlugin;

    class AnimGraphOptions 
        : public PluginOptions
    {
    public:
        AZ_RTTI(AnimGraphOptions, "{D7B6C210-8B33-4707-A46C-EB89D8232660}", PluginOptions);

        static const char* s_graphAnimationOptionName;
        static const char* s_showFPSOptionName;

        AnimGraphOptions();
        AnimGraphOptions& operator=(const AnimGraphOptions& other);

        void Save(QSettings* settings);
        static AnimGraphOptions Load(QSettings* settings);

        static void Reflect(AZ::ReflectContext* context);

        bool GetGraphAnimation() const { return m_graphAnimation; }
        void SetGraphAnimation(bool graphAnimation);

        bool GetShowFPS() const { return m_showFPS; }
        void SetShowFPS(bool showFPS);

    private:
        void OnGraphAnimationChangedCallback() const;
        void OnShowFPSChangedCallback() const;

        bool    m_graphAnimation;
        bool    m_showFPS;
    };
}   // namespace EMStudio
