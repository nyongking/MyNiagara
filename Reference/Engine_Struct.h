#ifndef Engine_Struct_h__
#define Engine_Struct_h__
#include "Engine_Typedef.h"

namespace Engine
{
	class CActorObject;
	class CActor;

#pragma region Shader ConstantBuffer Struct
	typedef struct tagRandomTexcoord
	{
		XMFLOAT2 vRandomTexcoord[64];
	}RANDOM_TEXCOORD;
	typedef struct tagDofConstData
	{
		float			fSensorHeight = 24.0f;
		float			fAperture = 2.8f; // 조리개 크기
		float			fFocusDistance = 50.0f; // 초점 평면거리
		float			fFocalLength = 0; // Fovy와 FocusDistance를 기반으로 구해지는 값.

		float			fDofBrightness = 1.5f; // Fovy와 FocusDistance를 기반으로 구해지는 값.
		float			fBaseBlurPower = 0.1f;
		float			fFadeRatio = 1.0f;
		int				isWhiteFade = 0;

		XMFLOAT3		vBlurColor = { 1.0f, 1.0f, 1.0f };
		float			dummy3;
	}CONST_DOF;
	typedef struct tagGlobalIBLConstData
	{
		float fStrengthIBL = 0.5f;
		int iSpecularBaseMipLevel = 2;
		float fRoughnessToMipFactor = 5.0f;
		float fHDRMaxLuminance = 1000.f;		// 16byte

		int iToneMappingFlag = 0;
		float fExposure = 1.0f;
		float fGamma = 2.2f;
		float dummy1;
	}CONST_IBL;
	typedef struct tagMaterialDefault
	{
		XMFLOAT4	Albedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f);		   // 16byte

		float		Roughness = 0.5f;					 
		float		Metallic = 0.0f;					 
		float		AO = 0.7f;
		float		Emissive = 0.0f;									// 16byte

		XMFLOAT4	MultipleAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);	// 16byte
		XMFLOAT3	EmissiveColor = XMFLOAT3(1.0f, 1.0f, 1.0f);
		float		dummy;
	}MATERIAL_PS;
	
	typedef struct tagBasicPixelConstData // 동적변화가 없는 애들위주로.
	{
		MATERIAL_PS Material; // 32Byte

		int			useAlbedoMap = 0;
		int			useNormalMap = 0;
		int			useAOMap = 0; // 12
		int			useMetallicMap = 0;		// 16Byte

		int			useRoughnessMap = 0;
		int			useEmissiveMap = 0;
		int			useORMHMap = 0;			// Oclusion, Roughness, Metalic, Height'
		int			useSpecularMap = 0;

		int			invertNormalMapY = 0;	// 16Byte  // LH, RH 에 따라 Y 축 반전이 필요한 경우가 있음.
		XMFLOAT3	dummy;
	}CONST_PS;


	typedef struct tagLightConstData
	{
		XMFLOAT3		vRadiance;		// 빛의 세기와 컬러를 결정.
		float			fFallOutStart;	// 감쇠(att) 시작 거리 : 해당 지점 이전은 반드시 1
		XMFLOAT3		vDirection;
		float			fFallOutEnd;	// 감쇠(att) 끝 거리 : 해당 지점이후는 0 >>> 이게 조명의 Range 역할도 함께 함.
		XMFLOAT3		vPosition;
		float			fSpotPower;

		// 후보정 값으로 남겨두겠음. 분위기 연출 등 
		XMFLOAT4		vDiffuse;
		XMFLOAT4		vAmbient;
		XMFLOAT4		vSpecular;

		// Shadow 
		int				isShadow; // 자기 자신이 shadow가 있는 light인지. >> 이 변수가 true라면, shadow map을 텍스쳐링하여 shadowfactor를 계산한다.
		float			fLightRadius; // 조명의 두께(반지름 pcss)
		float			fShadowFactor = 1.0f;
		float			dummy1;

		// Matrix
		XMMATRIX		LightViewProjMatrix; // 자기 자신의 view, proj 매트릭스.

	}CONST_LIGHT;

