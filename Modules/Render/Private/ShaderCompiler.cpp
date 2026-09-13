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
#include <Mon3tr/Render/ShaderCompiler.hpp>
#include <Mon3tr/Core/FileIO.hpp>
#include <dxcapi.h>

#include "ShaderConstants.hpp"

M3_DECLARE_LOG_CATEGORY(ShaderCompiler);

M3_DEFINE_LOG_CATEGORY(ShaderCompiler);

namespace mon3tr::render {
    static std::wstring_view ConvertShaderTypeToShaderProfile(const nvrhi::ShaderType shaderType) {
        switch (shaderType) {
            case nvrhi::ShaderType::Compute:
                return L"cs_6_8";
            case nvrhi::ShaderType::Vertex:
                return L"vs_6_8";
            case nvrhi::ShaderType::Hull:
                return L"hs_6_8";
            case nvrhi::ShaderType::Domain:
                return L"ds_6_8";
            case nvrhi::ShaderType::Geometry:
                return L"gs_6_8";
            case nvrhi::ShaderType::Pixel:
                return L"ps_6_8";
            case nvrhi::ShaderType::Amplification:
                return L"as_6_8";
            case nvrhi::ShaderType::Mesh:
                return L"ms_6_8";
            case nvrhi::ShaderType::RayGeneration:
            case nvrhi::ShaderType::AnyHit:
            case nvrhi::ShaderType::ClosestHit:
            case nvrhi::ShaderType::Miss:
            case nvrhi::ShaderType::Intersection:
            case nvrhi::ShaderType::Callable:
                return L"lib_6_8";
            default:
                M3_ASSERT(false);
                return std::wstring_view{};
        }
    }

    static std::wstring_view ConvertOptimizationLevelToFlag(const EShaderOptimizationLevel level) {
        switch (level) {
            default:
            case EShaderOptimizationLevel::None:
                return L"-Od";
            case EShaderOptimizationLevel::O0:
                return L"-O0";
            case EShaderOptimizationLevel::O1:
                return L"-O1";
            case EShaderOptimizationLevel::O2:
                return L"-O2";
            case EShaderOptimizationLevel::O3:
                return L"-O3";
        }
    }

    ShaderCompiler::EResult ShaderCompiler::Import(const asset::AssetImportPayload<ShaderCompiler>& payload) {
        Vector<const wchar_t*> arguments;
        arguments.reserve(16);

        // Entry Point
        arguments.emplace_back(L"-E");
        arguments.emplace_back(L"main");

        // Shader Profile
        arguments.emplace_back(L"-T");
        arguments.emplace_back(ConvertShaderTypeToShaderProfile(payload.ImporterSpecificDesc.ShaderType).data());

        // Optimization Level
        arguments.emplace_back(ConvertOptimizationLevelToFlag(payload.ImporterSpecificDesc.OptimizationLevel).data());

        // Should pack matrix in row major?
        if (payload.ImporterSpecificDesc.bPackMatricesInRowMajor) {
            arguments.emplace_back(L"-Zpr");
        }

        // Should disable validation?
        if (payload.ImporterSpecificDesc.bDisableValidation) {
            arguments.emplace_back(L"-Vd");
        }

        // Should treat warning as errors?
        if (payload.ImporterSpecificDesc.bTreatWarningAsErrors) {
            arguments.emplace_back(L"-WX");
        }

        // Should include debug info?
        if (payload.ImporterSpecificDesc.bIncludeDebugInfo) {
            arguments.emplace_back(L"-Zi");
            arguments.emplace_back(L"-Qembed_debug");
        } else {
            arguments.emplace_back(L"-Qstrip_debug");
        }

        // @todo include path도 Descriptor로 지정가능하도록?
        // Shader include path
        arguments.emplace_back(L"-I");
        arguments.emplace_back(L"/Shaders");

        nvrhi::RefCountPtr<IDxcUtils> dxcUtils;
        if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils)))) {
            return EResult::FailedToCreateDxcUtilsInstance;
        }

        nvrhi::RefCountPtr<IDxcIncludeHandler> dxcIncludeHandler;
        if (FAILED(dxcUtils->CreateDefaultIncludeHandler(&dxcIncludeHandler))) {
            return EResult::FailedToCreateDxcIncludeHandler;
        }

        nvrhi::RefCountPtr<IDxcLibrary> dxcLibrary;
        if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&dxcLibrary)))) {
            return EResult::FailedToCreateDxcLibraryInstance;
        }

        uint32                               codePage = CP_UTF8;
        nvrhi::RefCountPtr<IDxcBlobEncoding> sourceBlob;
        if (FAILED(dxcLibrary->CreateBlobFromFile(
            payload.ImportDesc.RawFilePath.c_str(),
            &codePage,
            &sourceBlob))) {
            return EResult::FailedToCreateBlobFromRawAssetFile;
        }

        nvrhi::RefCountPtr<IDxcCompiler3> dxcCompiler;
        if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler)))) {
            return EResult::FailedToCreateDxcCompilerInstance;
        }

        const DxcBuffer dxcBuffer{
            .Ptr = sourceBlob->GetBufferPointer(),
            .Size = sourceBlob->GetBufferSize(),
            .Encoding = codePage
        };
        nvrhi::RefCountPtr<IDxcResult> dxcResult;
        const HRESULT                  compileResult = dxcCompiler->Compile(
            &dxcBuffer,
            arguments.data(), static_cast<uint32>(arguments.size()),
            dxcIncludeHandler.Get(),
            IID_PPV_ARGS(&dxcResult));
        nvrhi::RefCountPtr<IDxcBlobUtf8> errorMessage;
        if (FAILED(compileResult) || SUCCEEDED(dxcResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errorMessage.GetAddressOf()), nullptr))) {
            if (errorMessage && errorMessage->GetStringLength() > 0) {
                M3_LOG(ShaderCompiler, Error, "Failed to compile shader from {}. Reasons: {}",
                       payload.ImportDesc.RawFilePath.string(),
                       errorMessage->GetStringPointer());
            } else {
                M3_LOG(ShaderCompiler, Error, "Failed to compile shader from {}. Reasons: Unknown",
                       payload.ImportDesc.RawFilePath.string());
            }

            return EResult::FailedToCompileShader;
        }

        nvrhi::RefCountPtr<IDxcBlob> compiledShaderBlob;
        if (FAILED(dxcResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&compiledShaderBlob), nullptr))) {
            return EResult::FailedToGetCompiledShaderBlob;
        }

        if (!WriteBlobToFile(payload.AssetBinaryPath, std::span{
                                 static_cast<const uint8*>(compiledShaderBlob->GetBufferPointer()),
                                 compiledShaderBlob->GetBufferSize()
                             })) {
            return EResult::FailedToWriteCompiledShaderBlobToAssetBinaryFile;
        }

        // Metadata 기록
        nlohmann::json shaderMetadata;
        shaderMetadata[internal::ShaderConstants::kShaderTypeJsonKey] = magic_enum::enum_name(payload.ImporterSpecificDesc.ShaderType);
        payload.MetadataRoot[internal::ShaderConstants::kShaderMetadataJsonKey] = shaderMetadata;

        return EResult::Success;
    }
}
