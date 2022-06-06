#pragma once
#include <d3d11.h>

namespace TTM {
	bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height, ID3D11Device* g_pd3dDevice);
}