#pragma endregion // Shader ConstantBuffer Struct

	typedef struct tagActorUserData
	{
		CActorObject*				pOwner = nullptr;
		_uint						iObjectGroup = 0;
		_bool						isInToScene = false;
	}ACTOR_USERDATA;

	typedef struct tagShapeUserData
	{
		_uint						iShapeInstanceID = 0;
		_uint						iShapeUse = 0;
		_uint						iShapeIndex = 0;
	}SHAPE_USERDATA;

	typedef struct tagCollisionInfo
	{
		ACTOR_USERDATA* pActorUserData;
		SHAPE_USERDATA* pShapeUserData;
	}COLL_INFO;

	typedef struct tagShapeDesc
	{

	}SHAPE_DESC;

	typedef struct tagShapeSphereDesc : public SHAPE_DESC
	{
		// 구의 반지름
		float fRadius = 0.5f;
	}SHAPE_SPHERE_DESC;

	typedef struct tagShapeBoxDesc : public SHAPE_DESC
	{
		// 박스의 x,y,z 각 축 방향의 절반 크기.
		XMFLOAT3 vHalfExtents = { 0.5f, 0.5f, 0.5f };
	}SHAPE_BOX_DESC;

	typedef struct tagShapeCapsuleDesc : public SHAPE_DESC
	{
		// 구 부분의 반지름.
		float fRadius = 0.5f;
		// 원기둥 부분의 높이의 절반.
		float fHalfHeight = 0.5f;
	}SHAPE_CAPSULE_DESC;

	typedef struct tagCookingDesc : public SHAPE_DESC
	{
		_uint	iShapeIndex = 0; // 쿠킹에 씁니다..
		_string strFilePath;
	}SHAPE_COOKING_DESC;

	typedef struct tagEngineDesc
	{
		HINSTANCE		hInst;
		HWND			hWnd;
		bool			isWindowed;
		unsigned int	iNumLevels;
		unsigned int	iViewportWidth;
		unsigned int	iViewportHeight;
		_uint			iStaticLevelID;
		_uint			eImportMode;
		_bool			isNewRenderer = false;
	}ENGINE_DESC;

	typedef struct
	{
		XMFLOAT4		vDiffuse;
		XMFLOAT4		vAmbient;
		XMFLOAT4		vSpecular;
	}MATERIAL_DESC;

