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
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

/* Standard Libs */
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <algorithm>
#include <memory>
#include <thread>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <tuple>
#include <stack>
#include <queue>
#include <stacktrace>
#include <type_traits>
#include <concepts>
#include <expected>
#include <shared_mutex>

/* Internal */
#include <Mon3tr/Core/Assertion.hpp>
#include <Mon3tr/Core/Types.hpp>
#include <Mon3tr/Core/Version.hpp>
#include <Mon3tr/Core/Memory.hpp>
#include <Mon3tr/Core/Platform.hpp>
#include <Mon3tr/Core/Handle.hpp>
#include <Mon3tr/Core/System.hpp>
#include <Mon3tr/Core/Log.hpp>
#include <Mon3tr/Core/Profiler.hpp>
#include <Mon3tr/Core/Container.hpp>
namespace m3 = mon3tr;

M3_DECLARE_MEM_CATEGORY(Core)

/* External Libs */
#pragma warning(push, 0)
#pragma warning(disable : 4996)
#include <cereal/cereal.hpp>
#include <simdutf.h>
#include <magic_enum/magic_enum.hpp>
#include <flecs.h>
#include <ankerl/unordered_dense.h>
#include <vfspp/VFS.h>
#include <SDL3/SDL.h>
#pragma warning(pop)