/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/
/*
 * Temporary dynamic tree structure used internally by GridMate.
 * To be replaced with a general Vis framework when that becomes available.
 */

#ifndef RR_DYNAMIC_BOUNDING_VOLUME_TREE_H
#define RR_DYNAMIC_BOUNDING_VOLUME_TREE_H

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Plane.h>

#include <GridMate/Containers/vector.h>
#include <AzCore/std/containers/fixed_vector.h>

namespace GridMate
{
    namespace Internal
    {
        /**
         *
        */
        class   DynamicTreeAabb : public AZ::Aabb
        {
        public:
            GM_CLASS_ALLOCATOR(DynamicTreeAabb);

            AZ_FORCE_INLINE explicit DynamicTreeAabb() {}
            AZ_FORCE_INLINE DynamicTreeAabb(const AZ::Aabb& aabb) : AZ::Aabb(aabb) {}
            AZ_FORCE_INLINE explicit DynamicTreeAabb(const AZ::Vector3& min,const AZ::Vector3& max) : AZ::Aabb(AZ::Aabb::CreateFromMinMax(min,max)) {}

            AZ_FORCE_INLINE static DynamicTreeAabb  CreateFromFacePoints(const AZ::Vector3& a, const AZ::Vector3& b, const AZ::Vector3& c)
            {
                DynamicTreeAabb vol(a,a);
                vol.AddPoint(b);
                vol.AddPoint(c);
                return vol;
            }

            AZ_FORCE_INLINE void                SignedExpand(const AZ::Vector3& e)
            {
                AZ::Vector3 zero = AZ::Vector3::CreateZero();
                AZ::Vector3 mxE = m_max + e;
                AZ::Vector3 miE = m_min + e;
                m_max = AZ::Vector3::CreateSelectCmpGreater(e,zero,mxE,m_max );
                m_min = AZ::Vector3::CreateSelectCmpGreater(e,zero,m_min,miE);
            }
            AZ_FORCE_INLINE int                 Classify(const AZ::Vector3& n,const float o,int s) const
            {
                AZ::Vector3 pi, px;
                switch(s)
                {
                case    (0+0+0):    px=m_min;
                                    pi=m_max; break;
                case    (1+0+0):    px=AZ::Vector3(m_max.GetX(),m_min.GetY(),m_min.GetZ());
                                    pi=AZ::Vector3(m_min.GetX(),m_max.GetY(),m_max.GetZ());break;
                case    (0+2+0):    px=AZ::Vector3(m_min.GetX(),m_max.GetY(),m_min.GetZ());
                                    pi=AZ::Vector3(m_max.GetX(),m_min.GetY(),m_max.GetZ());break;
                case    (1+2+0):    px=AZ::Vector3(m_max.GetX(),m_max.GetY(),m_min.GetZ());
                                    pi=AZ::Vector3(m_min.GetX(),m_min.GetY(),m_max.GetZ());break;
                case    (0+0+4):    px=AZ::Vector3(m_min.GetX(),m_min.GetY(),m_max.GetZ());
                                    pi=AZ::Vector3(m_max.GetX(),m_max.GetY(),m_min.GetZ());break;
                case    (1+0+4):    px=AZ::Vector3(m_max.GetX(),m_min.GetY(),m_max.GetZ());
                                    pi=AZ::Vector3(m_min.GetX(),m_max.GetY(),m_min.GetZ());break;
                case    (0+2+4):    px=AZ::Vector3(m_min.GetX(),m_max.GetY(),m_max.GetZ());
                                    pi=AZ::Vector3(m_max.GetX(),m_min.GetY(),m_min.GetZ());break;
                case    (1+2+4):    px=m_max;
                                    pi=m_min;break;
                }

                if (n.Dot(px) + o < 0.0f)
                {
                    return -1;
                }
                if (n.Dot(pi) + o > 0.0f)
                {
                    return  1;
                }

                return 0;
            }
            AZ_FORCE_INLINE float ProjectMinimum(const AZ::Vector3& v, unsigned signs) const
            {
                const AZ::Vector3*  b[]={&m_max,&m_min};
                const AZ::Vector3   p(  b[(signs>>0)&1]->GetX(),b[(signs>>1)&1]->GetY(),b[(signs>>2)&1]->GetZ());
                return p.Dot(v);
            }