#pragma region Vertex
	typedef struct ENGINE_DLL tagVtxPosition
	{
		XMFLOAT3		vPosition;
		static const unsigned int					iNumElements = { 1 };
		static const D3D11_INPUT_ELEMENT_DESC		Elements[iNumElements];
	}VTXPOS;

	typedef struct ENGINE_DLL tagVtxPosition_Texture
	{
		XMFLOAT3		vPosition;
		XMFLOAT2		vTexcoord;

		static const unsigned int					iNumElements = { 2 };
		static const D3D11_INPUT_ELEMENT_DESC		Elements[iNumElements];
	}VTXPOSTEX;

	typedef struct ENGINE_DLL tagVtxTrail
	{

		XMFLOAT3		vPosition;
		XMFLOAT2		vTexcoord;
		XMFLOAT2		vLifeTime;

		static const unsigned int					iNumElements = { 3 };
		static const D3D11_INPUT_ELEMENT_DESC		Elements[iNumElements];
	}VTXTRAIL;

	typedef struct ENGINE_DLL tagVtxBeam
	{
		XMFLOAT3		vPosition;
		float			fIndex;

		static const unsigned int					iNumElements = { 2 };
		static const D3D11_INPUT_ELEMENT_DESC		Elements[iNumElements];
	}VTXBEAM;

	typedef struct ENGINE_DLL tagVtxPosition_Color
	{
		XMFLOAT3		vPosition;
		XMFLOAT4		vColor;

		static const unsigned int					iNumElements = { 2 };
		static const D3D11_INPUT_ELEMENT_DESC		Elements[iNumElements];
	}VTXPOSCOLOR;

	typedef struct ENGINE_DLL tagVtxPosition_Cube
	{
		XMFLOAT3		vPosition;
		XMFLOAT3		vTexcoord;

		static const unsigned int					iNumElements = { 2 };
		static const D3D11_INPUT_ELEMENT_DESC		Elements[iNumElements];
	}VTXCUBE;

	typedef struct ENGINE_DLL tagVtxNormal_Texture
	{
		XMFLOAT3		vPosition;
		XMFLOAT3		vNormal;
		XMFLOAT2		vTexcoord;

		static const unsigned int					iNumElements = { 3 };
		static const D3D11_INPUT_ELEMENT_DESC		Elements[iNumElements];
	}VTXNORTEX;

	typedef struct ENGINE_DLL tagVtxMesh
	{
		XMFLOAT3		vPosition;
		XMFLOAT3		vNormal;
		XMFLOAT2		vTexcoord;
		XMFLOAT3		vTangent;

		static const unsigned int					iNumElements = { 4 };
		static const D3D11_INPUT_ELEMENT_DESC		Elements[iNumElements];
	}VTXMESH;

	typedef struct ENGINE_DLL tagVtxMeshID
	{
		XMFLOAT3		vPosition;
		XMFLOAT3		vNormal;
		XMFLOAT2		vTexcoord;
		XMFLOAT3		vTangent;
		unsigned int	iInstanceID;

		static const unsigned int					iNumElements = { 5 };
		static const D3D11_INPUT_ELEMENT_DESC		Elements[iNumElements];
	}VTXMESHID;

	typedef struct ENGINE_DLL tagVtxAnimMesh
	{
		XMFLOAT3		vPosition;
		XMFLOAT3		vNormal;
		XMFLOAT2		vTexcoord;
		XMFLOAT3		vTangent;
		XMUINT4			vBlendIndices; /* 메쉬클래스에 저장된, 인덱스벡터의 인덱스를 저장할 것임. (메쉬의 m_BoneIndices : 자기자신 메쉬에게 영향을 주는 본들의 Model 클래스 m_Bones 상의 index값을 저장하고있음. )*/
		XMFLOAT4		vBlendWeights; /* 정점은 최대 4개의 Bone의 영향을 받는다. 그래서 int 4개이고, float4개인 Weight는 얼만큼 영향을 받는가에 대한 0 ~ 1 사이의 실수값. x,y,z,w 를 모두 더하면 1이 나옴.*/
		static const unsigned int					iNumElements = { 6 };
		static const D3D11_INPUT_ELEMENT_DESC		Elements[iNumElements];
	}VTXANIMMESH;

	typedef struct ENGINE_DLL tagVtxPoint
	{
		_float3 vPosition;
		_float2 vSize;	

		static const unsigned int					iNumElements = { 2 };
		static const D3D11_INPUT_ELEMENT_DESC		Elements[iNumElements];
	} VTXPOINT;

	/* Sprite(Point) Particle의 인스턴싱 */
	typedef struct tagVtxPointInstance
	{
		XMFLOAT4		vRight, vUp, vLook, vTranslation;
		XMFLOAT2		vLifeTime;
		XMFLOAT4		vColor;
		XMFLOAT4		vUV;
		XMFLOAT3		vVelocity;
		XMFLOAT3		vAcceleration;
		float			fAlive;
		float			fRandom;

	}VTXPOINTINSTANCE;


	typedef struct tagVtxMeshInstance
	{
		XMFLOAT4		vRight, vUp, vLook, vTranslation;
		XMFLOAT2		vLifeTime;
		XMFLOAT4		vColor;
		XMFLOAT3		vVelocity;
		XMFLOAT3		vAcceleration;
		float			fAlive;
	}VTXMESHINSTANCE;

	typedef struct ENGINE_DLL tagPointParticle
	{
		static const unsigned int iNumElements = { 13 };		// Position + Size + VTXPOINTINSTANCE
		static const D3D11_INPUT_ELEMENT_DESC Elements[iNumElements];
	}VTXPOINTPARTICLE;

	typedef struct ENGINE_DLL tagMeshParticle
	{
		static const unsigned int iNumElements = { 13 };		// Mesh + VTXMESHINSTACNE
		static const D3D11_INPUT_ELEMENT_DESC Elements[iNumElements];
	}VTXMESHPARTICLE;


