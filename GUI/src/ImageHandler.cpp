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
#include <fstream>
#include <ppl.h>

#include "ImageHandler.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define __STDC_LIB_EXT1__
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace TTM {
    // Simple helper function to load an image into a DX11 texture with common settings
    bool LoadTextureFromFile(
        const char* filename, 
        ID3D11ShaderResourceView** out_srv,
        ID3D11ShaderResourceView** out_srv_gray, 
        int* out_width, 
        int* out_height, 
        ID3D11Device* g_pd3dDevice, 
        Engine::TalentType talentType
    ){
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
        unsigned char* image_data_gray = stbi_load(filename, &image_width, &image_height, NULL, 4);
        if (image_data == NULL || image_data_gray == NULL)
            return false;

        if (maskWidth == image_width && maskHeight == image_height) {
            for (int i = 0; i < maskWidth * maskHeight; i++) {
                *(image_data + 4 * i + 3) = *(talentMask + 4 * i + 3);
            }
        }

        for (int i = 0; i < image_width * image_height; i++) {
            unsigned char grayVal = static_cast<unsigned char>(0.299f * (*(image_data + 4 * i + 0)) + 0.587f * (*(image_data + 4 * i + 1)) + 0.114f * (*(image_data + 4 * i + 2)));
            *(image_data_gray + 4 * i + 0) = grayVal;
            *(image_data_gray + 4 * i + 1) = grayVal;
            *(image_data_gray + 4 * i + 2) = grayVal;
            *(image_data_gray + 4 * i + 3) = *(image_data + 4 * i + 3);
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

        ID3D11Texture2D* pTextureGray = NULL;
        D3D11_SUBRESOURCE_DATA subResourceGray;
        subResourceGray.pSysMem = image_data_gray;
        subResourceGray.SysMemPitch = desc.Width * 4;
        subResourceGray.SysMemSlicePitch = 0;
        g_pd3dDevice->CreateTexture2D(&desc, &subResourceGray, &pTextureGray);
        if (pTextureGray == NULL)
            return false;

        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDescGray;
        ZeroMemory(&srvDescGray, sizeof(srvDescGray));
        srvDescGray.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDescGray.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDescGray.Texture2D.MipLevels = desc.MipLevels;
        srvDescGray.Texture2D.MostDetailedMip = 0;
        g_pd3dDevice->CreateShaderResourceView(pTextureGray, &srvDescGray, out_srv_gray);
        pTextureGray->Release();

        *out_width = image_width;
        *out_height = image_height;
        stbi_image_free(image_data);
        stbi_image_free(image_data_gray);

        return true;
    }

    std::pair<
        std::vector<std::string>,
        std::pair<std::vector<ID3D11ShaderResourceView*>, std::vector<ID3D11ShaderResourceView*>>
    > LoadPackedIcons(
        const char* packedIconsPath,
        const char* metadataPath,
        int* width,
        int* height,
        ID3D11Device* g_pd3dDevice
    ) {
        std::vector<std::string> iconNames;
        std::pair<
            std::vector<ID3D11ShaderResourceView*>,
            std::vector<ID3D11ShaderResourceView*>
        > iconTextures;

        // load metadata
        std::ifstream metadataFile(metadataPath);
        std::string line;

        // load width and height
        std::getline(metadataFile, line);
        *width = std::atoi(line.c_str());
        std::getline(metadataFile, line);
        *height = std::atoi(line.c_str());

        // get number of icons
        std::getline(metadataFile, line);
        int numIcons = std::atoi(line.c_str());

        // get packed width
        std::getline(metadataFile, line);
        int packedWidth = std::atoi(line.c_str());

        // read icon names
        while (std::getline(metadataFile, line))
        {
            iconNames.push_back(line);
        }

        // Load from disk into a raw RGBA buffer
        int packed_image_width = DEFAULT_ICON_SIZE_W;
        int packed_image_height = DEFAULT_ICON_SIZE_H;
        unsigned char* packed_image_data = stbi_load(packedIconsPath, &packed_image_width, &packed_image_height, NULL, 4);
        if (packed_image_data == NULL || packed_image_height != numIcons || packed_image_width != packedWidth) {
            iconNames.clear();
            return { iconNames, iconTextures };
        }

        for (int iconNumber = 0; iconNumber < numIcons; iconNumber++) {
            iconTextures.first.emplace_back(nullptr);
            iconTextures.second.emplace_back(nullptr);
        }

        bool success = true;
        // for (int iconNumber = 0; iconNumber < numIcons; iconNumber++) {
        Concurrency::parallel_for(int(0), numIcons, [&](int iconNumber) {
            unsigned char* image_data = (unsigned char*)malloc(4 * *width * *height);
            unsigned char* image_data_gray = (unsigned char*)malloc(4 * *width * *height);

            if (image_data == NULL || image_data_gray == NULL) {
                success = false;
                return;
            }

            for (int i = 0; i < 4 * packedWidth; i++) {
                *(image_data + i) = *(packed_image_data + 4 * packedWidth * iconNumber + i);
            }

            for (int i = 0; i < packedWidth; i++) {
                unsigned char grayVal = static_cast<unsigned char>(0.299f * (*(image_data + 4 * i + 0)) + 0.587f * (*(image_data + 4 * i + 1)) + 0.114f * (*(image_data + 4 * i + 2)));
                *(image_data_gray + 4 * i + 0) = grayVal;
                *(image_data_gray + 4 * i + 1) = grayVal;
                *(image_data_gray + 4 * i + 2) = grayVal;
                *(image_data_gray + 4 * i + 3) = *(image_data + 4 * i + 3);
            }

            // Create texture
            D3D11_TEXTURE2D_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Width = *width;
            desc.Height = *height;
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
            if (pTexture == NULL) {
                success = false;
                stbi_image_free(image_data);
                stbi_image_free(image_data_gray);
                return;
            }

            // Create texture view
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            ZeroMemory(&srvDesc, sizeof(srvDesc));
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &iconTextures.first[iconNumber]);
            pTexture->Release();

            ID3D11Texture2D* pTextureGray = NULL;
            D3D11_SUBRESOURCE_DATA subResourceGray;
            subResourceGray.pSysMem = image_data_gray;
            subResourceGray.SysMemPitch = desc.Width * 4;
            subResourceGray.SysMemSlicePitch = 0;
            g_pd3dDevice->CreateTexture2D(&desc, &subResourceGray, &pTextureGray);
            if (pTextureGray == NULL) {
                success = false;
                stbi_image_free(image_data);
                stbi_image_free(image_data_gray);
                return;
            }

            // Create texture view
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDescGray;
            ZeroMemory(&srvDescGray, sizeof(srvDescGray));
            srvDescGray.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDescGray.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDescGray.Texture2D.MipLevels = desc.MipLevels;
            srvDescGray.Texture2D.MostDetailedMip = 0;
            g_pd3dDevice->CreateShaderResourceView(pTextureGray, &srvDescGray, &iconTextures.second[iconNumber]);
            pTextureGray->Release();

            stbi_image_free(image_data);
            stbi_image_free(image_data_gray);
            }
        );

        if (!success) {
            iconNames.clear();
            for (size_t i = 0; i < iconTextures.first.size(); i++) {
                if (iconTextures.first[i]) {
                    iconTextures.first[i]->Release();
                    iconTextures.first[i] = nullptr;
                }
            }
            for (size_t i = 0; i < iconTextures.second.size(); i++) {
                if (iconTextures.second[i]) {
                    iconTextures.second[i]->Release();
                    iconTextures.second[i] = nullptr;
                }
            }
            iconTextures.first.clear();
            iconTextures.second.clear();
            return { iconNames, iconTextures };
        }
        return { iconNames, iconTextures };
    }

    bool LoadTTMBanner(
        ID3D11ShaderResourceView** out_srv,
        int* out_width,
        int* out_height,
        ID3D11Device* g_pd3dDevice
    ) {
        // Load from disk into a raw RGBA buffer
        int image_width = DEFAULT_ICON_SIZE_W;
        int image_height = DEFAULT_ICON_SIZE_H;
        unsigned char* image_data = stbi_load("./resources/TTM_Banner.png", &image_width, &image_height, NULL, 4);
        if (image_data == NULL)
            return false;

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

    bool LoadDefaultTexture(
        ID3D11ShaderResourceView** out_srv,
        ID3D11ShaderResourceView** out_srv_gray,
        int* out_width, 
        int* out_height, 
        ID3D11Device* g_pd3dDevice,
        Engine::TalentType talentType
    ){
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
        unsigned char* image_data_gray = (unsigned char*)malloc(4 * image_width * image_height);
        if (image_data == NULL || image_data_gray == NULL)
            return false;

        for (int i = 0; i < 4 * image_width * image_height; i++) {
            *(image_data + i) = *(DEFAULT_ICON_DATA + i);
        }

        if (maskWidth == image_width && maskHeight == image_height) {
            for (int i = 0; i < maskWidth * maskHeight; i++) {
                *(image_data + 4 * i + 3) = *(talentMask + 4 * i + 3);
            }
        }

        for (int i = 0; i < image_width * image_height; i++) {
            unsigned char grayVal = static_cast<unsigned char>(0.299f * (*(image_data + 4 * i + 0)) + 0.587f * (*(image_data + 4 * i + 1)) + 0.114f * (*(image_data + 4 * i + 2)));
            *(image_data_gray + 4 * i + 0) = grayVal;
            *(image_data_gray + 4 * i + 1) = grayVal;
            *(image_data_gray + 4 * i + 2) = grayVal;
            *(image_data_gray + 4 * i + 3) = *(image_data + 4 * i + 3);
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

        ID3D11Texture2D* pTextureGray = NULL;
        D3D11_SUBRESOURCE_DATA subResourceGray;
        subResourceGray.pSysMem = image_data_gray;
        subResourceGray.SysMemPitch = desc.Width * 4;
        subResourceGray.SysMemSlicePitch = 0;
        g_pd3dDevice->CreateTexture2D(&desc, &subResourceGray, &pTextureGray);
        if (pTextureGray == NULL)
            return false;

        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDescGray;
        ZeroMemory(&srvDescGray, sizeof(srvDescGray));
        srvDescGray.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDescGray.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDescGray.Texture2D.MipLevels = desc.MipLevels;
        srvDescGray.Texture2D.MostDetailedMip = 0;
        g_pd3dDevice->CreateShaderResourceView(pTextureGray, &srvDescGray, out_srv_gray);
        pTextureGray->Release();

        *out_width = image_width;
        *out_height = image_height;
        stbi_image_free(image_data);
        stbi_image_free(image_data_gray);

        return true;
    }

    bool LoadIconGlowTexture(
        ID3D11ShaderResourceView** out_srv,
        int* out_width, 
        int* out_height, 
        ID3D11Device* g_pd3dDevice,
        Engine::TalentType talentType,
        float r, float g, float b)
    {
        int image_width = RED_ICON_GLOW_SIZE_W;
        int image_height = RED_ICON_GLOW_SIZE_H;
        unsigned char* image_data = (unsigned char*)malloc(4 * image_width * image_height);
        if (image_data == NULL)
            return false;
        const unsigned char(*TALENT_MASK_DATA_PTR)[6400] = NULL;
        switch (talentType) {
        case Engine::TalentType::ACTIVE: {TALENT_MASK_DATA_PTR = &ACTIVE_TALENT_MASK_DATA; }break;
        case Engine::TalentType::PASSIVE: {TALENT_MASK_DATA_PTR = &PASSIVE_TALENT_MASK_DATA; }break;
        case Engine::TalentType::SWITCH: {TALENT_MASK_DATA_PTR = &SWITCH_TALENT_MASK_DATA; }break;
        default: {TALENT_MASK_DATA_PTR = &ACTIVE_TALENT_MASK_DATA; }break;
        }
        const unsigned char rc = static_cast<unsigned char>(r * 255.0f);
        const unsigned char gc = static_cast<unsigned char>(g * 255.0f);
        const unsigned char bc = static_cast<unsigned char>(b * 255.0f);
        for (int i = 0; i < image_height; i++) {
            for (int j = 0; j < image_width; j++) {
                *(image_data + 4 * image_width * i + 4 * j + 0) = rc;
                *(image_data + 4 * image_width * i + 4 * j + 1) = gc;
                *(image_data + 4 * image_width * i + 4 * j + 2) = bc;
                //*(image_data + 4 * i + 3) = *(RED_ICON_GLOW_DATA + 4 * i + 3) * static_cast<bool>(255 - *(*TALENT_MASK_DATA_PTR + 4*i + 3));
                //TTMTODO: Replace with non-hardcoded dimensions
                if (i < 20 || i >= 60 || j < 20 || j >= 60) {
                    *(image_data + 4 * image_width * i + 4 * j + 3) = *(RED_ICON_GLOW_DATA + 4 * image_width * i + 4 * j + 3);
                }
                else {
                    *(image_data + 4 * image_width * i + 4 * j + 3) = *(RED_ICON_GLOW_DATA + 4 * image_width * i + 4 * j + 3) * static_cast<bool>(255 - *(*TALENT_MASK_DATA_PTR + 4 * (image_width/2) * (i - 20) + 4 * (j - 20) + 3));
                }
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

    bool LoadPurpleIconGlowTexture(ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height, ID3D11Device* g_pd3dDevice)
    {
        int image_width = PURPLE_ICON_GLOW_SIZE_W;
        int image_height = PURPLE_ICON_GLOW_SIZE_H;
        unsigned char* image_data = (unsigned char*)malloc(4 * image_width * image_height);
        if (image_data == NULL)
            return false;
        for (int i = 0; i < 4 * image_width * image_height; i++) {
            *(image_data + i) = *(PURPLE_ICON_GLOW_DATA + i);
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

    bool LoadIconMaskTexture(
        ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height, ID3D11Device* g_pd3dDevice,
        Engine::TalentType talentType,
        float r, float g, float b) {
        int image_width = ACTIVE_TALENT_MASK_SIZE_W;
        int image_height = ACTIVE_TALENT_MASK_SIZE_H;
        unsigned char* image_data = (unsigned char*)malloc(4 * image_width * image_height);
        if (image_data == NULL)
            return false;
        const unsigned char (*TALENT_MASK_DATA_PTR)[6400] = NULL;
        switch (talentType) {
        case Engine::TalentType::ACTIVE: {TALENT_MASK_DATA_PTR = &ACTIVE_TALENT_MASK_DATA; }break;
        case Engine::TalentType::PASSIVE: {TALENT_MASK_DATA_PTR = &PASSIVE_TALENT_MASK_DATA; }break;
        case Engine::TalentType::SWITCH: {TALENT_MASK_DATA_PTR = &SWITCH_TALENT_MASK_DATA; }break;
        default: {TALENT_MASK_DATA_PTR = &ACTIVE_TALENT_MASK_DATA; }break;
        }
        const unsigned char rc = static_cast<unsigned char>(r * 255.0f);
        const unsigned char gc = static_cast<unsigned char>(g * 255.0f);
        const unsigned char bc = static_cast<unsigned char>(b * 255.0f);
        for (int i = 0; i < image_width * image_height; i++) {
            *(image_data + 4 * i + 0) = rc;
            *(image_data + 4 * i + 1) = gc;
            *(image_data + 4 * i + 2) = bc;
            *(image_data + 4 * i + 3) = 255 - *(*TALENT_MASK_DATA_PTR + 4 * i + 3);
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
        ID3D11ShaderResourceView** out_srv_gray,
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

        unsigned char* image_data_gray = (unsigned char*)malloc(4 * image_width_1 * image_height_1);
        if (image_data_1 == NULL || image_data_2 == NULL || image_data_gray == NULL || image_width_1 != image_width_2 || image_height_1 != image_height_2)
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

        for (int i = 0; i < image_width_1 * image_height_1; i++) {
            unsigned char grayVal = static_cast<unsigned char>(0.299f * (*(split_image_data + 4 * i + 0)) + 0.587f * (*(split_image_data + 4 * i + 1)) + 0.114f * (*(split_image_data + 4 * i + 2)));
            *(image_data_gray + 4 * i + 0) = grayVal;
            *(image_data_gray + 4 * i + 1) = grayVal;
            *(image_data_gray + 4 * i + 2) = grayVal;
            *(image_data_gray + 4 * i + 3) = *(split_image_data + 4 * i + 3);
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

        ID3D11Texture2D* pTextureGray = NULL;
        D3D11_SUBRESOURCE_DATA subResourceGray;
        subResourceGray.pSysMem = image_data_gray;
        subResourceGray.SysMemPitch = desc.Width * 4;
        subResourceGray.SysMemSlicePitch = 0;
        g_pd3dDevice->CreateTexture2D(&desc, &subResourceGray, &pTextureGray);
        if (pTextureGray == NULL)
            return false;

        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDescGray;
        ZeroMemory(&srvDescGray, sizeof(srvDescGray));
        srvDescGray.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDescGray.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDescGray.Texture2D.MipLevels = desc.MipLevels;
        srvDescGray.Texture2D.MostDetailedMip = 0;
        g_pd3dDevice->CreateShaderResourceView(pTextureGray, &srvDescGray, out_srv_gray);
        pTextureGray->Release();

        *out_width = image_width_1;
        *out_height = image_height_1;
        stbi_image_free(image_data_1);
        stbi_image_free(image_data_2);
        stbi_image_free(split_image_data);
        stbi_image_free(image_data_gray);

        return true;
    }

    bool unpackIcons(const char* filename, std::vector<std::string> metaData) {
        int iconWidth = std::stoi(metaData[0]);
        int iconHeight = std::stoi(metaData[1]);
        int iconCount = std::stoi(metaData[2]);
        int packedWidth = std::stoi(metaData[3]);

        std::filesystem::path rootIconPath = Presets::getAppPath() / "resources" / "icons";

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