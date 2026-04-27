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
#include <Mon3tr/Asset/AssetUtils.hpp>

namespace mon3tr::asset {
    fs::path CreateAssetBinaryPath(const Guid& guid) {
        M3_ASSERT(guid.isValid());
        constexpr std::string_view kAssetBinaryPathFormat = "Assets\\{}.m3tr";
        return std::format(kAssetBinaryPathFormat, guid.str());
    }

    fs::path CreateAssetMetadataPath(const Guid& guid) {
        M3_ASSERT(guid.isValid());
        constexpr std::string_view kAssetMetadataPathFormat = "Assets\\{}.m3mt";
        return std::format(kAssetMetadataPathFormat, guid.str());
    }

    fs::path CreateAssetBinaryPlaceholderPath(const Guid& guid, const uint64 idx) {
        M3_ASSERT(guid.isValid());
        constexpr std::string_view kAssetPlaceholderBinaryPathFormat = "Assets\\Placeholders\\{}_{}.m3ph";
        return std::format(kAssetPlaceholderBinaryPathFormat, guid.str(), idx);
    }
}
