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
#include <Mon3tr/Render/RenderMinimal.hpp>

namespace mon3tr::internal {
    constexpr uint32 kRGSEmptyNode = 0xFFFFFFFF;
    constexpr uint16 kRGSInvalidDepth = 0xFFFF;
}

namespace mon3tr::render {
    enum class ERenderGraphCompileResult {
        Success,
        FoundCycleInGraph
    };

    // 두 핸들 타입이 기본적으로 같으나, 추후 확장성을 위해 분리!
    struct RGTextureHandle {
    public:
        [[nodiscard]] bool IsValid() const noexcept { return Index != InvalidIndex; }

    public:
        static constexpr uint32 InvalidIndex = 0xFFFFFFFF;
        uint32                  Index = InvalidIndex;
        uint32                  Version = 0;
    };

    struct RGBufferHandle {
    public:
        [[nodiscard]] bool IsValid() const noexcept { return Index != InvalidIndex; }

    public:
        static constexpr uint32 InvalidIndex = 0xFFFFFFFF;
        uint32                  Index = InvalidIndex;
        uint32                  Version = 0;
    };

    class RenderGraphScheduler;
    class RenderGraph;

    class RenderPass {
    public:
        virtual ~RenderPass() = default;

        virtual void Setup(RenderGraphScheduler& scheduler) = 0;

        virtual void Execute(RenderGraph& graph) = 0;
    };

    struct RGSResourceVersion {
    public:
        [[nodiscard]] bool HasWriteFromDependency() const noexcept { return WriteFromNodeDependency != internal::kRGSEmptyNode; }
        [[nodiscard]] bool HasAnySubsequentDependency() const noexcept { return !SubsequentNodeDependencies.empty(); }

        [[nodiscard]] bool IsUsedByAsyncCompute() const noexcept { return LastUsedAsyncComputeDepth != internal::kRGSInvalidDepth; }

    public:
        nvrhi::ResourceStates State = nvrhi::ResourceStates::Unknown;

        Vector<uint32, M3_MEM_CATEGORY(Render)> SubsequentNodeDependencies;
        uint32                                  WriteFromNodeDependency = internal::kRGSEmptyNode;

        // 해당 버전이 최초로 사용된 깊이
        uint16 FirstUsedDepth = internal::kRGSInvalidDepth;
        uint16 LastUsedAsyncComputeDepth = internal::kRGSInvalidDepth;
    };

    template<typename D, typename H>
    struct RGSResource {
    public:
        [[nodiscard]] bool IsExternal() const noexcept { return ExternalResource != nullptr; }

    public:
        D Desc;
        H ExternalResource;

        Vector<RGSResourceVersion, M3_MEM_CATEGORY(Render)> Versions;
    };

    using RGSTexture = RGSResource<nvrhi::TextureDesc, nvrhi::TextureHandle>;
    using RGSBuffer = RGSResource<nvrhi::BufferDesc, nvrhi::BufferHandle>;

    // RenderPass와 1대1 대응
    struct RGSNode {
    public:
        [[nodiscard]] bool HasAnyReadDependency(const RGTextureHandle texture) const {
            return HasAnyResourceDependency<RGTextureHandle>(texture, ReadTextures);
        }

        [[nodiscard]] bool HasAnyWriteDependency(const RGTextureHandle texture) const {
            return HasAnyResourceDependency<RGTextureHandle>(texture, WriteTextures);
        }

        [[nodiscard]] bool HasAnyReadDependency(const RGBufferHandle buffer) const {
            return HasAnyResourceDependency<RGBufferHandle>(buffer, ReadBuffers);
        }

        [[nodiscard]] bool HasAnyWriteDependency(const RGBufferHandle buffer) const {
            return HasAnyResourceDependency<RGBufferHandle>(buffer, WriteBuffers);
        }

        [[nodiscard]] bool HasAnyReadDependency() const noexcept { return !ReadTextures.empty() || !ReadBuffers.empty(); }
        [[nodiscard]] bool HasAnyWriteDependency() const noexcept { return !WriteTextures.empty() || !WriteBuffers.empty(); }

        void WriteTo(const RGTextureHandle texture) {
            WriteTextures.emplace_back(texture);
        }

        void WriteTo(const RGBufferHandle buffer) {
            WriteBuffers.emplace_back(buffer);
        }

        void ReadFrom(const RGTextureHandle texture) {
            ReadTextures.emplace_back(texture);
        }

        void ReadFrom(const RGBufferHandle buffer) {
            ReadBuffers.emplace_back(buffer);
        }

    private:
        template<typename T>
        [[nodiscard]] bool HasAnyResourceDependency(const T handle, const Vector<T, M3_MEM_CATEGORY(Render)>& container) const {
            return std::ranges::find_if(container.cbegin(), container.cend(),
                                        [handle](const T& target) {
                                            return target.Index == handle.Index;
                                        }) != container.cend();
        }

