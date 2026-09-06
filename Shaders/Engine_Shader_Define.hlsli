
#define PI 3.1415926

#define TONE_LINEAR 0
#define TONE_FILMIC 1
#define TONE_UNCHARTED2 2
#define TONE_LUMAREINHARD 3


#define RT_RENDER_DEAFULT 0
// uv를 돌려서 렌더. 종맵
#define RT_RENDER_ROTATE  1
// Start End를 사용한다.
#define RT_RENDER_UVRENDER  2
// 0~1 사이로 고정한다.
#define RT_RENDER_FRAC  3
// Start End를 사용하고 0~1 사이로 고정한다.
#define RT_RENDER_UVRENDER_FRAC  4

/* Normal Enum Flag */
#define NONEWRITE_NORMAL -1
#define POSITIVE_X 0 
#define NEGATIVE_X 1 
#define POSITIVE_Y 2 
#define NEGATIVE_Y 3 
#define POSITIVE_Z 4 
#define NEGATIVE_Z 5 

/* Shadow Flag */
#define IS_SHADOWLIGHT 1

/* Sksp */
#define SAVEBRDF 1


struct Material_PS
{
    float4 Albedo; // baseColor
    
    float Roughness;
    float Metallic;
    float AO;
    float Emissive;
    
    float4 MultipleAlbedo;
    float3 EmissiveColor;
    float dummy;
};



/* D3D11_SAMPLER_DESC */
sampler			LinearSampler = sampler_state
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = wrap;
	AddressV = wrap;
    AddressW = wrap;
};

sampler LinearSampler_Clamp = sampler_state
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = clamp;
    AddressV = clamp;
    AddressW = clamp;
};

//sampler ShadowCompareSampler = sampler_state
//{
//    Filter = COMPARISON_MIN_MAG_MIP_LINEAR; // PCF를 위한 선형 필터링
//    ComparisonFunc = COMPARISON_LESS_EQUAL; // 깊이 비교 함수
//    AddressU = CLAMP; // 텍스처 좌표 클램핑
//    AddressV = CLAMP;
//    AddressW = CLAMP;
//};

sampler			PointSampler = sampler_state
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = wrap;
	AddressV = wrap;
	AddressW = wrap;
};

sampler MirrorSampler = sampler_state
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Mirror;
    AddressV = Mirror;
};

RasterizerState		RS_Default
{
	FillMode = SOLID;
	CullMode = BACK;
	FrontCounterClockwise = false;
};

RasterizerState RS_Cull_None
{
    FillMode = SOLID;
    CullMode = NONE;
    FrontCounterClockwise = false;
};

RasterizerState		RS_Cull_Front
{
	FillMode = SOLID;
	CullMode = Front;
	FrontCounterClockwise = false;
};

RasterizerState		RS_Wireframe
{
	FillMode = Wireframe;
	CullMode = BACK;
	FrontCounterClockwise = false;
};


DepthStencilState		DSS_Default
{
	DepthEnable = true;
	DepthWriteMask = all;
	DepthFunc = less_equal;
};

DepthStencilState		DSS_None
{
	DepthEnable = false;
	DepthWriteMask = zero;
};

DepthStencilState DSS_WriteNone
{
    DepthEnable = true;
    DepthWriteMask = zero;
};

BlendState		BS_Default
{
	BlendEnable[0] = false;


};

BlendState		BS_AlphaBlend_OnlyDiffuse
{
	BlendEnable[0] = true;
	SrcBlend = SRC_ALPHA;
	DestBlend = INV_SRC_ALPHA;
	BlendOp = Add;	
};

BlendState BS_AlphaBlend_WithDepth
{
    BlendEnable[0] = true;
    BlendEnable[2] = true;
    SrcBlend = SRC_ALPHA;
    DestBlend = INV_SRC_ALPHA;
    BlendOp = Add;
};


BlendState BS_AlphaBlend
{
    BlendEnable[0] = true;
    BlendEnable[1] = true;
    SrcBlend = SRC_ALPHA;
    DestBlend = INV_SRC_ALPHA;
    BlendOp = Add;
};

BlendState		BS_OneBlend
{
	// 0번 렌더타겟, 1번 렌더타겟 둘다 해당 블렌드를 적용시키겠다. (shade, specular)
    BlendEnable[0] = true;
    BlendEnable[1] = true;

	// sourColor.rgb * 1 + DestColor.rgb * 1
    SrcBlend = ONE;
    DestBlend = ONE;
    BlendOp = Add;
};

BlendState BS_WeightAccumulate
{
    BlendEnable[0] = true;
    BlendEnable[1] = true;
    BlendEnable[2] = true;

    SrcBlend[0] = ONE;
    DestBlend[0] = ONE;
    BlendOp[0] = Add;
    SrcBlendAlpha[0] = ONE;
    DestBlendAlpha[0] = ONE;
    BlendOpAlpha[0] = Add;

    SrcBlend[1] = ZERO;
    DestBlend[1] = INV_SRC_COLOR;
    BlendOp[1] = Add;

    SrcBlend[2] = ONE;
    DestBlend[2] = ONE;
    BlendOp[2] = Add;
    SrcBlendAlpha[2] = ONE;
    DestBlendAlpha[2] = ONE;
    BlendOpAlpha[2] = Add;
    
};

