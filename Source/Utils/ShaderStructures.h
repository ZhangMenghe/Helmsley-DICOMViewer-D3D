#pragma once

	// Constant buffer used to send MVP matrices to the vertex shader.
	struct allConstantBuffer
	{
		DirectX::XMFLOAT4X4 model;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
		DirectX::XMFLOAT4 uCamPosInObjSpace;
	};
	struct ModelViewProjectionConstantBuffer
	{
		DirectX::XMFLOAT4X4 model;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
	};

	// Used to send per-vertex data to the vertex shader.
	struct VertexPositionColor
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 color;
	};
	// Used to send per-vertex data to the vertex shader.
	struct VertexPosTex2d
	{
		DirectX::XMFLOAT2 pos;
		DirectX::XMFLOAT2 tex;
	};
