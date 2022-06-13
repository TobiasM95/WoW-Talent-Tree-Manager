/*
    WoW Talent Tree Manager is an application for creating/editing/sharing talent trees and setups.
    Copyright(C) 2022 Tobias Mielich

    This program is free software : you can redistribute it and /or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see < https://www.gnu.org/licenses/>.

    Contact via https://github.com/TobiasM95/WoW-Talent-Tree-Manager/discussions or BuffMePls#2973 on Discord
*/

#include <vector>
#include <string>
#include <filesystem>

#include "ImageHandler.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define __STDC_LIB_EXT1__
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace TTM {
    // Simple helper function to load an image into a DX11 texture with common settings
    bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height, ID3D11Device* g_pd3dDevice, Engine::TalentType talentType)
    {
        const unsigned char* talentMask = nullptr;
        int maskWidth = 0;
        int maskHeight = 0;
        switch (talentType) {
        case Engine::TalentType::ACTIVE: {
            talentMask = ACTIVE_TALENT_MASK_DATA; 
            maskWidth = ACTIVE_TALENT_MASK_SIZE_W;
            maskHeight = ACTIVE_TALENT_MASK_SIZE_H;
        } break;
        case Engine::TalentType::PASSIVE: {
            talentMask = PASSIVE_TALENT_MASK_DATA;
            maskWidth = PASSIVE_TALENT_MASK_SIZE_W;
            maskHeight = PASSIVE_TALENT_MASK_SIZE_H;
        } break;
        case Engine::TalentType::SWITCH: {
            talentMask = SWITCH_TALENT_MASK_DATA;
            maskWidth = SWITCH_TALENT_MASK_SIZE_W;
            maskHeight = SWITCH_TALENT_MASK_SIZE_H;
        } break;
        }

        // Load from disk into a raw RGBA buffer
        int image_width = 0;
        int image_height = 0;
        unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
        if (image_data == NULL)
            return false;

        if (maskWidth == image_width && maskHeight == image_height) {
            for (int i = 0; i < maskWidth * maskHeight; i++) {
                *(image_data + 4 * i + 3) = *(talentMask + 4 * i + 3);
            }
        }

        // Create texture
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = image_width;
        desc.Height = image_height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        ID3D11Texture2D* pTexture = NULL;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = image_data;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
        if (pTexture == NULL)
            return false;

        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
        pTexture->Release();

        *out_width = image_width;
        *out_height = image_height;
        stbi_image_free(image_data);

        return true;
    }

    bool LoadDefaultTexture(ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height, ID3D11Device* g_pd3dDevice, Engine::TalentType talentType)
    {
        const unsigned char* talentMask = nullptr;
        int maskWidth = 0;
        int maskHeight = 0;
        switch (talentType) {
        case Engine::TalentType::ACTIVE: {
            talentMask = ACTIVE_TALENT_MASK_DATA;
            maskWidth = ACTIVE_TALENT_MASK_SIZE_W;
            maskHeight = ACTIVE_TALENT_MASK_SIZE_H;
        } break;
        case Engine::TalentType::PASSIVE: {
            talentMask = PASSIVE_TALENT_MASK_DATA;
            maskWidth = PASSIVE_TALENT_MASK_SIZE_W;
            maskHeight = PASSIVE_TALENT_MASK_SIZE_H;
        } break;
        case Engine::TalentType::SWITCH: {
            talentMask = SWITCH_TALENT_MASK_DATA;
            maskWidth = SWITCH_TALENT_MASK_SIZE_W;
            maskHeight = SWITCH_TALENT_MASK_SIZE_H;
        } break;
        }

        // Load from disk into a raw RGBA buffer
        int image_width = DEFAULT_ICON_SIZE_W;
        int image_height = DEFAULT_ICON_SIZE_H;
        unsigned char* image_data = (unsigned char*)malloc(4 * image_width * image_height);
        if (image_data == NULL)
            return false;
        for (int i = 0; i < 4 * image_width * image_height; i++) {
            *(image_data + i) = *(DEFAULT_ICON_DATA + i);
        }

        if (maskWidth == image_width && maskHeight == image_height) {
            for (int i = 0; i < maskWidth * maskHeight; i++) {
                *(image_data + 4 * i + 3) = *(talentMask + 4 * i + 3);
            }
        }

        // Create texture
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = image_width;
        desc.Height = image_height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        ID3D11Texture2D* pTexture = NULL;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = image_data;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
        if (pTexture == NULL)
            return false;

        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
        pTexture->Release();

        *out_width = image_width;
        *out_height = image_height;
        stbi_image_free(image_data);

        return true;
    }

    bool LoadRedIconGlowTexture(ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height, ID3D11Device* g_pd3dDevice)
    {
        // Load from disk into a raw RGBA buffer
        int image_width = RED_ICON_GLOW_SIZE_W;
        int image_height = RED_ICON_GLOW_SIZE_H;
        unsigned char* image_data = (unsigned char*)malloc(4 * image_width * image_height);
        if (image_data == NULL)
            return false;
        for (int i = 0; i < 4 * image_width * image_height; i++) {
            *(image_data + i) = *(RED_ICON_GLOW_DATA + i);
        }

        // Create texture
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = image_width;
        desc.Height = image_height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        ID3D11Texture2D* pTexture = NULL;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = image_data;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
        if (pTexture == NULL)
            return false;

        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
        pTexture->Release();

        *out_width = image_width;
        *out_height = image_height;
        stbi_image_free(image_data);

        return true;
    }

    bool LoadGreenIconGlowTexture(ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height, ID3D11Device* g_pd3dDevice)
    {
        // Load from disk into a raw RGBA buffer
        int image_width = GREEN_ICON_GLOW_SIZE_W;
        int image_height = GREEN_ICON_GLOW_SIZE_H;
        unsigned char* image_data = (unsigned char*)malloc(4 * image_width * image_height);
        if (image_data == NULL)
            return false;
        for (int i = 0; i < 4 * image_width * image_height; i++) {
            *(image_data + i) = *(GREEN_ICON_GLOW_DATA + i);
        }

        // Create texture
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = image_width;
        desc.Height = image_height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        ID3D11Texture2D* pTexture = NULL;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = image_data;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
        if (pTexture == NULL)
            return false;

        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
        pTexture->Release();

        *out_width = image_width;
        *out_height = image_height;
        stbi_image_free(image_data);

        return true;
    }

    bool LoadGoldIconGlowTexture(ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height, ID3D11Device* g_pd3dDevice)
    {
        // Load from disk into a raw RGBA buffer
        int image_width = GOLD_ICON_GLOW_SIZE_W;
        int image_height = GOLD_ICON_GLOW_SIZE_H;
        unsigned char* image_data = (unsigned char*)malloc(4 * image_width * image_height);
        if (image_data == NULL)
            return false;
        for (int i = 0; i < 4 * image_width * image_height; i++) {
            *(image_data + i) = *(GOLD_ICON_GLOW_DATA + i);
        }

        // Create texture
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = image_width;
        desc.Height = image_height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        ID3D11Texture2D* pTexture = NULL;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = image_data;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
        if (pTexture == NULL)
            return false;

        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
        pTexture->Release();

        *out_width = image_width;
        *out_height = image_height;
        stbi_image_free(image_data);

        return true;
    }

    bool LoadBlueIconGlowTexture(ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height, ID3D11Device* g_pd3dDevice)
    {
        // Load from disk into a raw RGBA buffer
        int image_width = BLUE_ICON_GLOW_SIZE_W;
        int image_height = BLUE_ICON_GLOW_SIZE_H;
        unsigned char* image_data = (unsigned char*)malloc(4 * image_width * image_height);
        if (image_data == NULL)
            return false;
        for (int i = 0; i < 4 * image_width * image_height; i++) {
            *(image_data + i) = *(BLUE_ICON_GLOW_DATA + i);
        }

        // Create texture
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = image_width;
        desc.Height = image_height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        ID3D11Texture2D* pTexture = NULL;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = image_data;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
        if (pTexture == NULL)
            return false;

        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
        pTexture->Release();

        *out_width = image_width;
        *out_height = image_height;
        stbi_image_free(image_data);

        return true;
    }

    bool LoadSplitTextureFromFile(
        const char* filename1,
        const char* filename2,
        ID3D11ShaderResourceView** out_srv,
        int* out_width,
        int* out_height,
        ID3D11Device* g_pd3dDevice
    )
    {
        const unsigned char* talentMask = SWITCH_TALENT_MASK_DATA;
        int maskWidth = SWITCH_TALENT_MASK_SIZE_W;
        int maskHeight = SWITCH_TALENT_MASK_SIZE_H;

        // Load from disk into a raw RGBA buffer
        int image_width_1 = 0;
        int image_height_1 = 0;
        unsigned char* image_data_1 = stbi_load(filename1, &image_width_1, &image_height_1, NULL, 4);
        int image_width_2 = 0;
        int image_height_2 = 0;
        unsigned char* image_data_2 = stbi_load(filename2, &image_width_2, &image_height_2, NULL, 4);
        if (image_data_1 == NULL || image_data_2 == NULL || image_width_1 != image_width_2 || image_height_1 != image_height_2)
            return false;

        unsigned char* split_image_data = (unsigned char*)malloc(image_width_1 * image_height_1 * 4);
        if (split_image_data == NULL)
            return false;
        int middle = image_width_1 / 2;
        int splitterWidth = image_width_1 / 40;
        for (int i = 0; i < image_width_1 * image_height_1; i++) {
            if (abs(i % image_width_1 - middle) < splitterWidth) {
                *(split_image_data + 4 * i + 0) = (unsigned char)0;
                *(split_image_data + 4 * i + 1) = (unsigned char)0;
                *(split_image_data + 4 * i + 2) = (unsigned char)0;
                *(split_image_data + 4 * i + 3) = (unsigned char)255;
            }
            else if (i % image_width_1 <= middle) {
                *(split_image_data + 4 * i + 0) = *(image_data_1 + 4 * i + 0);
                *(split_image_data + 4 * i + 1) = *(image_data_1 + 4 * i + 1);
                *(split_image_data + 4 * i + 2) = *(image_data_1 + 4 * i + 2);
                *(split_image_data + 4 * i + 3) = *(image_data_1 + 4 * i + 3);
            }
            else {
                *(split_image_data + 4 * i + 0) = *(image_data_2 + 4 * i + 0);
                *(split_image_data + 4 * i + 1) = *(image_data_2 + 4 * i + 1);
                *(split_image_data + 4 * i + 2) = *(image_data_2 + 4 * i + 2);
                *(split_image_data + 4 * i + 3) = *(image_data_2 + 4 * i + 3);
            }
            
        }

        if (maskWidth == image_width_1 && maskHeight == image_height_1) {
            for (int i = 0; i < maskWidth * maskHeight; i++) {
                *(split_image_data + 4 * i + 3) = *(talentMask + 4 * i + 3);
            }
        }

        // Create texture
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = image_width_1;
        desc.Height = image_height_1;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        ID3D11Texture2D* pTexture = NULL;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = split_image_data;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
        if (pTexture == NULL)
            return false;

        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
        pTexture->Release();

        *out_width = image_width_1;
        *out_height = image_height_1;
        stbi_image_free(image_data_1);
        stbi_image_free(image_data_2);
        stbi_image_free(split_image_data);

        return true;
    }

    bool unpackIcons(const char* filename, std::vector<std::string> metaData) {
        int iconWidth = std::stoi(metaData[0]);
        int iconHeight = std::stoi(metaData[1]);
        int iconCount = std::stoi(metaData[2]);
        int packedWidth = std::stoi(metaData[3]);

        std::filesystem::path rootIconPath("./resources/icons/");

        // load packed image file from disk into a raw RGBA buffer
        int image_width = 0;
        int image_height = 0;
        unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
        if (image_data == NULL || image_width != iconWidth * iconHeight)
            return false;

        unsigned char* iconData = (unsigned char*)malloc(iconWidth * iconHeight * 4);
        if (iconData == NULL) {
            return false;
        }
        for (int i = 0; i < iconCount; i++) {
            std::filesystem::path outPath(rootIconPath / metaData[i + 4]);
            std::filesystem::create_directory(outPath.parent_path());
            for (int pix = i * iconHeight * iconWidth; pix < (i+1) * iconHeight * iconWidth; pix++) {
                *(iconData + 4 * (pix % (iconWidth * iconHeight)) + 0) = *(image_data + 4 * pix + 0);
                *(iconData + 4 * (pix % (iconWidth * iconHeight)) + 1) = *(image_data + 4 * pix + 1);
                *(iconData + 4 * (pix % (iconWidth * iconHeight)) + 2) = *(image_data + 4 * pix + 2);
                *(iconData + 4 * (pix % (iconWidth * iconHeight)) + 3) = *(image_data + 4 * pix + 3);
            }
            stbi_write_png(outPath.string().c_str(), iconWidth, iconHeight, 4, iconData, iconWidth * 4);
        }
        stbi_image_free(image_data);
        stbi_image_free(iconData);

        return true;
    }
}