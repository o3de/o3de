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

#include <precompiled.h>
#include <sstream>

#include <AzCore/Math/Vector2.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <GraphCanvas/tools.h>

//////////
// Tools
//////////

QDebug operator<<(QDebug debug, const AZ::Entity* entity)
{
    QDebugStateSaver saver(debug);
    if (!entity)
    {
        debug.nospace() << "Entity(nullptr)";
        return debug;
    }

    const AZStd::string id = entity->GetId().ToString();
    const AZStd::string& name = entity->GetName();

    QString state;
    switch (entity->GetState())
    {
    case AZ::Entity::State::Init:
        state = "ES_INIT";
        break;
    case AZ::Entity::State::Constructed:
        state = "ES_CONSTRUCTED";
        break;
    case AZ::Entity::State::Active:
        state = "ES_ACTIVE";
        break;
    default:
        state = "ES_BAD_STATE";
        break;
    }

    debug.nospace() << "Entity(" << QString(id.c_str()) << ", " << state << ", \"" << QString(name.c_str()) << "\")";

    return debug;
}

QDebug operator<<(QDebug debug, const AZ::EntityId& entity)
{
    const AZ::Entity* actual = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(actual, &AZ::ComponentApplicationRequests::FindEntity, entity);

    return operator<<(debug, actual);
}

QDebug operator<<(QDebug debug, const AZ::Component* component)
{
    QDebugStateSaver saver(debug);
    if (!component)
    {
        debug.nospace() << "Component(nullptr)";
        return debug;
    }

    std::ostringstream converter;
    converter << std::hex << component->GetId();

    debug.nospace() << "Component(" << QString::fromStdString(converter.str()) << " {" << component->GetEntity() << "})";

    return debug;
}

QDebug operator<<(QDebug debug, const AZ::Vector2& position)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Vector2(" << position.GetX() << ", " << position.GetY() << ")";
    return debug;
}
