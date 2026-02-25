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
#include <chrono>
#include "Mon3tr/Core/Types.hpp"

namespace mon3tr {
    /**
     * @brief 프로그램 전역적으로 delta-time을 관리하는 클래스입니다.
     * @warning 메인 스레드에서의 프레임 시작을 기준점으로 하며, 메인 스레드에서만 쓰기 작업이 일어나야 합니다.
     * 프레임 시작 동기화가 보장되므로 타 스레드에서는 Lock이나 Atomic 없이 안전하게 읽을 수 있습니다.
     */
    class GlobalTimer {
    public:
        ~GlobalTimer() = default;

        // 싱글톤 복사/대입 방지
        GlobalTimer(const GlobalTimer&) = delete;

        GlobalTimer& operator=(const GlobalTimer&) = delete;

        [[nodiscard]] static GlobalTimer& GetInstance() noexcept {
            static GlobalTimer instance;
            return instance;
        }

        void BeginNewFrame();

        [[nodiscard]] uint64 GetFrameCounter() const noexcept { return frameCounter_; }

        ///< Delta Time을 초(seconds)단위로 반환 합니다.
        [[nodiscard]] double GetDeltaTime() const noexcept { return deltaTime_; }

        template<std::floating_point T>
        [[nodiscard]] T GetDeltaTime() const noexcept {
            return static_cast<T>(deltaTime_);
        }

        ///< Delta Time을 밀리초(milliseconds)단위로 반환합니다.
        [[nodiscard]] uint64 GetDeltaTimeMilli() const noexcept { return deltaTimeMilli_; }

        /**
         * @return 첫 프레임 시작부터 현재까지 지난 시간을 밀리초로 반환합니다.
         */
        [[nodiscard]] uint64 GetElapsedTimeFromFirstFrame() const noexcept {
            const auto elapsed = lastTime_ - startTime_;
            return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        }

    private:
        GlobalTimer() = default;

    private:
        std::chrono::high_resolution_clock::time_point startTime_ = std::chrono::high_resolution_clock::now();
        std::chrono::high_resolution_clock::time_point lastTime_ = std::chrono::high_resolution_clock::now();
        uint64                                         frameCounter_ = 0;
        uint64                                         deltaTimeMilli_ = 0;
        double                                         deltaTime_ = 0.0;
    };
}