            // Move the code here
            AZ_FORCE_INLINE friend bool IntersectAabbAabb(const DynamicTreeAabb& a,const DynamicTreeAabb& b);
            AZ_FORCE_INLINE friend bool IntersectAabbPoint(const DynamicTreeAabb& a, const AZ::Vector3& b);
            AZ_FORCE_INLINE friend bool IntersectAabbPlane(const DynamicTreeAabb& a, const AZ::Plane& b);
            AZ_FORCE_INLINE friend float Proximity(const DynamicTreeAabb& a, const DynamicTreeAabb& b);
            AZ_FORCE_INLINE friend int Select(const DynamicTreeAabb& o, const DynamicTreeAabb& a, const DynamicTreeAabb& b);
            AZ_FORCE_INLINE friend void Merge(const DynamicTreeAabb& a, const DynamicTreeAabb& b, DynamicTreeAabb& r);
            AZ_FORCE_INLINE friend bool NotEqual(const DynamicTreeAabb& a, const DynamicTreeAabb& b);
        private:
            AZ_FORCE_INLINE void AddSpan(const AZ::Vector3& d, float& smi, float& smx) const
            {
                AZ::Vector3 vecZero = AZ::Vector3::CreateZero();
                AZ::Vector3 mxD = m_max*d;
                AZ::Vector3 miD = m_min*d;
                AZ::Vector3 smiAdd = AZ::Vector3::CreateSelectCmpGreater(vecZero,d,mxD,miD);
                AZ::Vector3 smxAdd = AZ::Vector3::CreateSelectCmpGreater(vecZero,d,miD,mxD);
                AZ::Vector3 vecOne = AZ::Vector3::CreateOne();
                // sum components
                smi += smiAdd.Dot(vecOne);
                smx += smxAdd.Dot(vecOne);
            }
        };

        //
        AZ_FORCE_INLINE bool IntersectAabbAabb(const DynamicTreeAabb& a, const DynamicTreeAabb& b)
        {
            return a.Overlaps(b);
        }

        AZ_FORCE_INLINE bool IntersectAabbPlane(const DynamicTreeAabb& a, const AZ::Plane& b)
        {
            //use plane normal to quickly select the nearest corner of the aabb
            AZ::Vector3 testPoint = AZ::Vector3::CreateSelectCmpGreater(b.GetNormal(), AZ::Vector3::CreateZero(), a.GetMin(), a.GetMax());
            //test if nearest point is inside the plane
            return b.GetPointDist(testPoint) <= 0.0f;
        }

        //
        AZ_FORCE_INLINE float Proximity(const DynamicTreeAabb& a, const DynamicTreeAabb& b)
        {
            const AZ::Vector3 d=(a.m_min+a.m_max)-(b.m_min+b.m_max);
            // get abs and sum
            return d.GetAbs().Dot(AZ::Vector3::CreateOne());
        }

        //
        AZ_FORCE_INLINE int Select( const DynamicTreeAabb& o, const DynamicTreeAabb& a, const DynamicTreeAabb& b)
        {
            return Proximity(o,a) < Proximity(o,b);
        }

        //
        AZ_FORCE_INLINE void Merge(const DynamicTreeAabb& a, const DynamicTreeAabb& b, DynamicTreeAabb& r)
        {
            r.m_min = AZ::Vector3::CreateSelectCmpGreater(b.m_min,a.m_min,a.m_min,b.m_min);
            r.m_max = AZ::Vector3::CreateSelectCmpGreater(a.m_max,b.m_max,a.m_max,b.m_max);
        }

        //
        AZ_FORCE_INLINE bool NotEqual(   const DynamicTreeAabb& a, const DynamicTreeAabb& b)
        {
            return (a.m_min != b.m_min || a.m_max != b.m_max);
        }


        /* NodeType             */
        struct  DynamicTreeNode
        {
            GM_CLASS_ALLOCATOR(DynamicTreeNode);

            DynamicTreeAabb         m_volume;
            DynamicTreeNode*        m_parent;
            AZ_FORCE_INLINE bool    IsLeaf() const      { return(m_childs[1]==0); }
            AZ_FORCE_INLINE bool    IsInternal() const  { return(!IsLeaf()); }
            union
            {
                DynamicTreeNode*    m_childs[2];
                void*       m_data;
                int         m_dataAsInt;
            };
        };
    }

