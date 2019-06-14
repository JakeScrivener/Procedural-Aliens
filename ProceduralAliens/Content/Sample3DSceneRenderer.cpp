#include "pch.h"
#include "Sample3DSceneRenderer.h"

#include "..\Common\DirectXHelper.h"

using namespace ProceduralAliens;

using namespace DirectX;
using namespace Windows::Foundation;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_degreesPerSecond(45),
	m_indexCount(0),
	m_tracking(false),
	m_deviceResources(deviceResources)
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
	);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(
		&m_constantBufferData.projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
	);


	mCameraCB.backgroundColour = mBackgroundColour;
	mCameraCB.eyePos = mEyePosition;
	mCameraCB.lookAt = mLookAt;
	mCameraCB.upDir = mUp;
	mCameraCB.farPlane = mFarPlane;
	mCameraCB.nearPlane = mNearPlane;

	mLightCB.lightColour = mLightColour;
	mLightCB.lightPos = mLightPosition;

	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixIdentity());
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
	if (!m_tracking)
	{
		// Convert degrees to radians, then convert seconds to rotation angle
		float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
		double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
		float radians = static_cast<float>(fmod(totalRotation, XM_2PI));
		//Rotate(0);
		//XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixScaling(100, 10, 100) * XMMatrixTranslation(0, -10, 0)));
		XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixIdentity());
	}

	XMVECTORF32 eye = { mEyePosition.x, mEyePosition.y, mEyePosition.z, mEyePosition.w };
	XMVECTORF32 at = { mLookAt.x, mLookAt.y, mLookAt.z, mLookAt.w };
	XMVECTORF32 up = { mUp.x, mUp.y, mUp.z, mUp.w };

	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up)));

	mTimeCB.time = timer.GetTotalSeconds();
}

// Rotate the 3D cube model a set amount of radians.
void Sample3DSceneRenderer::Rotate(float radians)
{
	// Prepare to pass the updated model matrix to the shader
	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixRotationY(radians)));
}

void Sample3DSceneRenderer::DrawTerrain()
{
	auto context = m_deviceResources->GetD3DDeviceContext();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);

	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		m_vertexBuffer.GetAddressOf(),
		&stride,
		&offset
	);

	context->IASetIndexBuffer(
		m_indexBuffer.Get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
	);

	// Attach our vertex shader.
	context->VSSetShader(
		m_TerrainVS.Get(),
		nullptr,
		0
	);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	context->HSSetShader(
		m_TerrainHS.Get(),
		nullptr,
		0
	);

	context->DSSetShader(
		m_TerrainDS.Get(),
		nullptr,
		0
	);
	context->DSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	// Attach our pixel shader.
	context->PSSetShader(
		m_TerrainPS.Get(),
		nullptr,
		0
	);
	// Send the constant buffer to the graphics device.
	context->PSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);
	// Send the constant buffer to the graphics device.
	context->PSSetConstantBuffers1(
		1,
		1,
		m_constantBufferLight.GetAddressOf(),
		nullptr,
		nullptr
	);
	// Send the constant buffer to the graphics device.
	context->PSSetConstantBuffers1(
		2,
		1,
		m_constantBufferCamera.GetAddressOf(),
		nullptr,
		nullptr
	);

	// Draw the objects.
	context->DrawIndexed(
		m_indexCount,
		0,
		0
	);

	context->HSSetShader(
		nullptr,
		nullptr,
		0
	);

	context->DSSetShader(
		nullptr,
		nullptr,
		0
	);
}

