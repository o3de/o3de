//---------------------------------------------------------------------------------------
// Loads and processes TressFX files.
// Inputs are binary files/streams/blobs
// Outputs are raw data that will mostly end up on the GPU.
//-------------------------------------------------------------------------------------
//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#pragma once

#include <TressFX/AMD_Types.h>
#include <TressFX/AMD_TressFX.h>
#include <Math/Transform.h>

#include <TressFX/TressFXCommon.h>


#include <memory>
#include <assert.h>
#include <vector>
#include <unordered_map>
#include <string>

#include <AzCore/std/containers/unordered_map.h>

namespace AZ::Data
{
    class AssetDataStream;
}

namespace AMD
{
#define TRESSFX_MAX_INFLUENTIAL_BONE_COUNT 4

    static constexpr char TFXFileExtension[] = "tfx";               // Contains the hair vertices data.
    static constexpr char TFXBoneFileExtension[] = "tfxbone";       // Contains the hair skinning data.
    static constexpr char TFXMeshFileExtension[] = "tfxmesh";       // Contains the hair collision object.
    static constexpr char TFXCombinedFileExtension[] = "tfxhair";   // A container file with all the above data.
    static constexpr float s_hairBoundingBoxMaxExtent = 10.0f;      // Value used to check if the scale of the hair exceed usual.

    // The header struct for the .tfxhair file generated in cache. The .tfxhair file is a combined file of .tfx, .tfxbone
    // and .tfxmesh.
    struct TressFXCombinedHairFileHeader
    {
        uint64 offsetTFX = 0;
        uint64 offsetTFXBone = 0;
        uint64 offsetTFXMesh = 0;
    };

    struct TressFXBoneSkinningData
    {   // Possible improvement can be to use int 32 with high / low bits encoding
        float boneIndex[TRESSFX_MAX_INFLUENTIAL_BONE_COUNT];    
        float weight[TRESSFX_MAX_INFLUENTIAL_BONE_COUNT];
    };

    #define EI_Read(ptr, size, pFile) fread(ptr, size, 1, pFile)
    #define EI_Seek(pFile, offset) fseek(pFile, offset, SEEK_SET)

    using LocalToGlobalBoneIndexLookup = std::vector<uint32>; // Index -> TressFX Bone Index (Local set of bones)
                                                                // Value -> EmotionFX Bone Index (Global set of bones)
    using BoneNameToIndexMap = std::unordered_map<std::string, int>;

    class TressFXCollisionMesh
    {
    public:
        std::vector<AMD::float3> m_vertices;
        std::vector<AMD::float3> m_normals;
        std::vector<int> m_indices;

        // The skinning for the collision mesh only. Do not confuse it with hair skinning or the object skinning.
        std::vector<TressFXBoneSkinningData> m_boneSkinningData;

        std::vector<std::string> m_boneNames;

        // This function is mimicking LoadTressFXCollisionMeshData in TressFXBoneSkinnin
        bool LoadMeshData(AZ::Data::AssetDataStream* stream);
    };

    class TressFXAsset
    {
    public:
        TressFXAsset();
        ~TressFXAsset();

        // Hair data from *.tfx
        std::vector<Vector3> m_positions;   // Inspite of the confusing name, this is actually Vector4 with w=1.0
        std::vector<AMD::float2> m_strandUV;
        std::vector<AMD::float4> m_tangents;
        std::vector<Vector3> m_followRootOffsets;
        std::vector<AMD::int32>      m_strandTypes;
        std::vector<AMD::real32>     m_thicknessCoeffs;
        std::vector<AMD::real32>     m_restLengths;
        std::vector<AMD::int32>      m_triangleIndices;


        // Bone skinning data from *.tfxbone - this represents the bones Id affecting hairs (not that actual bone structure)
        std::vector<TressFXBoneSkinningData> m_boneSkinningData;

        // Stores a mapping of the bone index to actual bone names.
        std::vector<std::string> m_boneNames;

