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
#include <Mon3tr/Core/Assertion.hpp>

namespace mon3tr {
    class System {
    public:
        virtual ~System() {
            M3_ASSERT(!bIsInitialized_);
        }

        System(const System&) = delete;

        System& operator=(const System&) = delete;

        System(System&&) noexcept = delete;

        System& operator=(System&&) noexcept = delete;

        /**
         * @brief 최하위 클래스의 Shutdown 로직이 완료되면, 반드시 부모 클래스의 Shutdown을 명시적으로 호출하여야 합니다.
         */
        virtual void Shutdown() {
            M3_ASSERT(bIsInitialized_);
            bIsInitialized_ = false;
        }

        [[nodiscard]] inline bool IsInitialized() const noexcept { return bIsInitialized_; }

    protected:
        System() = default;

        /**
         * @brief 시스템의 모든 초기화가 완료되면, 성공 결과(aka. result enumerator)를 반환하기 직전, 해당 함수를 호출하여 초기화에 성공하였음을 마킹해야 합니다.
         */
        void MarkAsInitialized() { bIsInitialized_ = true; }

    private:
        bool bIsInitialized_ = false;
    };

}