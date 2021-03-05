/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Module/Environment.h>
#include <EMotionFX/Source/EMotionFXAllocatorInitializer.h>

#include "MotionSetBuilderWorker.h"
#include "AnimGraphBuilderWorker.h"

namespace AZ
{
    namespace Data
    {
        class AssetHandler;
    }
}

namespace EMotionFX
{
    namespace EMotionFXBuilder
    {
        //! The EMotionFXBuilderComponent is responsible for setting up the EMotionFXBuilderWorker.
        class EMotionFXBuilderComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(EMotionFXBuilderComponent, "{5484372D-E088-41CB-BFB4-73649DD9DB10}");

            EMotionFXBuilderComponent();
            ~EMotionFXBuilderComponent() override;

            static void Reflect(AZ::ReflectContext* context);

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

        private:
            //unique_ptr cannot be copied -> vector of unique_ptrs cannot be copied -> class cannot be copied
            EMotionFXBuilderComponent(const EMotionFXBuilderComponent&) = delete;

            AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler> > m_assetHandlers;

            // Creates a static shared pointer using the AZ EnvironmentVariable system.
            // This will prevent the EMotionFXAllocator from destroying too early by the other component
            static AZ::EnvironmentVariable<EMotionFXAllocatorInitializer> s_EMotionFXAllocator;

            MotionSetBuilderWorker m_motionSetBuilderWorker;
            AnimGraphBuilderWorker m_animGraphBuilderWorker;
        };
    }
}