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
#include <Mon3tr/Render/RenderGraph.hpp>

namespace mon3tr::render {
    static constexpr bool IsBufferWriteState([[maybe_unused]] const nvrhi::ResourceStates state) {
        switch (state) {
            case nvrhi::ResourceStates::ConstantBuffer:
            case nvrhi::ResourceStates::UnorderedAccess:
            case nvrhi::ResourceStates::CopyDest:
            case nvrhi::ResourceStates::AccelStructWrite:
            case nvrhi::ResourceStates::StreamOut:
                return true;
            default:
                return false;
        }
    }

    static constexpr bool IsBufferReadState([[maybe_unused]] const nvrhi::ResourceStates state) {
        switch (state) {
            case nvrhi::ResourceStates::ConstantBuffer:
            case nvrhi::ResourceStates::VertexBuffer:
            case nvrhi::ResourceStates::IndexBuffer:
            case nvrhi::ResourceStates::CopySource:
            case nvrhi::ResourceStates::AccelStructRead:
            case nvrhi::ResourceStates::ShaderResource:
                return true;
            default:
                return false;
        }
    }

    static constexpr bool IsTextureWriteState([[maybe_unused]] const nvrhi::ResourceStates state) {
        switch (state) {
            case nvrhi::ResourceStates::UnorderedAccess:
            case nvrhi::ResourceStates::DepthWrite:
            case nvrhi::ResourceStates::CopyDest:
            case nvrhi::ResourceStates::ResolveDest:
            case nvrhi::ResourceStates::RenderTarget:
                return true;
            default:
                return false;
        }
    }

    static constexpr bool IsTextureReadState([[maybe_unused]] const nvrhi::ResourceStates state) {
        switch (state) {
            case nvrhi::ResourceStates::ShaderResource:
            case nvrhi::ResourceStates::DepthRead:
            case nvrhi::ResourceStates::ResolveSource:
            case nvrhi::ResourceStates::Present:
            case nvrhi::ResourceStates::CopySource:
                return true;
            default:
                return false;
        }
    }

    void RenderGraphScheduler::BeginNewPass(const bool bIsEnabled, const std::string_view debugName) {
        nodes_.emplace_back();
        nodes_.back().DebugName = debugName;
        bIsCurrentNodeEnabled_ = bIsEnabled;
    }

    void RenderGraphScheduler::UseAsyncCompute() {
        nodes_.back().bUseAsyncCompute = true;
    }

    RGTextureHandle RenderGraphScheduler::CreateTexture(const nvrhi::TextureDesc& desc) {
        if (!bIsCurrentNodeEnabled_) {
            return {};
        }

        textures_.emplace_back(desc);
        return RGTextureHandle{
            .Index = static_cast<uint32>(textures_.size() - 1),
            .Version = 0
        };
    }

    RGBufferHandle RenderGraphScheduler::CreateBuffer(const nvrhi::BufferDesc& desc) {
        if (!bIsCurrentNodeEnabled_) {
            return {};
        }

        buffers_.emplace_back(desc);
        return RGBufferHandle{
            .Index = static_cast<uint32>(buffers_.size() - 1),
            .Version = 0
        };
    }

    RGTextureHandle RenderGraphScheduler::CreateExternalTexture(nvrhi::TextureHandle texture) {
        M3_PRE_COND(texture != nullptr);

        // 외부 리소스에 대해서는 현재 노드가 비활성화 되어있더라도 새로운 핸들을 만들어주지 않을 이유가 없음.
        textures_.emplace_back(
            RGSTexture{
                .Desc = texture->getDesc(),
                .ExternalResource = std::move(texture)
            });
        return RGTextureHandle{
            .Index = static_cast<uint32>(textures_.size() - 1),
            .Version = 0
        };
    }

    RGBufferHandle RenderGraphScheduler::CreateExternalBuffer(nvrhi::BufferHandle buffer) {
        M3_PRE_COND(buffer != nullptr);

        buffers_.emplace_back(
            RGSBuffer{
                .Desc = buffer->getDesc(),
                .ExternalResource = std::move(buffer)
            });
        return RGBufferHandle{
            .Index = static_cast<uint32>(buffers_.size() - 1),
            .Version = 0
        };
    }