    public:
        std::string DebugName;

        Vector<RGTextureHandle, M3_MEM_CATEGORY(Render)> ReadTextures;
        Vector<RGBufferHandle, M3_MEM_CATEGORY(Render)>  ReadBuffers;

        Vector<RGTextureHandle, M3_MEM_CATEGORY(Render)> WriteTextures;
        Vector<RGBufferHandle, M3_MEM_CATEGORY(Render)>  WriteBuffers;

        bool bUseAsyncCompute = false;
    };

    struct RGSDepth {
        Vector<uint32, M3_MEM_CATEGORY(Render)> Nodes;

        Vector<uint32, M3_MEM_CATEGORY(Render)> GraphicsWorkloadNodes;
        Vector<uint32, M3_MEM_CATEGORY(Render)> AsyncComputeWorkloadNodes;

        Vector<RGTextureHandle, M3_MEM_CATEGORY(Render)> TextureTransitions;
        Vector<RGBufferHandle, M3_MEM_CATEGORY(Render)>  BufferTransitions;

        // 만약, 해당 패스가 읽거나/쓰는 리소스 버전의 직전 버전이 하나라도 AsyncCompute 에서 실행되었다면, 해당 버전의 최소 깊이의 Async Compute Signal에 동기화가 필요
        // 단, 직전 버전의 최소 깊이가 자신과 같은 경우에만 해당!
        // 예를 들어 깊이 0에서 리소스 R에 Async Compute에서 쓰고, 깊이 1에서 리소스 R을 읽기(Async Compute에서), 깊이 2에서 다시 리소스 R을 읽는다면
        // 깊이 1에서 이미 깊이 0의 Async Compute Write Pass에 동기화가 완료된 시점이므로, 깊이 2에서 다시 깊이 0에 동기화 할 필요가 없다.
        // 여러 Depth의 Async Compute Workload에 걸쳐 리소스를 사용하더라도, 가장 마지막 Async Compute Workload에 대해서만 동기화가 필요함
        uint16 TargetDepthToWaitAsyncCompute = 0xFFFF;
        // 만약, 해당 패스가 읽거나/쓰는 리소스 버전 중 하나라도 AsyncCompute 에서 실행되고, 해당 버전의 최소 깊이가 현재 Depth와 동일하다면
        // Async Compute가 graphics queue에서 실행되는 wait 연산을 기다려야함.
        //bool               bShouldAsyncComputeWait = false; ///< 해당 Depth 내 어떤 패스가 Async Compute에 작업을 제출 하는가?
        [[nodiscard]] bool ShouldAsyncComputeWaitStateTransitions() const noexcept { return !AsyncComputeWorkloadNodes.empty(); }
    };

    enum class ERenderGraphScheduleResult : uint8 {
        Success,
        FoundCycleInGraph,
    };

    class RenderGraphScheduler {
        friend class RenderGraph;

    public:
        void BeginNewPass(bool bIsEnabled, std::string_view debugName = "None");

        void UseAsyncCompute();

        [[nodiscard]] RGTextureHandle CreateTexture(const nvrhi::TextureDesc& desc);

        [[nodiscard]] RGBufferHandle CreateBuffer(const nvrhi::BufferDesc& desc);

        [[nodiscard]] RGTextureHandle CreateExternalTexture(nvrhi::TextureHandle texture);

        [[nodiscard]] RGBufferHandle CreateExternalBuffer(nvrhi::BufferHandle buffer);

        [[nodiscard]] RGTextureHandle WriteTexture(RGTextureHandle texture, nvrhi::ResourceStates state);

        [[nodiscard]] RGBufferHandle WriteBuffer(RGBufferHandle buffer, nvrhi::ResourceStates state);

        [[nodiscard]] RGTextureHandle ReadTexture(RGTextureHandle texture, nvrhi::ResourceStates state);

        [[nodiscard]] RGBufferHandle ReadBuffer(RGBufferHandle buffer, nvrhi::ResourceStates state);

        ERenderGraphScheduleResult Schedule();

        void Clear();

        std::string ToString();

    private:
        bool TopologicalSort(Vector<uint16>& nodeDepths, uint16& maxNodeDepth);

        void GatherNodesToDepth(const Vector<uint16>& nodeDepths, const uint16 maxNodeDepth);

        void UpdatePerResourceVersionDepthInfo(const Vector<uint16>& nodeDepths);

        void CollectDepthStateTransitionsWithSyncPoint(const Vector<uint16>& nodeDepths);

        [[nodiscard]] uint32 GetCurrentNodeIndex() const noexcept { return static_cast<uint32>(nodes_.size() - 1); }

