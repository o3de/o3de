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

// include the shared structs
#include "SharedFileFormatStructs.h"


namespace EMotionFX
{
    namespace FileFormat // so now we are in the namespace EMotionFX::FileFormat
    {
        // collection of actor chunk IDs
        enum
        {
            ACTOR_CHUNK_NODE                  = 0,
            ACTOR_CHUNK_MESH                  = 1,
            ACTOR_CHUNK_SKINNINGINFO          = 2,
            ACTOR_CHUNK_STDMATERIAL           = 3,
            ACTOR_CHUNK_STDMATERIALLAYER      = 4,
            ACTOR_CHUNK_FXMATERIAL            = 5,
            ACTOR_CHUNK_LIMIT                 = 6,
            ACTOR_CHUNK_INFO                  = 7,
            ACTOR_CHUNK_MESHLODLEVELS         = 8,
            ACTOR_CHUNK_STDPROGMORPHTARGET    = 9,
            ACTOR_CHUNK_NODEGROUPS            = 10,
            ACTOR_CHUNK_NODES                 = 11,       // Actor_Nodes
            ACTOR_CHUNK_STDPMORPHTARGETS      = 12,       // Actor_MorphTargets
            ACTOR_CHUNK_MATERIALINFO          = 13,       // Actor_MaterialInfo
            ACTOR_CHUNK_NODEMOTIONSOURCES     = 14,       // Actor_NodeMotionSources
            ACTOR_CHUNK_ATTACHMENTNODES       = 15,       // Actor_AttachmentNodes
            ACTOR_CHUNK_MATERIALATTRIBUTESET  = 16,
            ACTOR_CHUNK_GENERICMATERIAL       = 17,       // Actor_GenericMaterial
            ACTOR_CHUNK_PHYSICSSETUP          = 18,
            ACTOR_CHUNK_SIMULATEDOBJECTSETUP  = 19,
            ACTOR_FORCE_32BIT                 = 0xFFFFFFFF
        };


        // material layer map types
        enum
        {
            ACTOR_LAYERID_UNKNOWN         = 0,// unknown layer
            ACTOR_LAYERID_AMBIENT         = 1,// ambient layer
            ACTOR_LAYERID_DIFFUSE         = 2,// a diffuse layer
            ACTOR_LAYERID_SPECULAR        = 3,// specular layer
            ACTOR_LAYERID_OPACITY         = 4,// opacity layer
            ACTOR_LAYERID_BUMP            = 5,// bump layer
            ACTOR_LAYERID_SELFILLUM       = 6,// self illumination layer
            ACTOR_LAYERID_SHINE           = 7,// shininess (for specular)
            ACTOR_LAYERID_SHINESTRENGTH   = 8,// shine strength (for specular)
            ACTOR_LAYERID_FILTERCOLOR     = 9,// filter color layer
            ACTOR_LAYERID_REFLECT         = 10,// reflection layer
            ACTOR_LAYERID_REFRACT         = 11,// refraction layer
            ACTOR_LAYERID_ENVIRONMENT     = 12,// environment map layer
            ACTOR_LAYERID_DISPLACEMENT    = 13,// displacement map layer
            ACTOR_LAYERID_FORCE_8BIT      = 0xFF// don't use more than 8 bit values
        };

        // When a struct is not aligned, memset the object to 0 before using it, otherwise you might end up with some garbage paddings.

        // the actor file type header
        // (aligned)
        struct Actor_Header
        {
            uint8 mFourcc[4];   // must be "ACTR"
            uint8 mHiVersion;   // high version (2  in case of v2.34)
            uint8 mLoVersion;   // low version  (34 in case of v2.34)
            uint8 mEndianType;  // the endian in which the data is saved [0=little, 1=big]
        };


        // (not aligned)
        struct Actor_Info
        {
            uint32  mNumLODs;               // the number of level of details
            uint32  mTrajectoryNodeIndex;   // the node number of the trajectory node used for motion extraction (NOTE: unused as there is no more trajectory node)
            uint32  mMotionExtractionNodeIndex;// the node number of the trajectory node used for motion extraction
            float   mRetargetRootOffset;
            uint8   mUnitType;              // maps to EMotionFX::EUnitType
            uint8   mExporterHighVersion;
            uint8   mExporterLowVersion;

            // followed by:
            // string : source application (e.g. "3ds Max 2011", "Maya 2011")
            // string : original filename of the 3dsMax/Maya file
            // string : compilation date of the exporter
            // string : the name of the actor
        };


