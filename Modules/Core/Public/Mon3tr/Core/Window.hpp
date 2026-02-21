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

namespace mon3tr {
    /**
     * @brief The description struct for initialize/create a window instance.
     */
    struct WindowDesc {
        std::string_view Title = "Mon3tr"; ///< 생성될 윈도우 창의 타이틀(제목)
        uint16 Width = 0;  ///< 생성될 윈도우 창의 가로 해상도(Pixel). 만약 0이라면 선택 가능한 가장 큰 해상도를 사용.
        uint16 Height = 0; ///< 생성될 윈도우 창의 세로 해상도(Pixel). 만약 0이라면 선택 가능한 가장 큰 해상도를 사용.
        bool bFullscreen = false; ///< 생성될 윈도우 창이 전체화면으로 생성 될지 설정.
        bool bBorderless = false; ///< 생성될 윈도우 창이 테두리없는 창일지 결정.
    };

    class Window {
    public:
        explicit Window(WindowDesc desc);

        ~Window();

        Window(Window const&) = delete;

        Window& operator=(Window const&) = delete;

        Window(Window&&) = delete;

        Window& operator=(Window&&) = delete;

        [[nodiscard]] uint16 GetWidth() const noexcept { return width_; }
        [[nodiscard]] uint16 GetHeight() const noexcept { return height_; }
        [[nodiscard]] bool IsFullscreen() const noexcept { return bIsFullscreen_; }
        [[nodiscard]] bool IsBorderless() const noexcept { return bIsBorderless_;}

    private:
        uint16 width_ = 0;
        uint16 height_ = 0;
        bool bIsFullscreen_ = false;
        bool bIsBorderless_ = false;

        SDL_Window* window_ = nullptr;
    };
}
