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
#include <Mon3tr/Core/Types.hpp>

namespace mon3tr::render {
    /* [-1, 1] -> [0, 255] */
    inline uint32 EncodeNormalX8Y8Z8(const math::Vector3& normal) {
        return (uint32) ((normal.x + 1.f) * 127.5f) |
               ((uint32) ((normal.y + 1.f) * 127.5f) << 8) |
               ((uint32) ((normal.z + 1.f) * 127.5f) << 16);
    }

    inline math::Vector3 DecodeNormalX8Y8Z8(const uint32 encodedNormal) {
        constexpr float kInvFactor = 1.f / 127.5f;
        return math::Vector3{
            ((float) (encodedNormal & 0xFF) * kInvFactor) - 1.f,
            ((float) ((encodedNormal >> 8) & 0xFF) * kInvFactor) - 1.f,
            ((float) ((encodedNormal >> 16) & 0xFF) * kInvFactor) - 1.f
        };
    }

    /* [-1, 1] -> [0, 1023] */
    inline uint32 EncodeNormalX10Y10Z10(const math::Vector3& normal) {
        return (uint32) ((normal.x + 1.f) * 511.5f) |
               ((uint32) ((normal.y + 1.f) * 511.5f) << 10) |
               ((uint32) ((normal.z + 1.f) * 511.5f) << 20);
    }

    inline math::Vector3 DecodeNormalX10Y10Z10(const uint32 encodedNormal) {
        constexpr float kInvFactor = 1.f / 511.5f;
        return math::Vector3{
            ((float) (encodedNormal & 0x3FF) * kInvFactor) - 1.f,
            ((float) ((encodedNormal >> 10) & 0x3FF) * kInvFactor) - 1.f,
            ((float) ((encodedNormal >> 20) & 0x3FF) * kInvFactor) - 1.f
        };
    }

    /* RGBA32F -> RGBA8_UINT */
    inline uint32 EncodeRGBA32F(const math::Vector4& rgba) {
        return (uint32) (rgba.x * 255.f) |
               ((uint32) (rgba.y * 255.f) << 8) |
               ((uint32) (rgba.z * 255.f) << 16) |
               ((uint32) (rgba.w * 255.f) << 24);
    }
}