void Sample3DSceneRenderer::DrawSpheres()
{
	auto context = m_deviceResources->GetD3DDeviceContext();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		m_vertexBuffer.GetAddressOf(),
		&stride,
		&offset
	);

	context->IASetIndexBuffer(
		m_indexBuffer.Get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
	);

	// Attach our vertex shader.
	context->VSSetShader(
		m_ShinySpheresVS.Get(),
		nullptr,
		0
	);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	// Attach our pixel shader.
	context->PSSetShader(
		m_ShinySpheresPS.Get(),
		nullptr,
		0
	);

	// Send the constant buffer to the graphics device.
	context->PSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);
	// Send the constant buffer to the graphics device.
	context->PSSetConstantBuffers1(
		1,
		1,
		m_constantBufferLight.GetAddressOf(),
		nullptr,
		nullptr
	);
	// Send the constant buffer to the graphics device.
	context->PSSetConstantBuffers1(
		2,
		1,
		m_constantBufferCamera.GetAddressOf(),
		nullptr,
		nullptr
	);


	// Draw the objects.
	context->DrawIndexed(
		m_indexCount,
		0,
		0
	);
}

void Sample3DSceneRenderer::DrawInfiniteShapes()
{
	auto context = m_deviceResources->GetD3DDeviceContext();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		m_vertexBuffer.GetAddressOf(),
		&stride,
		&offset
	);

	context->IASetIndexBuffer(
		m_indexBuffer.Get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
	);

	// Attach our vertex shader.
	context->VSSetShader(
		m_InfiniteShapesVS.Get(),
		nullptr,
		0
	);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	// Attach our pixel shader.
	context->PSSetShader(
		m_InfiniteShapesPS.Get(),
		nullptr,
		0
	);

	context->PSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	// Send the constant buffer to the graphics device.
	context->PSSetConstantBuffers1(
		1,
		1,
		m_constantBufferLight.GetAddressOf(),
		nullptr,
		nullptr
	);
	// Send the constant buffer to the graphics device.
	context->PSSetConstantBuffers1(
		2,
		1,
		m_constantBufferCamera.GetAddressOf(),
		nullptr,
		nullptr
	);
	// Send the constant buffer to the graphics device.
	context->PSSetConstantBuffers1(
		3,
		1,
		m_constantBufferTime.GetAddressOf(),
		nullptr,
		nullptr
	);


	// Draw the objects.
	context->DrawIndexed(
		m_indexCount,
		0,
		0
	);

}

void Sample3DSceneRenderer::DrawPrimitives()
{
	auto context = m_deviceResources->GetD3DDeviceContext();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		m_vertexBuffer.GetAddressOf(),
		&stride,
		&offset
	);

	context->IASetIndexBuffer(
		m_indexBuffer.Get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
	);

	// Attach our vertex shader.
	context->VSSetShader(
		m_primitivesVS.Get(),
		nullptr,
		0
	);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	// Attach our pixel shader.
	context->PSSetShader(
		m_primitivesPS.Get(),
		nullptr,
		0
	);

	context->PSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	// Send the constant buffer to the graphics device.
	context->PSSetConstantBuffers1(
		1,
		1,
		m_constantBufferLight.GetAddressOf(),
		nullptr,
		nullptr
	);
	// Send the constant buffer to the graphics device.
	context->PSSetConstantBuffers1(
		2,
		1,
		m_constantBufferCamera.GetAddressOf(),
		nullptr,
		nullptr
	);
	// Send the constant buffer to the graphics device.
	context->PSSetConstantBuffers1(
		3,
		1,
		m_constantBufferTime.GetAddressOf(),
		nullptr,
		nullptr
	);


	// Draw the objects.
	context->DrawIndexed(
		m_indexCount,
		0,
		0
	);

}

void Sample3DSceneRenderer::DrawFractal()
{
	auto context = m_deviceResources->GetD3DDeviceContext();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		m_vertexBuffer.GetAddressOf(),
		&stride,
		&offset
	);

	context->IASetIndexBuffer(
		m_indexBuffer.Get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
	);

	// Attach our vertex shader.
	context->VSSetShader(
		m_FractalVS.Get(),
		nullptr,
		0
	);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	// Attach our pixel shader.
	context->PSSetShader(
		m_FractalPS.Get(),
		nullptr,
		0
	);

	context->PSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	// Send the constant buffer to the graphics device.
	context->PSSetConstantBuffers1(
		1,
		1,
		m_constantBufferLight.GetAddressOf(),
		nullptr,
		nullptr
	);
	// Send the constant buffer to the graphics device.
	context->PSSetConstantBuffers1(
		2,
		1,
		m_constantBufferCamera.GetAddressOf(),
		nullptr,
		nullptr
	);
	// Send the constant buffer to the graphics device.
	context->PSSetConstantBuffers1(
		3,
		1,
		m_constantBufferTime.GetAddressOf(),
		nullptr,
		nullptr
	);


	// Draw the objects.
	context->DrawIndexed(
		m_indexCount,
		0,
		0
	);

}

