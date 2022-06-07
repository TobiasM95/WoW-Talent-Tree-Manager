#pragma once
#include <d3d11.h>

namespace TTM {
	struct TextureInfo {
		ID3D11ShaderResourceView* texture = nullptr;
		int width = 0;
		int height = 0;
	};

	bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height, ID3D11Device* g_pd3dDevice);
}