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
#include <Mon3tr/Core/FileIO.hpp>

namespace mon3tr {
    Vector<uint8> ReadBlobFromFile(const fs::path& blobFilePath) {
        Vector<uint8> blob{};
        if (!fs::exists(blobFilePath)) {
            return blob;
        }

        std::ifstream blobStream{blobFilePath, std::ios::in | std::ios::binary};
        if (!blobStream.is_open()) {
            return blob;
        }

        const uint64 sizeOfBlob = fs::file_size(blobFilePath);
        if (sizeOfBlob == 0) {
            return blob;
        }
        blob.resize(sizeOfBlob);
        blobStream.read(reinterpret_cast<char*>(blob.data()), sizeOfBlob);

        return blob;
    }

    bool WriteBlobToFile(const fs::path& blobFilePath, const std::span<const uint8> blob) {
        if (blob.data() == nullptr || blob.size() == 0) {
            return false;
        }

        std::ofstream blobStream{blobFilePath, std::ios::out | std::ios::binary};
        if (!blobStream.is_open()) {
            return false;
        }
        blobStream.write(reinterpret_cast<const char*>(blob.data()), blob.size());

        return true;
    }

    bool WriteBlobToFile(const fs::path& blobFilePath, const Vector<uint8>& blob) {
        return WriteBlobToFile(blobFilePath, std::span<const uint8>{blob.data(), blob.size()});
    }
}
