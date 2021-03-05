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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "Cry3DEngine_precompiled.h"

#include "CloudRenderNode.h"
#include "CloudsManager.h"
#include <CryPath.h>
#include "MatMan.h"

//////////////////////////////////////////////////////////////////////////
SCloudDescription::~SCloudDescription()
{
    delete m_pCloudTree;
    m_pCloudTree = 0;
    // Unregister itself from clouds manager.
    if (!filename.empty())
    {
        Get3DEngine()->GetCloudsManager()->Unregister(this);
    }
}

//////////////////////////////////////////////////////////////////////////
SCloudDescription* CCloudsManager::LoadCloud(const char* sFilename)
{
    string filename = PathUtil::ToUnixPath(sFilename);

    SCloudDescription* pCloud = stl::find_in_map(m_cloudsMap, filename, NULL);
    if (!pCloud)
    {
        XmlNodeRef root = GetISystem()->LoadXmlFromFile(filename);
        if (root)
        {
            pCloud = new SCloudDescription;
            pCloud->filename = filename;
            ParseCloudFromXml(root, pCloud);

            CloudParticles particles;
            particles.resize(pCloud->m_particles.size());
            for (uint32 i = 0; i < particles.size(); i++)
            {
                particles[i] = &pCloud->m_particles[i];
            }
            pCloud->m_pCloudTree = new SCloudQuadTree();
            pCloud->m_pCloudTree->Init(pCloud->m_bounds, particles);

            Register(pCloud);
        }
    }

    return pCloud;
}