    RGTextureHandle RenderGraphScheduler::WriteTexture(const RGTextureHandle texture, const nvrhi::ResourceStates state) {
        M3_PRE_COND(IsTextureWriteState(state));

        // 자연스럽게 후속 패스에 전달되지 않도록 함.
        if (!texture.IsValid() || !bIsCurrentNodeEnabled_) {
            return {};
        }

        return WriteTo(textures_, texture, state);
    }

    RGBufferHandle RenderGraphScheduler::WriteBuffer(const RGBufferHandle buffer, const nvrhi::ResourceStates state) {
        M3_PRE_COND(IsBufferWriteState(state));

        if (!buffer.IsValid() || !bIsCurrentNodeEnabled_) {
            return {};
        }

        return WriteTo(buffers_, buffer, state);
    }

    RGTextureHandle RenderGraphScheduler::ReadTexture(const RGTextureHandle texture, const nvrhi::ResourceStates state) {
        M3_PRE_COND(IsTextureReadState(state));

        if (!texture.IsValid() || !bIsCurrentNodeEnabled_) {
            return {};
        }

        return ReadFrom(textures_, texture, state);
    }

    RGBufferHandle RenderGraphScheduler::ReadBuffer(const RGBufferHandle buffer, const nvrhi::ResourceStates state) {
        M3_PRE_COND(IsBufferReadState(state));

        if (!buffer.IsValid() || !bIsCurrentNodeEnabled_) {
            return {};
        }

        return ReadFrom(buffers_, buffer, state);
    }

    bool RenderGraphScheduler::TopologicalSort(Vector<uint16>& nodeDepths, uint16& maxNodeDepth) {
        M3_PRE_COND(nodeDepths.empty());
        nodeDepths.resize(nodes_.size());
        std::ranges::fill(nodeDepths.begin(), nodeDepths.end(), internal::kRGSInvalidDepth);

        Vector<bool> visitedFlags;
        Vector<bool> onStackFlags;

        visitedFlags.resize(nodes_.size());
        onStackFlags.resize(nodes_.size());

        for (uint32 nodeIdx = 0; nodeIdx < nodes_.size(); ++nodeIdx) {
            onStackFlags[nodeIdx] = true;
            const bool bSucceeded = VisitNode(nodeIdx, visitedFlags, onStackFlags, 0, nodeDepths, maxNodeDepth);
            onStackFlags[nodeIdx] = false;
            if (!bSucceeded) {
                return false;
            }
        }

        return true;
    }

    void RenderGraphScheduler::GatherNodesToDepth(const Vector<uint16>& nodeDepths, const uint16 maxNodeDepth) {
        M3_ASSERT(maxNodeDepth > 0 && maxNodeDepth != internal::kRGSInvalidDepth);
        depths_.resize(maxNodeDepth + 1);
        for (uint32 nodeIdx = 0; nodeIdx < nodes_.size(); ++nodeIdx) {
            if (nodeDepths[nodeIdx] == internal::kRGSInvalidDepth) {
                continue;
            }

            RGSDepth& depth = depths_[nodeDepths[nodeIdx]];
            depth.Nodes.emplace_back(nodeIdx);

            if (nodes_[nodeIdx].bUseAsyncCompute) {
                depth.AsyncComputeWorkloadNodes.emplace_back(nodeIdx);
            } else {
                depth.GraphicsWorkloadNodes.emplace_back(nodeIdx);
            }
        }
    }

