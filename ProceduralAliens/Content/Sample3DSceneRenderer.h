#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"
#include <vector>
#include "DDSTextureLoader.h"

namespace ProceduralAliens
{
	// This sample renderer instantiates a basic rendering pipeline.
	class Sample3DSceneRenderer
	{
	public:
		Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void ReleaseDeviceDependentResources();
		void Update(DX::StepTimer const& timer);
		void Render();
		void StartTracking();
		void TrackingUpdate(float positionX);
		void StopTracking();
		bool IsTracking() { return m_tracking; }
		void MoveEye(const DirectX::XMFLOAT4 & pTranslate);


	private:
		void Rotate(float radians);

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// Direct3D resources for cube geometry.
		Microsoft::WRL::ComPtr<ID3D11InputLayout>			m_inputLayout;

		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_indexBuffer;

		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_plantBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_plantIndexBuffer;

		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_snakeBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_snakeIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_snakeBuffer2;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_snakeIndexBuffer2;

		Microsoft::WRL::ComPtr<ID3D11VertexShader>			m_TerrainVS;
		Microsoft::WRL::ComPtr<ID3D11HullShader>			m_TerrainHS;
		Microsoft::WRL::ComPtr<ID3D11DomainShader>			m_TerrainDS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_TerrainPS;

		Microsoft::WRL::ComPtr<ID3D11VertexShader>			m_ShinySpheresVS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_ShinySpheresPS;

		Microsoft::WRL::ComPtr<ID3D11VertexShader>			m_InfiniteShapesVS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_InfiniteShapesPS;

		Microsoft::WRL::ComPtr<ID3D11VertexShader>			m_primitivesVS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_primitivesPS;

		Microsoft::WRL::ComPtr<ID3D11VertexShader>			m_FractalVS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_FractalPS;

		Microsoft::WRL::ComPtr<ID3D11VertexShader>			m_QuadPlantsVS;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader>		m_QuadPlantsGS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_QuadPlantsPS;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_plantTexture;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>			m_sampler;

		Microsoft::WRL::ComPtr<ID3D11VertexShader>			m_SnakeVS;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader>		m_SnakeGS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_SnakePS;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_snakeTexture;

		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_constantBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_constantBufferLight;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_constantBufferCamera;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_constantBufferTime;

		Microsoft::WRL::ComPtr<ID3D11RasterizerState>		m_wireframeRasterizerState;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>		m_solidRasterizerState;

		Microsoft::WRL::ComPtr<ID3D11BlendState>			m_AlphaBlend;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState>		m_particleStencilState;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState>		m_depthStencilState;

		// System resources for cube geometry.
		ModelViewProjectionConstantBuffer	m_constantBufferData;
		uint32	m_indexCount;
		uint32 m_plantIndexCount;
		uint32 m_snakeIndexCount;
		uint32 m_snakeIndexCount2;

		LightConstantBuffer mLightCB;
		DirectX::XMFLOAT4 mLightColour = DirectX::XMFLOAT4(1, 1, 1, 1);
		DirectX::XMFLOAT4 mLightPosition = DirectX::XMFLOAT4(100, 20, 0, 1);

		CameraConstantBuffer mCameraCB;
		DirectX::XMFLOAT4 mEyePosition = DirectX::XMFLOAT4(0, 0, -15, 1);
		DirectX::XMFLOAT4 mLookAt = DirectX::XMFLOAT4(0, 0, 0, 1);
		DirectX::XMFLOAT4 mUp = DirectX::XMFLOAT4(0, 1, 0, 1);
		DirectX::XMFLOAT4 mBackgroundColour = DirectX::XMFLOAT4(1, 1, 1, 1);
		float mNearPlane = 1;
		float mFarPlane = 1000;

		TimeConstantBuffer mTimeCB;

		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		float	m_degreesPerSecond;
		bool	m_tracking;

		void DrawTerrain();
		void DrawSpheres();
		void DrawInfiniteShapes();
		void DrawPrimitives();
		void DrawPlants();
		void DrawSnake();
		void DrawFractal();
	};
}