        template<typename RenderGraphHandle, typename SchedulerResource>
        RenderGraphHandle WriteTo(Vector<SchedulerResource, M3_MEM_CATEGORY(Render)>& container,
                                  const RenderGraphHandle                             resource,
                                  const nvrhi::ResourceStates                         state) {
            M3_PRE_COND(resource.Index < container.size());
            M3_PRE_COND(state != nvrhi::ResourceStates::Unknown);
            M3_PRE_COND(!nodes_.back().HasAnyReadDependency(resource));
            M3_PRE_COND(!nodes_.back().HasAnyWriteDependency(resource));

            SchedulerResource& targetResource = container[resource.Index];
            const bool         bIsWriteAfterCreate = resource.Version == 0 && targetResource.Versions.empty();
            M3_ASSERT(resource.Version == (targetResource.Versions.size() - 1) || bIsWriteAfterCreate);

            RGSResourceVersion& newVersion = targetResource.Versions.emplace_back();
            newVersion.WriteFromNodeDependency = GetCurrentNodeIndex();
            newVersion.State = state;

            const RenderGraphHandle newHandle = RenderGraphHandle{
                .Index = resource.Index,
                .Version = static_cast<uint32>(targetResource.Versions.size() - 1)
            };

            if (!bIsWriteAfterCreate) {
                RGSResourceVersion& baseVersion = targetResource.Versions[resource.Version];
                baseVersion.SubsequentNodeDependencies.emplace_back(GetCurrentNodeIndex());
            }

            nodes_.back().WriteTo(newHandle);

            return newHandle;
        }

        template<typename RenderGraphHandle, typename SchedulerResource>
        RenderGraphHandle ReadFrom(Vector<SchedulerResource, M3_MEM_CATEGORY(Render)>& container,
                                   const RenderGraphHandle                             resource,
                                   const nvrhi::ResourceStates                         state) {
            M3_PRE_COND(resource.Index < container.size());
            M3_PRE_COND(state != nvrhi::ResourceStates::Unknown);
            M3_PRE_COND(!nodes_.back().HasAnyReadDependency(resource));
            M3_PRE_COND(!nodes_.back().HasAnyWriteDependency(resource));

            SchedulerResource& targetResource = container[resource.Index];

            M3_ASSERT(!targetResource.Versions.empty());
            M3_ASSERT((resource.Version + 2) >= targetResource.Versions.size());

            // 전달 받은 버전 이후에 버전이 없다면 Write와 동일한 처리 (새로운 버전의 생성)
            // 전달 받은 버전의 리소스에 대해서 동일한 상태인 경우 한번에 여러 패스에서 동시에 읽을 수 있다
            if (const bool bShouldCreateNewVersion = (resource.Version + 1) == targetResource.Versions.size();
                bShouldCreateNewVersion) {
                RGSResourceVersion& newVersion = targetResource.Versions.emplace_back();
                newVersion.State = state;
            }

            M3_ASSERT(targetResource.Versions.back().State == state);
            RenderGraphHandle newHandle = RenderGraphHandle{
                .Index = resource.Index,
                .Version = static_cast<uint32>(targetResource.Versions.size() - 1)
            };
            M3_ASSERT(resource.Version == (newHandle.Version - 1));

            RGSResourceVersion& baseVersion = targetResource.Versions[resource.Version];
            baseVersion.SubsequentNodeDependencies.emplace_back(GetCurrentNodeIndex());

            nodes_.back().ReadFrom(newHandle);

            return newHandle;
        }

        bool VisitNode(uint32 nodeIdx, Vector<bool>& visited, Vector<bool>& onStack, uint16 depth, Vector<uint16>& nodeDepths, uint16& maxNodeDepth);

    private:
        // RenderGraph::textures_와 1대1 대응되어야 함
        Vector<RGSTexture, M3_MEM_CATEGORY(Render)> textures_;
        // RenderGraph::buffers_와 1대1 대응되어야 함
        Vector<RGSBuffer, M3_MEM_CATEGORY(Render)> buffers_;

        // RenderGraph::Passes와 1대1 대응 되어야함
        Vector<RGSNode, M3_MEM_CATEGORY(Render)> nodes_;

        bool bIsCurrentNodeEnabled_ = false;

        // depths_[0] = D0 -> D1 transitions 포함, 컴파일 이후 유효
        Vector<RGSDepth, M3_MEM_CATEGORY(Render)> depths_;
    };

    class RenderGraph {
    public:
        [[nodiscard]] ERenderGraphCompileResult Compile();

        void Execute();

        void Clear();

        // void Reset(); // 리소스들 및 패스들의 완전한 할당 해제

    private:
        nvrhi::DeviceHandle renderDevice_;

        bool                                                  bIsCompiled = false;
        Vector<nvrhi::TextureHandle, M3_MEM_CATEGORY(Render)> textures_;
        Vector<nvrhi::BufferHandle, M3_MEM_CATEGORY(Render)>  buffers_;

        Vector<RenderPass*, M3_MEM_CATEGORY(Render)> passes_;

        RenderGraphScheduler scheduler_;
    };
}
