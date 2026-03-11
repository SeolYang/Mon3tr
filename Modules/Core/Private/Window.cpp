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
#include <Mon3tr/Core/Window.hpp>

M3_DEFINE_LOG_CATEGORY(Window);

namespace mon3tr {
    EWindowInitializeResult Window::Initialize(const WindowDesc& desc) {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            M3_LOG(Window, Fatal, "Failed to initialize SDL: {}", SDL_GetError());
            return EWindowInitializeResult::SDLInitializationFailed;
        }

        int32 targetWidth = desc.Width;
        int32 targetHeight = desc.Height;

        if (targetWidth <= 0 || targetHeight <= 0) {
            int                  numDisplays = 0;
            const SDL_DisplayID* displayIds = SDL_GetDisplays(&numDisplays);

            if (displayIds == nullptr || numDisplays == 0) {
                targetWidth = kWindowWidthFallback;
                targetHeight = kWindowHeightFallback;
            } else {
                const SDL_DisplayMode* currentDisplayMode = SDL_GetCurrentDisplayMode(displayIds[0]);
                targetWidth = currentDisplayMode->w;
                targetHeight = currentDisplayMode->h;
            }
        }

        SDL_WindowFlags windowFlags = 0;
        windowFlags |= desc.bBorderless ? SDL_WINDOW_BORDERLESS : 0;
        bIsBorderless_ = desc.bBorderless;
        //windowFlags |= desc.bFullscreen ? SDL_WINDOW_FULLSCREEN : 0;

        SDL_Window* window = SDL_CreateWindow(desc.Title.data(), targetWidth, targetHeight, windowFlags);
        if (window == nullptr) {
            M3_LOG(Window, Fatal, "Failed to create window: {}", SDL_GetError());
            return EWindowInitializeResult::SDLWindowCreationFailed;
        }
        width_ = targetWidth;
        height_ = targetHeight;
        window_ = window;

        MarkAsInitialized();
        return EWindowInitializeResult::Success;
    }

    void Window::Shutdown() {
        if (window_ != nullptr) {
            SDL_DestroyWindow(window_);
            window_ = nullptr;
        }

        SDL_Quit();
        System::Shutdown();
    }

    void* Window::GetNative() {
        if (window_ == nullptr) {
            M3_ASSERT(false);
            return nullptr;
        }

        const SDL_PropertiesID props = SDL_GetWindowProperties(window_);
        void*                  nativeHandle = nullptr;
#ifdef M3_PLATFORM_WINDOWS
        nativeHandle = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#else
        M3_UNIMPLEMENTED();
#endif
        if (nativeHandle == nullptr) {
            M3_LOG(Window, Warning, "Failed to get native handle pointer from sdl. {}", SDL_GetError());
        }

        return nativeHandle;
    }

    void Window::Resize(const int32 newWidth, const int32 newHeight) {
        if (newWidth == width_ && newHeight == height_) {
            return;
        }

        const int32 oldWidth = width_;
        const int32 oldHeight = height_;
        width_ = newWidth;
        height_ = newHeight;
        bIsResized_ = SDL_SetWindowSize(window_, width_, height_);

        if (bIsResized_) {
            M3_LOG(Window, Info, "The window resized from {}x{} to {}x{}.", oldWidth, oldHeight, newWidth, newHeight);
        } else {
            M3_LOG(Window, Warning, "Failed to resize window: {}", SDL_GetError());
        }
    }

    void Window::SetBorderless(const bool bIsBorderless) {
        if (bIsBorderless == bIsBorderless_) {
            return;
        }

        if (!SDL_SetWindowBordered(window_, !bIsBorderless)) {
            M3_LOG(Window, Warning, "Failed to switch window borderless: {}", SDL_GetError());
            return;
        }

        bIsBorderless_ = bIsBorderless;
        if (bIsBorderless_) {
            M3_LOG(Window, Info, "The Window switched to borderless.");
        } else {
            M3_LOG(Window, Info, "The Window switched to bordered.");
        }
    }

    void Window::SetTitle(const std::string_view title) {
        if (!SDL_SetWindowTitle(window_, title.data())) {
            M3_LOG(Window, Warning, "Failed to set title: {}", SDL_GetError());
        }
    }

    bool Window::HandleResize() {
        const bool bShouldHandleResize = bIsResized_;
        bIsResized_ = false;
        return bShouldHandleResize;
    }
}
