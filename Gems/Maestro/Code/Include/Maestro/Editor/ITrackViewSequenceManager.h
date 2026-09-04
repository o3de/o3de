/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

#include <QString>

struct IAnimSequence;
class CTrackViewSequence;

//! Interface to expose TrackViewSequenceManager functionality to SequenceComponent
struct ITrackViewSequenceManager
{
    AZ_TYPE_INFO(ITrackViewSequenceManager, "{F49A421A-04C6-4F2A-BC73-BE205CD33019}");

    virtual IAnimSequence* OnCreateSequenceObject(
        QString name, bool isLegacySequence = true, AZ::EntityId entityId = AZ::EntityId()) = 0;

    //! Notifies of the delete of a sequence entity OR legacy sequence object
    //! @param entityId The Sequence Component Entity Id OR the legacy sequence object Id packed in the lower 32-bits,
    //! as returned from IAnimSequence::GetSequenceEntityId()
    virtual void OnDeleteSequenceEntity(const AZ::EntityId& entityId) = 0;

    //! Get the first sequence with the given name. They may be more than one sequence with this name.
    //! Only intended for use with scripting or other cases where a user provides a name.
    virtual CTrackViewSequence* GetSequenceByName(QString name) const = 0;

    //! Get the sequence with the given EntityId. For legacy support, legacy sequences can be found by giving
    //! the sequence ID in the lower 32 bits of the EntityId.
    virtual CTrackViewSequence* GetSequenceByEntityId(const AZ::EntityId& entityId) const = 0;

    virtual void OnCreateSequenceComponent(AZStd::intrusive_ptr<IAnimSequence>& sequence) = 0;
    virtual void OnSequenceActivated(const AZ::EntityId& entityId) = 0;
    virtual void OnSequenceDeactivated(const AZ::EntityId& entityId) = 0;
};

//! Interface to expose TrackViewSequence functionality to SequenceComponent
struct ITrackViewSequence
{
    virtual void Load() = 0;
};