BlendState BS_InvAlphaBlend
{
    BlendEnable[0] = true;

    SrcBlend = INV_SRC_ALPHA;
    DestBlend = SRC_ALPHA;
    BlendOp = Add;
};

static const float2 poissonDisk[16] =
{
    float2(-0.94201624, -0.39906216), float2(0.94558609, -0.76890725),
            float2(-0.094184101, -0.92938870), float2(0.34495938, 0.29387760),
            float2(-0.91588581, 0.45771432), float2(-0.81544232, -0.87912464),
            float2(-0.38277543, 0.27676845), float2(0.97484398, 0.75648379),
            float2(0.44323325, -0.97511554), float2(0.53742981, -0.47373420),
            float2(-0.26496911, -0.41893023), float2(0.79197514, 0.19090188),
            float2(-0.24188840, 0.99706507), float2(-0.81409955, 0.91437590),
            float2(0.19984126, 0.78641367), float2(0.14383161, -0.14100790)
};
        
static const float2 diskSamples64[64] =
{
    float2(0.0, 0.0),
            float2(-0.12499844227275288, 0.000624042775189866), float2(0.1297518688031755, -0.12006020382326336),
            float2(-0.017851253586055427, 0.21576916541852392), float2(-0.1530983013115895, -0.19763833164521946),
            float2(0.27547541035593626, 0.0473106572479027), float2(-0.257522587854559, 0.16562643733622642),
            float2(0.0842605283808073, -0.3198048832600703), float2(0.1645196099088727, 0.3129429627830483),
            float2(-0.3528833088400373, -0.12687935349026194), float2(0.36462214742013344, -0.1526456341030772),
            float2(-0.17384046457324884, 0.37637015407303087), float2(-0.1316547617859344, -0.4125130588224921),
            float2(0.3910687393754993, 0.2240317858770442), float2(-0.45629121277761536, 0.10270505898899496),
            float2(0.27645268679640483, -0.3974278701387824), float2(0.06673001731984558, 0.49552709793561556),
            float2(-0.39574431915605623, -0.33016879600548193), float2(0.5297612167716342, -0.024557141621887494),
            float2(-0.3842909284448636, 0.3862583103507092), float2(0.0230336562454131, -0.5585422550532486),
            float2(0.36920334463249477, 0.43796562686149154), float2(-0.5814490172413539, -0.07527974727019048),
            float2(0.4903718680780365, -0.3448339179919178), float2(-0.13142003698572613, 0.5981043168868373),
            float2(-0.31344141845114937, -0.540721256470773), float2(0.608184438565748, 0.19068741092811003),
            float2(-0.5882602609696388, 0.27536315179038107), float2(0.25230610046544444, -0.6114259003901626),
            float2(0.23098706800827415, 0.6322736546883326), float2(-0.6076303951666067, -0.31549215975943595),
            float2(0.6720886334230931, -0.1807536135834609), float2(-0.37945598830371974, 0.5966683776943834),
            float2(-0.1251555455510758, -0.7070792667147104), float2(0.5784815570900413, 0.44340623372555477),
            float2(-0.7366710399837763, 0.0647362251696953), float2(0.50655463562529, -0.553084443034271),
            float2(8.672987356252326e-05, 0.760345311340794), float2(-0.5205650355786364, -0.5681215043747359),
            float2(0.7776435491294021, 0.06815798190547596), float2(-0.6273416101921778, 0.48108471615868836),
            float2(0.1393236805531513, -0.7881712453757264), float2(0.4348773806743975, 0.6834703093608201),
            float2(-0.7916014213464706, -0.21270211499241704), float2(0.7357897682897174, -0.38224784745000717),
            float2(-0.2875567908732709, 0.7876776574352392), float2(-0.3235695699691864, -0.7836151691933712),
            float2(0.7762165924462436, 0.3631291803355136), float2(-0.8263007976064866, 0.2592816844184794),
            float2(0.4386452756167397, -0.7571098481588484), float2(0.18988542402304126, 0.8632459242554175),
            float2(-0.7303253445407815, -0.5133224046555819), float2(0.8939004035324556, -0.11593993515830946),
            float2(-0.5863762307291154, 0.6959079795748251), float2(-0.03805753378232556, -0.9177699189461416),
            float2(0.653979655650389, 0.657027860897389), float2(-0.9344208130797295, -0.04310155546401203),
            float2(0.7245109901504777, -0.6047386420191574), float2(-0.12683493131695708, 0.9434844461875473),
            float2(-0.5484582700240663, -0.7880790100251422), float2(0.9446610338564589, 0.2124041692463835),
            float2(-0.8470120123194587, 0.48548496473788055), float2(0.29904134279525085, -0.9377229203230629),
            float2(0.41623562331748715, 0.9006236205438447),
};
static const float3 ssaoKernel[64] =
{
    float3(0.07754522f, 0.05039949f, 0.03803461f),
    float3(0.04775108f, -0.05825478f, 0.06610756f),
    float3(-0.02925943f, -0.07803568f, 0.05684076f),
    float3(-0.07045165f, 0.01677723f, 0.07179490f),
    float3(-0.06949495f, -0.04720068f, 0.06048166f),
    float3(-0.07828869f, -0.00289531f, 0.07064918f),
    float3(0.06416717f, -0.08457160f, 0.01936027f),
    float3(-0.02305810f, 0.05986683f, 0.09029688f),
    float3(-0.04412014f, -0.08730699f, 0.05866137f),
    float3(0.04093422f, 0.07821991f, 0.07798954f),
    float3(-0.07127929f, -0.09456250f, 0.02923226f),
    float3(0.07791481f, 0.09330459f, 0.03532396f),
    float3(0.09277309f, -0.09338456f, 0.00131541f),
    float3(-0.08841953f, 0.07795525f, 0.07007597f),
    float3(0.02717574f, 0.07448203f, 0.11908780f),
    float3(0.04960655f, -0.13558113f, 0.03858505f),
    float3(-0.05548238f, 0.02314721f, 0.14422196f),
    float3(0.04785667f, 0.10393176f, 0.11679254f),
    float3(-0.11449096f, 0.08113398f, 0.09805914f),
    float3(0.13854586f, -0.00290691f, 0.11381001f),
    float3(0.09738749f, -0.10379773f, 0.12265641f),
    float3(-0.03890277f, 0.07910890f, 0.17606174f),
    float3(0.17299977f, 0.10600706f, 0.03758905f),
    float3(-0.14304960f, -0.12379817f, 0.10473086f),
    float3(-0.13687483f, -0.14586107f, 0.10639734f),
    float3(-0.05502319f, -0.06696088f, 0.22093843f),
    float3(-0.20463680f, 0.00746728f, 0.14084653f),
    float3(-0.20703392f, -0.15640610f, 0.01918511f),
    float3(0.03610169f, 0.23705160f, 0.12896425f),
    float3(0.21051489f, 0.18198550f, 0.06058156f),
    float3(0.17645208f, 0.01122058f, 0.23957494f),
    float3(-0.09147923f, 0.12079933f, 0.27176808f),
    float3(0.25443184f, -0.15336360f, 0.13179167f),
    float3(-0.16229259f, 0.18566449f, 0.23302841f),
    float3(0.19577273f, 0.02168918f, 0.29414517f),
    float3(0.00236975f, -0.34835414f, 0.12217447f),
    float3(-0.26931661f, -0.10391234f, 0.25439217f),
    float3(0.33241036f, 0.22039873f, 0.03966020f),
    float3(-0.13406925f, -0.16877316f, 0.35730653f),
    float3(-0.39632578f, 0.12757136f, 0.12322593f),
    float3(0.28470560f, 0.03770726f, 0.34846747f),
    float3(0.29431923f, -0.25757285f, 0.25948325f),
    float3(-0.45734672f, -0.08834208f, 0.14415660f),
    float3(0.26403564f, -0.38421382f, 0.19743026f),
    float3(-0.07760755f, 0.42733943f, 0.29562373f),
    float3(0.50096531f, 0.06715895f, 0.20369067f),
    float3(-0.33435008f, -0.45342114f, 0.04216734f),
    float3(0.15359790f, 0.49464835f, 0.27275580f),
    float3(-0.20837073f, 0.56446341f, 0.07417385f),
    float3(-0.30282398f, 0.53278172f, 0.13519317f),
    float3(0.41008556f, -0.49178995f, 0.10763030f),
    float3(-0.40189149f, -0.18955964f, 0.50346244f),
    float3(0.24802191f, 0.01938632f, 0.64802817f),
    float3(-0.51299588f, 0.40951215f, 0.28901205f),
    float3(0.26559496f, -0.42039554f, 0.54899633f),
    float3(-0.11696100f, -0.55000134f, 0.51821156f),
    float3(-0.50841625f, -0.44507908f, 0.40747657f),
    float3(0.52807113f, 0.14435644f, 0.60226377f),
    float3(0.43009279f, 0.40729271f, 0.59440946f),
    float3(-0.37059568f, -0.37701419f, 0.68448235f),
    float3(0.07934389f, -0.75609217f, 0.46469132f),
    float3(-0.22372532f, 0.88925894f, 0.03405800f),
    float3(-0.30032054f, -0.36361699f, 0.81848276f),
    float3(-0.96923048f, -0.07104343f, 0.02265543f)
};
