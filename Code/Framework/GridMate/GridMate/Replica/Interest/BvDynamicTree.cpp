/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

// Modifications copyright Amazon.com, Inc. or its affiliates.  

///BvDynamicTree implementation by Nathanael Presson
#include <GridMate/Replica/Interest/BvDynamicTree.h>

namespace GridMate
{
    //
    struct btDbvtNodeEnumerator : BvDynamicTree::ICollideCollector
    {
        BvDynamicTree::ConstNodeArrayType   nodes;
        void Process(const BvDynamicTree::NodeType* n) { nodes.push_back(n); }
    };

    //
    static AZ_FORCE_INLINE int  indexof(const BvDynamicTree::NodeType* node)
    {
        return (node->m_parent->m_childs[1]==node);
    }

    //
    static AZ_FORCE_INLINE BvDynamicTree::VolumeType    merge(  const BvDynamicTree::VolumeType& a, const BvDynamicTree::VolumeType& b)
    {
        BvDynamicTree::VolumeType   res;
        Merge(a,b,res);
        return res;
    }

    // volume+edge lengths
    static AZ_FORCE_INLINE float size(const BvDynamicTree::VolumeType& a)
    {
        const AZ::Vector3 edges = a.GetExtents();
        return edges.GetX()*edges.GetY()*edges.GetZ() + edges.Dot(AZ::Vector3::CreateOne());
    }

    //
    static void                     getmaxdepth(const BvDynamicTree::NodeType* node,int depth,int& maxdepth)
    {
        if(node->IsInternal())
        {
            getmaxdepth(node->m_childs[0],depth+1,maxdepth);
            getmaxdepth(node->m_childs[0],depth+1,maxdepth);
        }
        else
            maxdepth= AZ::GetMax(maxdepth,depth);
    }



    //=========================================================================
    // insertleaf
    // [3/4/2009]
    //=========================================================================
    void
    BvDynamicTree::insertleaf( NodeType* root, NodeType* leaf)
    {
        if(!m_root)
        {
            m_root  =   leaf;
            leaf->m_parent  =   0;
        }
        else
        {
            if(!root->IsLeaf())
            {
                do  {
                    root=root->m_childs[Select( leaf->m_volume,
                        root->m_childs[0]->m_volume,
                        root->m_childs[1]->m_volume)];
                } while(!root->IsLeaf());
            }
            NodeType*   prev = root->m_parent;
            NodeType*   node = createnode(prev,leaf->m_volume,root->m_volume,0);
            if(prev)
            {
                prev->m_childs[indexof(root)]   =   node;
                node->m_childs[0]               =   root;root->m_parent=node;
                node->m_childs[1]               =   leaf;leaf->m_parent=node;
                do  {
                    if(!prev->m_volume.Contains(node->m_volume))
                        Merge(prev->m_childs[0]->m_volume,prev->m_childs[1]->m_volume,prev->m_volume);
                    else
                        break;
                    node=prev;
                } while(0!=(prev=node->m_parent));
            }
            else
            {
                node->m_childs[0]   =   root;root->m_parent=node;
                node->m_childs[1]   =   leaf;leaf->m_parent=node;
                m_root  =   node;
            }
        }
    }

    //=========================================================================
    // removeleaf
    // [3/4/2009]
    //=========================================================================
    BvDynamicTree::NodeType*
    BvDynamicTree::removeleaf( NodeType* leaf)
    {
        if(leaf==m_root)
        {
            m_root=0;
            return 0;
        }
        else
        {
            NodeType*   parent=leaf->m_parent;
            NodeType*   prev=parent->m_parent;
            NodeType*   sibling=parent->m_childs[1-indexof(leaf)];
            if(prev)
            {
                prev->m_childs[indexof(parent)]=sibling;
                sibling->m_parent=prev;
                deletenode(parent);
                while(prev)
                {
                    const VolumeType pb=prev->m_volume;
                    Merge(prev->m_childs[0]->m_volume,prev->m_childs[1]->m_volume,prev->m_volume);
                    if(NotEqual(pb,prev->m_volume))
                    {
                        prev=prev->m_parent;
                    } else break;
                }
                return prev?prev:m_root;
            }
            else
            {
                m_root=sibling;
                sibling->m_parent=0;
                deletenode(parent);
                return m_root;
            }
        }
    }


    //=========================================================================
    // fetchleaves
    // [3/4/2009]
    //=========================================================================
    void
    BvDynamicTree::fetchleaves(NodeType* root,NodeArrayType& leaves,int depth)
    {
        if(root->IsInternal()&&depth)
        {
            fetchleaves(root->m_childs[0],leaves,depth-1);
            fetchleaves(root->m_childs[1],leaves,depth-1);
            deletenode(root);
        }
        else
        {
            leaves.push_back(root);
        }
    }

    //=========================================================================
    // split
    // [3/4/2009]
    //=========================================================================
    void
    BvDynamicTree::split(const NodeArrayType& leaves, NodeArrayType& left, NodeArrayType& right, const AZ::Vector3& org, const AZ::Vector3& axis)
    {
        left.resize(0);
        right.resize(0);
        for(size_t i = 0, ni = leaves.size(); i < ni; ++i)
        {
            if (axis.Dot(leaves[i]->m_volume.GetCenter() - org) < 0.0f)
            {
                left.push_back(leaves[i]);
            }
            else
            {
                right.push_back(leaves[i]);
            }
        }
    }

    //=========================================================================
    // bounds
    // [3/4/2009]
    //=========================================================================
    BvDynamicTree::VolumeType
    BvDynamicTree::bounds(const NodeArrayType& leaves)
    {
        VolumeType volume=leaves[0]->m_volume;
        for(size_t i=1,ni=leaves.size();i<ni;++i)
        {
            Merge(volume,leaves[i]->m_volume,volume);
        }
        return volume;
    }

