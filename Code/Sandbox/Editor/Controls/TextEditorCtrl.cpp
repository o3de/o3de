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

#include "EditorDefs.h"

#include "TextEditorCtrl.h"

#define GRP_KEYWORD     0
#define GRP_CONSTANTS   1
#define GRP_DIRECTIVE   2
#define GRP_PRAGMA      3


// CTextEditorCtrl

CTextEditorCtrl::CTextEditorCtrl(QWidget* pParent)
    : QTextEdit(pParent)
    , m_sc(document())
{
    /*
    //reconfigure CSyntaxColorizer's default keyword groupings
    LPTSTR sKeywords = "for,for,else,main,struct,enum,switch,auto,"
        "template,explicit,this,bool,extern,thread,break,false,"
        "throw,case,namespace,true,catch,new,try,float,noreturn,"
        "char,operator,typedef,class,friend,private,const,goto,"
        "protected,typename,if,public,union,continue,inline,"
        "unsigned,using,directive,default,int,return,delete,short,"
        "signed,virtual,sizeof,void,do,static,double,long,while";
    LPTSTR sDirectives = "#define,#elif,#else,#endif,#error,#ifdef,"
        "#ifndef,#import,#include,#line,#pragma,#undef";
    LPTSTR sPragmas = "comment,optimize,auto_inline,once,warning,"
        "component,pack,function,intrinsic,setlocale,hdrstop,message";

    m_sc.ClearKeywordList();
    m_sc.AddKeyword(sKeywords,QColor(0,0,255),GRP_KEYWORD);
    m_sc.AddKeyword(sDirectives,QColor(0,0,255),GRP_DIRECTIVE);
    m_sc.AddKeyword(sPragmas,QColor(0,0,255),GRP_PRAGMA);
    m_sc.AddKeyword("REM,Rem,rem",QColor(255,0,255),4);
    */
    const char* sKeywords = "Shader,ShadeLayer,HW,LightStyle,ValueString,Orient,Origin,Params,Array,Template,Templates,"
        "Version,CGVProgram,CGVPParam,Name,"
        "DeclareLightMaterial,Side,Ambient,Diffuse,Specular,Emission,Shininess,"
        "Layer,Map,RGBGen,RgbGen,AlphaGen,NoDepthTest,Blend,TexCoordMod,Scale,UScale,VScale,ShiftNoise,Noise,SRange,TRange,"
        "Cull,Sort,State,NoCull,ShadowMapGen,Conditions,Vars,DepthWrite,NoColorMask,Portal,LMNoAlpha,"
        "TexColorOp,TexStage,TexType,TexFilter,TexGen,UpdateStyle,EvalLight,Style,TexDecal,Tex1Decal,TexBump,"
        "RCParam,RCombiner,RShader,TSParam,Reg,Comp,DepthMask,AlphaFunc,Light,LightType,ClipPlane,PlaneS,PlaneT,"
        "PolygonOffset,NoLightmap,ShineMap,Turbulence,tcMod,Procedure,TessSize,Spark,Sequence,Maps,Time,Loop,"
        "Mask,Public,float,RenderParams,User,"
        "rgbGen,blend,map,"
        "Translate,Identity,Rotate,RotateX,RotateY,RotateZ,Div,DeformGen,Scroll,UScroll,VScroll,Angle"
        "Type,Level,Amp,Phase,Freq,DeformVertexes,FlareSize,NoLight,Const,Start,"
        "Matrix,FLOAT,BYTE,Verts,Vertex,Normal,Normals,Color,Texture0,Texture1,Texture2,Texture3,Texture4,TNormals";

    const char* sConstants = "Decal,None,Nearest,TwoSided,RCRGBToAlpha,OcclusionTest,NoSet,Replace,FromClient,"
        "Opaque,MonitorNoise,Point,Front,Back,Water,TriLinear,"
        "MuzzleFlash,FromObj,Modulate,Base,SphereMap,Add,Glare,Additive,Intensity,White,Sin,Cos,Tan,"
        "$Diffuse,$None,$Specular,$Whiteimage,$Environment,$Glare,$Opacity,$Flare";

    const char* sDirectives = "#define,#elif,#else,#endif,#error,#ifdef,"
        "#ifndef,#import,#include,#line,#pragma,#undef";

    m_sc.ClearKeywordList();
    m_sc.AddKeyword(sKeywords, QColor(0, 0, 255), GRP_KEYWORD);
    m_sc.AddKeyword(sConstants, QColor(180, 0, 110), GRP_CONSTANTS);
    m_sc.AddKeyword(sDirectives, QColor(160, 0, 160), GRP_DIRECTIVE);

    m_sc.SetCommentColor(QColor(0, 128, 128));
    m_sc.SetStringColor(QColor(0, 128, 0));

    m_bModified = true;

    QFont font;
    font.setFamily("Courier New");
    font.setFixedPitch(true);
    font.setPointSize(10);
    setFont(font);

    setLineWrapMode(NoWrap);

    connect(this, &QTextEdit::textChanged, this, &CTextEditorCtrl::OnChange);
}

CTextEditorCtrl::~CTextEditorCtrl()
{
}


// CTextEditorCtrl message handlers

void CTextEditorCtrl::LoadFile(const QString& sFileName)
{
    if (m_filename == sFileName)
    {
        return;
    }

    m_filename = sFileName;

    clear();

    CCryFile file(sFileName.toUtf8().data(), "rb");
    if (file.Open(sFileName.toUtf8().data(), "rb"))
    {
        size_t length = file.GetLength();

        QByteArray text;
        text.resize(length);
        file.ReadRaw(text.data(), length);

        setPlainText(text);
    }

    m_bModified = false;
}

//////////////////////////////////////////////////////////////////////////
void CTextEditorCtrl::SaveFile(const QString& sFileName)
{
    if (sFileName.isEmpty())
    {
        return;
    }

    if (!CFileUtil::OverwriteFile(sFileName.toUtf8().data()))
    {
        return;
    }

    QFile file(sFileName);
    file.open(QFile::WriteOnly);

    file.write(toPlainText().toUtf8());

    m_bModified = false;
}

//////////////////////////////////////////////////////////////////////////
void CTextEditorCtrl::OnChange()
{

    m_bModified = true;
}

#include <Controls/moc_TextEditorCtrl.cpp>