//////////////////////////////////////////////////////////////////////////
void CCloudsManager::ParseCloudFromXml(XmlNodeRef root, SCloudDescription* pCloud)
{
    assert(pCloud);

    pCloud->m_bounds.min = Vec3(0, 0, 0);
    pCloud->m_bounds.max = Vec3(0, 0, 0);
    pCloud->m_pMaterial = 0;
    const char* sMtlName = root->getAttr("Material");
    if (sMtlName && sMtlName[0] != '\0')
    {
        pCloud->m_pMaterial = GetMatMan()->LoadMaterial(sMtlName);

        if (!pCloud->m_pMaterial)
        {
            CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_ERROR, "Error: Failed to load cloud material" /*,sMtlName*/);
        }
    }

    int numRows = 1;
    int numCols = 1;
    root->getAttr("TextureNumRows", numRows);
    root->getAttr("TextureNumCols", numCols);
    if (numRows < 1)
    {
        numRows = 1;
    }
    if (numCols < 1)
    {
        numCols = 1;
    }

    pCloud->m_textureRows = numRows;
    pCloud->m_textureCols = numCols;

    pCloud->m_numSprites = root->getChildCount();
    pCloud->m_particles.reserve(pCloud->m_numSprites);
    pCloud->m_particles.clear();

    float xTextureStep = 1.0f / numCols;
    float yTextureStep = 1.0f / numRows;

    Vec3 pos(0, 0, 0);
    int texID = 0;
    float angle = 0;
    float radius = 0;
    Vec2 uv[2];
    if (pCloud->m_numSprites > 0)
    {
        pCloud->m_bounds.Reset();
    }
    for (int i = 0; i < root->getChildCount(); i++)
    {
        XmlNodeRef child = root->getChild(i);
        child->getAttr("Pos", pos);
        child->getAttr("texID", texID);
        child->getAttr("Radius", radius);
        if (!child->getAttr("Angle", angle))
        {
            angle = 0;
        }

        int x = texID % numCols;
        int y = texID / numCols;

        uv[0].x = x * xTextureStep;
        uv[0].y = y * yTextureStep;
        uv[1].x = (x + 1) * xTextureStep;
        uv[1].y = (y + 1) * yTextureStep;

        SCloudParticle sprite(pos, radius, radius, 0, 0, uv);
        pCloud->m_particles.push_back(sprite);

        pCloud->m_bounds.Add(pos - Vec3(radius, radius, radius));
        pCloud->m_bounds.Add(pos + Vec3(radius, radius, radius));
    }

    // Offset particles so that bounding box is centered at origin.
    pCloud->m_offset = -pCloud->m_bounds.GetCenter();
    pCloud->m_bounds.min += pCloud->m_offset;
    pCloud->m_bounds.max += pCloud->m_offset;

    for (uint32 i = 0; i < pCloud->m_particles.size(); i++)
    {
        pCloud->m_particles[i].SetPosition(pCloud->m_particles[i].GetPosition() + pCloud->m_offset);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCloudsManager::Register(SCloudDescription* desc)
{
    assert(desc);
    m_cloudsMap[desc->filename] = desc;
}

//////////////////////////////////////////////////////////////////////////
void CCloudsManager::Unregister(SCloudDescription* desc)
{
    assert(desc);
    m_cloudsMap.erase(desc->filename);
}

//////////////////////////////////////////////////////////////////////////
void CCloudsManager::AddCloudRenderNode(CCloudRenderNode* pNode)
{
    m_cloudNodes.push_back(pNode);
}

//////////////////////////////////////////////////////////////////////////
void CCloudsManager::RemoveCloudRenderNode(CCloudRenderNode* pNode)
{
    int size = m_cloudNodes.size();
    for (int i = 0; i < size; i++)
    {
        if (m_cloudNodes[i] == pNode)
        {
            if (i < size - 1)
            {
                m_cloudNodes[i] = m_cloudNodes[size - 1];
            }
            m_cloudNodes.resize(size - 1);
            break;
        }
    }
}

//!!! WARNING
//bool Sp_IsDraw = false;
//Matrix34 Sp_Mat;
//Vec3 Sp_Pos;
//float Sp_Rad;


//////////////////////////////////////////////////////////////////////////
bool CCloudsManager::CheckIntersectClouds(const Vec3& p1, const Vec3& p2)
{
    for (std::vector<CCloudRenderNode*>::iterator it = m_cloudNodes.begin(); it != m_cloudNodes.end(); ++it)
    {
        //Sp_IsDraw = false;
        if ((*it)->CheckIntersection(p1, p2))
        {
            //Sp_Mat = (*it)->m_offsetedMatrix;
            //Sp_IsDraw = true;
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CCloudsManager::MoveClouds()
{
    FUNCTION_PROFILER_3DENGINE;

    std::vector<CCloudRenderNode*>::iterator it(m_cloudNodes.begin());
    std::vector<CCloudRenderNode*>::iterator itEnd(m_cloudNodes.end());
    for (; it != itEnd; ++it)
    {
        (*it)->MoveCloud();
    }
}

//////////////////////////////////////////////////////////////////////////
void SCloudQuadTree::Init(const AABB& bounds, const CloudParticles& particles, int maxlevel)
{
    m_bounds = bounds;

    if (m_level >= maxlevel)
    {
        m_particles.resize(particles.size());
        memcpy(&m_particles[0], &particles[0], particles.size() * sizeof(SCloudParticle*));
        return;
    }

    CloudParticles parts;
    for (int k = 0; k < 4; k++)
    {
        AABB bnds;
        parts.resize(0);

        Vec3 centr = (bounds.min + bounds.max) / 2;

        if (k == 0)
        {
            bnds = AABB(bounds.min, Vec3(centr.x, centr.y, bounds.max.z));
        }
        else if (k == 1)
        {
            bnds = AABB(Vec3(bounds.min.x, centr.y, bounds.min.z), Vec3(centr.x, bounds.max.y, bounds.max.z));
        }
        else if (k == 2)
        {
            bnds = AABB(Vec3(centr.x, bounds.min.y, bounds.min.z), Vec3(bounds.max.x, centr.y, bounds.max.z));
        }
        else if (k == 3)
        {
            bnds = AABB(Vec3(centr.x, centr.y, bounds.min.z), bounds.max);
        }
        for (uint32 i = 0; i < particles.size(); i++)
        {
            SCloudParticle* pPtcl = particles[i];
            if (bnds.IsOverlapSphereBounds(pPtcl->GetPosition(), pPtcl->GetRadiusX()) ||
                bnds.IsContainSphere(pPtcl->GetPosition(), pPtcl->GetRadiusX()))
            {
                parts.push_back(pPtcl);
            }
        }
        if (!parts.empty())
        {
            m_pQuads[k] = new SCloudQuadTree(m_level + 1);
            m_pQuads[k]->Init(bnds, parts, maxlevel);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
bool SCloudQuadTree::CheckIntersection(const Vec3& p1, const Vec3& p2)
{
    Vec3 outp, outp2;
    if (Intersect::Lineseg_AABB(Lineseg(p1, p2), m_bounds, outp))
    {
        if (m_pQuads[0] && m_pQuads[0]->CheckIntersection(p1, p2))
        {
            return true;
        }
        if (m_pQuads[1] && m_pQuads[1]->CheckIntersection(p1, p2))
        {
            return true;
        }
        if (m_pQuads[2] && m_pQuads[2]->CheckIntersection(p1, p2))
        {
            return true;
        }
        if (m_pQuads[3] && m_pQuads[3]->CheckIntersection(p1, p2))
        {
            return true;
        }
        if (!m_particles.empty())
        {
            for (uint32 i = 0; i < m_particles.size(); i++)
            {
                //if(Intersect::Lineseg_Sphere(Lineseg(p1, p2), Sphere (m_particles[i]->GetPosition(), m_particles[i]->GetRadiusX()*(2.0f/3)), outp, outp2))
                if (Intersect::Lineseg_Sphere(Lineseg(p1, p2), Sphere (m_particles[i]->GetPosition(), m_particles[i]->GetRadiusX()), outp, outp2))
                {
                    //Sp_Pos = m_particles[i]->GetPosition();
                    //Sp_Rad = m_particles[i]->GetRadiusX()/2;
                    return true;
                }
            }
        }
    }
    return false;
}

/*
//!!! WARNING
extern bool Sp_IsDraw;
extern Matrix34 Sp_Mat;
extern Vec3 Sp_Pos;
extern float Sp_Rad;


void C3DEngine::Hack_GetSprite(bool & IsDraw, Matrix34 & Mat, Vec3 & Pos, float & Rad)
{
    IsDraw = Sp_IsDraw;
    Mat = Sp_Mat;
    Pos = Sp_Pos;
    Rad = Sp_Rad;
}
*/