void Sample3DSceneRenderer::DrawPlants()
{
	auto context = m_deviceResources->GetD3DDeviceContext();

	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	context->OMSetDepthStencilState(m_particleStencilState.Get(), 0);

	context->IASetVertexBuffers(
		0,
		1,
		m_plantBuffer.GetAddressOf(),
		&stride,
		&offset
	);

	context->IASetIndexBuffer(
		m_plantIndexBuffer.Get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
	);

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	const auto blendSample = 0xffffffff;
	context->OMSetBlendState(m_AlphaBlend.Get(), blendFactor, blendSample);

	// Attach our vertex shader.
	context->VSSetShader(
		m_QuadPlantsVS.Get(),
		nullptr,
		0
	);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	context->GSSetShader(
		m_QuadPlantsGS.Get(),
		nullptr,
		0
	);
	// Send the constant buffer to the graphics device.
	context->GSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);
	context->GSSetConstantBuffers1(
		1,
		1,
		m_constantBufferCamera.GetAddressOf(),
		nullptr,
		nullptr
	);
	context->GSSetConstantBuffers1(
		2,
		1,
		m_constantBufferTime.GetAddressOf(),
		nullptr,
		nullptr
	);

	// Attach our pixel shader.
	context->PSSetShader(
		m_QuadPlantsPS.Get(),
		nullptr,
		0
	);
	context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

	//set texture
	context->PSSetShaderResources(
		0,
		1,
		m_plantTexture.GetAddressOf()
	);

	// Draw the objects.
	context->DrawIndexed(
		m_plantIndexCount,
		0,
		0
	);

	context->GSSetShader(
		nullptr,
		nullptr,
		0
	);

	context->OMSetBlendState(nullptr, nullptr, blendSample);
	context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
}

void Sample3DSceneRenderer::DrawSnake()
{
	auto context = m_deviceResources->GetD3DDeviceContext();

	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

	context->IASetVertexBuffers(
		0,
		1,
		m_snakeBuffer.GetAddressOf(),
		&stride,
		&offset
	);

	context->IASetIndexBuffer(
		m_snakeIndexBuffer.Get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
	);


	// Attach our vertex shader.
	context->VSSetShader(
		m_SnakeVS.Get(),
		nullptr,
		0
	);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	context->GSSetShader(
		m_SnakeGS.Get(),
		nullptr,
		0
	);
	// Send the constant buffer to the graphics device.
	context->GSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);
	context->GSSetConstantBuffers1(
		1,
		1,
		m_constantBufferCamera.GetAddressOf(),
		nullptr,
		nullptr
	);
	context->GSSetConstantBuffers1(
		2,
		1,
		m_constantBufferTime.GetAddressOf(),
		nullptr,
		nullptr
	);

	// Attach our pixel shader.
	context->PSSetShader(
		m_SnakePS.Get(),
		nullptr,
		0
	);
	context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

	//set texture
	context->PSSetShaderResources(
		0,
		1,
		m_snakeTexture.GetAddressOf()
	);

	// Draw the objects.
	context->DrawIndexed(
		m_snakeIndexCount,
		0,
		0
	);

	context->IASetVertexBuffers(
		0,
		1,
		m_snakeBuffer2.GetAddressOf(),
		&stride,
		&offset
	);

	context->IASetIndexBuffer(
		m_snakeIndexBuffer2.Get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
	);

	context->DrawIndexed(
		m_snakeIndexCount2,
		0,
		0
	);


	context->GSSetShader(
		nullptr,
		nullptr,
		0
	);
}

