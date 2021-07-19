----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------
function ConstVec(v)
    return v
end

----------------------------------------------------
-- global Vectors table and some vector functions
----------------------------------------------------
g_Vectors =
{
    v000=ConstVec({x=0,y=0,z=0}),
    v001=ConstVec({x=0,y=0,z=1}),
    v010=ConstVec({x=0,y=1,z=0}),
    v011=ConstVec({x=0,y=1,z=1}),
    v100=ConstVec({x=1,y=0,z=0}),
    v101=ConstVec({x=1,y=0,z=1}),
    v110=ConstVec({x=1,y=1,z=0}),
    v111=ConstVec({x=1,y=1,z=1}),

    up = ConstVec({x=0,y=0,z=1}),
    down = ConstVec({x=0,y=0,z=-1}),

    temp={x=0,y=0,z=0},
    tempColor={x=0,y=0,z=0},
    
    temp_v1={x=0,y=0,z=0},
    temp_v2={x=0,y=0,z=0},
    temp_v3={x=0,y=0,z=0},
    temp_v4={x=0,y=0,z=0},
    temp_v5={x=0,y=0,z=0},
    temp_v6={x=0,y=0,z=0},
    
    vecMathTemp1={x=0,y=0,z=0},
    vecMathTemp2={x=0,y=0,z=0},    
}

-----------------------------------------------------------------------------
-- Math constants
-----------------------------------------------------------------------------
g_Rad2Deg = 180/math.pi;
g_Deg2Rad = math.pi/180;
g_Pi = math.pi;
g_2Pi = 2*math.pi;
g_Pi2 = 0.5*math.pi;

-----------------------------------------------------------------------------
-- Import commonly used math functions from math. table to global namespace.
-----------------------------------------------------------------------------
random = math.random;
math.randomseed(os.time()); -- seed and pop
random();

-----------------------------------------------------------------------------

function IsNullVector(a)
    return (a.x==0 and a.y==0 and a.z ==0);
end

function IsNotNullVector(a)
    return (a.x~=0 or a.y~=0 or a.z ~=0);
end


function LengthSqVector(a)
    return (a.x * a.x + a.y * a.y + a.z * a.z);
end

function LengthVector(a)

    return math.sqrt(LengthSqVector(a));
end

function DistanceSqVectors(a, b)
    local x = a.x-b.x;
    local y = a.y-b.y;
    local z = a.z-b.z;
    return x*x + y*y + z*z;
end

function DistanceSqVectors2d(a, b)
    local x = a.x-b.x;
    local y = a.y-b.y;
    return x*x + y*y;
end

function DistanceVectors(a, b)
    local x = a.x-b.x;
    local y = a.y-b.y;
    local z = a.z-b.z;
    return math.sqrt(x*x + y*y + z*z);
end

-----------------------------------------------------------------------------
function dotproduct3d(a,b)
   return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
end

-----------------------------------------------------------------------------
function dotproduct2d(a,b)
   return (a.x * b.x) + (a.y * b.y);
end

function LogVec(name,v)
    Log("%s = (%f %f %f)",name,v.x,v.y,v.z);
end

function ZeroVector(dest)
    dest.x=0;
    dest.y=0;
    dest.z=0;
end

function CopyVector(dest,src)
    dest.x=src.x;
    dest.y=src.y;
    dest.z=src.z;
end
----------------------------------
function SumVectors(a,b)
    return {x=a.x+b.x,y=a.y+b.y,z=a.z+b.z};
end
function NegVector(a)
    a.x = -a.x;
    a.y = -a.y;
    a.z = -a.z;
end
----------------------------------
function SubVectors(dest,a,b)

    dest.x = a.x - b.x;
    dest.y = a.y - b.y;
    dest.z = a.z - b.z;
end

----------------------------------
function FastSumVectors(dest,a,b)
    dest.x=a.x+b.x;
    dest.y=a.y+b.y;
    dest.z=a.z+b.z;
end

----------------------------------
function DifferenceVectors(a,b)
    return {x=a.x-b.x,y=a.y-b.y,z=a.z-b.z};
end

----------------------------------
function FastDifferenceVectors(dest,a,b)
    dest.x=a.x-b.x;
    dest.y=a.y-b.y;
    dest.z=a.z-b.z;
end

----------------------------------
function ProductVectors(a,b)
    return {x=a.x*b.x,y=a.y*b.y,z=a.z*b.z};
end

----------------------------------
function FastProductVectors(dest,a,b)
    dest.x=a.x*b.x;
    dest.y=a.y*b.y;
    dest.z=a.z*b.z;
end


----------------------------------
function ScaleVector(a,b)
    return {x=a.x*b,y=a.y*b,z=a.z*b};
end

function ScaleVectorInPlace(a,b)
    a.x=a.x*b;
    a.y=a.y*b;
    a.z=a.z*b;
end

----------------------------------
function NormalizeVector(a)
    local len=math.sqrt(LengthSqVector(a));
    local multiplier;
    if(len>0)then
        multiplier=1/len;
    else
        multiplier=0.0001;
    end
    a.x=a.x*multiplier;
    a.y=a.y*multiplier;
    a.z=a.z*multiplier;
    return a
