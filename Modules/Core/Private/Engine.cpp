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
#include <Mon3tr/Core/Engine.hpp>

M3_DECLARE_MEM_CATEGORY(Engine);

M3_DEFINE_MEM_CATEGORY(Engine);

namespace mon3tr {
    enki::TaskScheduler* Engine::taskScheduler_ = nullptr;

    void Engine::Initialize(const EngineCoreInitDesc& desc) {
        M3_ASSERT(desc.TaskScheduler != nullptr);
        InitializeECSModule(desc.TaskScheduler);
    }

    void Engine::InitializeECSModule(enki::TaskScheduler* taskScheduler) {
        taskScheduler_ = taskScheduler;

        ecs_os_set_api_defaults();
        ecs_os_api_t api = ecs_os_api;

        // 엔진에서 외부에 노출하는 Create/Destroy 함수의 경우 realloc과 같은 동작을 지원하지 않기 때문에
        // 해당 API들을 직접적으로 사용하는 대신 snmalloc에서 제공하는 libc API를 사용하도록 한다.
        api.malloc_ = [](const ecs_size_t size) { return internal::libc::Malloc(size); };
        api.free_ = [](void* ptr) { internal::libc::Free(ptr); };
        api.realloc_ = [](void* ptr, const ecs_size_t size) { return internal::libc::Realloc(ptr, size); };
        api.calloc_ = [](const ecs_size_t size) { return internal::libc::Calloc(size); };

        api.task_new_ = [](ecs_os_thread_callback_t callback, void* param) {
            M3_ASSERT(taskScheduler_ != nullptr);
            enki::TaskSet* task = Create<enki::TaskSet, M3_MEM_CATEGORY(Engine)>(
                1,
                [callback, param]([[maybe_unused]] const enki::TaskSetPartition range, [[maybe_unused]] const uint32_t threadNum) {
                    callback(param);
                });
            taskScheduler_->AddTaskSetToPipe(task);
            return reinterpret_cast<ecs_os_thread_t>(task);
        };

        api.task_join_ = [](ecs_os_thread_t thread) -> void* {
            M3_ASSERT(taskScheduler_ != nullptr);
            enki::TaskSet* task = reinterpret_cast<enki::TaskSet*>(thread);
            taskScheduler_->WaitforTaskSet(task);
            Destroy<enki::TaskSet, M3_MEM_CATEGORY(Engine)>(task);
            return nullptr;
        };

        ecs_os_set_api(&api);
    }
}