#pragma endregion // Vertex

	typedef struct tagKeyFrame
	{
		void Set_Matrix(FXMMATRIX _matWorld)
		{
			XMVECTOR vTmpScale, vTmpRotation, vTmpPosition;
			XMMatrixDecompose(&vTmpScale, &vTmpRotation, &vTmpPosition, _matWorld);
			XMStoreFloat4(&vRotation, vTmpRotation);
			XMStoreFloat3(&vPosition, vTmpPosition);
			XMStoreFloat3(&vScale, vTmpScale);
		}
		XMFLOAT4X4 Get_Matrix()
		{
			XMFLOAT4X4 matWorld;
			XMStoreFloat4x4(&matWorld, XMMatrixAffineTransformation(XMLoadFloat3(&vScale), XMVectorSet(0.f, 0.f, 0.f, 1.f), XMLoadFloat4(&vRotation), XMVectorSetW(XMLoadFloat3(&vPosition), 1.f)));
			return matWorld;
		}
		float	 fTrackPosition; // 애니메이션 트랙위에서의 포지션을 정의.
		XMFLOAT3 vScale;
		XMFLOAT4 vRotation; // 쿼터니언을 사용하기 위해서.
		XMFLOAT3 vPosition;
	}KEYFRAME;

#pragma region Camera 관련
	typedef struct tagArmDataDesc
	{
		_float				fLength = 1.f;
		_float2				fLengthTime = {};
		_uint				iLengthRatioType = {};

		_float2				fMoveTimeAxisY = {};
		_float2				fMoveTimeAxisRight = {};			// 안 쓸 듯
		_float2				fRotationPerSecAxisY = {};			// 안 쓸 듯
		_float2				fRotationPerSecAxisRight = {};		// 안 쓸 듯

		_uint				iRotationRatioType = {};
		_float3				vDesireArm = {};		// 최종 벡터
	} ARM_DATA;

	typedef struct tagCameraSubDataDesc
	{
		_float				fZoomTime = {};
		_uint				iZoomLevel = {};
		_uint				iZoomRatioType = {};

		_float				fAtOffsetTime = {};
		_float3				vAtOffset = {};
		_uint				iAtRatioType = {};
	} SUB_DATA;

	typedef struct tagCameraArmReturnDataDesc
	{
		_float3				vPreArm = {};
		_float				fPreLength = {};
		_int				iTriggerID = {};

		_bool				hasSubData = {};
		_uint				iZoomLevel = {};
		_float3				vAtOffset = {};
	} RETURN_ARMDATA;

	typedef struct tagCutSceneKeyFrameDesc
	{
		_float3				vPosition = {};
		_float				fTimeStamp = {};
		
		_uint				iZoomLevel = {};
		_uint				iZoomRatioType = {};
		
		_float3				vAt = {};
		_bool				bLookTarget = {};
		_uint				iAtRatioType = {};
	} CUTSCENE_KEYFRAME;
	
	typedef struct tagCutSceneData
	{
		_float3				vPosition = {};
		_float3				vRotation = {};

		_float3				vAt = {};
		_float				fFovy = {};
	} CUTSCENE_DATA;

	typedef struct tagCutSceneSubData
	{
		_float2				fTotalTime = {};
		_uint				iNextCameraType = {};
		_bool				isPause = { false };		// 끝나고 대기할지 말지
		_int				iDelayFrame = { -1 };		// 몇 프레임 후에 시작할지
	} CUTSCENE_SUB_DATA;

	typedef struct tagInitialData
	{
		_float3				vPosition = {};
		_float3				vAt = {};
		_float				fLength = {};
		_uint				iZoomLevel = {};

		_float				fInitialTime = {};
	} INITIAL_DATA;
