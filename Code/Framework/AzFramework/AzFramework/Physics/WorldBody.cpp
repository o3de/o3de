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

#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/WorldBody.h>


namespace Physics
{
    void WorldBodyConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<WorldBodyConfiguration>()
                ->Version(1)
                ->Field("name", &WorldBodyConfiguration::m_debugName)
                ;
        }
    }

    void WorldBody::SetUserData(void* userData) 
    {
        m_customUserData = userData;
    }
} // namespace Physics