        // (not aligned)
        struct Actor_Info2
        {
            uint32  mNumLODs;               // the number of level of details
            uint32  mMotionExtractionNodeIndex;// the node number of the trajectory node used for motion extraction
            uint32  mRetargetRootNodeIndex; // the retargeting root node index, most likely pointing to the hip or pelvis or MCORE_INVALIDINDEX32 when not set
            uint8   mUnitType;              // maps to EMotionFX::EUnitType
            uint8   mExporterHighVersion;
            uint8   mExporterLowVersion;

            // followed by:
            // string : source application (e.g. "3ds Max 2011", "Maya 2011")
            // string : original filename of the 3dsMax/Maya file
            // string : compilation date of the exporter
            // string : the name of the actor
        };


        // (aligned)
        struct Actor_Info3
        {
            uint32  mNumLODs;               // the number of level of details
            uint32  mMotionExtractionNodeIndex;// the node number of the trajectory node used for motion extraction
            uint32  mRetargetRootNodeIndex; // the retargeting root node index, most likely pointing to the hip or pelvis or MCORE_INVALIDINDEX32 when not set
            uint8   mUnitType;              // maps to EMotionFX::EUnitType
            uint8   mExporterHighVersion;
            uint8   mExporterLowVersion;
            uint8   mOptimizeSkeleton;

            // followed by:
            // string : source application (e.g. "3ds Max 2011", "Maya 2011")
            // string : original filename of the 3dsMax/Maya file
            // string : compilation date of the exporter
            // string : the name of the actor
        };


        // a node header
        // (not aligned)
        struct Actor_Node
        {
            FileQuaternion  mLocalQuat; // the local rotation (before hierarchy)
            FileVector3     mLocalPos;  // the local translation (before hierarchy)
            FileVector3     mLocalScale;// the local scale (before hierarchy)
            uint32          mSkeletalLODs;// each bit representing if the node is active or not, in the give LOD (bit number)
            uint32          mParentIndex;// parent node number, or 0xFFFFFFFF in case of a root node
            uint32          mNumChilds; // the number of child nodes
            uint8           mNodeFlags; // #1 bit boolean specifies whether we have to include this node in the bounds calculation or not
            float           mOBB[16];

            // followed by:
            // string : node name (the unique name of the node)
        };


        // a mesh LOD level
        // (aligned)
        struct Actor_MeshLODLevel
        {
            uint32 mLODLevel;
            uint32 mSizeInBytes;

            // followed by:
            // array[uint8] The LOD model memory file
        };


        // uv (texture) coordinate
        // (aligned)
        struct Actor_UV
        {
            float   mU;
            float   mV;
        };


        // (not aligned)
        struct Actor_SkinningInfo
        {
            uint32  mNodeIndex;             // the node number in the actor
            uint32  mLOD;                   // the level of detail
            uint32  mNumLocalBones;         // number of local bones to reserve space for, this represents how many bones are used by the mesh the skinning is linked to
            uint32  mNumTotalInfluences;    // the total number of influences of all vertices together
            uint8   mIsForCollisionMesh;    // is it for a collision mesh?

            // followed by:
            //   Actor_SkinInfluence[mNumTotalInfluences]
            //   Actor_SkinningInfoTableEntry[mesh.GetNumOrgVerts()]
        };


        // (aligned)
        struct Actor_SkinningInfoTableEntry
        {
            uint32  mStartIndex;// index inside the SkinInfluence array
            uint32  mNumElements;// the number of influences for this item/entry that follow from the given start index
        };


        // a skinning influence
        // (not aligned)
        struct Actor_SkinInfluence
        {
            float   mWeight;
            uint16  mNodeNr;
        };


        // standard material, with integrated set of standard material layers
        // (aligned)
        struct Actor_StandardMaterial
        {
            uint32      mLOD;           // the level of detail
            FileColor   mAmbient;       // ambient color
            FileColor   mDiffuse;       // diffuse color
            FileColor   mSpecular;      // specular color
            FileColor   mEmissive;      // self illumination color
            float       mShine;         // shine
            float       mShineStrength; // shine strength
            float       mOpacity;       // the opacity amount [1.0=full opac, 0.0=full transparent]
            float       mIOR;           // index of refraction
            uint8       mDoubleSided;   // double sided?
            uint8       mWireFrame;     // render in wireframe?
            uint8       mTransparencyType;// F=filter / S=substractive / A=additive / U=unknown
            uint8       mNumLayers;     // the number of material layers