    void RenderGraphScheduler::UpdatePerResourceVersionDepthInfo(const Vector<uint16>& nodeDepths) {
        constexpr auto UpdateResourceVersionDepthInfo = [](const auto& handles, auto& container, const uint16 nodeDepth,
                                                           const bool  bIsNodeExecutedOnAsyncCompute) {
            for (const auto handle: handles) {
                RGSResourceVersion& version = container[handle.Index].Versions[handle.Version];
                version.FirstUsedDepth = std::min(version.FirstUsedDepth, nodeDepth);

                if ((!version.IsUsedByAsyncCompute() || version.LastUsedAsyncComputeDepth <= nodeDepth) && bIsNodeExecutedOnAsyncCompute) {
                    version.LastUsedAsyncComputeDepth = nodeDepth;
                }
            }
        };

        // 1. 모든 노드를 순회하며 해당 노드에서 사용하는 모든 리소스들에 대해 최초 사용 Depth, 마지막으로 사용된 Async Compute 워크로드 포함 Depth를 기록
        for (uint32 nodeIdx = 0; nodeIdx < nodes_.size(); ++nodeIdx) {
            const uint16   nodeDepth = nodeDepths[nodeIdx];
            const RGSNode& node = nodes_[nodeIdx];
            UpdateResourceVersionDepthInfo(node.ReadTextures, textures_, nodeDepth, node.bUseAsyncCompute);
            UpdateResourceVersionDepthInfo(node.WriteTextures, textures_, nodeDepth, node.bUseAsyncCompute);
            UpdateResourceVersionDepthInfo(node.ReadBuffers, buffers_, nodeDepth, node.bUseAsyncCompute);
            UpdateResourceVersionDepthInfo(node.WriteBuffers, buffers_, nodeDepth, node.bUseAsyncCompute);
        }
    }

    void RenderGraphScheduler::CollectDepthStateTransitionsWithSyncPoint(const Vector<uint16>& nodeDepths) {
        constexpr auto CollectData =
                [](const auto& handles, const auto& resourceContainer, RGSDepth& depth, auto& transitionContainer, const uint16 nodeDepth) {
            for (const auto handle: handles) {
                const auto&               resource = resourceContainer[handle.Index];
                const RGSResourceVersion& version = resource.Versions[handle.Version];
                if (version.FirstUsedDepth != nodeDepth) {
                    continue;
                }
                transitionContainer.emplace_back(handle);

                if (handle.Version == 0) {
                    continue;
                }
                const RGSResourceVersion& prevVersion = resource.Versions[handle.Version - 1];
                if (prevVersion.IsUsedByAsyncCompute()) {
                    if (depth.TargetDepthToWaitAsyncCompute == internal::kRGSInvalidDepth) {
                        depth.TargetDepthToWaitAsyncCompute = prevVersion.LastUsedAsyncComputeDepth;
                    } else {
                        depth.TargetDepthToWaitAsyncCompute = std::max(depth.TargetDepthToWaitAsyncCompute, prevVersion.LastUsedAsyncComputeDepth);
                    }
                }
            }
        };

        for (uint32 nodeIdx = 0; nodeIdx < nodes_.size(); ++nodeIdx) {
            const uint16 nodeDepth = nodeDepths[nodeIdx];
            if (nodeDepth == internal::kRGSInvalidDepth) {
                continue;
            }

            RGSDepth&      depth = depths_[nodeDepth];
            const RGSNode& node = nodes_[nodeIdx];
            CollectData(node.ReadTextures, textures_, depth, depth.TextureTransitions, nodeDepth);
            CollectData(node.WriteTextures, textures_, depth, depth.TextureTransitions, nodeDepth);
            CollectData(node.ReadBuffers, buffers_, depth, depth.BufferTransitions, nodeDepth);
            CollectData(node.WriteBuffers, buffers_, depth, depth.BufferTransitions, nodeDepth);
        }
    }