    /**
    * Implementation of dynamic aabb tree, based on the bullet dynamic tree (btDbvt).
    *
    * The BvDynamicTree class implements a fast dynamic bounding volume tree based on axis aligned bounding boxes (aabb tree).
    * This BvDynamicTree is used for soft body collision detection and for the btDbvtBroadphase. It has a fast insert, remove and update of nodes.
    * Unlike the BvTreeQuantized, nodes can be dynamically moved around, which allows for change in topology of the underlying data structure.
    */
    class   BvDynamicTree
    {
    public:
        using Ptr = AZStd::intrusive_ptr<BvDynamicTree>;

        GM_CLASS_ALLOCATOR(BvDynamicTree);

        typedef Internal::DynamicTreeAabb   VolumeType;
        typedef Internal::DynamicTreeNode   NodeType;

        typedef vector<NodeType*>           NodeArrayType;
        typedef vector<const NodeType*>     ConstNodeArrayType;

    private:

        /* Stack element    */
        struct  sStkNN
        {
            const NodeType* a;
            const NodeType* b;
            sStkNN() {}
            sStkNN(const NodeType* na,const NodeType* nb) : a(na), b(nb) {}
        };
        struct  sStkNP
        {
            const NodeType* node;
            int mask;
            sStkNP(const NodeType* n, unsigned m) : node(n), mask(m) {}
        };
        struct  sStkNPS
        {
            const NodeType* node;
            int mask;
            float value;
            sStkNPS() {}
            sStkNPS(const NodeType* n, unsigned m, const float v) : node(n), mask(m), value(v) {}
        };
        struct  sStkCLN
        {
            const NodeType* node;
            NodeType* parent;
            sStkCLN(const NodeType* n, NodeType* p) : node(n), parent(p) {}
        };

    public:
        /* ICollideCollector templated collectors should implement this functions or inherit from this class */
        struct  ICollideCollector
        {
            void Process(const NodeType*, const NodeType*) {}
            void Process(const NodeType*) {}
            void Process(const NodeType* n, const float) { Process(n); }
            bool Descent(const NodeType*) { return true; }
            bool AllLeaves(const NodeType*) { return true; }
        };

        /* IWriter  */
        struct  IWriter
        {
            virtual ~IWriter() {}
            virtual void Prepare(const NodeType* root,int numnodes) = 0;
            virtual void WriteNode(const NodeType*, int index, int parent, int child0, int child1) = 0;
            virtual void WriteLeaf(const NodeType*, int index, int parent) = 0;
        };
        /* IClone   */
        struct  IClone
        {
            virtual ~IClone() {}
            virtual void CloneLeaf(NodeType*) {}
        };

        // Constants
        enum
        {
            SIMPLE_STACKSIZE = 64,
            DOUBLE_STACKSIZE = SIMPLE_STACKSIZE * 2
        };

        // Methods
        BvDynamicTree();
        ~BvDynamicTree();