            // followed by:
            // string : material name
            // Actor_StandardMaterialLayer2[ mNumLayers ]
        };


        // a material layer (version 2)
        // (aligned)
        struct Actor_StandardMaterialLayer
        {
            float   mAmount;        // the amount, between 0 and 1
            float   mUOffset;       // u offset (horizontal texture shift)
            float   mVOffset;       // v offset (vertical texture shift)
            float   mUTiling;       // horizontal tiling factor
            float   mVTiling;       // vertical tiling factor
            float   mRotationRadians;// texture rotation in radians
            uint16  mMaterialNumber;// the parent material number (as read from the file, where 0 means the first material)
            uint8   mMapType;       // the map type (see enum in somewhere near the top of file)
            uint8   mBlendMode;     // blend mode that is used to control how successive layers of textures are combined together

            // followed by:
            // string : texture filename
        };


        // (aligned)
        struct Actor_GenericMaterial
        {
            uint32      mLOD;       // the level of detail

            // followed by:
            // string : material name
        };


        // a vertex attribute layer (added layer name)
        // (not aligned)
        struct Actor_VertexAttributeLayer
        {
            uint32  mLayerTypeID;   // the type of vertex attribute layer
            uint32  mAttribSizeInBytes;// the size of a single vertex attribute of this type, in bytes
            uint8   mEnableDeformations;// enable deformations on this layer?
            uint8   mIsScale;       // is this a scale value, or not? (coordinate system conversion thing)

            // followed by:
            // string : layername
            // (sizeof(mAttribSizeInBytes) * mesh.numVertices) bytes, or mesh.numVertices mDataType objects
        };


        // a submesh (with polygon support)
        // (aligned)
        struct Actor_SubMesh
        {
            uint32  mNumIndices;    // number of indices
            uint32  mNumVerts;      // number of vertices
            uint32  mNumPolygons;   // number of polygons
            uint32  mMaterialIndex; // material number, indexes into the file, so 0 means the first read material
            uint32  mNumBones;      // the number of bones used by this submesh

            // followed by:
            // uint32[mNumIndices]
            // uint8[mNumPolys]
            // uint32[mNumBones]
        };


        // a mesh (now using Actor_VertexAttributeLayer2)
        // (not aligned)
        struct Actor_Mesh
        {
            uint32  mNodeIndex;     // the node number this mesh belongs to (0 means the first node in the file, 1 means the second, etc.)
            uint32  mLOD;           // the level of detail
            uint32  mNumOrgVerts;   // number of original vertices
            uint32  mNumPolygons;   // number of polygons
            uint32  mTotalVerts;    // total number of vertices (of all submeshes)
            uint32  mTotalIndices;  // total number of indices (of all submeshes)
            uint32  mNumSubMeshes;  // number of submeshes to follow
            uint32  mNumLayers;     // the number of layers to follow
            uint8   mIsCollisionMesh;// is this mesh a collision mesh or a normal mesh?
            uint8   mIsTriangleMesh;// is this mesh a pure triangle mesh?

            // followed by:
            // Actor_VertexAttributeLayer2[mNumLayers]
            // Actor_SubMesh2[mNumSubMeshes]
        };

        //-------------------------------------------------------

        // node limit information
        // (aligned)
        struct Actor_Limit
        {
            FileVector3     mTranslationMin;// the minimum translation values
            FileVector3     mTranslationMax;// the maximum translation value.
            FileVector3     mRotationMin;   // the minimum rotation values
            FileVector3     mRotationMax;   // the maximum rotation values
            FileVector3     mScaleMin;      // the minimum scale values
            FileVector3     mScaleMax;      // the maximum scale values
            uint8           mLimitFlags[9]; // the limit type activation flags
            uint32          mNodeNumber;    // the node number where this info belongs to
        };


        // a  morph target mesh
        // (aligned)
        struct Actor_MorphTarget
        {
            float   mRangeMin;          // the slider min
            float   mRangeMax;          // the slider max
            uint32  mLOD;               // the level of detail to which this expression part belongs to
            uint32  mNumMeshDeformDeltas;// the number of mesh deform data objects to follow
            uint32  mNumTransformations;// the number of transformations to follow
            uint32  mPhonemeSets;       // the number of phoneme sets to follow

