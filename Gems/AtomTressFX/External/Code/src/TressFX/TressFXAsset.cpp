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

//#include <Math/Transform.h>
#include <Math/Vector3D.h>
#include <TressFX/TressFXAsset.h>
#include <TressFX/TressFXFileFormat.h>

#include <vector>
#include <string>

#include <AzCore/Asset/AssetDataStream.h>
#include <AzCore/Serialization/Locale.h>
#include <AzCore/Math/Aabb.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AMD
{
    static void GetTangentVectors(const Vector3& n, Vector3& t0, Vector3& t1)
    {
        if (fabsf(n[2]) > 0.707f)
        {
            float a = n[1] * n[1] + n[2] * n[2];
            float k = 1.0f / sqrtf(a);
            t0[0] = 0;
            t0[1] = -n[2] * k;
            t0[2] = n[1] * k;

            t1[0] = a * k;
            t1[1] = -n[0] * t0[2];
            t1[2] = n[0] * t0[1];
        }
        else
        {
            float a = n[0] * n[0] + n[1] * n[1];
            float k = 1.0f / sqrtf(a);
            t0[0] = -n[1] * k;
            t0[1] = n[0] * k;
            t0[2] = 0;

            t1[0] = -n[2] * t0[1];
            t1[1] = n[2] * t0[0];
            t1[2] = a * k;
        }
    }

    static float GetRandom(float Min, float Max)
    {
        return ((float(rand()) / float(RAND_MAX)) * (Max - Min)) + Min;
    }

    // This function is mimicking LoadTressFXCollisionMeshData in TressFXBoneSkinning with a few tweaks.
    // 1) It reads from an asset data stream instead of a std file stream.
    // 2) Reading all data to an array of strings at once instead of reading line by line.
    bool TressFXCollisionMesh::LoadMeshData(AZ::Data::AssetDataStream* stream)
    {
        if (!stream->IsOpen())
        {
            return false;
        }

        // interpret the data in the culture invariant locale so that it doesn't matter
        // what locale the machine of the user is set to.
        AZ::Locale::ScopedSerializationLocale scopedLocale;

        int numOfBones;
        AZStd::vector<AZStd::string> boneNames;

        // Read the stream to a buffer and parse it line by line
        AZStd::vector<char> charBuffer;
        charBuffer.resize_no_construct(stream->GetLength() + 1);
        stream->Read(stream->GetLength(), charBuffer.data());

        AZStd::vector<AZStd::string> sTokens;
        AZStd::vector<AZStd::string> fileLines;
        AzFramework::StringFunc::Tokenize(charBuffer.data(), fileLines, "\r\n");

        for (int lineIdx = 0; lineIdx < fileLines.size(); ++lineIdx)
        {
            const AZStd::string& line = fileLines[lineIdx];

            if (line.length() == 0)
                continue;

            // If # is in the very first column in the line, it is a comment.
            if (line[0] == '#')
                continue;

            sTokens.clear();
            AzFramework::StringFunc::Tokenize(line.data(), sTokens, " ");

            AZStd::string token;
            if (!sTokens.empty())
            {
                token = sTokens[0];
            }
            else
            {
                token = line;
            }

            // Load bone names.
            if (token.find("numOfBones") != std::string::npos)
            {
                numOfBones = atoi(sTokens[1].c_str());
                int countBone = 0;

                // Continue reading the file.
                for (++lineIdx; lineIdx < fileLines.size(); ++lineIdx)
                {
                    // Next line
                    const AZStd::string& sLine = fileLines[lineIdx];

                    if (sLine.length() == 0)
                        continue;

                    // If # is in the very first column in the line, it is a comment.
                    if (sLine[0] == '#')
                        continue;

                    sTokens.clear();
                    AzFramework::StringFunc::Tokenize(sLine.data(), sTokens, " ");
                    m_boneNames.push_back(sTokens[1].c_str());
                    countBone++;

                    if (countBone == numOfBones)
                        break;
                }
            }

            if (token.find("numOfVertices") != std::string::npos) // load bone indices and weights for each strand
            {
                const int numVertices = (AMD::uint32)atoi(sTokens[1].c_str());
                m_boneSkinningData.resize(numVertices);
                m_vertices.resize(numVertices);
                m_normals.resize(numVertices);
                memset(m_boneSkinningData.data(), 0, sizeof(TressFXBoneSkinningData) * numVertices);

                // Continue reading the file.
                int index = 0;
                for (++lineIdx; lineIdx < fileLines.size(); ++lineIdx)
                {
                    // Next line
                    const AZStd::string& sLine = fileLines[lineIdx];

                    if (sLine.length() == 0)
                        continue;

                    // If # is in the very first column in the line, it is a comment.
                    if (sLine[0] == '#')
                        continue;

                    sTokens.clear();
                    AzFramework::StringFunc::Tokenize(sLine.data(), sTokens, " ");
                    assert(sTokens.size() == 15);

                    int vertexIndex = atoi(sTokens[0].c_str());
                    AZ_UNUSED(vertexIndex);
                    assert(vertexIndex == index);

                    AMD::float3& pos = m_vertices[index];
                    pos.x = (float)atof(sTokens[1].c_str());
                    pos.y = (float)atof(sTokens[2].c_str());
                    pos.z = (float)atof(sTokens[3].c_str());

                    AMD::float3& normal = m_normals[index];
                    normal.x = (float)atof(sTokens[4].c_str());
                    normal.y = (float)atof(sTokens[5].c_str());
                    normal.z = (float)atof(sTokens[6].c_str());

                    TressFXBoneSkinningData skinData;

                    // Those indices stored in the skin data refer to local bones specific to this TressFX asset, not emotionFx/global bones.
                    // Bone indices are local to the skinning data (start from 0), while global bone indices are mapped to the entire skeleton.
                    int boneIndex = atoi(sTokens[7].c_str());
                    skinData.boneIndex[0] = (float)boneIndex;

                    boneIndex = atoi(sTokens[8].c_str());
                    skinData.boneIndex[1] = (float)boneIndex;

                    boneIndex = atoi(sTokens[9].c_str());
                    skinData.boneIndex[2] = (float)boneIndex;

                    boneIndex = atoi(sTokens[10].c_str());
                    skinData.boneIndex[3] = (float)boneIndex;

                    skinData.weight[0] = (float)atof(sTokens[11].c_str());
                    skinData.weight[1] = (float)atof(sTokens[12].c_str());
                    skinData.weight[2] = (float)atof(sTokens[13].c_str());
                    skinData.weight[3] = (float)atof(sTokens[14].c_str());

                    m_boneSkinningData[index] = skinData;

                    ++index;

                    if (index == numVertices)
                        break;
                }
            }
            else if (token.find("numOfTriangles") != std::string::npos) // triangle indices
            {
                const int numTriangles = atoi(sTokens[1].c_str());
                int numIndices = numTriangles * 3;
                m_indices.resize(numIndices);
                int index = 0;

                // Continue reading the file.
                for (++lineIdx; lineIdx < fileLines.size(); ++lineIdx)
                {
                    // next line
                    const AZStd::string& sLine = fileLines[lineIdx];

                    if (sLine.length() == 0)
                        continue;

                    // If # is in the very first column in the line, it is a comment.
                    if (sLine[0] == '#')
                        continue;

                    sTokens.clear();
                    AzFramework::StringFunc::Tokenize(sLine.data(), sTokens, " ");
                    assert(sTokens.size() == 4);

                    int triangleIndex = atoi(sTokens[0].c_str());
                    AZ_UNUSED(triangleIndex);
                    assert(triangleIndex == index);

                    m_indices[index * 3 + 0] = atoi(sTokens[1].c_str());
                    m_indices[index * 3 + 1] = atoi(sTokens[2].c_str());
                    m_indices[index * 3 + 2] = atoi(sTokens[3].c_str());

                    ++index;

                    if (index == numTriangles)
                        break;
                }
            }
        }

        // [To Do] - check if the follow bone is required for the collision mesh.
        // m_pScene = scene;
        // m_followBone = scene->GetBoneIdByName(skinNumber, followBone);

        return true;
    }

    TressFXAsset::TressFXAsset()
        : m_numTotalStrands(0)
        , m_numTotalVertices(0)
        , m_numVerticesPerStrand(0)
        , m_numGuideStrands(0)
        , m_numGuideVertices(0)
        , m_numFollowStrandsPerGuide(0)
    {
    }

    TressFXAsset::~TressFXAsset()
    {
    }

    bool TressFXAsset::LoadHairData(FILE* ioObject)
    {
        TressFXTFXFileHeader header = {};

        // read the header
        [[maybe_unused]] auto eiSeekResult = EI_Seek(ioObject, 0); // make sure the stream pos is at the beginning. 
        [[maybe_unused]] auto eiReadResult = EI_Read((void*)&header, sizeof(TressFXTFXFileHeader), ioObject);

        // If the tfx version is lower than the current major version, exit. 
        if (header.version < AMD_TRESSFX_VERSION_MAJOR)
        {
            return false;
        }

        unsigned int numStrandsInFile = header.numHairStrands;

        // We make the number of strands be multiple of TRESSFX_SIM_THREAD_GROUP_SIZE. 
        m_numGuideStrands = (numStrandsInFile - numStrandsInFile % TRESSFX_SIM_THREAD_GROUP_SIZE) + TRESSFX_SIM_THREAD_GROUP_SIZE;

        m_numVerticesPerStrand = header.numVerticesPerStrand;

        // Make sure number of vertices per strand is greater than two and less than or equal to
        // thread group size (64). Also thread group size should be a mulitple of number of
        // vertices per strand. So possible number is 4, 8, 16, 32 and 64.
        TRESSFX_ASSERT(m_numVerticesPerStrand > 2 && m_numVerticesPerStrand <= TRESSFX_SIM_THREAD_GROUP_SIZE && TRESSFX_SIM_THREAD_GROUP_SIZE % m_numVerticesPerStrand == 0);

        m_numFollowStrandsPerGuide = 0;
        m_numTotalStrands = m_numGuideStrands; // Until we call GenerateFollowHairs, the number of total strands is equal to the number of guide strands. 
        m_numGuideVertices = m_numGuideStrands * m_numVerticesPerStrand;
        m_numTotalVertices = m_numGuideVertices; // Again, the total number of vertices is equal to the number of guide vertices here. 

        TRESSFX_ASSERT(m_numTotalVertices % TRESSFX_SIM_THREAD_GROUP_SIZE == 0); // number of total vertices should be multiple of thread group size. 
                                                                                 // This assert is actually redundant because we already made m_numGuideStrands
                                                                                 // and m_numTotalStrands are multiple of thread group size. 
                                                                                 // Just demonstrating the requirement for number of vertices here in case 
                                                                                 // you are to make your own loader. 

        m_positions.resize(m_numTotalVertices); // size of m_positions = number of total vertices * sizeo of each position vector. 

                                                // Read position data from the io stream. 
        eiSeekResult = EI_Seek(ioObject, header.offsetVertexPosition);
        eiReadResult = EI_Read((void*)m_positions.data(), numStrandsInFile * m_numVerticesPerStrand * sizeof(AMD::float4), ioObject); // note that the position data in io stream contains only guide hairs. If we call GenerateFollowHairs
                                                                                                                       // to generate follow hairs, m_positions will be re-allocated. 

                                                                                                                       // We need to make up some strands to fill up the buffer because the number of strands from stream is not necessarily multile of thread size. 
        AMD::int32 numStrandsToMakeUp = m_numGuideStrands - numStrandsInFile;

        for (AMD::int32 i = 0; i < numStrandsToMakeUp; ++i)
        {
            for (AMD::int32 j = 0; j < m_numVerticesPerStrand; ++j)
            {
                AMD::int32 indexLastVertex = (numStrandsInFile - 1) * m_numVerticesPerStrand + j;
                AMD::int32 indexVertex = (numStrandsInFile + i) * m_numVerticesPerStrand + j;
                m_positions[indexVertex] = m_positions[indexLastVertex];
            }
        }

        // Read strand UVs
        eiSeekResult = EI_Seek(ioObject, header.offsetStrandUV);
        m_strandUV.resize(m_numTotalStrands); // If we call GenerateFollowHairs to generate follow hairs, 
                                              // m_strandUV will be re-allocated. 

        eiReadResult = EI_Read((void*)m_strandUV.data(), numStrandsInFile * sizeof(AMD::float2), ioObject);

        // Fill up the last empty space
        AMD::int32 indexLastStrand = (numStrandsInFile - 1);

        for (int i = 0; i < numStrandsToMakeUp; ++i)
        {
            AMD::int32 indexStrand = (numStrandsInFile + i);
            m_strandUV[indexStrand] = m_strandUV[indexLastStrand];
        }

        m_followRootOffsets.resize(m_numTotalStrands);

        // Fill m_followRootOffsets with zeros
        memset(m_followRootOffsets.data(), 0, m_numTotalStrands * sizeof(AMD::float4));

        return true;
    }

    bool TressFXAsset::LoadHairData(AZ::Data::AssetDataStream* stream)
    {
        TressFXTFXFileHeader header = {};

        // read the header
        stream->Read(sizeof(TressFXTFXFileHeader), (void*)&header);

        // If the tfx version is lower than the current major version, exit.
        if (header.version < AMD_TRESSFX_VERSION_MAJOR)
        {
            return false;
        }

        unsigned int numStrandsInFile = header.numHairStrands;

        // We make the number of strands be multiple of TRESSFX_SIM_THREAD_GROUP_SIZE.
        m_numGuideStrands = (numStrandsInFile - numStrandsInFile % TRESSFX_SIM_THREAD_GROUP_SIZE) + TRESSFX_SIM_THREAD_GROUP_SIZE;

        m_numVerticesPerStrand = header.numVerticesPerStrand;

        // Make sure number of vertices per strand is greater than two and less than or equal to
        // thread group size (64). Also thread group size should be a mulitple of number of
        // vertices per strand. So possible number is 4, 8, 16, 32 and 64.
        TRESSFX_ASSERT(
            m_numVerticesPerStrand > 2 && m_numVerticesPerStrand <= TRESSFX_SIM_THREAD_GROUP_SIZE &&
            TRESSFX_SIM_THREAD_GROUP_SIZE % m_numVerticesPerStrand == 0);

        m_numFollowStrandsPerGuide = 0;
        m_numTotalStrands =
            m_numGuideStrands; // Until we call GenerateFollowHairs, the number of total strands is equal to the number of guide strands.
        m_numGuideVertices = m_numGuideStrands * m_numVerticesPerStrand;
        m_numTotalVertices = m_numGuideVertices; // Again, the total number of vertices is equal to the number of guide vertices here.

        TRESSFX_ASSERT(
            m_numTotalVertices % TRESSFX_SIM_THREAD_GROUP_SIZE == 0); // number of total vertices should be multiple of thread group size.
                                                                      // This assert is actually redundant because we already made
                                                                      // m_numGuideStrands and m_numTotalStrands are multiple of thread
                                                                      // group size. Just demonstrating the requirement for number of
                                                                      // vertices here in case you are to make your own loader.

        m_positions.resize(m_numTotalVertices); // size of m_positions = number of total vertices * sizeo of each position vector.

        // Read position data from the io stream.
        stream->Seek(header.offsetVertexPosition + sizeof(TressFXCombinedHairFileHeader), AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
        stream->Read(numStrandsInFile * m_numVerticesPerStrand * sizeof(AMD::float4), (void*)m_positions.data());
        // note that the position data in io stream contains only guide hairs. If we call GenerateFollowHairs
        // to generate follow hairs, m_positions will be re-allocated.

        // We need to make up some strands to fill up the buffer because the number of strands from stream is not necessarily multile of
        // thread size.
        AMD::int32 numStrandsToMakeUp = m_numGuideStrands - numStrandsInFile;

        for (AMD::int32 i = 0; i < numStrandsToMakeUp; ++i)
        {
            for (AMD::int32 j = 0; j < m_numVerticesPerStrand; ++j)
            {
                AMD::int32 indexLastVertex = (numStrandsInFile - 1) * m_numVerticesPerStrand + j;
                AMD::int32 indexVertex = (numStrandsInFile + i) * m_numVerticesPerStrand + j;
                m_positions[indexVertex] = m_positions[indexLastVertex];
            }
        }

        // Calculate bounding box and check if it exported in meters.
        AZ::Aabb bbox = AZ::Aabb::CreateNull();
        for (AMD::int32 i = 0; i < m_numTotalVertices; ++i)
        {
            bbox.AddPoint(AZ::Vector3(m_positions[i].x, m_positions[i].y, m_positions[i].z));
        }
        AZ_Error("TressFXAsset", bbox.GetXExtent() < s_hairBoundingBoxMaxExtent &&
            bbox.GetYExtent() < s_hairBoundingBoxMaxExtent && bbox.GetZExtent() < s_hairBoundingBoxMaxExtent,
            "Hair units seem to be in cm, creating extremely large hair - please export again using meters");

        // Read strand UVs
        stream->Seek(header.offsetStrandUV + sizeof(TressFXCombinedHairFileHeader), AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
        m_strandUV.resize(m_numTotalStrands); // If we call GenerateFollowHairs to generate follow hairs,
                                              // m_strandUV will be re-allocated.

        stream->Read(numStrandsInFile * sizeof(AMD::float2), (void*)m_strandUV.data());

        // Fill up the last empty space
        AMD::int32 indexLastStrand = (numStrandsInFile - 1);

        for (int i = 0; i < numStrandsToMakeUp; ++i)
        {
            AMD::int32 indexStrand = (numStrandsInFile + i);
            m_strandUV[indexStrand] = m_strandUV[indexLastStrand];
        }

        m_followRootOffsets.resize(m_numTotalStrands);

        // Fill m_followRootOffsets with zeros
        memset(m_followRootOffsets.data(), 0, m_numTotalStrands * sizeof(AMD::float4));

        return true;
    }

    // This generates follow hairs around loaded guide hairs procedually with random distribution within the max radius input. 
    // Calling this is optional. 
    bool TressFXAsset::GenerateFollowHairs(int numFollowHairsPerGuideHair, float tipSeparationFactor, float maxRadiusAroundGuideHair)
    {
        TRESSFX_ASSERT(numFollowHairsPerGuideHair >= 0);

        m_numFollowStrandsPerGuide = numFollowHairsPerGuideHair;

        // Nothing to do, just exit. 
        if (numFollowHairsPerGuideHair == 0)
            return false;

        // Recompute total number of hair strands and vertices with considering number of follow hairs per a guide hair. 
        m_numTotalStrands = m_numGuideStrands * (m_numFollowStrandsPerGuide + 1);
        m_numTotalVertices = m_numTotalStrands * m_numVerticesPerStrand;

        // keep the old buffers until the end of this function. 
        std::vector<Vector3> positionsGuide = m_positions;
        std::vector<AMD::float2> strandUVGuide = m_strandUV;

        // re-allocate all buffers
        m_positions.resize(m_numTotalVertices);
        m_strandUV.resize(m_numTotalStrands);

        m_followRootOffsets.resize(m_numTotalStrands);

        // type-cast to Vector3 to handle data easily. 
        Vector3* pos = m_positions.data();
        Vector3* followOffset = m_followRootOffsets.data();

        // Generate follow hairs
        for (int i = 0; i < m_numGuideStrands; i++)
        {
            int indexGuideStrand = i * (m_numFollowStrandsPerGuide + 1);
            int indexRootVertMaster = indexGuideStrand * m_numVerticesPerStrand;

            memcpy(&pos[indexRootVertMaster], &positionsGuide[i * m_numVerticesPerStrand], sizeof(Vector3) * m_numVerticesPerStrand);
            m_strandUV[indexGuideStrand] = strandUVGuide[i];

            followOffset[indexGuideStrand].Set(0, 0, 0);
            followOffset[indexGuideStrand].w = (float)indexGuideStrand;
            Vector3 v01 = pos[indexRootVertMaster + 1] - pos[indexRootVertMaster];
            v01.Normalize();

            // Find two orthogonal unit tangent vectors to v01
            Vector3 t0, t1;
            GetTangentVectors(v01, t0, t1);

            for (int j = 0; j < m_numFollowStrandsPerGuide; j++)
            {
                int indexStrandFollow = indexGuideStrand + j + 1;
                int indexRootVertFollow = indexStrandFollow * m_numVerticesPerStrand;

                m_strandUV[indexStrandFollow] = m_strandUV[indexGuideStrand];

                // offset vector from the guide strand's root vertex position
                Vector3 offset = GetRandom(-maxRadiusAroundGuideHair, maxRadiusAroundGuideHair) * t0 + GetRandom(-maxRadiusAroundGuideHair, maxRadiusAroundGuideHair) * t1;
                followOffset[indexStrandFollow] = offset;
                followOffset[indexStrandFollow].w = (float)indexGuideStrand;

                for (int k = 0; k < m_numVerticesPerStrand; k++)
                {
                    const Vector3* guideVert = &pos[indexRootVertMaster + k];
                    Vector3* followVert = &pos[indexRootVertFollow + k];

                    float factor = tipSeparationFactor * ((float)k / ((float)m_numVerticesPerStrand)) + 1.0f;
                    *followVert = *guideVert + offset * factor;
                    (*followVert).w = guideVert->w;
                }
            }
        }

        return true;
    }

    bool TressFXAsset::ProcessAsset()
    {
        m_strandTypes.resize(m_numTotalStrands);
        m_tangents.resize(m_numTotalVertices);
        m_restLengths.resize(m_numTotalVertices);
        m_thicknessCoeffs.resize(m_numTotalVertices);
        m_triangleIndices.resize(GetNumHairTriangleIndices());

        // compute tangent vectors
        ComputeStrandTangent();

        // compute thickness coefficients
        ComputeThicknessCoeffs();

        // compute rest lengths
        ComputeRestLengths();

        // triangle index
        FillTriangleIndexArray();

        for (int i = 0; i < m_numTotalStrands; i++)
            m_strandTypes[i] = 0;

        return true;
    }

    void TressFXAsset::FillTriangleIndexArray()
    {
        TRESSFX_ASSERT(m_numTotalVertices == m_numTotalStrands * m_numVerticesPerStrand);
        TRESSFX_ASSERT(m_triangleIndices.size() != 0);

        AMD::int32 id = 0;
        int iCount = 0;

        for (int i = 0; i < m_numTotalStrands; i++)
        {
            for (int j = 0; j < m_numVerticesPerStrand - 1; j++)
            {
                m_triangleIndices[iCount++] = 2 * id;
                m_triangleIndices[iCount++] = 2 * id + 1;
                m_triangleIndices[iCount++] = 2 * id + 2;
                m_triangleIndices[iCount++] = 2 * id + 2;
                m_triangleIndices[iCount++] = 2 * id + 1;
                m_triangleIndices[iCount++] = 2 * id + 3;

                id++;
            }

            id++;
        }

        TRESSFX_ASSERT(iCount == 6 * m_numTotalStrands * (m_numVerticesPerStrand - 1)); // iCount == GetNumHairTriangleIndices()
    }

    void TressFXAsset::ComputeStrandTangent()
    {
        Vector3* pos = (Vector3*)m_positions.data();
        Vector3* tan = (Vector3*)m_tangents.data();

        for (int iStrand = 0; iStrand < m_numTotalStrands; ++iStrand)
        {
            int indexRootVertMaster = iStrand * m_numVerticesPerStrand;

            // vertex 0
            {
                Vector3& vert_0 = pos[indexRootVertMaster];
                Vector3& vert_1 = pos[indexRootVertMaster + 1];

                Vector3 tangent = vert_1 - vert_0;
                tangent.Normalize();
                tan[indexRootVertMaster] = tangent;
            }

            // vertex 1 through n-1
            for (int i = 1; i < (int)m_numVerticesPerStrand - 1; i++)
            {
                Vector3& vert_i_minus_1 = pos[indexRootVertMaster + i - 1];
                Vector3& vert_i = pos[indexRootVertMaster + i];
                Vector3& vert_i_plus_1 = pos[indexRootVertMaster + i + 1];

                Vector3 tangent_pre = vert_i - vert_i_minus_1;
                tangent_pre.Normalize();

                Vector3 tangent_next = vert_i_plus_1 - vert_i;
                tangent_next.Normalize();

                Vector3 tangent = tangent_pre + tangent_next;
                tangent = tangent.Normalize();

                tan[indexRootVertMaster + i] = tangent;
            }
        }
    }

    void TressFXAsset::ComputeThicknessCoeffs()
    {
        Vector3* pos = (Vector3*)m_positions.data();

        int index = 0;

        for (int iStrand = 0; iStrand < m_numTotalStrands; ++iStrand)
        {
            int   indexRootVertMaster = iStrand * m_numVerticesPerStrand;
            float strandLength = 0;
            float tVal = 0;

            // vertex 1 through n
            for (int i = 1; i < (int)m_numVerticesPerStrand; ++i)
            {
                Vector3& vert_i_minus_1 = pos[indexRootVertMaster + i - 1];
                Vector3& vert_i = pos[indexRootVertMaster + i];

                Vector3 vec = vert_i - vert_i_minus_1;
                float        disSeg = vec.Length();

                tVal += disSeg;
                strandLength += disSeg;
            }

            for (int i = 0; i < (int)m_numVerticesPerStrand; ++i)
            {
                tVal /= strandLength;
                m_thicknessCoeffs[index++] = sqrt(1.f - tVal * tVal);
            }
        }
    }

    void TressFXAsset::ComputeRestLengths()
    {
        Vector3* pos = (Vector3*)m_positions.data();
        float* restLen = (float*)m_restLengths.data();

        int index = 0;

        // Calculate rest lengths
        for (int i = 0; i < m_numTotalStrands; i++)
        {
            int indexRootVert = i * m_numVerticesPerStrand;

            for (int j = 0; j < m_numVerticesPerStrand - 1; j++)
            {
                restLen[index++] =
                    (pos[indexRootVert + j] - pos[indexRootVert + j + 1]).Length();
            }

            // Since number of edges are one less than number of vertices in hair strand, below
            // line acts as a placeholder.
            restLen[index++] = 0;
        }
    }

    void TressFXAsset::GetBonesNames(FILE* ioObject, std::vector<std::string>& boneNames)
    {
        AMD::int32 numOfBones = 0;
        [[maybe_unused]] auto eiSeekResult = EI_Seek(ioObject, 0);
        [[maybe_unused]] auto eiReadResult = EI_Read((void*)&numOfBones, sizeof(AMD::int32), ioObject);

        //    boneNames.reserve(numOfBones);
        boneNames.resize(numOfBones);
        for (int i = 0; i < numOfBones; i++)
        {
            int boneIndex;
            eiReadResult = EI_Read((char*)&boneIndex, sizeof(AMD::int32), ioObject);

            AMD::int32 charLen = 0;
            eiReadResult = EI_Read((char*)&charLen, sizeof(AMD::int32), ioObject); // character length includes null termination already.

            char boneName[128];
            eiReadResult = EI_Read(boneName, sizeof(char) * charLen, ioObject);
            boneName[charLen] = '\0';   // adding 0 termination to be on the safe side.
            boneNames[i] = std::string(boneName);
        }
    }

    void TressFXAsset::GetBonesNames(AZ::Data::AssetDataStream* stream, std::vector<std::string>& boneNames)
    {
        AMD::int32 numOfBones = 0;
        stream->Read(sizeof(AMD::int32), &numOfBones);

        boneNames.resize(numOfBones);
        for (int i = 0; i < numOfBones; i++)
        {
            int boneIndex;
            stream->Read(sizeof(AMD::int32), &boneIndex);

            AMD::int32 charLen = 0;
            stream->Read(sizeof(AMD::int32), &charLen); // character length includes null termination already.

            char boneName[128];
            stream->Read(sizeof(char) * charLen, &boneName);
            boneName[charLen] = '\0'; // adding 0 termination to be on the safe side.
            boneNames[i] = std::string(boneName);
        }
    }


    bool TressFXAsset::LoadBoneData(FILE* ioObject, std::vector<int32_t> skeletonBoneIndices)
    {
        m_boneSkinningData.resize(0);

        AMD::int32 numOfBones = 0;
        [[maybe_unused]] auto eiSeekResult = EI_Seek(ioObject, 0);
        [[maybe_unused]] auto eiReadResult = EI_Read((void*)&numOfBones, sizeof(AMD::int32), ioObject);

        if (skeletonBoneIndices.size() != numOfBones)
        {
            TRESSFX_ASSERT(skeletonBoneIndices.size() != numOfBones);
            return false;
        }

        for (int i = 0; i < numOfBones; i++)
        {
            int boneIndex;
            eiReadResult = EI_Read((char*)&boneIndex, sizeof(AMD::int32), ioObject);

            AMD::int32 charLen = 0;
            eiReadResult = EI_Read((char*)&charLen, sizeof(AMD::int32), ioObject); // character length includes null termination already.

            char boneName[128];
            eiReadResult = EI_Read(boneName, sizeof(char) * charLen, ioObject);
        }

        // Reading the number of strands 
        AMD::int32 numOfStrandsInStream = 0;
        eiReadResult = EI_Read((char*)&numOfStrandsInStream, sizeof(AMD::int32), ioObject);

        //If the number of strands from the input stream (tfxbone) is bigger than what we already know from tfx, something is wrong.
        if (m_numGuideStrands < numOfStrandsInStream)
            return 0;

        m_boneSkinningData.resize(m_numTotalStrands);

        TressFXBoneSkinningData skinData = TressFXBoneSkinningData();
        for (int i = 0; i < numOfStrandsInStream; ++i)
        {
            AMD::int32 index = 0; // Well, we don't really use this here. 
            eiReadResult = EI_Read((char*)&index, sizeof(AMD::int32), ioObject);

            for (AMD::int32 j = 0; j < TRESSFX_MAX_INFLUENTIAL_BONE_COUNT; ++j)
            {
                AMD::int32 boneIndex;
                eiReadResult = EI_Read((char*)&boneIndex, sizeof(AMD::int32), ioObject);
                assert(boneIndex >= 0);
                skinData.boneIndex[j] = (float)skeletonBoneIndices[boneIndex]; // Change the joint index to be what the engine wants
                eiReadResult = EI_Read((char*)&skinData.weight[j], sizeof(AMD::real32), ioObject);
            }

#if defined(AZ_ENABLE_TRACING)
            float weightSum = skinData.weight[0] + skinData.weight[1] + skinData.weight[2] + skinData.weight[3];
            AZ_Assert(weightSum > 0.0f, "Weight sum should be greater than 0");
#endif // AZ_ENABLE_TRACING

            assert(skinData.weight[0] != 0.0f);

            // If bone index is -1, then it means that there is no bone associated to this. In this case we simply replace it with zero.
            // This is safe because the corresponding weight should be zero anyway.
            for (AMD::int32 j = 0; j < TRESSFX_MAX_INFLUENTIAL_BONE_COUNT; ++j)
            {
                if (skinData.boneIndex[j] == -1.f)
                {
                    skinData.boneIndex[j] = 0;
                    skinData.weight[j] = 0;
                }
            }

            // Setting the data for the leading strand of each group
            m_boneSkinningData[i * (m_numFollowStrandsPerGuide + 1)] = skinData;
        }

        // Once the entire skinning data was filled, fill skinning info for markup hair
        //  found at the end of the array
        for (int i = numOfStrandsInStream; i < m_numGuideStrands; ++i)
        {
            m_boneSkinningData[i * (m_numFollowStrandsPerGuide + 1)] = skinData;
        }

        return true;
    }

    bool TressFXAsset::LoadBoneData(AZ::Data::AssetDataStream* stream)
    {
        m_boneSkinningData.resize(0);

        AMD::int32 numOfBones = 0;
        stream->Read(sizeof(AMD::int32), &numOfBones);

        m_boneNames.resize(numOfBones);
        for (int i = 0; i < numOfBones; i++)
        {
            int boneIndex;
            stream->Read(sizeof(AMD::int32), &boneIndex);

            AMD::int32 charLen = 0;
            stream->Read(sizeof(AMD::int32), &charLen); // character length includes null termination already.

            char boneName[128];
            stream->Read(sizeof(char) * charLen, &boneName);
            boneName[charLen] = '\0'; // adding 0 termination to be on the safe side.
            m_boneNames[i] = std::string(boneName);
        }

        // Reading the number of strands
        AMD::int32 numOfStrandsInStream = 0;
        stream->Read(sizeof(AMD::int32), &numOfStrandsInStream);

        // If the number of strands from the input stream (tfxbone) is bigger than what we already know from tfx, something is wrong.
        if (m_numGuideStrands < numOfStrandsInStream)
            return 0;

        //AMD::int32 boneSkinningMemSize = m_numTotalStrands * sizeof(TressFXBoneSkinningData);
        m_boneSkinningData.resize(m_numTotalStrands);

        TressFXBoneSkinningData skinData = TressFXBoneSkinningData();
        for (int i = 0; i < numOfStrandsInStream; ++i)
        {
            AMD::int32 index = 0; // Well, we don't really use this here.
            stream->Read(sizeof(AMD::int32), &index);

            for (AMD::int32 j = 0; j < TRESSFX_MAX_INFLUENTIAL_BONE_COUNT; ++j)
            {
                AMD::int32 boneIndex;
                stream->Read(sizeof(AMD::int32), &boneIndex);
                assert(boneIndex >= 0);
                skinData.boneIndex[j] = (float)boneIndex; // Stores the bone index from tfx directly
                stream->Read(sizeof(AMD::real32), &skinData.weight[j]);
            }

#if defined(AZ_ENABLE_TRACING)
            float weightSum = skinData.weight[0] + skinData.weight[1] + skinData.weight[2] + skinData.weight[3];
            AZ_Assert(weightSum > 0.0f, "Weight sum should be greater than 0");
#endif // AZ_ENABLE_TRACING

            assert(skinData.weight[0] != 0.0f);

            // If bone index is -1, then it means that there is no bone associated to this. In this case we simply replace it with zero.
            // This is safe because the corresponding weight should be zero anyway.
            for (AMD::int32 j = 0; j < TRESSFX_MAX_INFLUENTIAL_BONE_COUNT; ++j)
            {
                if (skinData.boneIndex[j] == -1.f)
                { 
                    skinData.boneIndex[j] = 0;
                    skinData.weight[j] = 0;
                }
            }

            // Setting the data for the leading strand of each group
            m_boneSkinningData[i * (m_numFollowStrandsPerGuide + 1)] = skinData;
        }

        // Once the entire skinning data was filled, fill skinning info for markup hair
        //  found at the end of the array
        for (int i = numOfStrandsInStream; i < m_numGuideStrands; ++i)
        {
            m_boneSkinningData[i * (m_numFollowStrandsPerGuide + 1)] = skinData;
        }

        return true;
    }

    bool TressFXAsset::LoadCombinedHairData(AZ::Data::AssetDataStream* stream)
    {
        // Seek to the beginning of the file
        stream->Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);

        // Read the header of .tfxhair file
        TressFXCombinedHairFileHeader header;
        stream->Read(sizeof(TressFXCombinedHairFileHeader), (void*)&header);

        // Load the hair data
        stream->Seek(header.offsetTFX, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
        bool success = LoadHairData(stream);
        if (!success)
        {
            AZ_Warning("Hair Gem", false, "Loading: Error in Guide Hair asset data");
            return false;
        }

        // Adi: Hack code
        const int numFollowHairs = 2;
        const float tipSeparationFactor = 2.0f;
        const float maxRadiusAroundGuideHair = 0.012f;

        success = GenerateFollowHairs(numFollowHairs, tipSeparationFactor, maxRadiusAroundGuideHair);
        success &= ProcessAsset();
        if (!success)
        {
            AZ_Warning("Hair Gem", false, "Loading: Error in Follow Hair asset data");
            return false;
        }

        // Seek to the beginning of tfxbone file
        stream->Seek(header.offsetTFXBone, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);

        // Load the bones data
        success &= LoadBoneData(stream);
        if (!success)
        {
            AZ_Warning("Hair Gem", false, "Loading: Error in Hair Bones asset data");
            return false;
        }

        // Since the tfxmesh file could be optional, check if we need to export it.
        if (header.offsetTFXMesh != stream->GetLength())
        {
            // Seek to the beginning of tfxmesh file
            stream->Seek(header.offsetTFXMesh, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
            m_collisionMesh = std::make_unique<TressFXCollisionMesh>();
            success &= m_collisionMesh->LoadMeshData(stream);
            if (!success)
            {
                AZ_Warning("Hair Gem", false, "Loading: Possibly Error in Hair collision object data - this file is optional");
                return false;
            }
        }

        return true;
    }

    bool TressFXAsset::GenerateLocaltoGlobalHairBoneIndexLookup(const BoneNameToIndexMap& globalBoneIndexMap, LocalToGlobalBoneIndexLookup& outLookup)
    {
        return GenerateLocaltoGlobalBoneIndexLookup(globalBoneIndexMap, m_boneNames, outLookup);
    }

    bool TressFXAsset::GenerateLocaltoGlobalCollisionBoneIndexLookup(const BoneNameToIndexMap& globalBoneIndexMap, LocalToGlobalBoneIndexLookup& outLookup)
    {
        if (m_collisionMesh)
        {
            return GenerateLocaltoGlobalBoneIndexLookup(globalBoneIndexMap, m_collisionMesh->m_boneNames, outLookup);
        }
        return true; // do not touch outlookUp, simply return true if there is no associated collision mesh
    }

    bool TressFXAsset::GenerateLocaltoGlobalBoneIndexLookup( const BoneNameToIndexMap& boneIndicesMap,
        const std::vector<std::string>& boneNames, LocalToGlobalBoneIndexLookup& outLookup)
    {
        uint32 numMismatchedBone = 0;
        outLookup.resize(boneNames.size());
        for (int i = 0; i < m_boneNames.size(); ++i)
        {
            const std::string& boneName = m_boneNames[i];
            if (boneIndicesMap.find(boneName) == boneIndicesMap.end())
            {
                // Error handling.
                numMismatchedBone++;
                continue;
            }
            outLookup[i] = boneIndicesMap.at(boneName);
        }

        if (numMismatchedBone > 0)
        {
            AZ_Error( "Hair Gem", false, "%zu bones cannot be found under the emotionfx actor. "
                "It is likely that the hair asset is incompatible with the actor asset.", numMismatchedBone);
            return false;
        }
        return true;
    }
} // namespace AMD
