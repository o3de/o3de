#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(FILES
    resource.h
    GridMate/NetworkGridMate.h
    GridMate/NetworkGridMate.cpp
    GridMate/NetworkGridMateEntityEventBus.h
    GridMate/NetworkGridMateSessionEvents.h
    GridMate/NetworkGridMateSessionEvents.cpp
    GridMate/NetworkGridMateSystemEvents.cpp
    GridMate/NetworkGridmateDebug.h
    GridMate/NetworkGridMateCommon.h
    GridMate/NetworkGridMateProfiling.h
    GridMate/NetworkGridMateSystemEvents.h
    GridMate/NetworkGridmateDebug.cpp
    GridMate/NetworkGridmateMarshaling.h
    GridMate/Compatibility/GridMateNetSerialize.cpp
    GridMate/Compatibility/GridMateNetSerialize.h
    GridMate/Compatibility/GridMateNetSerializeAspectProfiles.cpp
    GridMate/Compatibility/GridMateNetSerializeAspectProfiles.h
    GridMate/Compatibility/GridMateNetSerializeAspects.inl
    GridMate/Compatibility/GridMateRMI.cpp
    GridMate/Compatibility/GridMateRMI.h
    GridMate/Replicas/EntityReplica.cpp
    GridMate/Replicas/EntityReplica.h
    GridMate/Replicas/EntityScriptReplicaChunk.cpp
    GridMate/Replicas/EntityScriptReplicaChunk.h
    GridMate/Replicas/EntityReplicaSpawnParams.cpp
    GridMate/Replicas/EntityReplicaSpawnParams.h
    CryNetwork_precompiled.h
    CryNetwork_precompiled.cpp
)