        NodeType* GetRoot() const { return m_root; }
        void Clear();
        bool Empty() const { return 0 == m_root; }
        int GetNumLeaves() const { return m_leaves; }
        void OptimizeBottomUp();
        void OptimizeTopDown(int bu_treshold = 128);
        void OptimizeIncremental(int passes);
        NodeType* Insert(const VolumeType& box,void* data);
        void Update(NodeType* leaf, int lookahead=-1);
        void Update(NodeType* leaf, VolumeType& volume);
        bool Update(NodeType* leaf, VolumeType& volume, const AZ::Vector3& velocity, const float margin);
        bool Update(NodeType* leaf, VolumeType& volume, const AZ::Vector3& velocity);
        bool Update(NodeType* leaf, VolumeType& volume, const float margin);
        void Remove(NodeType* leaf);
        void Write(IWriter* iwriter) const;
        void Clone(BvDynamicTree& dest, IClone* iclone=0) const;
        static int GetMaxDepth(const NodeType* node);
        static int CountLeaves(const NodeType* node);
        static void ExtractLeaves(const NodeType* node, /*btAlignedObjectArray<const NodeType*>&*/vector<const NodeType*>& leaves);
    #if DBVT_ENABLE_BENCHMARK
        static void     Benchmark();
    #else
        static void     Benchmark(){}
    #endif
        /**
         * Collector should inherit from ICollide
         */
        template<class Collector>
        static inline void      enumNodes(  const NodeType* root, Collector& collector)
        {
            collector.Process(root);
            if(root->IsInternal())
            {
                enumNodes(root->m_childs[0],collector);
                enumNodes(root->m_childs[1],collector);
            }
        }
        template<class Collector>
        static void     enumLeaves( const NodeType* root,Collector& collector)
        {
            if(root->IsInternal())
            {
                enumLeaves(root->m_childs[0],collector);
                enumLeaves(root->m_childs[1],collector);
            }
            else
            {
                collector.Process(root);
            }
        }
        template<class Collector>
        void        collideTT(  const NodeType* root0,const NodeType* root1,Collector& collector) const
        {
            if(root0&&root1)
            {
                size_t                          depth=1;
                size_t                          treshold=DOUBLE_STACKSIZE-4;
                vector<sStkNN>  stkStack;
                stkStack.resize(DOUBLE_STACKSIZE);
                stkStack[0]=sStkNN(root0,root1);
                do  {
                    sStkNN  p=stkStack[--depth];
                    if(depth>treshold)
                    {
                        stkStack.resize(stkStack.size()*2);
                        treshold=stkStack.size()-4;
                    }
                    if(p.a==p.b)
                    {
                        if(p.a->IsInternal())
                        {
                            stkStack[depth++]=sStkNN(p.a->m_childs[0],p.a->m_childs[0]);
                            stkStack[depth++]=sStkNN(p.a->m_childs[1],p.a->m_childs[1]);
                            stkStack[depth++]=sStkNN(p.a->m_childs[0],p.a->m_childs[1]);
                        }
                    }
                    else if(IntersectAabbAabb(p.a->m_volume,p.b->m_volume))
                    {
                        if(p.a->IsInternal())
                        {
                            if(p.b->IsInternal())
                            {
                                stkStack[depth++]=sStkNN(p.a->m_childs[0],p.b->m_childs[0]);
                                stkStack[depth++]=sStkNN(p.a->m_childs[1],p.b->m_childs[0]);
                                stkStack[depth++]=sStkNN(p.a->m_childs[0],p.b->m_childs[1]);
                                stkStack[depth++]=sStkNN(p.a->m_childs[1],p.b->m_childs[1]);
                            }
                            else
                            {
                                stkStack[depth++]=sStkNN(p.a->m_childs[0],p.b);
                                stkStack[depth++]=sStkNN(p.a->m_childs[1],p.b);
                            }
                        }
                        else
                        {
                            if(p.b->IsInternal())
                            {
                                stkStack[depth++]=sStkNN(p.a,p.b->m_childs[0]);
                                stkStack[depth++]=sStkNN(p.a,p.b->m_childs[1]);
                            }
                            else
                            {
                                collector.Process(p.a,p.b);
                            }
                        }
                    }
                } while(depth);
            }
        }
        template<class Collector>
        void        collideTTpersistentStack(   const NodeType* root0, const NodeType* root1,Collector& collector)
        {
            if(root0&&root1)
            {
                size_t  depth=1;
                size_t  treshold=DOUBLE_STACKSIZE-4;

                m_stkStack.resize(DOUBLE_STACKSIZE);
                m_stkStack[0]=sStkNN(root0,root1);
                do
                {
                    sStkNN  p=m_stkStack[--depth];
                    if(depth>treshold)
                    {
                        m_stkStack.resize(m_stkStack.size()*2);
                        treshold=m_stkStack.size()-4;
                    }
                    if(p.a==p.b)
                    {
                        if(p.a->IsInternal())
                        {
                            m_stkStack[depth++]=sStkNN(p.a->m_childs[0],p.a->m_childs[0]);
                            m_stkStack[depth++]=sStkNN(p.a->m_childs[1],p.a->m_childs[1]);
                            m_stkStack[depth++]=sStkNN(p.a->m_childs[0],p.a->m_childs[1]);
                        }
                    }
                    else if(IntersectAabbAabb(p.a->m_volume,p.b->m_volume))
                    {
                        if(p.a->IsInternal())
                        {
                            if(p.b->IsInternal())
                            {
                                m_stkStack[depth++]=sStkNN(p.a->m_childs[0],p.b->m_childs[0]);
                                m_stkStack[depth++]=sStkNN(p.a->m_childs[1],p.b->m_childs[0]);
                                m_stkStack[depth++]=sStkNN(p.a->m_childs[0],p.b->m_childs[1]);
                                m_stkStack[depth++]=sStkNN(p.a->m_childs[1],p.b->m_childs[1]);
                            }
                            else
                            {
                                m_stkStack[depth++]=sStkNN(p.a->m_childs[0],p.b);
                                m_stkStack[depth++]=sStkNN(p.a->m_childs[1],p.b);
                            }
                        }
                        else
                        {
                            if(p.b->IsInternal())
                            {
                                m_stkStack[depth++]=sStkNN(p.a,p.b->m_childs[0]);
                                m_stkStack[depth++]=sStkNN(p.a,p.b->m_childs[1]);
                            }
                            else
                            {
                                collector.Process(p.a,p.b);
                            }
                        }
                    }
                } while(depth);
            }
        }
        template<class Collector>
        void        collideTV(  const NodeType* root,   const VolumeType& volume,   Collector& collector) const
        {
            if(root)
            {
//              ATTRIBUTE_ALIGNED16(VolumeType)     volume(vol);
//              btAlignedObjectArray<const NodeType*>   stack;
                AZStd::fixed_vector<const NodeType*,SIMPLE_STACKSIZE> stack;
                //stack.reserve(SIMPLE_STACKSIZE);
                stack.push_back(root);
                do  {
                    const NodeType* n=stack[stack.size()-1];
                    stack.pop_back();
                    if(IntersectAabbAabb(n->m_volume,volume))
                    {
                        if(n->IsInternal())
                        {
                            stack.push_back(n->m_childs[0]);
                            stack.push_back(n->m_childs[1]);
                        }
                        else
                        {
                            collector.Process(n);
                        }
                    }
                } while(!stack.empty());
            }
        }