void Sample3DSceneRenderer::StartTracking()
{
	m_tracking = true;
}

// When tracking, the 3D cube can be rotated around its Y axis by tracking pointer position relative to the output screen width.
void Sample3DSceneRenderer::TrackingUpdate(float positionX)
{
	if (m_tracking)
	{
		float radians = XM_2PI * 2.0f * positionX / m_deviceResources->GetOutputSize().Width;
		Rotate(radians);
	}
}

void Sample3DSceneRenderer::StopTracking()
{
	m_tracking = false;
}

void Sample3DSceneRenderer::MoveEye(const DirectX::XMFLOAT4 & pTranslate)
{
	XMStoreFloat4(&mEyePosition, XMLoadFloat4(&mEyePosition) + XMLoadFloat4(&pTranslate));
	XMStoreFloat4(&mLookAt, XMLoadFloat4(&mLookAt) + XMLoadFloat4(&pTranslate));
	//XMStoreFloat4(&mUp, XMLoadFloat4(&mUp) + XMLoadFloat4(&pTranslate));
	mCameraCB.eyePos = mEyePosition;
	mCameraCB.lookAt = mLookAt;
	//mCameraCB.upDir = mUp;
}

// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render()
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(
		m_constantBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData,
		0,
		0,
		0
	);
	context->UpdateSubresource1(
		m_constantBufferCamera.Get(),
		0,
		NULL,
		&mCameraCB,
		0,
		0,
		0
	);
	context->UpdateSubresource1(
		m_constantBufferLight.Get(),
		0,
		NULL,
		&mLightCB,
		0,
		0,
		0
	);
	context->UpdateSubresource1(
		m_constantBufferTime.Get(),
		0,
		NULL,
		&mTimeCB,
		0,
		0,
		0
	);

	context->RSSetState(m_solidRasterizerState.Get());
	context->IASetInputLayout(m_inputLayout.Get());
	context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);

	DrawInfiniteShapes();
	DrawPrimitives();
    DrawFractal();
	DrawSpheres();

	DrawSnake();
	DrawTerrain();
	DrawPlants();
}