        // counts on hair data
        AMD::int32 m_numTotalStrands;
        AMD::int32 m_numTotalVertices;
        AMD::int32 m_numVerticesPerStrand;
        AMD::int32 m_numGuideStrands;
        AMD::int32 m_numGuideVertices;
        AMD::int32 m_numFollowStrandsPerGuide;

        // Currently we only deal with a single collision mesh assumed to be the skinned mesh.
        // This might not always be the case and a new scheme might be required.
        std::unique_ptr<TressFXCollisionMesh> m_collisionMesh;

        // Loads *.tfx hair data 
        bool LoadHairData(FILE* ioObject);

        // Generates follow hairs procedurally.  If numFollowHairsPerGuideHair is zero, then this function won't do anything. 
        bool GenerateFollowHairs(int numFollowHairsPerGuideHair = 0, float tipSeparationFactor = 0, float maxRadiusAroundGuideHair = 0);

        // Computes various parameters for simulation and rendering. After calling this function, data is ready to be passed to hair object. 
        bool ProcessAsset();

        //! Given the *.tfxbon file that contains bones hair skinning data, this method returns
        //!  the used bones names array of the current hair object so it can be matched with
        //!  the skeleton bones names to get the indices in the global skeleton array.
        void GetBonesNames(FILE* ioObject, std::vector<std::string>& boneNames);
        //! Given the bones hair skinning data file and the pairing local indices to skeleton indices array,
        //!  this method convert all bones indices per vertex and creates the skinning data array. 
        bool LoadBoneData(FILE* ioObject, std::vector<int32_t> skeletonBoneIndices);

        // This function load the combined hair data (.tfxhair file) generated in the cache using the custom builder.
        // Inside the function it will call LoadHairData, LoadBoneData and LoadCollisionData function. The TressFX asset
        // will be fully populated after this function return true.
        bool LoadCombinedHairData(AZ::Data::AssetDataStream* stream);

        // Similar function to the above but read the assetDataStream instead. Those function will eventually replacing their counterpart.
        // Notice: The loadCombineHairData will control the seek position of the stream, which guaranteed those stream will be at the correct
        // position when entering those function.
        bool LoadHairData(AZ::Data::AssetDataStream* stream);
        void GetBonesNames(AZ::Data::AssetDataStream* stream, std::vector<std::string>& boneNames);
        bool LoadBoneData(AZ::Data::AssetDataStream* stream);

        inline AMD::uint32 GetNumHairSegments() { return m_numTotalStrands * (m_numVerticesPerStrand - 1); }
        inline AMD::uint32 GetNumHairTriangleIndices() { return 6 * GetNumHairSegments(); }
        inline AMD::uint32 GetNumHairLineIndices() { return 2 * GetNumHairSegments(); }

        // Generates a local to global bone index lookup for hair and collision. We are passing only a subset of the full bone information found in an
        // emfx actor to the shader. The purpose of the index lookup is to map an emfx actor bone to the subset consisting of
        // TressFX bones. Essentially, the full set of bones found in an emfx actor are the global bones, while the bones in a
        // TressFX asset are the local bones.
        bool GenerateLocaltoGlobalHairBoneIndexLookup(const BoneNameToIndexMap& globalBoneIndexMap, LocalToGlobalBoneIndexLookup& outLookup);
        bool GenerateLocaltoGlobalCollisionBoneIndexLookup(const BoneNameToIndexMap& globalBoneIndexMap, LocalToGlobalBoneIndexLookup& outLookup);
    private:

        // Generates a local to global bone index lookup for specified bones only. This can be called if we only need information
        // for certain bones such as ones contained in a collision mesh. 
        bool GenerateLocaltoGlobalBoneIndexLookup(const BoneNameToIndexMap& globalBoneIndexMap /*Global BoneNames to BoneIndex*/,
            const std::vector<std::string>& boneNames /*BoneNames used by this skinning data*/, LocalToGlobalBoneIndexLookup& outLookup);

        // Helper functions for ProcessAsset. 
        void ComputeThicknessCoeffs();
        void ComputeStrandTangent();
        void ComputeRestLengths();
        void FillTriangleIndexArray();
    };
    // #endif // AMD_TRESSFX_ASSET_H

} // namespace AMD