    //=========================================================================
    // bottomup
    // [3/4/2009]
    //=========================================================================
    void
    BvDynamicTree::bottomup( NodeArrayType& leaves )
    {
        while(leaves.size()>1)
        {
            float minsize = std::numeric_limits<float>::max();
            int         minidx[2]={-1,-1};
            for(unsigned int i=0;i<leaves.size();++i)
            {
                for(unsigned int j=i+1;j<leaves.size();++j)
                {
                    const float sz = size(merge(leaves[i]->m_volume,leaves[j]->m_volume));
                    if(sz<minsize)
                    {
                        minsize     =   sz;
                        minidx[0]   =   i;
                        minidx[1]   =   j;
                    }
                }
            }
            NodeType*   n[] =   {leaves[minidx[0]],leaves[minidx[1]]};
            NodeType*   p   =   createnode(0,n[0]->m_volume,n[1]->m_volume,0);
            p->m_childs[0]      =   n[0];
            p->m_childs[1]      =   n[1];
            n[0]->m_parent      =   p;
            n[1]->m_parent      =   p;
            leaves[minidx[0]]   =   p;
            //leaves.swap(minidx[1],leaves.size()-1);
            leaves[minidx[1]] = leaves.back();
            leaves.pop_back();
        }
    }

    //=========================================================================
    // topdown
    // [3/4/2009]
    //=========================================================================
    BvDynamicTree::NodeType*
    BvDynamicTree::topdown(NodeArrayType& leaves,int bu_treshold)
    {
        static const AZ::Vector3    axis[]= { AZ::Vector3(1.0f,0.0f,0.0f), AZ::Vector3(0.0f,1.0f,0.0f), AZ::Vector3(0.0f,0.0f,1.0f)};
        if(leaves.size()>1)
        {
            if(leaves.size()>(unsigned int)bu_treshold)
            {
                const VolumeType    vol=bounds(leaves);
                const AZ::Vector3       org=vol.GetCenter();
                NodeArrayType           sets[2];
                int                     bestaxis=-1;
                int                     bestmidp=(int)leaves.size();
                int                     splitcount[3][2]={{0,0},{0,0},{0,0}};

                for(unsigned int i=0;i<leaves.size();++i)
                {
                    const AZ::Vector3   x=leaves[i]->m_volume.GetCenter()-org;
                    for(int j=0;j<3;++j)
                    {
                        ++splitcount[j][x.Dot(axis[j]) > 0.0f ? 1 : 0];
                    }
                }
                for(unsigned int i=0;i<3;++i)
                {
                    if((splitcount[i][0]>0)&&(splitcount[i][1]>0))
                    {
                        // todo just remove the sign bit...
                        const int midp = (int)fabsf((float)(splitcount[i][0]-splitcount[i][1]));
                        if(midp<bestmidp)
                        {
                            bestaxis=i;
                            bestmidp=midp;
                        }
                    }
                }
                if(bestaxis>=0)
                {
                    sets[0].reserve(splitcount[bestaxis][0]);
                    sets[1].reserve(splitcount[bestaxis][1]);
                    split(leaves,sets[0],sets[1],org,axis[bestaxis]);
                }
                else
                {
                    sets[0].reserve(leaves.size()/2+1);
                    sets[1].reserve(leaves.size()/2);
                    for(size_t i=0,ni=leaves.size();i<ni;++i)
                    {
                        sets[i&1].push_back(leaves[i]);
                    }
                }
                BvDynamicTree::NodeType*    node=createnode(0,vol,0);
                node->m_childs[0] = topdown(sets[0],bu_treshold);
                node->m_childs[1] = topdown(sets[1],bu_treshold);
                node->m_childs[0]->m_parent=node;
                node->m_childs[1]->m_parent=node;
                return(node);
            }
            else
            {
                bottomup(leaves);
                return(leaves[0]);
            }
        }
        return(leaves[0]);
    }

    //=========================================================================
    // sort
    // [3/4/2009]
    //=========================================================================
    AZ_FORCE_INLINE BvDynamicTree::NodeType*
    BvDynamicTree::sort(NodeType* n,NodeType*& r)
    {
        BvDynamicTree::NodeType*    p=n->m_parent;
        AZ_Assert(n->IsInternal(), "We can call this only for internal nodes!");
        if(p>n)
        {
            const int   i=indexof(n);
            const int   j=1-i;
            NodeType*   s=p->m_childs[j];
            NodeType*   q=p->m_parent;
            AZ_Assert(n==p->m_childs[i], "");
            if(q) q->m_childs[indexof(p)]=n; else r=n;
            s->m_parent=n;
            p->m_parent=n;
            n->m_parent=q;
            p->m_childs[0]=n->m_childs[0];
            p->m_childs[1]=n->m_childs[1];
            n->m_childs[0]->m_parent=p;
            n->m_childs[1]->m_parent=p;
            n->m_childs[i]=p;
            n->m_childs[j]=s;
            AZStd::swap(p->m_volume,n->m_volume);
            return(p);
        }
        return(n);
    }

    #if 0
    static DBVT_INLINE NodeType*    walkup(NodeType* n,int count)
    {
        while(n&&(count--)) n=n->parent;
        return(n);
    }
    #endif

    //
    // Api
    //

    //
    BvDynamicTree::BvDynamicTree()
    {
        m_root      =   0;
        m_free      =   0;
        m_lkhd      =   -1;
        m_leaves    =   0;
        m_opath     =   0;
    }

    //
    BvDynamicTree::~BvDynamicTree()
    {
        Clear();
    }

    //
    void            BvDynamicTree::Clear()
    {
        if(m_root)  recursedeletenode(m_root);
        delete m_free;
        m_free=0;
    }

    //
    void            BvDynamicTree::OptimizeBottomUp()
    {
        if(m_root)
        {
            NodeArrayType leaves;
            leaves.reserve(m_leaves);
            fetchleaves(m_root,leaves);
            bottomup(leaves);
            m_root=leaves[0];
        }
    }

    //
    void
    BvDynamicTree::OptimizeTopDown(int bu_treshold)
    {
        if(m_root)
        {
            NodeArrayType   leaves;
            leaves.reserve(m_leaves);
            fetchleaves(m_root,leaves);
            m_root=topdown(leaves,bu_treshold);
        }
    }