end

----------------------------------
function FastScaleVector(dest,a,b)
    dest.x=a.x*b;
    dest.y=a.y*b;
    dest.z=a.z*b;
end


--linear interpolation
----------------------------------
function LerpColors(a,b,k)
    g_Vectors.tempColor.x = a.x+(b.x-a.x)*k
    g_Vectors.tempColor.y = a.y+(b.y-a.y)*k
    g_Vectors.tempColor.z = a.z+(b.z-a.z)*k
    return g_Vectors.tempColor;
end

----------------------------------
function Lerp(a,b,k)
    return (a + (b - a)*k);
end

----------------------------------
function __max(a, b)
    if (a > b) then
        return a;
    else
        return b;
    end
end

----------------------------------
function __min(a, b)
    if (a < b) then
        return a;
    else
        return b;
    end
end

----------------------------------
function clamp(_n, _min, _max)
    if (_n > _max) then _n = _max; end
    if (_n < _min) then _n = _min; end
    return _n;
end

----------------------------------
function Interpolate(actual,goal,speed)
    
    local delta = goal - actual;
    
    if (math.abs(delta)<0.001) then
        return goal;
    end
        
    local res = actual + delta * __min(speed,1.0);
    
    return res;
end

-----------------------------------
function sgn(a)
  if (a == 0) then
    return 0;
  elseif (a > 0) then 
    return 1;
  else
    return -1;  
  end
end

-----------------------------------
function sgnnz(a)
  return (a>=0) and 1 or -1;
end

-----------------------------------
function sqr(a)
  return a*a;
end
  
-----------------
function randomF(a,b)

    if (a>b) then
        local c = b;
        b = a;
        a = c;
    end
    
    local delta = b-a;
    
    return (a + math.random()*delta);
end

function VecRotate90_Z(v)
    local x = v.x;
     v.x = v.y;
     v.y = -x;
end

function VecRotateMinus90_Z(v)
    local x = v.x;
     v.x = -v.y;
     v.y = x;
end

function iff(c,a,b)
    if c then return a else return b end
end

-----------------------------------------------------------------------------
-- calculate cross product
function crossproduct3d( dest, p, q )

        dest.x = p.y*q.z-p.z*q.y;
        dest.y = p.z*q.x-p.x*q.z;
        dest.z = p.x*q.y-p.y*q.x;

end

-- rotate vector p around vector r by angle
-- the length of r needs to be 1
function RotateVectorAroundR( dest, p, r, angle )

        -- p' = v1 + v2 +v3
        -- v1 = pcosA
        -- v2 = r**psinA;
        -- v3 = r< r,p >( 1- cosA );

        local cosValue = math.cos( angle );
        local sinValue = math.sin( angle );
        
        local v1 = {};
        local v2 = {};
        local v3 = {};
        local vTmp = {};

        -- v1
        CopyVector( v1, p );
        FastScaleVector( v1, v1, cosValue );

        -- v2
        CopyVector( vTmp, p );
        FastScaleVector( vTmp, vTmp, sinValue );
        crossproduct3d( v2, r, vTmp ); 
        
        -- v3
        CopyVector( v3, r );
        FastScaleVector( v3, v3, dotproduct3d( r, p ) );
        FastScaleVector( v3, v3, 1.0 - cosValue );

        -- p'
        CopyVector( dest, v1 );
        FastSumVectors( dest, v1, v2 );
        FastSumVectors( dest, dest, v3 );
    
end
    
-- project P to the surface whose normal vector is N.
function ProjectVector( dest, P, N )

        -- projected vector is vector X (output)
        -- X =P+(-P*N)N

        local minusP ={};
        FastScaleVector( minusP , P , -1.0 );

        local t = dotproduct3d( minusP, N );
        CopyVector( dest , N );
        FastScaleVector( dest , dest ,  t );
        FastSumVectors( dest , dest , P );

end

-- get a distance between line(pt+q) and point(a)
function DistanceLineAndPoint( a, p, q )

    -- d=|| p * (a-q) || / ||p|| 

    local length = LengthVector( p );
    local outerProduct;
    local vOuterProduct = {};
    local vTmp = {};
    local d;
    
    SubVectors( vTmp, a, q );
    crossproduct3d( vOuterProduct, p, vTmp );
    outerProduct =LengthVector( vOuterProduct );

    if ( length>0.01 ) then
        d = outerProduct/length;
    else
        d = 0.0;
    end

    return    d;
        
end

-- Get normalized direction vector from point 'a' to point 'b'
function GetDirection(a, b)
    local dir = {};
    SubVectors(dir, b, a);
    NormalizeVector(dir);
    return dir;
end

function GetAngleBetweenVectors2D(a, b)
    return math.acos(dotproduct2d(a, b));
end

function GetAngleBetweenVectors(a, b)
    return math.acos(clamp(dotproduct3d(a, b), -1, 1));
end