        template<class Collector>
        void collideTP(const NodeType* root, const AZ::Plane& plane, Collector& collector) const
        {
            if (root)
            {
                AZStd::fixed_vector<const NodeType*,SIMPLE_STACKSIZE> stack;
                stack.push_back(root);
                do
                {
                    const NodeType* n=stack[stack.size()-1];
                    stack.pop_back();
                    if (IntersectAabbPlane(n->m_volume, plane))
                    {
                        if(n->IsInternal())
                        {
                            stack.push_back(n->m_childs[0]);
                            stack.push_back(n->m_childs[1]);
                        }
                        else
                        {
                            collector.Process(n);
                        }
                    }
                } while (!stack.empty());
            }
        }

        ///rayTest is a re-entrant ray test, and can be called in parallel as long as the btAlignedAlloc is thread-safe (uses locking etc)
        ///rayTest is slower than rayTestInternal, because it builds a local stack, using memory allocations, and it recomputes signs/rayDirectionInverses each time
        template<class Collector>
        static void rayTest( const NodeType* root, const AZ::Vector3& rayFrom, const AZ::Vector3& rayTo, Collector& collector)
        {
            if(root)
            {
                AZ::Vector3 ray = rayTo-rayFrom;
                AZ::Vector3 rayDir = ray.GetNormalized();

                ///what about division by zero? --> just set rayDirection[i] to INF/1e30
                AZ::Vector3 rayDirectionInverse = AZ::Vector3::CreateSelectCmpEqual(rayDir,AZ::Vector3::CreateZero(),AZ::Vector3(1e30),rayDir.GetReciprocal());

                unsigned int signs[3];// = { rayDirectionInverse[0] < 0.0f, rayDirectionInverse[1] < 0.0f, rayDirectionInverse[2] < 0.0f };
                signs[0] = rayDirectionInverse.GetX() < 0.0f;
                signs[1] = rayDirectionInverse.GetY() < 0.0f;
                signs[2] = rayDirectionInverse.GetZ() < 0.0f;

                //float lambda_max = rayDir.Dot(ray);

                AZ::Vector3 resultNormal;

                //btAlignedObjectArray<const NodeType*> stack;
                vector<const NodeType*> stack;

                int                             depth=1;
                int                             treshold=DOUBLE_STACKSIZE-2;

                stack.resize(DOUBLE_STACKSIZE);
                stack[0]=root;
                AZ::Vector3 bounds[2];
                do  {
                    const NodeType* node=stack[--depth];

                    bounds[0] = node->m_volume.GetMin();
                    bounds[1] = node->m_volume.GetMax();

                    //float tmin = 1.0f;
                    //float lambda_min = 0.0f;
                    // todo..
                    unsigned int result1 = /*btRayAabb2(rayFrom,rayDirectionInverse,signs,bounds,tmin,lambda_min,lambda_max)*/0;
#ifdef COMPARE_BTRAY_AABB2
                    float param = 1.0f;
                    bool result2 = /*btRayAabb(rayFrom,rayTo,node->volume.GetMin(),node->volume.GetMax(),param,resultNormal)*/0;
                    AZ_Assert(result1 == result2, "");
#endif //TEST_BTRAY_AABB2
                    if(result1)
                    {
                        if(node->IsInternal())
                        {
                            if(depth>treshold)
                            {
                                stack.resize(stack.size()*2);
                                treshold=stack.size()-2;
                            }
                            stack[depth++]=node->m_childs[0];
                            stack[depth++]=node->m_childs[1];
                        }
                        else
                        {
                            collector.Process(node);
                        }
                    }
                } while(depth);

            }
        }