    //
    void
    BvDynamicTree::OptimizeIncremental(int passes)
    {
        if(passes<0) passes=m_leaves;
        if(m_root&&(passes>0))
        {
            do  {
                NodeType*       node=m_root;
                unsigned    bit=0;
                while(node->IsInternal())
                {
                    node=sort(node,m_root)->m_childs[(m_opath>>bit)&1];
                    bit=(bit+1)&(sizeof(unsigned)*8-1);
                }
                Update(node);
                ++m_opath;
            } while(--passes);
        }
    }

    //
    BvDynamicTree::NodeType*
    BvDynamicTree::Insert(const VolumeType& volume,void* data)
    {
        NodeType*   leaf=createnode(0,volume,data);
        insertleaf(m_root,leaf);
        ++m_leaves;
        return(leaf);
    }

    //
    void
    BvDynamicTree::Update(NodeType* leaf,int lookahead)
    {
        NodeType*   root=removeleaf(leaf);
        if(root)
        {
            if(lookahead>=0)
            {
                for(int i=0;(i<lookahead)&&root->m_parent;++i)
                {
                    root=root->m_parent;
                }
            } else root=m_root;
        }
        insertleaf(root,leaf);
    }

    //
    void
    BvDynamicTree::Update(NodeType* leaf,VolumeType& volume)
    {
        NodeType*   root=removeleaf(leaf);
        if(root)
        {
            if(m_lkhd>=0)
            {
                for(int i=0;(i<m_lkhd)&&root->m_parent;++i)
                {
                    root=root->m_parent;
                }
            } else root=m_root;
        }
        leaf->m_volume=volume;
        insertleaf(root,leaf);
    }

    //
    bool
    BvDynamicTree::Update(NodeType* leaf,VolumeType& volume,const AZ::Vector3& velocity,const float margin)
    {
        if(leaf->m_volume.Contains(volume)) return(false);
        volume.Expand(AZ::Vector3(margin));
        volume.SignedExpand(velocity);
        Update(leaf,volume);
        return(true);
    }

    //
    bool
    BvDynamicTree::Update(NodeType* leaf,VolumeType& volume,const AZ::Vector3& velocity)
    {
        if(leaf->m_volume.Contains(volume)) return(false);
        volume.SignedExpand(velocity);
        Update(leaf,volume);
        return(true);
    }

    //
    bool
    BvDynamicTree::Update(NodeType* leaf,VolumeType& volume,const float margin)
    {
        if(leaf->m_volume.Contains(volume)) return(false);
        volume.Expand(AZ::Vector3(margin));
        Update(leaf,volume);
        return(true);
    }

    //
    void
    BvDynamicTree::Remove(NodeType* leaf)
    {
        removeleaf(leaf);
        deletenode(leaf);
        --m_leaves;
    }

    template<class T>
    int findLinearSearch(BvDynamicTree::ConstNodeArrayType& arr, const T& key)
    {
        size_t numElements = arr.size();
        int index       = (int)numElements;

        for(size_t i=0;i<numElements;++i)
        {
            if (arr[i] == key)
            {
                index = (int)i;
                break;
            }
        }
        return index;
    }

    //
    void
    BvDynamicTree::Write(IWriter* iwriter) const
    {
        btDbvtNodeEnumerator    nodes;
        nodes.nodes.reserve(m_leaves*2);
        enumNodes(m_root,nodes);
        iwriter->Prepare(m_root,(unsigned int)nodes.nodes.size());
        for(unsigned int i=0;i<(unsigned int)nodes.nodes.size();++i)
        {
            const NodeType* n=nodes.nodes[i];
            int         p=-1;
            if(n->m_parent) p = findLinearSearch(nodes.nodes,n->m_parent);
            if(n->IsInternal())
            {
                const int   c0=findLinearSearch(nodes.nodes,n->m_childs[0]);
                const int   c1=findLinearSearch(nodes.nodes,n->m_childs[1]);
                iwriter->WriteNode(n,i,p,c0,c1);
            }
            else
            {
                iwriter->WriteLeaf(n,i,p);
            }
        }
    }

    //
    void
    BvDynamicTree::Clone(BvDynamicTree& dest,IClone* iclone) const
    {
        dest.Clear();
        if(m_root!=0)
        {
            vector<sStkCLN> stack;
            stack.reserve(m_leaves);
            stack.push_back(sStkCLN(m_root,0));
            do  {
                const size_t    i=stack.size()-1;
                const sStkCLN   e=stack[i];
                NodeType*           n= dest.createnode(e.parent,e.node->m_volume,e.node->m_data);
                stack.pop_back();
                if(e.parent!=0)
                    e.parent->m_childs[i&1]=n;
                else
                    dest.m_root=n;
                if(e.node->IsInternal())
                {
                    stack.push_back(sStkCLN(e.node->m_childs[0],n));
                    stack.push_back(sStkCLN(e.node->m_childs[1],n));
                }
                else
                {
                    iclone->CloneLeaf(n);
                }
            } while(!stack.empty());
        }
    }

    //
    int
    BvDynamicTree::GetMaxDepth(const NodeType* node)
    {
        int depth=0;
        if(node) getmaxdepth(node,1,depth);
        return depth ;
    }

    //
    int
    BvDynamicTree::CountLeaves(const NodeType* node)
    {
        if(node->IsInternal())
            return(CountLeaves(node->m_childs[0])+CountLeaves(node->m_childs[1]));
        else
            return(1);
    }

    //
    void
    BvDynamicTree::ExtractLeaves(const NodeType* node,vector<const NodeType*>& leaves)
    {
        if(node->IsInternal())
        {
            ExtractLeaves(node->m_childs[0],leaves);
            ExtractLeaves(node->m_childs[1],leaves);
        }
        else
        {
            leaves.push_back(node);
        }
    }

    //
    #if DBVT_ENABLE_BENCHMARK

    #include <stdio.h>
    #include <stdlib.h>
    #include <RockNRoll/LinearMath/btQuickProf.h>

