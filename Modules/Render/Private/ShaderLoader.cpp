/*
 * Copyright (c) 2026 SeolYang(Yang Kyowon)
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <Mon3tr/Render/ShaderLoader.hpp>
#include <Mon3tr/Render/Shader.hpp>
#include <Mon3tr/Core/FileIO.hpp>

#include "ShaderConstants.hpp"

namespace mon3tr::render {
    std::expected<asset::Asset*, ShaderLoader::EResult> ShaderLoader::Load(const asset::AssetLoadPayload<ShaderLoader>& payload) {
        M3_ASSERT(!payload.AssetBinaryPath.empty());

        if (payload.LoaderSpecificDesc.RenderDevice == nullptr) {
            return std::unexpected(EResult::InvalidRenderDevice);
        }

        const auto metadataItr = payload.MetadataRoot.find(internal::ShaderConstants::kShaderMetadataJsonKey);
        if (metadataItr == payload.MetadataRoot.cend()) {
            return std::unexpected(EResult::MetadataDoesNotExist);
        }
        const nlohmann::json& shaderMetadata = *metadataItr;
        const auto            shaderTypeItr = shaderMetadata.find(internal::ShaderConstants::kShaderTypeJsonKey);
        if (shaderTypeItr == shaderMetadata.cend()) {
            return std::unexpected(EResult::ShaderTypeDoesNotExist);
        }

        const Vector<uint8> shaderBinary = ReadBlobFromFile(payload.AssetBinaryPath);
        if (shaderBinary.empty()) {
            return std::unexpected(EResult::EmptyShaderBinary);
        }

        const auto shaderTypeOpt = magic_enum::enum_cast<nvrhi::ShaderType>(shaderTypeItr->get<std::string>());
        if (!shaderTypeOpt.has_value()) {
            return std::unexpected(EResult::InvalidShaderTypeMetadata);
        }
        nvrhi::ShaderHandle shader = payload.LoaderSpecificDesc.RenderDevice->createShader(
            nvrhi::ShaderDesc{
                .shaderType = *shaderTypeOpt,
                .debugName = payload.Label.string(),
                .entryName = "main",
            },
            shaderBinary.data(), shaderBinary.size());
        if (shader == nullptr) {
            return std::unexpected(EResult::FailedToCreateShader);
        }

        Shader* shaderAsset = Create<Shader, M3_MEM_CATEGORY(Asset)>();
        shaderAsset->handle_ = std::move(shader);
        return shaderAsset;
    }
}