#pragma endregion

#pragma region Binary 관련

	/* Binary_Manager Struct */
	typedef struct tagFbxMesh
	{
		char szName[MAX_PATH];
		unsigned int iMaterialIndex;
		unsigned int iNumVertices;
		vector<VTXANIMMESH> vecVertices;
		vector<unsigned int> vecBoneIndices;
		vector<XMFLOAT4X4> vecOffsetMatrix;
		unsigned int iNumFaces;
		unsigned int* pIndices; // 인덱스 개수만큼 동적할당한 포인터를 담아서 전달할거임. 저장은 당연히 _uint연속적으로 데이터에 넣어야겠지.
		unsigned int iNumBones;

	}FBX_MESH;
	typedef struct tagFbxMaterial
	{
		/* txt 파일을 읽으면서 수행할 로직. */
		/* 1. Material 텍스쳐 25개를 순회한다. */
		/* 2. 각 텍스쳐마다 장수를 확인한다. */
		/* 3. 0이면 vector 인덱스를 1 올리고, 0이 아니라면, 그 횟수로 반복문 수행. >>> max_path 만큼 문자열을 읽어 저장한다.*/
		/* 4. 이때 char* 타입 */
		vector<wstring> vecTextureFullPath[25];
	}FBX_MATERIAL;
	typedef struct tagFbxBone
	{
		char szName[MAX_PATH];
		_float4x4 TransformationMatrix;
		_int m_iParentBoneIndex;

	}FBX_BONE;
	typedef struct tagFbxChannel
	{
		char szName[MAX_PATH];
		unsigned int iNumKeyFrames;
		unsigned int iBoneIndex;
		vector<KEYFRAME> vecKeyFrames;
	}FBX_CHANNEL;
	typedef struct tagFbxAnimation
	{
		char szName[MAX_PATH];
		unsigned int iNumChannels;
		_float fDuration;
		_float fTickPerSec;
		vector<FBX_CHANNEL> vecChannels;

	}FBX_ANIMATION;
	typedef struct tagNonAnimFbx
	{
		/* 만약 더 최적화 한다면, 구조체를 안쓰고 바로 값넣어주기 or 벡터 다 빼고 포인터타입으로 IndexBuffer처럼 값얕복으로 전달.*/
		unsigned int iNumMeshes;
		unsigned int iNumMaterials;

		vector<FBX_MESH> vecMeshes;

		vector<FBX_MATERIAL> vecMaterials;
	}NONANIM_FBX;

	typedef struct tagAnimFbx
	{
		/* 만약 더 최적화 한다면, 구조체를 안쓰고 바로 값넣어주기 or 벡터 다 빼고 포인터타입으로 IndexBuffer처럼 값얕복으로 전달.*/
		unsigned int eModelType;
		unsigned int iNumDebugBones;
		unsigned int iNumMeshes;
		unsigned int iNumAnimations;
		unsigned int iNumMaterials;
		vector<FBX_BONE> vecBones;
		vector<FBX_MESH> vecMeshes;
		vector<FBX_ANIMATION> vecAnimations;
		vector<FBX_MATERIAL> vecMaterials;
	}ANIM_FBX;

	typedef struct tagRaycastHit
	{
		_float3 vPosition;
		_float3 vNormal;
	}RAYCASTHIT;
#pragma endregion

	








}


#endif // Engine_Struct_h__