        ///rayTestInternal is faster than rayTest, because it uses a persistent stack (to reduce dynamic memory allocations to a minimum) and it uses precomputed signs/rayInverseDirections
        ///rayTestInternal is used by btDbvtBroadphase to accelerate world ray casts
        template<class Collector>
        void        rayTestInternal(const NodeType* root, const AZ::Vector3& rayFrom, const AZ::Vector3& rayTo, const AZ::Vector3& rayDirectionInverse, unsigned int signs[3], const float lambda_max, const AZ::Vector3& aabbMin, const AZ::Vector3& aabbMax, Collector& collector) const
        {
            (void)rayFrom;(void)rayTo;(void)rayDirectionInverse;(void)signs;(void)lambda_max;
            if(root)
            {
                AZ::Vector3 resultNormal;

                int depth=1;
                int treshold=DOUBLE_STACKSIZE-2;
                vector<const NodeType*> stack;
                stack.resize(DOUBLE_STACKSIZE);
                stack[0]=root;
                AZ::Vector3 bounds[2];
                do
                {
                    const NodeType* node=stack[--depth];
                    bounds[0] = node->m_volume.GetMin()+aabbMin;
                    bounds[1] = node->m_volume.GetMax()+aabbMax;

                    //float tmin = 1.0f;
                    //float lambda_min = 0.0f;
                    unsigned int result1=false;
                    // todo...
                    result1 = /*btRayAabb2(rayFrom,rayDirectionInverse,signs,bounds,tmin,lambda_min,lambda_max)*/false;
                    if(result1)
                    {
                        if(node->IsInternal())
                        {
                            if(depth>treshold)
                            {
                                stack.resize(stack.size()*2);
                                treshold=stack.size()-2;
                            }
                            stack[depth++]=node->m_childs[0];
                            stack[depth++]=node->m_childs[1];
                        }
                        else
                        {
                            collector.Process(node);
                        }
                    }
                } while(depth);
            }
        }