    bool RenderGraphScheduler::VisitNode(const uint32 nodeIdx, Vector<bool>& visited, Vector<bool>& onStack, const uint16 depth, Vector<uint16>& nodeDepths,
                                         uint16&      maxNodeDepth) {
        M3_PRE_COND(nodeIdx < nodes_.size());
        M3_PRE_COND(nodes_.size() == visited.size());
        M3_PRE_COND(nodes_.size() == onStack.size());
        M3_PRE_COND(depth != internal::kRGSInvalidDepth);

        const RGSNode& node = nodes_[nodeIdx];
        // culled node
        if (!node.HasAnyReadDependency() && !node.HasAnyWriteDependency()) {
            return true;
        }

        nodeDepths[nodeIdx] = (nodeDepths[nodeIdx] == internal::kRGSInvalidDepth) ? depth : std::max(nodeDepths[nodeIdx], depth);
        maxNodeDepth = std::max(maxNodeDepth, depth);

        if (visited[nodeIdx]) {
            return true;
        }

        auto VisitSubsequentNodes = [&](const auto& handles, auto& resourceContainer) {
            for (const auto handle: handles) {
                M3_ASSERT(handle.Index < resourceContainer.size());
                auto& resource = resourceContainer[handle.Index];
                M3_ASSERT(handle.Version < resource.Versions.size());
                RGSResourceVersion& version = resource.Versions[handle.Version];

                for (const uint32 subsequentNodeIdx: version.SubsequentNodeDependencies) {
                    // Cycle detected
                    if (onStack[subsequentNodeIdx]) {
                        return false;
                    }

                    onStack[subsequentNodeIdx] = true;
                    if (!VisitNode(subsequentNodeIdx, visited, onStack, depth + 1, nodeDepths, maxNodeDepth)) {
                        return false;
                    }
                    onStack[subsequentNodeIdx] = false;
                }
            }

            return true;
        };

        if (!VisitSubsequentNodes(node.WriteTextures, textures_)) {
            return false;
        }

        if (!VisitSubsequentNodes(node.ReadTextures, textures_)) {
            return false;
        }

        if (!VisitSubsequentNodes(node.WriteBuffers, buffers_)) {
            return false;
        }

        if (!VisitSubsequentNodes(node.ReadBuffers, buffers_)) {
            return false;
        }

        visited[nodeIdx] = true;
        return true;
    }

