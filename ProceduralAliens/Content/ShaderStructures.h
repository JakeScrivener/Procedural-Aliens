#pragma once

namespace ProceduralAliens
{
	// Constant buffer used to send MVP matrices to the vertex shader.
	struct ModelViewProjectionConstantBuffer
	{
		DirectX::XMFLOAT4X4 model;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
	};

	struct LightConstantBuffer
	{
		DirectX::XMFLOAT4 lightPos;
		DirectX::XMFLOAT4 lightColour;
	};

	struct CameraConstantBuffer
	{
		DirectX::XMFLOAT4 eyePos;
		DirectX::XMFLOAT4 lookAt;
		DirectX::XMFLOAT4 upDir;
		DirectX::XMFLOAT4 backgroundColour;
		float nearPlane;
		float farPlane;
		DirectX::XMFLOAT2 padding;
	};

	struct TimeConstantBuffer
	{
		float time;
		DirectX::XMFLOAT3 padding;
	};

	// Used to send per-vertex data to the vertex shader.
	struct VertexPositionColor
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 color;
	};
}