        template<class Collector>
        static void collideKDOP(const NodeType* root,   const AZ::Vector3* normals, const float* offsets, int count, Collector& collector)
        {
            (void)root;(void)normals;(void)offsets;(void)count;(void)collector;
/*          if(root)
            {
                const int                       inside=(1<<count)-1;
                btAlignedObjectArray<sStkNP>    stack;
                int                             signs[sizeof(unsigned)*8];
                btAssert(count<int (sizeof(signs)/sizeof(signs[0])));
                for(int i=0;i<count;++i)
                {
                    signs[i]=   ((normals[i].x()>=0)?1:0)+
                                ((normals[i].y()>=0)?2:0)+
                                ((normals[i].z()>=0)?4:0);
                }
                stack.reserve(SIMPLE_STACKSIZE);
                stack.push_back(sStkNP(root,0));
                do  {
                    sStkNP  se=stack[stack.size()-1];
                    bool    out=false;
                    stack.pop_back();
                    for(int i=0,j=1;(!out)&&(i<count);++i,j<<=1)
                    {
                        if(0==(se.mask&j))
                        {
                            const int   side=se.node->volume.Classify(normals[i],offsets[i],signs[i]);
                            switch(side)
                            {
                            case    -1: out=true;break;
                            case    +1: se.mask|=j;break;
                            }
                        }
                    }
                    if(!out)
                    {
                        if((se.mask!=inside)&&(se.node->isinternal()))
                        {
                            stack.push_back(sStkNP(se.node->childs[0],se.mask));
                            stack.push_back(sStkNP(se.node->childs[1],se.mask));
                        }
                        else
                        {
                            if(policy.AllLeaves(se.node)) enumLeaves(se.node,policy);
                        }
                    }
                } while(!stack.empty());
            }*/
        }
        template<class Collector>
        static void collideOCL( const NodeType* root,   const AZ::Vector3* normals, const float* offsets, const AZ::Vector3& sortaxis, int count, Collector& collector, bool fullsort=true)
        {
            (void)root;(void)normals;(void)offsets;(void)sortaxis;(void)count;(void)offsets;(void)collector;(void)fullsort;
/*              if(root)
                {
                    const unsigned                  srtsgns=(sortaxis[0]>=0?1:0)+
                        (sortaxis[1]>=0?2:0)+
                        (sortaxis[2]>=0?4:0);
                    const int                       inside=(1<<count)-1;
                    btAlignedObjectArray<sStkNPS>   stock;
                    btAlignedObjectArray<int>       ifree;
                    btAlignedObjectArray<int>       stack;
                    int                             signs[sizeof(unsigned)*8];
                    btAssert(count<int (sizeof(signs)/sizeof(signs[0])));
                    for(int i=0;i<count;++i)
                    {
                        signs[i]=   ((normals[i].x()>=0)?1:0)+
                            ((normals[i].y()>=0)?2:0)+
                            ((normals[i].z()>=0)?4:0);
                    }
                    stock.reserve(SIMPLE_STACKSIZE);
                    stack.reserve(SIMPLE_STACKSIZE);
                    ifree.reserve(SIMPLE_STACKSIZE);
                    stack.push_back(allocate(ifree,stock,sStkNPS(root,0,root->volume.ProjectMinimum(sortaxis,srtsgns))));
                    do  {
                        const int   id=stack[stack.size()-1];
                        sStkNPS     se=stock[id];
                        stack.pop_back();ifree.push_back(id);
                        if(se.mask!=inside)
                        {
                            bool    out=false;
                            for(int i=0,j=1;(!out)&&(i<count);++i,j<<=1)
                            {
                                if(0==(se.mask&j))
                                {
                                    const int   side=se.node->volume.Classify(normals[i],offsets[i],signs[i]);
                                    switch(side)
                                    {
                                    case    -1: out=true;break;
                                    case    +1: se.mask|=j;break;
                                    }
                                }
                            }
                            if(out) continue;
                        }
                        if(policy.Descent(se.node))
                        {
                            if(se.node->isinternal())
                            {
                                const NodeType* pns[]={ se.node->childs[0],se.node->childs[1]};
                                sStkNPS     nes[]={ sStkNPS(pns[0],se.mask,pns[0]->volume.ProjectMinimum(sortaxis,srtsgns)),
                                    sStkNPS(pns[1],se.mask,pns[1]->volume.ProjectMinimum(sortaxis,srtsgns))};
                                const int   q=nes[0].value<nes[1].value?1:0;
                                int         j=stack.size();
                                if(fsort&&(j>0))
                                {
                                    // Insert 0
                                    j=nearest(&stack[0],&stock[0],nes[q].value,0,stack.size());
                                    stack.push_back(0);
#if DBVT_USE_MEMMOVE
                                    memmove(&stack[j+1],&stack[j],sizeof(int)*(stack.size()-j-1));
#else
                                    for(int k=stack.size()-1;k>j;--k) stack[k]=stack[k-1];
#endif
                                    stack[j]=allocate(ifree,stock,nes[q]);
                                    // Insert 1
                                    j=nearest(&stack[0],&stock[0],nes[1-q].value,j,stack.size());
                                    stack.push_back(0);
#if DBVT_USE_MEMMOVE
                                    memmove(&stack[j+1],&stack[j],sizeof(int)*(stack.size()-j-1));
#else
                                    for(int k=stack.size()-1;k>j;--k) stack[k]=stack[k-1];
#endif
                                    stack[j]=allocate(ifree,stock,nes[1-q]);
                                }
                                else
                                {
                                    stack.push_back(allocate(ifree,stock,nes[q]));
                                    stack.push_back(allocate(ifree,stock,nes[1-q]));
                                }
                            }
                            else
                            {
                                policy.Process(se.node,se.value);
                            }
                        }
                    } while(stack.size());
                }*/
        }

