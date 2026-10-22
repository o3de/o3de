#pragma OutputFormatHint(default R16G16B16A16_FLOAT)
#pragma OutputFormatHint(target 0 R32)
#pragma OutputFormatHint(target 1 R32G32)
#pragma OutputFormatHint(target 2 R32A32)
#pragma OutputFormatHint(target 3 R16G16B16A16_UNORM)
#pragma OutputFormatHint(target 4 R16G16B16A16_SNORM)
#pragma OutputFormatHint(target 5 R16G16B16A16_UINT)
#pragma OutputFormatHint(target 6 R16G16B16A16_SINT)
#pragma OutputFormatHint(target 7 R32G32B32A32)
struct PSOut_All
{
float4 m_color0 : SV_Target0 ;
float4 m_color1 : SV_Target1 ;
float4 m_color2 : SV_Target2 ;
float4 m_color3 : SV_Target3 ;
float4 m_color4 : SV_Target4 ;
float4 m_color5 : SV_Target5 ;
float4 m_color6 : SV_Target6 ;
float4 m_color7 : SV_Target7 ;
} ;

struct PSOut_Half
{
float4 m_color0 : SV_Target0 ;
float4 m_color1 : SV_Target1 ;
float4 m_color2 : SV_Target2 ;
float4 m_color3 : SV_Target3 ;
} ;

PSOut_All MainPS_All()
{
PSOut_All psOut ;
psOut . m_color0 = float4 ( 1 , 1 , 1 , 1 ) ;
psOut . m_color1 = float4 ( 1 , 1 , 1 , 1 ) ;
psOut . m_color2 = float4 ( 1 , 1 , 1 , 1 ) ;
psOut . m_color3 = float4 ( 1 , 1 , 1 , 1 ) ;
psOut . m_color4 = float4 ( 1 , 1 , 1 , 1 ) ;
psOut . m_color5 = float4 ( 1 , 1 , 1 , 1 ) ;
psOut . m_color6 = float4 ( 1 , 1 , 1 , 1 ) ;
psOut . m_color7 = float4 ( 1 , 1 , 1 , 1 ) ;
return psOut ;
} 

PSOut_Half MainPS_Half()
{
PSOut_Half psOut ;
psOut . m_color0 = float4 ( 1 , 1 , 1 , 1 ) ;
psOut . m_color1 = float4 ( 1 , 1 , 1 , 1 ) ;
psOut . m_color2 = float4 ( 1 , 1 , 1 , 1 ) ;
psOut . m_color3 = float4 ( 1 , 1 , 1 , 1 ) ;
return psOut ;
} 