            // followed by:
            // string :  morph target name
            // Actor_MorphTargetMeshDeltas[ mNumDeformDatas ]
            // Actor_MorphTargetTransform[ mNumTransformations ]
        };


        // a chunk that contains all morph targets in the file
        // (aligned)
        struct Actor_MorphTargets
        {
            uint32  mNumMorphTargets;   // the number of morph targets to follow
            uint32  mLOD;               // the LOD level the morph targets are for

            // followed by:
            // Actor_MorphTarget[ mNumMorphTargets ]
        };


        // morph target deformation data
        // (aligned)
        struct Actor_MorphTargetMeshDeltas
        {
            uint32  mNodeIndex;
            float   mMinValue;      // minimum range value for x y and z components of the compressed position vectors
            float   mMaxValue;      // maximum range value for x y and z components of the compressed position vectors
            uint32  mNumVertices;   // the number of deltas

            // followed by:
            // File16BitVector3[ mNumVertices ]  (delta position values)
            // File8BitVector3[ mNumVertices ]   (delta normal values)
            // File8BitVector3[ mNumVertices ]   (delta tangent values)
            // uint32[ mNumVertices ]            (vertex numbers)
        };

       // morph target deformation data (including bitangents)
        struct Actor_MorphTargetMeshDeltas2
        {
            uint32  mNodeIndex;
            float   mMinValue;      // minimum range value for x y and z components of the compressed position vectors
            float   mMaxValue;      // maximum range value for x y and z components of the compressed position vectors
            uint32  mNumVertices;   // the number of deltas

            // followed by:
            // File16BitVector3[ mNumVertices ]  (delta position values)
            // File8BitVector3[ mNumVertices ]   (delta normal values)
            // File8BitVector3[ mNumVertices ]   (delta tangent values)
            // File8BitVector3[ mNumVertices ]   (delta bitangent values)
            // uint32[ mNumVertices ]            (vertex numbers)
        };


        // a morph target transformation
        // (aligned)
        struct Actor_MorphTargetTransform
        {
            uint32          mNodeIndex;         // the node name where the transform belongs to
            FileQuaternion  mRotation;          // the node rotation
            FileQuaternion  mScaleRotation;     // the node delta scale rotation
            FileVector3     mPosition;          // the node delta position
            FileVector3     mScale;             // the node delta scale
        };


        // a node group
        // (not aligned)
        struct Actor_NodeGroup
        {
            uint16  mNumNodes;
            uint8   mDisabledOnDefault; // 0 = no, 1 = yes

            // followed by:
            // string : name
            // uint16 [mNumNodes]
        };


        // (aligned)
        struct Actor_Nodes
        {
            uint32          mNumNodes;
            uint32          mNumRootNodes;
            FileVector3     mStaticBoxMin;
            FileVector3     mStaticBoxMax;
            // followed by Actor_Node4[mNumNodes] or Actor_NODE5[mNumNodes] (for v2)
        };


        // material statistics, which appears before the actual material chunks
        // (aligned)
        struct Actor_MaterialInfo
        {
            uint32  mLOD;                   // the level of detail
            uint32  mNumTotalMaterials;     // total number of materials to follow (including default/extra material)
            uint32  mNumStandardMaterials;  // the number of standard materials in the file
            uint32  mNumFXMaterials;        // the number of fx materials in the file
            uint32  mNumGenericMaterials;   // number of generic materials
        };


        // node motion sources used for the motion mirroring feature
        // (aligned)
        struct Actor_NodeMotionSources2
        {
            uint32 mNumNodes;
            // followed by uint16[mNumNodes]    // an index per node, which indicates the index of the node to extract the motion data from in case mirroring for a given motion is enabled. This array can be nullptr in case no mirroring data has been setup.
            // followed by uint8[mNumNodes]     // axis identifier (0=X, 1=Y, 2=Z)
            // followed by uint8[mNumNodes]     // flags identifier (see Actor::MirrorFlags)
        };


        // list of node number which are used for attachments
        // (aligned)
        struct Actor_AttachmentNodes
        {
            uint32 mNumNodes;
            // followed by uint16[mNumNodes]    // an index per attachment node
        };


        // material attribute set
        // (aligned)
        struct Actor_MaterialAttributeSet
        {
            uint32  mMaterialIndex;
            uint32  mLODLevel;
            // followed by MCore::AttributeSet  // see MCore::AttributeSet::CalcStreamSize() for the exact size
        };
    } // namespace FileFormat
}   // namespace EMotionFX