    ERenderGraphScheduleResult RenderGraphScheduler::Schedule() {
        M3_PRE_COND(!nodes_.empty());
        // Topological Sorting을 사용하여 노드의 depth 파악
        // 같은 Depth에 속하는 노드는 실행 순서에 상관이 없으며, Depth 자체가 의존성에 근거한 실제 실행 순서를 의미하게됨
        // 또한 같은 Depth에 속하는 노드들은 병렬적으로 명령어 기록 및 제출을 통한 실행(별도의 동기화 없이도)이 가능함

        // 뎁스별 노드 분류

        // 뎁스 별 상태 전이 정보 수집
        // 상태 전이 자체는 현재 버전 이전 버전을 보기만 해도 어떤 상태에서 어떤 상태로 전이해야하는지는 알 수 있음
        // Modern GAPI에서는 Queue의 타입 마다 호환되는 전이가능한 '상태'가 있음.
        // 일반적으로 Graphics Queue는 모든 종류의 파이프라인(rasterize, mesh, compute, copy, ...)이 모두 호환되므로 모든 종류의 상태 전이가 가능함
        // 구현을 간단하게 만들려면, Depth 마다 상태 전이를 모두 모은후, Depth에 속하는 모든 패스가 완료되면 graphics queue에서 한번에 상태 전이를 마친 후
        // 상태 전이에 동기화 해서 다음 Depth들의 패스들을 실행하도록 하는 것이 좋아 보임.
        // 다만 이럴 경우에 해당 Depth에서 가장 병목지점이 되는 패스에 전체적인 성능이 제한됨

        // 노드 관계는 OK, 하지만 Depth 0에서 사용된 리소스가 항상 Depth 1에서 동기화/상태전이가 발생해야하는건 아님
        // 하지만 그렇다고 현재 꼭 실제로 사용되기 시작할 지점에서 동기화가 필요하다는 것도 아니긴함.

        // @2026-03-17 실제 동기화 시점을 어떻게 찾는가.
        // Graphics 와 Async Compute Queue를 사용한다고 가정하자
        // 정확하게는 '동기화'란 해당 Depth를 실행하기 위해 어떤 패스의 작업이 완료되어야 하는가?
        // 그리고 이런 동기화 작업을 어떻게 해야 최소화 할 수 있는가?
        // 현재 기준으로 알고있는 데이터는 '어떤 패스가 어느 뎁스에 속하는가'
        // 만약 해당 패스가 사용한 리소스가 사용되는 다음으로 가까운 뎁스가 어디인지 알 수 있다면
        // 어떤 뎁스가 어떤 패스에 대해 동기화를 진행해야 하는지 알 수 있다.

        // 하나의 Depth = 각 Queue 마다 하나의 ExecuteCommandLists로 통합
        // Async와 Graphics Queue를 하나씩 사용한다면?
        //
        // @2026-03-18
        // 각 패스의 종료 지점마다 각 큐에서 하나의 signal을 발생시킨다 가정 -> 각 패스가 개별적인 ExecuteCommandLists로 강제됨
        // -> 성능적으로 패널티 -> ExecuteCommandLists은 제출된 커맨드들을 실행한다음, 다음 ExecuteCommandLists로 제출된 커맨드를 실행하기전 필요로하는 모든 cache flush가
        // 완료됨을 보장. 실질적으로 패스별로 resource barrier가 필요 없는 경우(같은 depth) 이런 구조는 불필요

        // @ref https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#excessive-flush-operations
        // @ref https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandqueue-executecommandlists
        // 한 Depth 내의 같은 큐에서 실행될 패스들은 하나의 ExecuteCommandLists Scope로 묶는다
        // Depth에 속한 state transitions를 수행하기 전에 앞선 패스들에 대한 동기화가 필요한 경우,
        // 특히 다른 Queue(Async Queue)에 대한 동기화가 필요 할 수 있다.
        // 그냥 Depth 별 동기화가 답인가?
        // 일반적으로 Async Compute를 사용하여 렌더링 작업을 한다고 하여도, 실제 리소스 사용 지점이 그렇게 멀 가능성이 크게 없을 것 같다.
        // 그렇게 되면 결국 정리되는 방식은
        // 1. Depth에서 Queue별로 Pass들을 하나의 ExecuteCommandLists Scope로 묶는다 (Depth Scope라 부르자)
        // 2. Async Compute에서는 각 Depth Scope의 제출마다(Async Compute를 타는 패스가 있다면) Signal 커맨드를 제출 한다.
        // 3. 다음 Depth Scope State Transition 직전에 해당 Depth Scope에서 Async Compute에 관여된 리소스를 사용한다면
        // 해당 DSST 직전에 해당 리소스 버전이 마지막으로 사용된 최소 Depth Scope의 Async Compute의 Signal을 Wait 한다.
        // 4. 해당 Depth Scope에서 Async Compute가 사용된다면, ExecuteCommandLists를 수행하기 전에 DSST 작업에 대한 Wait를 수행한다.
        // gcq.ExecuteCmdLists(DSST); gcq.Signal(A); gcq.ExecuteCmdLists(...); acq.Wait(A); acq.ExecuteCmdLists(...);

        // 만약에 리소스의 버전이 사용되는 최소 Depth를 알고있다면, 반대로 어떤 Depth가 해당 Resource를 사용한 Depth와 직접적인
        // 동기화가 필요한지 여부도 알 수 있다. 예를들어 같은 상태의 버전 리소스 R을 Depth 1과 Depth 2에서 읽는다 치자
        // 만약 Depth 1 이 리소스 R에 대한 State Transition이 최초로 일어나는(해당 버전이 처음으로 사용되는 지점)이라고 한다면,
        // Depth 2 에서는 State Transition이 필요없을 뿐더러, 해당 리소스가 Async Queue에서 사용되었다 하더라도 Async Queue와 직접적인
        // 동기화 과정은 필요가 없다. (이미 Depth 1 시점에 동기화 되었으므로)

        // 1. 모든 노드를 순회하며 해당 노드에서 사용하는 모든 리소스들에 대해 최초 사용 Depth, 마지막으로 사용된 Async Compute 워크로드 포함 Depth를 기록

        // 2. 다시 한번 노드를 순회하여 해당 노드에서 사용하는 리소스 버전에 대한 핸들을 각 Depth의 Transitions 배열에 등록
        // 2-1. 단, 이 Depth가 리소스 버전의 첫 Depth와 일치하는 경우에만 등록
        // 2-2. 만약 직전 리소스 버전이 AsyncCompute에서 사용된 경우 AsyncComputeWaitTarget = max(ori, new)
        // 2-3. 만약 해당 리소스 버진이 AsyncCompute에서 사용되어야 하는 경우 bShouldAsyncComputeWait = true
        // => 해당 Depth의 패스중 애초에 Async Compute가 하나라도 포함되면으로 변경

        Vector<uint16> nodeDepths;
        uint16         maxNodeDepth = 0;
        if (!TopologicalSort(nodeDepths, maxNodeDepth)) {
            return ERenderGraphScheduleResult::FoundCycleInGraph;
        }

        GatherNodesToDepth(nodeDepths, maxNodeDepth);
        UpdatePerResourceVersionDepthInfo(nodeDepths);
        CollectDepthStateTransitionsWithSyncPoint(nodeDepths);

        return ERenderGraphScheduleResult::Success;
    }