        template<class Collector>
        static void collideTU(const NodeType* root, Collector& collector)
        {
            (void)root;(void)collector;
/*          if(root)
            {
                btAlignedObjectArray<const NodeType*>   stack;
                stack.reserve(SIMPLE_STACKSIZE);
                stack.push_back(root);
                do  {
                    const NodeType* n=stack[stack.size()-1];
                    stack.pop_back();
                    if(policy.Descent(n))
                    {
                        if(n->isinternal())
                        { stack.push_back(n->childs[0]);stack.push_back(n->childs[1]); }
                        else
                        { policy.Process(n); }
                    }
                } while(stack.size()>0);
            }*/
        }

    private:
        BvDynamicTree(const BvDynamicTree&) {}

        // Helpers
        //static AZ_FORCE_INLINE int    nearest(const int* i,const BvDynamicTree::sStkNPS* a,const float& v,int l,int h)
        //{
        //  int m=0;
        //  while(l<h)
        //  {
        //      m=(l+h)>>1;
        //      if(a[i[m]].value>=v) l=m+1; else h=m;
        //  }
        //  return h;
        //}
        //static AZ_FORCE_INLINE int    allocate( int_fixed_stack_type& ifree, stknps_fixed_stack_type& stock, const sStkNPS& value)
        //{
        //  int i;
        //  if( !ifree.empty() )
        //  {
        //      i=ifree[ifree.size()-1];
        //      ifree.pop_back();
        //      stock[i]=value;
        //  }
        //  else
        //  {
        //      i=stock.size();
        //      stock.push_back(value);
        //  }
        //  return i;
        //}
        //

        AZ_FORCE_INLINE void deletenode( NodeType* node)
        {
            //btAlignedFree(pdbvt->m_free);
            delete m_free;
            m_free=node;
        }

        void recursedeletenode( NodeType* node)
        {
            if(!node->IsLeaf())
            {
                recursedeletenode(node->m_childs[0]);
                recursedeletenode(node->m_childs[1]);
            }

            if( node == m_root ) m_root=0;
            deletenode(node);
        }


        AZ_FORCE_INLINE NodeType*   createnode( NodeType* parent, void* data)
        {
            NodeType*   node;
            if(m_free)
            { node=m_free;m_free=0; }
            else
            { node = aznew NodeType(); }
            node->m_parent      =   parent;
            node->m_data        =   data;
            node->m_childs[1]   =   0;
            return node;
        }
        AZ_FORCE_INLINE NodeType*   createnode( BvDynamicTree::NodeType* parent, const VolumeType& volume, void* data)
        {
            NodeType*   node = createnode(parent,data);
            node->m_volume=volume;
            return node;
        }
        //
        AZ_FORCE_INLINE NodeType*   createnode( BvDynamicTree::NodeType* parent, const VolumeType& volume0, const VolumeType& volume1, void* data)
        {
            NodeType*   node = createnode(parent,data);
            Merge(volume0,volume1,node->m_volume);
            return node;
        }
        void        insertleaf( NodeType* root, NodeType* leaf);
        NodeType*   removeleaf( NodeType* leaf);
        void        fetchleaves(NodeType* root,NodeArrayType& leaves,int depth=-1);
        void        split(const NodeArrayType& leaves,NodeArrayType& left,NodeArrayType& right,const AZ::Vector3& org,const AZ::Vector3& axis);
        VolumeType  bounds(const NodeArrayType& leaves);
        void        bottomup( NodeArrayType& leaves );
        NodeType*   topdown(NodeArrayType& leaves,int bu_treshold);
        AZ_FORCE_INLINE NodeType*   sort(NodeType* n,NodeType*& r);

        NodeType*       m_root;
        NodeType*       m_free;
        int             m_lkhd;
        int             m_leaves;
        unsigned        m_opath;

        //btAlignedObjectArray<sStkNN>  m_stkStack;
        // Profile and choose static or dynamic vector.
        typedef AZStd::fixed_vector<sStkNN,DOUBLE_STACKSIZE> stknn_fixed_stack_type;
        typedef AZStd::fixed_vector<int,SIMPLE_STACKSIZE> int_fixed_stack_type;
        typedef AZStd::fixed_vector<sStkNPS,SIMPLE_STACKSIZE> stknps_fixed_stack_type;

        stknn_fixed_stack_type  m_stkStack;
    };
}

#endif // RR_DYNAMIC_BOUNDING_VOLUME_TREE_H
#pragma once
