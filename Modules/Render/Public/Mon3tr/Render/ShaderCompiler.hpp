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
#pragma once
#include <Mon3tr/Core/CoreMinimal.hpp>
#include <Mon3tr/Asset/AssetMinimal.hpp>
#include <Mon3tr/Render/RenderMinimal.hpp>

namespace mon3tr::asset {
    class AssetManager;
}

namespace mon3tr::render {
    enum class EShaderCompileResult : uint16 {
        Success,
        FailedToCreateDxcUtilsInstance,
        FailedToCreateDxcIncludeHandler,
        FailedToCreateDxcLibraryInstance,
        FailedToCreateBlobFromRawAssetFile,
        FailedToCreateDxcCompilerInstance,
        FailedToCompileShader,
        FailedToGetCompiledShaderBlob,
        FailedToWriteCompiledShaderBlobToAssetBinaryFile,
    };

    enum class EShaderOptimizationLevel : uint16 {
        None,
        O0,
        O1,
        O2,
        O3,
    };

    class ShaderCompiler {
        friend class asset::AssetManager;

    public:
        struct Desc {
            // None/AllGraphics/AllRayTracing/All/bitwise combined Types are invalid argument!
            nvrhi::ShaderType ShaderType = nvrhi::ShaderType::None;

            EShaderOptimizationLevel OptimizationLevel = EShaderOptimizationLevel::O3; // -Od ~ -O3
            bool                     bPackMatricesInRowMajor = false;                  // -Zpr
            bool                     bDisableValidation = false;                       // -Vd
            bool                     bTreatWarningAsErrors = false;                    // -WX
            bool                     bIncludeDebugInfo = false;                        // -Zi
        };

    private:
        [[nodiscard]] static EShaderCompileResult Import(const asset::AssetImportPayload<ShaderCompiler>& payload);

    public:
        static constexpr uint64 kVersion = 1;
        static constexpr std::string_view kName = "ShaderCompiler";
    };
}
