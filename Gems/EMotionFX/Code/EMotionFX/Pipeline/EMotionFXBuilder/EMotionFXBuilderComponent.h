/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Module/Environment.h>

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

            EMotionFXBuilderComponent() = default;

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

            MotionSetBuilderWorker m_motionSetBuilderWorker;
            AnimGraphBuilderWorker m_animGraphBuilderWorker;
        };
    }
}
