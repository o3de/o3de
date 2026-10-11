/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPIExt/Rules/SocketSetupRule.h>


namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            SocketSetupRule::SocketSetupRule()
                : ExternalToolRule<AZStd::shared_ptr<EMotionFX::SocketSetup>>()
            {
            }


            SocketSetupRule::SocketSetupRule(const AZStd::shared_ptr<EMotionFX::SocketSetup>& data)
                : SocketSetupRule()
            {
                m_data = data;
            }


            void SocketSetupRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<SocketSetupRule>()
                        ->Version(1)
                        ->Field("data", &SocketSetupRule::m_data)
                        ;
                }
            }
        } // Rule
    } // Pipeline
} // EMotionFX