    void RenderGraphScheduler::Clear() {
        textures_.clear();
        buffers_.clear();
        nodes_.clear();
        bIsCurrentNodeEnabled_ = false;
        depths_.clear();
    }

    std::string RenderGraphScheduler::ToString() {
        constexpr std::string_view kGraphDefFormat = "digraph G {{ rankdir=\"LR\"\n {} }}";
        constexpr std::string_view kDepthDefFormat =
                "subgraph cluster_depth_{} {{ label=\"Depth {}\n#Read Tex: {}\n#Write Tex: {}\n#Read Buf: {}\n#Write Buf:{}\" {} }}";
        constexpr std::string_view kDepthSyncPointDefFormat = "depth_{}_sync_point [label=\"Depth {} Sync Point\", shape=box]";
        constexpr std::string_view kDepthGraphicsEndPointDefFormat = "depth_{}_g_endpoint [label=\"Depth {} Graphics Workload\", shape=box]";
        constexpr std::string_view kDepthAsyncComputeEndPointDefFormat = "depth_{}_ac_endpoint [label=\"Depth {} Async Compute Workload\", shape=box]";
        constexpr std::string_view kPassNodeDefFormat = "p_{} [label=\"{}\n#Read Tex: {}\n#Write Tex: {}\n#Read Buf: {}\n#Write Buf:{}\", shape=box]";
        constexpr std::string_view kGraphicsNodeDependencyFormat = "depth_{}_sync_point->p_{}->depth_{}_g_endpoint"; // sync point to node to end point
        constexpr std::string_view kAsyncComputeNodeDependencyFormat = "depth_{}_sync_point->p_{}->depth_{}_ac_endpoint";
        constexpr std::string_view kDepthDependencyFormat = "depth_{}_g_endpoint->depth_{}_sync_point";

        constexpr auto ConcatStringVec = [](const Vector<std::string>& contents) {
            std::string result;
            for (const auto& content: contents) {
                result = result.empty() ? content : std::format("{}\n{}", result, content);
            }
            return result;
        };

        Vector<std::string> graphContents;
        graphContents.reserve(depths_.size());
        Vector<std::string> depthContents;
        for (uint16 depthIdx = 0; depthIdx < depths_.size(); ++depthIdx) {
            const RGSDepth& depth = depths_[depthIdx];
            depthContents.clear();
            depthContents.reserve(depth.Nodes.size() * 2 + 1);

            uint64 numReadTextures = 0;
            uint64 numWriteTextures = 0;
            uint64 numReadBuffers = 0;
            uint64 numWriteBuffers = 0;

            graphContents.emplace_back(std::format(kDepthSyncPointDefFormat, depthIdx, depthIdx));
            if (depthIdx > 0) {
                graphContents.emplace_back(std::format(kDepthDependencyFormat, depthIdx - 1, depthIdx));
            }

            if (depth.TargetDepthToWaitAsyncCompute != internal::kRGSInvalidDepth) {
                M3_ASSERT(depth.TargetDepthToWaitAsyncCompute < depthIdx);
                graphContents.emplace_back(std::format("depth_{}_ac_endpoint->depth_{}_sync_point", depth.TargetDepthToWaitAsyncCompute, depthIdx));
            }

            if (!depth.GraphicsWorkloadNodes.empty()) {
                graphContents.emplace_back(std::format(kDepthGraphicsEndPointDefFormat, depthIdx, depthIdx));
                for (const uint32 nodeIdx: depth.GraphicsWorkloadNodes) {
                    const RGSNode& node = nodes_[nodeIdx];
                    depthContents.emplace_back(std::format(kPassNodeDefFormat, nodeIdx, node.DebugName,
                                                           node.ReadTextures.size(), node.WriteTextures.size(),
                                                           node.ReadBuffers.size(), node.WriteBuffers.size()));
                    depthContents.emplace_back(std::format(kGraphicsNodeDependencyFormat, depthIdx, nodeIdx, depthIdx));

                    numReadTextures += node.ReadTextures.size();
                    numWriteTextures += node.WriteTextures.size();
                    numReadBuffers += node.ReadBuffers.size();
                    numWriteBuffers += node.WriteBuffers.size();
                }
            }

            if (!depth.AsyncComputeWorkloadNodes.empty()) {
                graphContents.emplace_back(std::format(kDepthAsyncComputeEndPointDefFormat, depthIdx, depthIdx));
                for (const uint32 nodeIdx: depth.AsyncComputeWorkloadNodes) {
                    const RGSNode& node = nodes_[nodeIdx];
                    depthContents.emplace_back(std::format(kPassNodeDefFormat, nodeIdx, node.DebugName,
                                                           node.ReadTextures.size(), node.WriteTextures.size(),
                                                           node.ReadBuffers.size(), node.WriteBuffers.size()));
                    depthContents.emplace_back(std::format(kAsyncComputeNodeDependencyFormat, depthIdx, nodeIdx, depthIdx));

                    numReadTextures += node.ReadTextures.size();
                    numWriteTextures += node.WriteTextures.size();
                    numReadBuffers += node.ReadBuffers.size();
                    numWriteBuffers += node.WriteBuffers.size();
                }
            }

            graphContents.emplace_back(std::format(kDepthDefFormat, depthIdx, depthIdx,
                                                   numReadTextures, numWriteTextures,
                                                   numReadBuffers, numWriteBuffers,
                                                   ConcatStringVec(depthContents)));
        }

        return std::format(kGraphDefFormat, ConcatStringVec(graphContents));
    }