void Sample3DSceneRenderer::CreateDeviceDependentResources()
{
	// Load shaders asynchronously.
	auto loadTerrainVS = DX::ReadDataAsync(L"TerrainVS.cso");
	auto loadTerrainHS = DX::ReadDataAsync(L"TerrainHS.cso");
	auto loadTerrainDS = DX::ReadDataAsync(L"TerrainDS.cso");
	auto loadTerrainPS = DX::ReadDataAsync(L"TerrainPS.cso");

	auto loadShinySpheresVS = DX::ReadDataAsync(L"ShinySpheresVS.cso");
	auto loadShinySpheresPS = DX::ReadDataAsync(L"ShinySpheresPS.cso");

	auto loadInfiniteShapesVS = DX::ReadDataAsync(L"InfiniteShapesVS.cso");
	auto loadInfiniteShapesPS = DX::ReadDataAsync(L"InfiniteShapesPS.cso");

	auto loadPrimitivesVS = DX::ReadDataAsync(L"primitivesVS.cso");
	auto loadPrimitivesPS = DX::ReadDataAsync(L"primitivesPS.cso");

	auto loadPlantsVS = DX::ReadDataAsync(L"PlantsVS.cso");
	auto loadPlantsGS = DX::ReadDataAsync(L"PlantsGS.cso");
	auto loadPlantsPS = DX::ReadDataAsync(L"PlantsPS.cso");

	auto loadSnakeVS = DX::ReadDataAsync(L"SnakeVS.cso");
	auto loadSnakeGS = DX::ReadDataAsync(L"SnakeGS.cso");
	auto loadSnakePS = DX::ReadDataAsync(L"SnakePS.cso");

	auto loadFractalVS = DX::ReadDataAsync(L"FractalVS.cso");
	auto loadFractalPS = DX::ReadDataAsync(L"FractalPS.cso");

	auto createTerrainVS = loadTerrainVS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_TerrainVS
			)
		);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_inputLayout
			)
		);
	});
	auto createTerrainHS = loadTerrainHS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateHullShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_TerrainHS
			)
		);
	});
	auto createTerrainDS = loadTerrainDS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateDomainShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_TerrainDS
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
			)
		);
	});
	auto createTerrainPS = loadTerrainPS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_TerrainPS
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
			)
		);
		CD3D11_BUFFER_DESC constantBufferDesc2(sizeof(LightConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc2,
				nullptr,
				&m_constantBufferLight
			)
		);
		CD3D11_BUFFER_DESC constantBufferDesc3(sizeof(CameraConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc3,
				nullptr,
				&m_constantBufferCamera
			)
		);
	});

	auto createShinySpheresVS = loadShinySpheresVS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_ShinySpheresVS
			)
		);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_inputLayout
			)
		);
	});
	auto createShinySpheresPS = loadShinySpheresPS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_ShinySpheresPS
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
			)
		);
		CD3D11_BUFFER_DESC constantBufferDesc2(sizeof(LightConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc2,
				nullptr,
				&m_constantBufferLight
			)
		);
		CD3D11_BUFFER_DESC constantBufferDesc3(sizeof(CameraConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc3,
				nullptr,
				&m_constantBufferCamera
			)
		);
	});

	auto createInfiniteShapesVS = loadInfiniteShapesVS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_InfiniteShapesVS
			)
		);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_inputLayout
			)
		);
	});
	auto createInfiniteShapesPS = loadInfiniteShapesPS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_InfiniteShapesPS
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
			)
		);
		CD3D11_BUFFER_DESC constantBufferDesc2(sizeof(LightConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc2,
				nullptr,
				&m_constantBufferLight
			)
		);
		CD3D11_BUFFER_DESC constantBufferDesc3(sizeof(CameraConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc3,
				nullptr,
				&m_constantBufferCamera
			)
		);
	});

	auto createPrimitivesVS = loadPrimitivesVS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_primitivesVS
			)
		);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_inputLayout
			)
		);
	});
	auto createPrimitivesShapesPS = loadPrimitivesPS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_primitivesPS
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
			)
		);
		CD3D11_BUFFER_DESC constantBufferDesc2(sizeof(LightConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc2,
				nullptr,
				&m_constantBufferLight
			)
		);
		CD3D11_BUFFER_DESC constantBufferDesc3(sizeof(CameraConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc3,
				nullptr,
				&m_constantBufferCamera
			)
		);
	});

	auto createPlantsVS = loadPlantsVS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_QuadPlantsVS
			)
		);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_inputLayout
			)
		);
	});
	auto createPlantsGS = loadPlantsGS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateGeometryShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_QuadPlantsGS
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
			)
		);
		CD3D11_BUFFER_DESC constantBufferDesc2(sizeof(CameraConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc2,
				nullptr,
				&m_constantBufferCamera
			)
		);
	});
	auto createPlantsPS = loadPlantsPS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_QuadPlantsPS
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
			)
		);
		CD3D11_BUFFER_DESC constantBufferDesc1(sizeof(TimeConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc1,
				nullptr,
				&m_constantBufferTime
			)
		);
	});

	auto createSnakeVS = loadSnakeVS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_SnakeVS
			)
		);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_inputLayout
			)
		);
	});
	auto createSnakeGS = loadSnakeGS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateGeometryShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_SnakeGS
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
			)
		);
		CD3D11_BUFFER_DESC constantBufferDesc2(sizeof(CameraConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc2,
				nullptr,
				&m_constantBufferCamera
			)
		);
	});
	auto createSnakePS = loadSnakePS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_SnakePS
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
			)
		);
		CD3D11_BUFFER_DESC constantBufferDesc1(sizeof(LightConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBufferLight
			)
		);
		CD3D11_BUFFER_DESC constantBufferDesc2(sizeof(CameraConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBufferCamera
			)
		);
		CD3D11_BUFFER_DESC constantBufferDesc3(sizeof(TimeConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc1,
				nullptr,
				&m_constantBufferTime
			)
		);
	});

	auto createFractalVS = loadFractalVS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_FractalVS
			)
		);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_inputLayout
			)
		);
	});
	auto createFractalPS = loadFractalPS.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_FractalPS
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
			)
		);
		CD3D11_BUFFER_DESC constantBufferDesc2(sizeof(LightConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc2,
				nullptr,
				&m_constantBufferLight
			)
		);
		CD3D11_BUFFER_DESC constantBufferDesc3(sizeof(CameraConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc3,
				nullptr,
				&m_constantBufferCamera
			)
		);
	});

	// Once both shaders are loaded, create the mesh.
	auto createCubeTask = (createTerrainVS && createTerrainPS).then([this]() {

		// Load mesh vertices. Each vertex has a position and a color.
		static const VertexPositionColor cubeVertices[] =
		{
			{XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f)},
			{XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f)},
			{XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f)},
			{XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f)},
			{XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f)},
			{XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f)},
			{XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f)},
			{XMFLOAT3(0.5f,  0.5f,  0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
		};

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = cubeVertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(cubeVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_vertexBuffer
			)
		);

		// Load mesh indices. Each trio of indices represents
		// a triangle to be rendered on the screen.
		// For example: 0,2,1 means that the vertices with indexes
		// 0, 2 and 1 from the vertex buffer compose the 
		// first triangle of this mesh.
		static const unsigned short cubeIndices[] =
		{
			0,2,1, // -x
			1,2,3,

			4,5,6, // +x
			5,7,6,

			0,1,5, // -y
			0,5,4,

			2,6,7, // +y
			2,7,3,

			0,4,6, // -z
			0,6,2,

			1,3,7, // +z
			1,7,5,
		};

		m_indexCount = ARRAYSIZE(cubeIndices);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = cubeIndices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(cubeIndices), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&m_indexBuffer
			)
		);
	});

	auto createPlantTask = (createPlantsVS && createPlantsGS && createPlantsPS).then([this]() {

		srand(time(NULL));
		std::vector<VertexPositionColor> plantVertices;
		for (int i = -50; i < 50; ++i)
		{
			for (int j = -50; j < 50; ++j)
			{
				if ((rand() % 10 + 1) > 7)
				{
					plantVertices.emplace_back(VertexPositionColor{ XMFLOAT3(i, 0, j), XMFLOAT3(1.0f, 1.0f, 1.0f) });
				}
			}
		}

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = &(plantVertices[0]);
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(plantVertices.size() * sizeof(VertexPositionColor), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_plantBuffer
			)
		);

		std::vector<unsigned short> plantIndices;
		for (int i = 0; i < plantVertices.size(); ++i)
		{
			plantIndices.emplace_back(i);
		}

		m_plantIndexCount = plantIndices.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };

		indexBufferData.pSysMem = &(plantIndices[0]);
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(plantIndices.size() * sizeof(unsigned short), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&m_plantIndexBuffer
			)
		);
	});

	//load texture
	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(
			m_deviceResources->GetD3DDevice(),
			L"Assets/plants.dds",
			nullptr,
			m_plantTexture.GetAddressOf()
		)
	);

	auto createSnakeTask = (createSnakeVS && createSnakeGS && createSnakePS).then([this]() {

		srand(time(NULL));
		std::vector<VertexPositionColor> snakeVertices;
		for (int i = 0; i < 15; ++i)
		{
			snakeVertices.emplace_back(VertexPositionColor{ XMFLOAT3(-5, 0, i+25), XMFLOAT3(1.0f, 1.0f, 1.0f) });
		}

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = &(snakeVertices[0]);
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(snakeVertices.size() * sizeof(VertexPositionColor), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_snakeBuffer
			)
		);

		std::vector<unsigned short> snakeIndices;
		for (int i = 0; i < snakeVertices.size(); ++i)
		{
			snakeIndices.emplace_back(i);
		}

		m_snakeIndexCount = snakeIndices.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };

		indexBufferData.pSysMem = &(snakeIndices[0]);
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(snakeIndices.size() * sizeof(unsigned short), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&m_snakeIndexBuffer
			)
		);
	});

	auto createSnakeTask2 = (createSnakeVS && createSnakeGS && createSnakePS).then([this]() {

		srand(time(NULL));
		std::vector<VertexPositionColor> snakeVertices2;
		for (int i = 0; i < 15; ++i)
		{
			snakeVertices2.emplace_back(VertexPositionColor{ XMFLOAT3(10, 0, i+10), XMFLOAT3(1.0f, 1.0f, 1.0f) });
		}

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = &(snakeVertices2[0]);
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(snakeVertices2.size() * sizeof(VertexPositionColor), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_snakeBuffer2
			)
		);

		std::vector<unsigned short> snakeIndices2;
		for (int i = 0; i < snakeVertices2.size(); ++i)
		{
			snakeIndices2.emplace_back(i);
		}

		m_snakeIndexCount2 = snakeIndices2.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };

		indexBufferData.pSysMem = &(snakeIndices2[0]);
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(snakeIndices2.size() * sizeof(unsigned short), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&m_snakeIndexBuffer2
			)
		);
	});

	//load texture
	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(
			m_deviceResources->GetD3DDevice(),
			L"Assets/scales.dds",
			nullptr,
			m_snakeTexture.GetAddressOf()
		)
	);

	D3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC(D3D11_DEFAULT);
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rasterizerDesc,
		m_wireframeRasterizerState.GetAddressOf());

	D3D11_RASTERIZER_DESC rasterizerDesc2 = CD3D11_RASTERIZER_DESC(D3D11_DEFAULT);
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rasterizerDesc,
		m_solidRasterizerState.GetAddressOf());

	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(D3D11_BLEND_DESC));
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;

	m_deviceResources->GetD3DDevice()->CreateBlendState(&blendDesc, m_AlphaBlend.GetAddressOf());

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	m_deviceResources->GetD3DDevice()->CreateDepthStencilState(&depthStencilDesc, m_particleStencilState.GetAddressOf());

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc2;
	ZeroMemory(&depthStencilDesc2, sizeof(D3D11_DEPTH_STENCIL_DESC));
	depthStencilDesc2.DepthEnable = TRUE;
	depthStencilDesc2.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc2.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	m_deviceResources->GetD3DDevice()->CreateDepthStencilState(&depthStencilDesc2, m_depthStencilState.GetAddressOf());

	//Set the sampler
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MinLOD = -FLT_MAX;
	samplerDesc.MaxLOD = FLT_MAX;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, m_sampler.GetAddressOf());

	// Once the cube is loaded, the object is ready to be rendered.
	auto complete = (createCubeTask && createPlantTask).then([this]() {
		m_loadingComplete = true;
	});

}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_TerrainVS.Reset();
	m_TerrainHS.Reset();
	m_TerrainDS.Reset();
	m_TerrainPS.Reset();
	m_InfiniteShapesVS.Reset();
	m_InfiniteShapesPS.Reset();
	m_inputLayout.Reset();
	m_constantBuffer.Reset();
	m_constantBufferCamera.Reset();
	m_constantBufferLight.Reset();
	m_constantBufferTime.Reset();
	m_vertexBuffer.Reset();
	m_indexBuffer.Reset();
	m_AlphaBlend.Reset();
	m_depthStencilState.Reset();
	m_particleStencilState.Reset();
	m_plantBuffer.Reset();
	m_plantIndexBuffer.Reset();
	m_plantTexture.Reset();
	m_sampler.Reset();
	m_QuadPlantsGS.Reset();
	m_QuadPlantsPS.Reset();
	m_QuadPlantsVS.Reset();
	m_ShinySpheresPS.Reset();
	m_ShinySpheresVS.Reset();
	m_solidRasterizerState.Reset();
	m_wireframeRasterizerState.Reset();
}