    /*
    q6600,2.4ghz

    /Ox /Ob2 /Oi /Ot /I "." /I "..\.." /I "..\..\src" /D "NDEBUG" /D "_LIB" /D "_WINDOWS" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE" /D "WIN32"
    /GF /FD /MT /GS- /Gy /arch:SSE2 /Zc:wchar_t- /Fp"..\..\out\release8\build\libbulletcollision\libbulletcollision.pch"
    /Fo"..\..\out\release8\build\libbulletcollision\\"
    /Fd"..\..\out\release8\build\libbulletcollision\bulletcollision.pdb"
    /W3 /nologo /c /Wp64 /Zi /errorReport:prompt

    Benchmarking dbvt...
    World scale: 100.000000
    Extents base: 1.000000
    Extents range: 4.000000
    Leaves: 8192
    sizeof(VolumeType): 32 bytes
    sizeof(NodeType):   44 bytes
    [1] VolumeType intersections: 3499 ms (-1%)
    [2] VolumeType merges: 1934 ms (0%)
    [3] BvDynamicTree::collideTT: 5485 ms (-21%)
    [4] BvDynamicTree::collideTT self: 2814 ms (-20%)
    [5] BvDynamicTree::collideTT xform: 7379 ms (-1%)
    [6] BvDynamicTree::collideTT xform,self: 7270 ms (-2%)
    [7] BvDynamicTree::rayTest: 6314 ms (0%),(332143 r/s)
    [8] insert/remove: 2093 ms (0%),(1001983 ir/s)
    [9] updates (teleport): 1879 ms (-3%),(1116100 u/s)
    [10] updates (jitter): 1244 ms (-4%),(1685813 u/s)
    [11] optimize (incremental): 2514 ms (0%),(1668000 o/s)
    [12] VolumeType notequal: 3659 ms (0%)
    [13] culling(OCL+fullsort): 2218 ms (0%),(461 t/s)
    [14] culling(OCL+qsort): 3688 ms (5%),(2221 t/s)
    [15] culling(KDOP+qsort): 1139 ms (-1%),(7192 t/s)
    [16] insert/remove batch(256): 5092 ms (0%),(823704 bir/s)
    [17] VolumeType select: 3419 ms (0%)
    */

    struct btDbvtBenchmark
    {
        struct NilPolicy : BvDynamicTree::ICollide
        {
            NilPolicy() : m_pcount(0),m_depth(-SIMD_INFINITY),m_checksort(true)     {}
            void    Process(const NodeType*,const NodeType*)                { ++m_pcount; }
            void    Process(const NodeType*)                                    { ++m_pcount; }
            void    Process(const NodeType*,btScalar depth)
            {
                ++m_pcount;
                if(m_checksort)
                { if(depth>=m_depth) m_depth=depth; else printf("wrong depth: %f (should be >= %f)\r\n",depth,m_depth); }
            }
            int         m_pcount;
            btScalar    m_depth;
            bool        m_checksort;
        };
        struct P14 : BvDynamicTree::ICollide
        {
            struct Node
            {
                const NodeType* leaf;
                btScalar            depth;
            };
            void Process(const NodeType* leaf,btScalar depth)
            {
                Node    n;
                n.leaf  =   leaf;
                n.depth =   depth;
            }
            static int sortfnc(const Node& a,const Node& b)
            {
                if(a.depth<b.depth) return(+1);
                if(a.depth>b.depth) return(-1);
                return(0);
            }
            btAlignedObjectArray<Node>      m_nodes;
        };
        struct P15 : BvDynamicTree::ICollide
        {
            struct Node
            {
                const NodeType* leaf;
                btScalar            depth;
            };
            void Process(const NodeType* leaf)
            {
                Node    n;
                n.leaf  =   leaf;
                n.depth =   dot(leaf->volume.GetCenter(),m_axis);
            }
            static int sortfnc(const Node& a,const Node& b)
            {
                if(a.depth<b.depth) return(+1);
                if(a.depth>b.depth) return(-1);
                return(0);
            }
            btAlignedObjectArray<Node>      m_nodes;
            btAZ::Vector3                       m_axis;
        };
        static btScalar         RandUnit()
        {
            return(rand()/(btScalar)RAND_MAX);
        }
        static btAZ::Vector3        RandAZ::Vector3()
        {
            return(btAZ::Vector3(RandUnit(),RandUnit(),RandUnit()));
        }
        static btAZ::Vector3        RandAZ::Vector3(btScalar cs)
        {
            return(RandAZ::Vector3()*cs-btAZ::Vector3(cs,cs,cs)/2);
        }
        static VolumeType   RandVolume(btScalar cs,btScalar eb,btScalar es)
        {
            return(VolumeType::FromCE(RandAZ::Vector3(cs),btAZ::Vector3(eb,eb,eb)+RandAZ::Vector3()*es));
        }
        static btTransform      RandTransform(btScalar cs)
        {
            btTransform t;
            t.setOrigin(RandAZ::Vector3(cs));
            t.setRotation(btQuaternion(RandUnit()*SIMD_PI*2,RandUnit()*SIMD_PI*2,RandUnit()*SIMD_PI*2).normalized());
            return(t);
        }
        static void             RandTree(btScalar cs,btScalar eb,btScalar es,int leaves,BvDynamicTree& dbvt)
        {
            dbvt.clear();
            for(int i=0;i<leaves;++i)
            {
                dbvt.Insert(RandVolume(cs,eb,es),0);
            }
        }
    };