    ERenderGraphCompileResult RenderGraph::Compile() {
        M3_PRE_COND(!passes_.empty());
        M3_PRE_COND(renderDevice_ != nullptr);

        scheduler_.Clear();
        for (RenderPass* pass: passes_) {
            pass->Setup(scheduler_);
        }

        if (scheduler_.Schedule() == ERenderGraphScheduleResult::FoundCycleInGraph) {
            return ERenderGraphCompileResult::FoundCycleInGraph;
        }

        // @2026-03-21 스케줄러에 포함된 리소스 정보에 따라 리소스 생성
        // if ExternalResource != nullptr -> ExternalResource 사용
        M3_ASSERT(textures_.empty());
        for (const RGSTexture& schedulerTexture: scheduler_.textures_) {
            if (schedulerTexture.ExternalResource != nullptr) {
                // @2026-03-23 스케줄러에서 실제로 사용되지 않으니 여기로 move 시켜야하나?
                textures_.emplace_back(schedulerTexture.ExternalResource);
            } else {
                textures_.emplace_back(renderDevice_->createTexture(schedulerTexture.Desc));
            }
        }

        M3_ASSERT(buffers_.empty());
        for (const RGSBuffer& schedulerBuffer: scheduler_.buffers_) {
            if (schedulerBuffer.ExternalResource != nullptr) {
                buffers_.emplace_back(schedulerBuffer.ExternalResource);
            } else {
                buffers_.emplace_back(renderDevice_->createBuffer(schedulerBuffer.Desc));
            }
        }

        return ERenderGraphCompileResult::Success;
    }

    // 2026-03-21
    // Inter-Queue Synchronization에 아래 메서드들 사용
    // renderDevice_->executeCommandLists() -> return instance id
    // renderDevice_->queueWaitForCommandList()
    // a command list per pass vs command lists per pass
    // scheduler.RequestCmdLists(N) -> Execute.. assert(NumCmdLists > 0)
    // depth -> graphics workload/async workload 각각 -> (Vector로 개수 precalc후 선행 할당, span으로 pass의 요청 subspan 전달, passes[nodeIdx]->Execute(CmdListsSpan) ->
}
