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
#include <Mon3tr/Render/RenderMinimal.hpp>

namespace mon3tr::render {
    struct StaticMeshVertexBase {
        math::Vector3 Position;
        math::Vector3 Normal;
        math::Vector3 Tangent;
        math::Vector3 Bitangent;
        math::Vector2 TexCoords;
    };

    struct StaticMeshVertex {
        math::Vector3 Position; // [0, 12)
        /* x: 10 Bits, y: 10 Bits, z: 10 Bits, Pad: 2 Bits, [-1, 1] -> [0, 1023] */
        uint32 QuantizedNormal; // [12, 16)
        /* x: 10 Bits, y: 10 Bits, z: 10 Bits, Pad: 2 Bits, [-1, 1] -> [0, 1023] */
        uint32 QuantizedTangent; // [16, 20)
        /* x: 10 Bits, y: 10 Bits, z: 10 Bits, Pad: 2 Bits, [-1, 1] -> [0, 1023] */
        uint32 QuantizedBitangent; // [20, 24)
        /* (F32, F32) -> (F16, F16) = HLSL(float16_t, float16_t); https://github.com/zeux/meshoptimizer/blob/master/src/quantization.cpp*/
        uint32 QuantizedTexCoords[2]; // [24, 32)
    };
}