    void            BvDynamicTree::benchmark()
    {
        static const btScalar   cfgVolumeCenterScale        =   100;
        static const btScalar   cfgVolumeExentsBase         =   1;
        static const btScalar   cfgVolumeExentsScale        =   4;
        static const int        cfgLeaves                   =   8192;
        static const bool       cfgEnable                   =   true;

        //[1] VolumeType intersections
        bool                    cfgBenchmark1_Enable        =   cfgEnable;
        static const int        cfgBenchmark1_Iterations    =   8;
        static const int        cfgBenchmark1_Reference     =   3499;
        //[2] VolumeType merges
        bool                    cfgBenchmark2_Enable        =   cfgEnable;
        static const int        cfgBenchmark2_Iterations    =   4;
        static const int        cfgBenchmark2_Reference     =   1945;
        //[3] BvDynamicTree::collideTT
        bool                    cfgBenchmark3_Enable        =   cfgEnable;
        static const int        cfgBenchmark3_Iterations    =   512;
        static const int        cfgBenchmark3_Reference     =   5485;
        //[4] BvDynamicTree::collideTT self
        bool                    cfgBenchmark4_Enable        =   cfgEnable;
        static const int        cfgBenchmark4_Iterations    =   512;
        static const int        cfgBenchmark4_Reference     =   2814;
        //[5] BvDynamicTree::collideTT xform
        bool                    cfgBenchmark5_Enable        =   cfgEnable;
        static const int        cfgBenchmark5_Iterations    =   512;
        static const btScalar   cfgBenchmark5_OffsetScale   =   2;
        static const int        cfgBenchmark5_Reference     =   7379;
        //[6] BvDynamicTree::collideTT xform,self
        bool                    cfgBenchmark6_Enable        =   cfgEnable;
        static const int        cfgBenchmark6_Iterations    =   512;
        static const btScalar   cfgBenchmark6_OffsetScale   =   2;
        static const int        cfgBenchmark6_Reference     =   7270;
        //[7] BvDynamicTree::rayTest
        bool                    cfgBenchmark7_Enable        =   cfgEnable;
        static const int        cfgBenchmark7_Passes        =   32;
        static const int        cfgBenchmark7_Iterations    =   65536;
        static const int        cfgBenchmark7_Reference     =   6307;
        //[8] insert/remove
        bool                    cfgBenchmark8_Enable        =   cfgEnable;
        static const int        cfgBenchmark8_Passes        =   32;
        static const int        cfgBenchmark8_Iterations    =   65536;
        static const int        cfgBenchmark8_Reference     =   2105;
        //[9] updates (teleport)
        bool                    cfgBenchmark9_Enable        =   cfgEnable;
        static const int        cfgBenchmark9_Passes        =   32;
        static const int        cfgBenchmark9_Iterations    =   65536;
        static const int        cfgBenchmark9_Reference     =   1879;
        //[10] updates (jitter)
        bool                    cfgBenchmark10_Enable       =   cfgEnable;
        static const btScalar   cfgBenchmark10_Scale        =   cfgVolumeCenterScale/10000;
        static const int        cfgBenchmark10_Passes       =   32;
        static const int        cfgBenchmark10_Iterations   =   65536;
        static const int        cfgBenchmark10_Reference    =   1244;
        //[11] optimize (incremental)
        bool                    cfgBenchmark11_Enable       =   cfgEnable;
        static const int        cfgBenchmark11_Passes       =   64;
        static const int        cfgBenchmark11_Iterations   =   65536;
        static const int        cfgBenchmark11_Reference    =   2510;
        //[12] VolumeType notequal
        bool                    cfgBenchmark12_Enable       =   cfgEnable;
        static const int        cfgBenchmark12_Iterations   =   32;
        static const int        cfgBenchmark12_Reference    =   3677;
        //[13] culling(OCL+fullsort)
        bool                    cfgBenchmark13_Enable       =   cfgEnable;
        static const int        cfgBenchmark13_Iterations   =   1024;
        static const int        cfgBenchmark13_Reference    =   2231;
        //[14] culling(OCL+qsort)
        bool                    cfgBenchmark14_Enable       =   cfgEnable;
        static const int        cfgBenchmark14_Iterations   =   8192;
        static const int        cfgBenchmark14_Reference    =   3500;
        //[15] culling(KDOP+qsort)
        bool                    cfgBenchmark15_Enable       =   cfgEnable;
        static const int        cfgBenchmark15_Iterations   =   8192;
        static const int        cfgBenchmark15_Reference    =   1151;
        //[16] insert/remove batch
        bool                    cfgBenchmark16_Enable       =   cfgEnable;
        static const int        cfgBenchmark16_BatchCount   =   256;
        static const int        cfgBenchmark16_Passes       =   16384;
        static const int        cfgBenchmark16_Reference    =   5138;
        //[17] select
        bool                    cfgBenchmark17_Enable       =   cfgEnable;
        static const int        cfgBenchmark17_Iterations   =   4;
        static const int        cfgBenchmark17_Reference    =   3390;

        btClock                 wallclock;
        printf("Benchmarking dbvt...\r\n");
        printf("\tWorld scale: %f\r\n",cfgVolumeCenterScale);
        printf("\tExtents base: %f\r\n",cfgVolumeExentsBase);
        printf("\tExtents range: %f\r\n",cfgVolumeExentsScale);
        printf("\tLeaves: %u\r\n",cfgLeaves);
        printf("\tsizeof(VolumeType): %u bytes\r\n",sizeof(VolumeType));
        printf("\tsizeof(NodeType):   %u bytes\r\n",sizeof(NodeType));
        if(cfgBenchmark1_Enable)
        {// Benchmark 1
            srand(380843);
            btAlignedObjectArray<VolumeType>    volumes;
            btAlignedObjectArray<bool>          results;
            volumes.resize(cfgLeaves);
            results.resize(cfgLeaves);
            for(int i=0;i<cfgLeaves;++i)
            {
                volumes[i]=btDbvtBenchmark::RandVolume(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale);
            }
            printf("[1] VolumeType intersections: ");
            wallclock.reset();
            for(int i=0;i<cfgBenchmark1_Iterations;++i)
            {
                for(int j=0;j<cfgLeaves;++j)
                {
                    for(int k=0;k<cfgLeaves;++k)
                    {
                        results[k]=Intersect(volumes[j],volumes[k]);
                    }
                }
            }
            const int time=(int)wallclock.getTimeMilliseconds();
            printf("%u ms (%i%%)\r\n",time,(time-cfgBenchmark1_Reference)*100/time);
        }
        if(cfgBenchmark2_Enable)
        {// Benchmark 2
            srand(380843);
            btAlignedObjectArray<VolumeType>    volumes;
            btAlignedObjectArray<VolumeType>    results;
            volumes.resize(cfgLeaves);
            results.resize(cfgLeaves);
            for(int i=0;i<cfgLeaves;++i)
            {
                volumes[i]=btDbvtBenchmark::RandVolume(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale);
            }
            printf("[2] VolumeType merges: ");
            wallclock.reset();
            for(int i=0;i<cfgBenchmark2_Iterations;++i)
            {
                for(int j=0;j<cfgLeaves;++j)
                {
                    for(int k=0;k<cfgLeaves;++k)
                    {
                        Merge(volumes[j],volumes[k],results[k]);
                    }
                }
            }
            const int time=(int)wallclock.getTimeMilliseconds();
            printf("%u ms (%i%%)\r\n",time,(time-cfgBenchmark2_Reference)*100/time);
        }
        if(cfgBenchmark3_Enable)
        {// Benchmark 3
            srand(380843);
            BvDynamicTree                       dbvt[2];
            btDbvtBenchmark::NilPolicy  policy;
            btDbvtBenchmark::RandTree(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale,cfgLeaves,dbvt[0]);
            btDbvtBenchmark::RandTree(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale,cfgLeaves,dbvt[1]);
            dbvt[0].optimizeTopDown();
            dbvt[1].optimizeTopDown();
            printf("[3] BvDynamicTree::collideTT: ");
            wallclock.reset();
            for(int i=0;i<cfgBenchmark3_Iterations;++i)
            {
                BvDynamicTree::collideTT(dbvt[0].m_root,dbvt[1].m_root,policy);
            }
            const int time=(int)wallclock.getTimeMilliseconds();
            printf("%u ms (%i%%)\r\n",time,(time-cfgBenchmark3_Reference)*100/time);
        }
        if(cfgBenchmark4_Enable)
        {// Benchmark 4
            srand(380843);
            BvDynamicTree                       dbvt;
            btDbvtBenchmark::NilPolicy  policy;
            btDbvtBenchmark::RandTree(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale,cfgLeaves,dbvt);
            dbvt.optimizeTopDown();
            printf("[4] BvDynamicTree::collideTT self: ");
            wallclock.reset();
            for(int i=0;i<cfgBenchmark4_Iterations;++i)
            {
                BvDynamicTree::collideTT(dbvt.m_root,dbvt.m_root,policy);
            }
            const int time=(int)wallclock.getTimeMilliseconds();
            printf("%u ms (%i%%)\r\n",time,(time-cfgBenchmark4_Reference)*100/time);
        }
        if(cfgBenchmark5_Enable)
        {// Benchmark 5
            srand(380843);
            BvDynamicTree                               dbvt[2];
            btAlignedObjectArray<btTransform>   transforms;
            btDbvtBenchmark::NilPolicy          policy;
            transforms.resize(cfgBenchmark5_Iterations);
            for(int i=0;i<transforms.size();++i)
            {
                transforms[i]=btDbvtBenchmark::RandTransform(cfgVolumeCenterScale*cfgBenchmark5_OffsetScale);
            }
            btDbvtBenchmark::RandTree(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale,cfgLeaves,dbvt[0]);
            btDbvtBenchmark::RandTree(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale,cfgLeaves,dbvt[1]);
            dbvt[0].optimizeTopDown();
            dbvt[1].optimizeTopDown();
            printf("[5] BvDynamicTree::collideTT xform: ");
            wallclock.reset();
            for(int i=0;i<cfgBenchmark5_Iterations;++i)
            {
                BvDynamicTree::collideTT(dbvt[0].m_root,dbvt[1].m_root,transforms[i],policy);
            }
            const int time=(int)wallclock.getTimeMilliseconds();
            printf("%u ms (%i%%)\r\n",time,(time-cfgBenchmark5_Reference)*100/time);
        }
        if(cfgBenchmark6_Enable)
        {// Benchmark 6
            srand(380843);
            BvDynamicTree                               dbvt;
            btAlignedObjectArray<btTransform>   transforms;
            btDbvtBenchmark::NilPolicy          policy;
            transforms.resize(cfgBenchmark6_Iterations);
            for(int i=0;i<transforms.size();++i)
            {
                transforms[i]=btDbvtBenchmark::RandTransform(cfgVolumeCenterScale*cfgBenchmark6_OffsetScale);
            }
            btDbvtBenchmark::RandTree(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale,cfgLeaves,dbvt);
            dbvt.optimizeTopDown();
            printf("[6] BvDynamicTree::collideTT xform,self: ");
            wallclock.reset();
            for(int i=0;i<cfgBenchmark6_Iterations;++i)
            {
                BvDynamicTree::collideTT(dbvt.m_root,dbvt.m_root,transforms[i],policy);
            }
            const int time=(int)wallclock.getTimeMilliseconds();
            printf("%u ms (%i%%)\r\n",time,(time-cfgBenchmark6_Reference)*100/time);
        }
        if(cfgBenchmark7_Enable)
        {// Benchmark 7
            srand(380843);
            BvDynamicTree                               dbvt;
            btAlignedObjectArray<btAZ::Vector3>     rayorg;
            btAlignedObjectArray<btAZ::Vector3>     raydir;
            btDbvtBenchmark::NilPolicy          policy;
            rayorg.resize(cfgBenchmark7_Iterations);
            raydir.resize(cfgBenchmark7_Iterations);
            for(int i=0;i<rayorg.size();++i)
            {
                rayorg[i]=btDbvtBenchmark::RandAZ::Vector3(cfgVolumeCenterScale*2);
                raydir[i]=btDbvtBenchmark::RandAZ::Vector3(cfgVolumeCenterScale*2);
            }
            btDbvtBenchmark::RandTree(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale,cfgLeaves,dbvt);
            dbvt.optimizeTopDown();
            printf("[7] BvDynamicTree::rayTest: ");
            wallclock.reset();
            for(int i=0;i<cfgBenchmark7_Passes;++i)
            {
                for(int j=0;j<cfgBenchmark7_Iterations;++j)
                {
                    BvDynamicTree::rayTest(dbvt.m_root,rayorg[j],rayorg[j]+raydir[j],policy);
                }
            }
            const int   time=(int)wallclock.getTimeMilliseconds();
            unsigned    rays=cfgBenchmark7_Passes*cfgBenchmark7_Iterations;
            printf("%u ms (%i%%),(%u r/s)\r\n",time,(time-cfgBenchmark7_Reference)*100/time,(rays*1000)/time);
        }
        if(cfgBenchmark8_Enable)
        {// Benchmark 8
            srand(380843);
            BvDynamicTree                               dbvt;
            btDbvtBenchmark::RandTree(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale,cfgLeaves,dbvt);
            dbvt.optimizeTopDown();
            printf("[8] Insert/remove: ");
            wallclock.reset();
            for(int i=0;i<cfgBenchmark8_Passes;++i)
            {
                for(int j=0;j<cfgBenchmark8_Iterations;++j)
                {
                    dbvt.remove(dbvt.Insert(btDbvtBenchmark::RandVolume(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale),0));
                }
            }
            const int   time=(int)wallclock.getTimeMilliseconds();
            const int   ir=cfgBenchmark8_Passes*cfgBenchmark8_Iterations;
            printf("%u ms (%i%%),(%u ir/s)\r\n",time,(time-cfgBenchmark8_Reference)*100/time,ir*1000/time);
        }
        if(cfgBenchmark9_Enable)
        {// Benchmark 9
            srand(380843);
            BvDynamicTree                                       dbvt;
            btAlignedObjectArray<const NodeType*>   leaves;
            btDbvtBenchmark::RandTree(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale,cfgLeaves,dbvt);
            dbvt.optimizeTopDown();
            dbvt.extractLeaves(dbvt.m_root,leaves);
            printf("[9] updates (teleport): ");
            wallclock.reset();
            for(int i=0;i<cfgBenchmark9_Passes;++i)
            {
                for(int j=0;j<cfgBenchmark9_Iterations;++j)
                {
                    dbvt.update(const_cast<NodeType*>(leaves[rand()%cfgLeaves]),
                        btDbvtBenchmark::RandVolume(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale));
                }
            }
            const int   time=(int)wallclock.getTimeMilliseconds();
            const int   up=cfgBenchmark9_Passes*cfgBenchmark9_Iterations;
            printf("%u ms (%i%%),(%u u/s)\r\n",time,(time-cfgBenchmark9_Reference)*100/time,up*1000/time);
        }
        if(cfgBenchmark10_Enable)
        {// Benchmark 10
            srand(380843);
            BvDynamicTree                                       dbvt;
            btAlignedObjectArray<const NodeType*>   leaves;
            btAlignedObjectArray<btAZ::Vector3>             vectors;
            vectors.resize(cfgBenchmark10_Iterations);
            for(int i=0;i<vectors.size();++i)
            {
                vectors[i]=(btDbvtBenchmark::RandAZ::Vector3()*2-btAZ::Vector3(1,1,1))*cfgBenchmark10_Scale;
            }
            btDbvtBenchmark::RandTree(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale,cfgLeaves,dbvt);
            dbvt.optimizeTopDown();
            dbvt.extractLeaves(dbvt.m_root,leaves);
            printf("[10] updates (jitter): ");
            wallclock.reset();

            for(int i=0;i<cfgBenchmark10_Passes;++i)
            {
                for(int j=0;j<cfgBenchmark10_Iterations;++j)
                {
                    const btAZ::Vector3&    d=vectors[j];
                    NodeType*       l=const_cast<NodeType*>(leaves[rand()%cfgLeaves]);
                    VolumeType      v=VolumeType::FromMM(l->volume.GetMin()+d,l->volume.GetMax()+d);
                    dbvt.update(l,v);
                }
            }
            const int   time=(int)wallclock.getTimeMilliseconds();
            const int   up=cfgBenchmark10_Passes*cfgBenchmark10_Iterations;
            printf("%u ms (%i%%),(%u u/s)\r\n",time,(time-cfgBenchmark10_Reference)*100/time,up*1000/time);
        }
        if(cfgBenchmark11_Enable)
        {// Benchmark 11
            srand(380843);
            BvDynamicTree                                       dbvt;
            btDbvtBenchmark::RandTree(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale,cfgLeaves,dbvt);
            dbvt.optimizeTopDown();
            printf("[11] optimize (incremental): ");
            wallclock.reset();
            for(int i=0;i<cfgBenchmark11_Passes;++i)
            {
                dbvt.optimizeIncremental(cfgBenchmark11_Iterations);
            }
            const int   time=(int)wallclock.getTimeMilliseconds();
            const int   op=cfgBenchmark11_Passes*cfgBenchmark11_Iterations;
            printf("%u ms (%i%%),(%u o/s)\r\n",time,(time-cfgBenchmark11_Reference)*100/time,op/time*1000);
        }
        if(cfgBenchmark12_Enable)
        {// Benchmark 12
            srand(380843);
            btAlignedObjectArray<VolumeType>    volumes;
            btAlignedObjectArray<bool>              results;
            volumes.resize(cfgLeaves);
            results.resize(cfgLeaves);
            for(int i=0;i<cfgLeaves;++i)
            {
                volumes[i]=btDbvtBenchmark::RandVolume(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale);
            }
            printf("[12] VolumeType notequal: ");
            wallclock.reset();
            for(int i=0;i<cfgBenchmark12_Iterations;++i)
            {
                for(int j=0;j<cfgLeaves;++j)
                {
                    for(int k=0;k<cfgLeaves;++k)
                    {
                        results[k]=NotEqual(volumes[j],volumes[k]);
                    }
                }
            }
            const int time=(int)wallclock.getTimeMilliseconds();
            printf("%u ms (%i%%)\r\n",time,(time-cfgBenchmark12_Reference)*100/time);
        }
        if(cfgBenchmark13_Enable)
        {// Benchmark 13
            srand(380843);
            BvDynamicTree                               dbvt;
            btAlignedObjectArray<btAZ::Vector3>     vectors;
            btDbvtBenchmark::NilPolicy          policy;
            vectors.resize(cfgBenchmark13_Iterations);
            for(int i=0;i<vectors.size();++i)
            {
                vectors[i]=(btDbvtBenchmark::RandAZ::Vector3()*2-btAZ::Vector3(1,1,1)).normalized();
            }
            btDbvtBenchmark::RandTree(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale,cfgLeaves,dbvt);
            dbvt.optimizeTopDown();
            printf("[13] culling(OCL+fullsort): ");
            wallclock.reset();
            for(int i=0;i<cfgBenchmark13_Iterations;++i)
            {
                static const btScalar   offset=0;
                policy.m_depth=-SIMD_INFINITY;
                dbvt.collideOCL(dbvt.m_root,&vectors[i],&offset,vectors[i],1,policy);
            }
            const int   time=(int)wallclock.getTimeMilliseconds();
            const int   t=cfgBenchmark13_Iterations;
            printf("%u ms (%i%%),(%u t/s)\r\n",time,(time-cfgBenchmark13_Reference)*100/time,(t*1000)/time);
        }
        if(cfgBenchmark14_Enable)
        {// Benchmark 14
            srand(380843);
            BvDynamicTree                               dbvt;
            btAlignedObjectArray<btAZ::Vector3>     vectors;
            btDbvtBenchmark::P14                policy;
            vectors.resize(cfgBenchmark14_Iterations);
            for(int i=0;i<vectors.size();++i)
            {
                vectors[i]=(btDbvtBenchmark::RandAZ::Vector3()*2-btAZ::Vector3(1,1,1)).normalized();
            }
            btDbvtBenchmark::RandTree(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale,cfgLeaves,dbvt);
            dbvt.optimizeTopDown();
            policy.m_nodes.reserve(cfgLeaves);
            printf("[14] culling(OCL+qsort): ");
            wallclock.reset();
            for(int i=0;i<cfgBenchmark14_Iterations;++i)
            {
                static const btScalar   offset=0;
                policy.m_nodes.resize(0);
                dbvt.collideOCL(dbvt.m_root,&vectors[i],&offset,vectors[i],1,policy,false);
                policy.m_nodes.quickSort(btDbvtBenchmark::P14::sortfnc);
            }
            const int   time=(int)wallclock.getTimeMilliseconds();
            const int   t=cfgBenchmark14_Iterations;
            printf("%u ms (%i%%),(%u t/s)\r\n",time,(time-cfgBenchmark14_Reference)*100/time,(t*1000)/time);
        }
        if(cfgBenchmark15_Enable)
        {// Benchmark 15
            srand(380843);
            BvDynamicTree                               dbvt;
            btAlignedObjectArray<btAZ::Vector3>     vectors;
            btDbvtBenchmark::P15                policy;
            vectors.resize(cfgBenchmark15_Iterations);
            for(int i=0;i<vectors.size();++i)
            {
                vectors[i]=(btDbvtBenchmark::RandAZ::Vector3()*2-btAZ::Vector3(1,1,1)).normalized();
            }
            btDbvtBenchmark::RandTree(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale,cfgLeaves,dbvt);
            dbvt.optimizeTopDown();
            policy.m_nodes.reserve(cfgLeaves);
            printf("[15] culling(KDOP+qsort): ");
            wallclock.reset();
            for(int i=0;i<cfgBenchmark15_Iterations;++i)
            {
                static const btScalar   offset=0;
                policy.m_nodes.resize(0);
                policy.m_axis=vectors[i];
                dbvt.collideKDOP(dbvt.m_root,&vectors[i],&offset,1,policy);
                policy.m_nodes.quickSort(btDbvtBenchmark::P15::sortfnc);
            }
            const int   time=(int)wallclock.getTimeMilliseconds();
            const int   t=cfgBenchmark15_Iterations;
            printf("%u ms (%i%%),(%u t/s)\r\n",time,(time-cfgBenchmark15_Reference)*100/time,(t*1000)/time);
        }
        if(cfgBenchmark16_Enable)
        {// Benchmark 16
            srand(380843);
            BvDynamicTree                               dbvt;
            btAlignedObjectArray<NodeType*> batch;
            btDbvtBenchmark::RandTree(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale,cfgLeaves,dbvt);
            dbvt.optimizeTopDown();
            batch.reserve(cfgBenchmark16_BatchCount);
            printf("[16] Insert/remove batch(%u): ",cfgBenchmark16_BatchCount);
            wallclock.reset();
            for(int i=0;i<cfgBenchmark16_Passes;++i)
            {
                for(int j=0;j<cfgBenchmark16_BatchCount;++j)
                {
                    batch.push_back(dbvt.Insert(btDbvtBenchmark::RandVolume(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale),0));
                }
                for(int j=0;j<cfgBenchmark16_BatchCount;++j)
                {
                    dbvt.remove(batch[j]);
                }
                batch.resize(0);
            }
            const int   time=(int)wallclock.getTimeMilliseconds();
            const int   ir=cfgBenchmark16_Passes*cfgBenchmark16_BatchCount;
            printf("%u ms (%i%%),(%u bir/s)\r\n",time,(time-cfgBenchmark16_Reference)*100/time,int(ir*1000.0/time));
        }
        if(cfgBenchmark17_Enable)
        {// Benchmark 17
            srand(380843);
            btAlignedObjectArray<VolumeType>    volumes;
            btAlignedObjectArray<int>           results;
            btAlignedObjectArray<int>           indices;
            volumes.resize(cfgLeaves);
            results.resize(cfgLeaves);
            indices.resize(cfgLeaves);
            for(int i=0;i<cfgLeaves;++i)
            {
                indices[i]=i;
                volumes[i]=btDbvtBenchmark::RandVolume(cfgVolumeCenterScale,cfgVolumeExentsBase,cfgVolumeExentsScale);
            }
            for(int i=0;i<cfgLeaves;++i)
            {
                btSwap(indices[i],indices[rand()%cfgLeaves]);
            }
            printf("[17] VolumeType select: ");
            wallclock.reset();
            for(int i=0;i<cfgBenchmark17_Iterations;++i)
            {
                for(int j=0;j<cfgLeaves;++j)
                {
                    for(int k=0;k<cfgLeaves;++k)
                    {
                        const int idx=indices[k];
                        results[idx]=Select(volumes[idx],volumes[j],volumes[k]);
                    }
                }
            }
            const int time=(int)wallclock.getTimeMilliseconds();
            printf("%u ms (%i%%)\r\n",time,(time-cfgBenchmark17_Reference)*100/time);
        }
        printf("\r\n\r\n");
    }
    #endif
}
