/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include <MCore/Source/StandardHeaders.h>


namespace EMotionFX
{
    /**
     * The default alignment of returned memory addresses, in bytes.
     */
    #define EMFX_DEFAULT_ALIGNMENT  4


    /**
     * The memory categories of the EMotion FX classes.
     * Each EMotion FX class or template is linked to a given memory category.
     * The memory manager can be used to request memory usage statistics per category.
     * This is useful to see how much memory is allocated in each category.
     */
    enum
    {
        EMFX_MEMCATEGORY_GEOMETRY_MATERIALS                 = 200,
        EMFX_MEMCATEGORY_GEOMETRY_MESHES                    = 201,
        EMFX_MEMCATEGORY_GEOMETRY_DEFORMERS                 = 202,
        EMFX_MEMCATEGORY_GEOMETRY_VERTEXATTRIBUTES          = 203,
        EMFX_MEMCATEGORY_GEOMETRY_PMORPHTARGETS             = 204,

        EMFX_MEMCATEGORY_MOTIONS_MOTIONINSTANCES            = 300,
        EMFX_MEMCATEGORY_MOTIONS_MOTIONSYSTEMS              = 301,
        EMFX_MEMCATEGORY_MOTIONS_SKELETALMOTIONS            = 302,
        EMFX_MEMCATEGORY_MOTIONS_INTERPOLATORS              = 304,
        EMFX_MEMCATEGORY_MOTIONS_KEYTRACKS                  = 305,
        EMFX_MEMCATEGORY_MOTIONS_MOTIONLINKS                = 306,
        EMFX_MEMCATEGORY_EVENTS                             = 307,
        EMFX_MEMCATEGORY_MOTIONS_MISC                       = 308,
        EMFX_MEMCATEGORY_MOTIONS_MOTIONSETS                 = 310,
        EMFX_MEMCATEGORY_MOTIONS_MOTIONMANAGER              = 311,
        EMFX_MEMCATEGORY_EVENTHANDLERS                      = 312,
        EMFX_MEMCATEGORY_EYEBLINKER                         = 313,
        EMFX_MEMCATEGORY_MOTIONS_GROUPS                     = 314,
        EMFX_MEMCATEGORY_MOTIONINSTANCEPOOL                 = 315,

        EMFX_MEMCATEGORY_NODES                              = 400,
        EMFX_MEMCATEGORY_ACTORS                             = 401,
        EMFX_MEMCATEGORY_ACTORINSTANCES                     = 402,
        EMFX_MEMCATEGORY_NODEATTRIBUTES                     = 403,
        EMFX_MEMCATEGORY_NODESMISC                          = 404,
        EMFX_MEMCATEGORY_NODEMAP                            = 405,
        EMFX_MEMCATEGORY_RIGSYSTEM                          = 406,

        EMFX_MEMCATEGORY_TRANSFORMDATA                      = 500,
        EMFX_MEMCATEGORY_POSE                               = 503,
        EMFX_MEMCATEGORY_TRANSFORM                          = 504,
        EMFX_MEMCATEGORY_SKELETON                           = 505,

        EMFX_MEMCATEGORY_CONSTRAINTS                        = 510,

        EMFX_MEMCATEGORY_ANIMGRAPH                         = 550,
        EMFX_MEMCATEGORY_ANIMGRAPH_MANAGER                 = 551,
        EMFX_MEMCATEGORY_ANIMGRAPH_INSTANCE                = 552,
        EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREES              = 553,
        EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES          = 554,
        EMFX_MEMCATEGORY_ANIMGRAPH_STATEMACHINES           = 555,
        EMFX_MEMCATEGORY_ANIMGRAPH_STATES                  = 556,
        EMFX_MEMCATEGORY_ANIMGRAPH_CONNECTIONS             = 557,
        EMFX_MEMCATEGORY_ANIMGRAPH_ATTRIBUTEVALUES         = 558,
        EMFX_MEMCATEGORY_ANIMGRAPH_ATTRIBUTEINFOS          = 559,
        EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTUNIQUEDATA        = 560,
        EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTS                 = 561,
        EMFX_MEMCATEGORY_ANIMGRAPH_TRANSITIONS             = 563,
        EMFX_MEMCATEGORY_ANIMGRAPH_SYNCTRACK               = 564,
        EMFX_MEMCATEGORY_ANIMGRAPH_POSE                    = 565,
        EMFX_MEMCATEGORY_ANIMGRAPH_PROCESSORS              = 566,
        EMFX_MEMCATEGORY_ANIMGRAPH_GAMECONTROLLER          = 567,
        EMFX_MEMCATEGORY_ANIMGRAPH_EVENTBUFFERS            = 568,
        EMFX_MEMCATEGORY_ANIMGRAPH_POSEPOOL                = 569,
        EMFX_MEMCATEGORY_ANIMGRAPH_NODES                   = 570,
        EMFX_MEMCATEGORY_ANIMGRAPH_NODEGROUP               = 571,
        EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE              = 572,
        EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTDATAPOOL          = 573,
        EMFX_MEMCATEGORY_ANIMGRAPH_REFCOUNTEDDATA          = 574,

        EMFX_MEMCATEGORY_WAVELETCACHE                       = 575,
        EMFX_MEMCATEGORY_WAVELETSKELETONMOTION              = 576,

        EMFX_MEMCATEGORY_IMPORTER                           = 600,
        EMFX_MEMCATEGORY_IDGENERATOR                        = 601,
        EMFX_MEMCATEGORY_ACTORMANAGER                       = 603,
        EMFX_MEMCATEGORY_UPDATESCHEDULERS                   = 604,
        EMFX_MEMCATEGORY_ATTACHMENTS                        = 605,
        EMFX_MEMCATEGORY_EMOTIONFXMANAGER                   = 606,
        EMFX_MEMCATEGORY_FILEPROCESSORS                     = 607,
        EMFX_MEMCATEGORY_EMSTUDIODATA                       = 608,
        EMFX_MEMCATEGORY_RECORDER                           = 609,
        EMFX_MEMCATEGORY_IK                                 = 612,

        EMFX_MEMCATEGORY_MESHBUILDER                        = 700,
        EMFX_MEMCATEGORY_MESHBUILDER_SKINNINGINFO           = 701,
        EMFX_MEMCATEGORY_MESHBUILDER_SUBMESH                = 702,
        EMFX_MEMCATEGORY_MESHBUILDER_VERTEXLOOKUP           = 703,
        EMFX_MEMCATEGORY_MESHBUILDER_VERTEXATTRIBUTELAYER   = 704,
    };


    enum
    {
        EMFX_MEMORYGROUP_ACTORS         = 101,
        EMFX_MEMORYGROUP_MOTIONS        = 102
    };
}   // namespace EMotionFX
