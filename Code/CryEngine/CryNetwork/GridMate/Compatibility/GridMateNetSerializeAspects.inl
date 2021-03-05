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

ADD_ASPECT(eEA_Script,                 0)
ADD_ASPECT(eEA_Physics,                1)
ADD_ASPECT(eEA_GameClientStatic,       2)
ADD_ASPECT(eEA_GameServerStatic,       3)
ADD_ASPECT(eEA_GameClientDynamic,      4)
ADD_ASPECT(eEA_GameServerDynamic,      5)
ADD_ASPECT(eEA_GameClientA,            6)
ADD_ASPECT(eEA_GameServerA,            7)
ADD_ASPECT(eEA_GameClientB,            8)
ADD_ASPECT(eEA_GameServerB,            9)
ADD_ASPECT(eEA_GameClientC,            10)
ADD_ASPECT(eEA_GameServerC,            11)
ADD_ASPECT(eEA_GameClientD,            12)
ADD_ASPECT(eEA_GameClientE,            13)
ADD_ASPECT(eEA_GameClientF,            14)
ADD_ASPECT(eEA_GameClientG,            15)
ADD_ASPECT(eEA_GameClientH,            16)
ADD_ASPECT(eEA_GameClientI,            17)
ADD_ASPECT(eEA_GameClientJ,            18)
ADD_ASPECT(eEA_GameServerD,            19)
ADD_ASPECT(eEA_GameClientK,            20)
ADD_ASPECT(eEA_Aspect29,               21)
ADD_ASPECT(eEA_Aspect30,               22)
ADD_ASPECT(eEA_Aspect31,               23)
ADD_ASPECT(eEA_GameClientO,            24)
ADD_ASPECT(eEA_GameClientP,            25)

// We currently don't have room for these due to replica data set limits.
// If a game requires more, the recommendation is just not use the shim,
// and use replica/replica chunks instead. That's the long term plan anyway,
// so it's unlikely we'll bother supporting more for the shim. If the need
// does arise, it can be done by devoting two separate replica chunks to
// the entity replica for handling aspects.
//eEA_GameServerE = 0x10000000u, // aspect 28
//eEA_GameClientL = 0x00800000u, // aspect 23
//eEA_GameClientM = 0x01000000u, // aspect 24
//eEA_GameClientN = 0x02000000u, // aspect 25
