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
    // @todo state가 bit operator로 결합될수있기에, 이 점을 주의할것!

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
            internal::RGSTexture{
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
            internal::RGSBuffer{
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

    bool RenderGraphScheduler::DFS(Vector<uint16>& nodeDepths, uint16& maxNodeDepth) {
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

            internal::RGSDepth& depth = depths_[nodeDepths[nodeIdx]];
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
                internal::RGSResourceVersion& version = container[handle.Index].Versions[handle.Version];
                version.FirstUsedDepth = std::min(version.FirstUsedDepth, nodeDepth);

                if ((!version.IsUsedByAsyncCompute() || version.LastUsedAsyncComputeDepth <= nodeDepth) && bIsNodeExecutedOnAsyncCompute) {
                    version.LastUsedAsyncComputeDepth = nodeDepth;
                }
            }
        };

        // 1. 모든 노드를 순회하며 해당 노드에서 사용하는 모든 리소스들에 대해 최초 사용 Depth, 마지막으로 사용된 Async Compute 워크로드 포함 Depth를 기록
        for (uint32 nodeIdx = 0; nodeIdx < nodes_.size(); ++nodeIdx) {
            const uint16             nodeDepth = nodeDepths[nodeIdx];
            const internal::RGSNode& node = nodes_[nodeIdx];
            UpdateResourceVersionDepthInfo(node.ReadTextures, textures_, nodeDepth, node.bUseAsyncCompute);
            UpdateResourceVersionDepthInfo(node.WriteTextures, textures_, nodeDepth, node.bUseAsyncCompute);
            UpdateResourceVersionDepthInfo(node.ReadBuffers, buffers_, nodeDepth, node.bUseAsyncCompute);
            UpdateResourceVersionDepthInfo(node.WriteBuffers, buffers_, nodeDepth, node.bUseAsyncCompute);
        }
    }

    void RenderGraphScheduler::CollectDepthStateTransitionsWithSyncPoint(const Vector<uint16>& nodeDepths) {
        constexpr auto CollectData =
                [](const auto& handles, const auto& resourceContainer, internal::RGSDepth& depth, auto& transitionContainer, const uint16 nodeDepth) {
            for (const auto handle: handles) {
                const auto&                         resource = resourceContainer[handle.Index];
                const internal::RGSResourceVersion& version = resource.Versions[handle.Version];
                if (version.FirstUsedDepth != nodeDepth) {
                    continue;
                }

                const bool                          bIsFirstVersion = handle.Version == 0;
                const internal::RGSResourceVersion& transitionOriginVersion =
                        bIsFirstVersion ? resource.Versions.back() : resource.Versions[handle.Version - 1];
                transitionContainer.emplace_back(
                    internal::RGSResourceStateTransition{
                        .ResourceIndex = handle.Index,
                        .bIsFirstTransition = bIsFirstVersion,
                        .Before = transitionOriginVersion.State, .After = version.State
                    }
                );

                if (bIsFirstVersion) {
                    continue;
                }

                const internal::RGSResourceVersion& prevVersion = resource.Versions[handle.Version - 1];
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

            internal::RGSDepth&      depth = depths_[nodeDepth];
            const internal::RGSNode& node = nodes_[nodeIdx];
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

        const internal::RGSNode& node = nodes_[nodeIdx];
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
                internal::RGSResourceVersion& version = resource.Versions[handle.Version];

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

        Vector<uint16> nodeDepths;
        uint16         maxNodeDepth = 0;
        if (!DFS(nodeDepths, maxNodeDepth)) {
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
            const internal::RGSDepth& depth = depths_[depthIdx];
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
                    const internal::RGSNode& node = nodes_[nodeIdx];
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
                    const internal::RGSNode& node = nodes_[nodeIdx];
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

    ERenderGraphCompileResult RenderGraph::Compile(flecs::world& world, const flecs::system precedingSystem) {
        M3_PRE_COND(!passes_.empty());
        M3_PRE_COND(renderDevice_ != nullptr);
        M3_PRE_COND(precedingSystem.is_alive());

        if (bIsCompiled_) {
            bIsCompiled_ = false;
            bIsFirstExecutionAfterCompile_ = true;
            ClearCompiledData();
        }

        for (RenderPass* pass: passes_) {
            scheduler_.BeginNewPass(pass->IsEnabled(), pass->GetName());
            pass->Setup(scheduler_);
        }

        if (scheduler_.Schedule() == ERenderGraphScheduleResult::FoundCycleInGraph) {
            return ERenderGraphCompileResult::FoundCycleInGraph;
        }

        M3_ASSERT(depths_.empty());
        depths_.reserve(scheduler_.depths_.size());
        for (const internal::RGSDepth& depthInfo: scheduler_.depths_) {
            internal::RGDepth& depth = depths_.emplace_back();
            depth.GraphicsCmdLists.resize(depthInfo.GraphicsWorkloadNodes.size());
            depth.GraphicsCmdListsToSubmit.reserve(depthInfo.GraphicsWorkloadNodes.size());
            depth.AsyncComputeCmdLists.resize(depthInfo.AsyncComputeWorkloadNodes.size());
            depth.AsyncComputeCmdListsToSubmit.reserve(depthInfo.AsyncComputeWorkloadNodes.size());

            depth.StateTransitionCmdList = renderDevice_->createCommandList();
            depth.StateTransitionCmdList->setEnableAutomaticBarriers(false);

            for (nvrhi::CommandListHandle& cmdList: depth.GraphicsCmdLists) {
                cmdList = renderDevice_->createCommandList(nvrhi::CommandListParameters{.queueType = nvrhi::CommandQueue::Graphics});
                depth.GraphicsCmdListsToSubmit.emplace_back(cmdList);
            }
            for (nvrhi::CommandListHandle& cmdList: depth.AsyncComputeCmdLists) {
                cmdList = renderDevice_->createCommandList(nvrhi::CommandListParameters{.queueType = nvrhi::CommandQueue::Compute});
                depth.AsyncComputeCmdListsToSubmit.emplace_back(cmdList);
            }
        }

        // @2026-03-21 스케줄러에 포함된 리소스 정보에 따라 리소스 생성
        // if ExternalResource != nullptr -> ExternalResource 사용
        M3_ASSERT(textures_.empty());
        for (const internal::RGSTexture& schedulerTexture: scheduler_.textures_) {
            if (schedulerTexture.ExternalResource != nullptr) {
                // @2026-03-23 스케줄러에서 실제로 사용되지 않으니 여기로 move 시켜야하나?
                textures_.emplace_back(schedulerTexture.ExternalResource);
            } else {
                textures_.emplace_back(renderDevice_->createTexture(schedulerTexture.Desc));
            }
        }

        M3_ASSERT(buffers_.empty());
        for (const internal::RGSBuffer& schedulerBuffer: scheduler_.buffers_) {
            if (schedulerBuffer.ExternalResource != nullptr) {
                buffers_.emplace_back(schedulerBuffer.ExternalResource);
            } else {
                buffers_.emplace_back(renderDevice_->createBuffer(schedulerBuffer.Desc));
            }
        }

        // Build Execution Graph!
        M3_ASSERT(depthComponents_.empty());
        M3_ASSERT(depthExecutionSystems_.empty());
        M3_ASSERT(passEntities_.empty());
        depthComponents_.reserve(scheduler_.depths_.size());
        depthExecutionSystems_.reserve(scheduler_.depths_.size());
        depthSubmissionTasks_.reserve(scheduler_.depths_.size());
        passEntities_.resize(passes_.size());

        constexpr std::string_view kDepthEntityNameFormat = "RG.Depth{}";
        constexpr std::string_view kPassEntityNameFormat = "RG.Pass{}";
        constexpr std::string_view kDepthExecutionSystemNameFormat = "RG.Depth{}.Execute";
        constexpr std::string_view kDepthSubmissionTaskNameFormat = "RG.Depth{}.Submit";
        for (size_t depthIdx = 0; depthIdx < scheduler_.depths_.size(); ++depthIdx) {
            // 하나의 Depth를 일종의 가상의 Tag Component로 취급
            //< 해당 Depth를 가상의 Component로 생성
            const flecs::entity depthComponent = world.component(std::format(kDepthEntityNameFormat, depthIdx).c_str());
            depthComponents_.emplace_back(depthComponent);

            //< 하나의 Pass를 특정 Depth 컴포넌트와 실행에 필요한 데이터를 가지는 하나의 엔티티로 표현
            const internal::RGSDepth& depth = scheduler_.depths_[depthIdx];
            uint16                    graphicsWorkloadCounter = 0;
            uint16                    asyncComputeWorkloadCounter = 0;
            for (const uint32 passIdx: depth.Nodes) {
                const bool bIsAsyncComputeWorkload = scheduler_.nodes_[passIdx].bUseAsyncCompute;
                passEntities_[passIdx] = world.entity(std::format(kPassEntityNameFormat, passIdx).c_str())
                        .add(depthComponent)
                        .set(internal::RGExecution{
                            passIdx,
                            bIsAsyncComputeWorkload ? asyncComputeWorkloadCounter : graphicsWorkloadCounter,
                            bIsAsyncComputeWorkload
                        });

                if (bIsAsyncComputeWorkload) {
                    ++asyncComputeWorkloadCounter;
                } else {
                    ++graphicsWorkloadCounter;
                }
            }

            // Outer dependency system -> Depth 0 Exec -> Depth 0 Submit -> Depth 1 Exec -> Depth 1 Submit ...
            //< 하나의 Pass가 특정 Depth Component와 실행에 필요한 데이터를 가지는 엔티티이므로, 해당 엔티티들을 선별하여 실행
            //< 즉, 커맨드 레코딩이 실행되는 시스템을 정의
            //< @warning 각 패스들의 커맨드 레코딩은 여러 워커 스레드에 의해 동시적으로 수행 될 수 있으므로 race condition에 유의!
            const flecs::entity depthExecutionSystem = world.system<internal::RGExecution>(std::format(kDepthExecutionSystemNameFormat, depthIdx).c_str())
                    .with(depthComponent)
                    .multi_threaded()
                    .each([this, depthIdx]([[maybe_unused]] flecs::iter& itr, [[maybe_unused]] size_t idx, const internal::RGExecution& execution) {
                        RenderPass* pass = this->passes_[execution.PassIdx];
                        M3_ASSERT(pass != nullptr);
                        pass->Execute(*this, execution.bIsAsyncComputeWorkload
                                                 ? *this->depths_[depthIdx].AsyncComputeCmdLists[execution.WorkloadIdx]
                                                 : *this->depths_[depthIdx].GraphicsCmdLists[execution.WorkloadIdx]);
                        // Command List 할당->pass의 Execute에 전달->depthExecutionSystem 다음으로 SubmitTask로 동기화 및 제출 작업 진행
                    });

            if (depthIdx == 0) {
                depthExecutionSystem.depends_on(precedingSystem);
            } else {
                depthExecutionSystem.depends_on(depthSubmissionTasks_[depthIdx - 1]);;
            }
            depthExecutionSystems_.emplace_back(depthExecutionSystem);

            //< 앞서 Depth Execution System에 의해 기록된 커맨드 리스트를 종합하여 실제로 GPU Command Queue에 제출
            //< 컴파일 과정에 알아낸 정보를 기반으로, 리소스 상태 전이를 비롯한 Graphics Queue와 Async Compute Queue간의 동기화 또한 수행
            const flecs::system depthSubmissionTask = world.system(std::format(kDepthSubmissionTaskNameFormat, depthIdx).c_str())
                    .run([this, depthIdx]([[maybe_unused]] flecs::iter& itr) {
                        internal::RGDepth&        depthToSubmit = this->depths_[depthIdx];
                        const internal::RGSDepth& depthInfoToSubmit = this->scheduler_.depths_[depthIdx];

                        //< Sync with Async Compute Queue
                        if (depthInfoToSubmit.TargetDepthToWaitAsyncCompute != internal::kRGSInvalidDepth) {
                            const internal::RGDepth& depthToWait = this->depths_[depthInfoToSubmit.TargetDepthToWaitAsyncCompute];
                            M3_ASSERT(depthToWait.AsyncComputeSyncPoint != internal::RGDepth::InvalidSyncPoint);
                            renderDevice_->queueWaitForCommandList(
                                nvrhi::CommandQueue::Graphics,
                                nvrhi::CommandQueue::Compute,
                                depthToWait.AsyncComputeSyncPoint);
                        }

                        //< State Transition Command List Opened
                        depthToSubmit.StateTransitionCmdList->open();
                        bool   bAnyTransitionsExist = false;
                        uint64 stateTransitionSyncPoint = internal::RGDepth::InvalidSyncPoint;
                        for (const internal::RGSResourceStateTransition& textureTransition: depthInfoToSubmit.TextureTransitions) {
                            if (this->bIsFirstExecutionAfterCompile_ && textureTransition.bIsFirstTransition) {
                                continue;
                            }

                            if (textureTransition.Before == textureTransition.After) {
                                if (textureTransition.Before == nvrhi::ResourceStates::UnorderedAccess) {
                                    depthToSubmit.StateTransitionCmdList->setEnableUavBarriersForTexture(
                                        this->textures_[textureTransition.ResourceIndex].Get(),
                                        true);

                                    nvrhi::utils::TextureUavBarrier(
                                        depthToSubmit.StateTransitionCmdList.Get(),
                                        this->textures_[textureTransition.ResourceIndex].Get());
                                }
                            } else {
                                depthToSubmit.StateTransitionCmdList->beginTrackingTextureState(
                                    this->textures_[textureTransition.ResourceIndex].Get(),
                                    nvrhi::TextureSubresourceSet{},
                                    textureTransition.Before);
                                depthToSubmit.StateTransitionCmdList->setTextureState(
                                    this->textures_[textureTransition.ResourceIndex].Get(),
                                    nvrhi::TextureSubresourceSet{},
                                    textureTransition.After);
                            }

                            bAnyTransitionsExist = true;
                        }

                        for (const internal::RGSResourceStateTransition& bufferTransition: depthInfoToSubmit.BufferTransitions) {
                            if (this->bIsFirstExecutionAfterCompile_ && bufferTransition.bIsFirstTransition) {
                                continue;
                            }

                            if (bufferTransition.Before == bufferTransition.After) {
                                if (bufferTransition.Before == nvrhi::ResourceStates::UnorderedAccess) {
                                    depthToSubmit.StateTransitionCmdList->setEnableUavBarriersForBuffer(
                                        this->buffers_[bufferTransition.ResourceIndex].Get(),
                                        true);

                                    nvrhi::utils::BufferUavBarrier(
                                        depthToSubmit.StateTransitionCmdList.Get(),
                                        this->buffers_[bufferTransition.ResourceIndex].Get());
                                }
                            } else {
                                depthToSubmit.StateTransitionCmdList->beginTrackingBufferState(
                                    this->buffers_[bufferTransition.ResourceIndex].Get(),
                                    bufferTransition.Before);
                                depthToSubmit.StateTransitionCmdList->setBufferState(
                                    this->buffers_[bufferTransition.ResourceIndex].Get(),
                                    bufferTransition.After);
                            }

                            bAnyTransitionsExist = true;
                        }
                        depthToSubmit.StateTransitionCmdList->commitBarriers();
                        depthToSubmit.StateTransitionCmdList->close();
                        //< State Transition Command Lit Closed

                        //< Submit Resource State Transitions
                        if (bAnyTransitionsExist) {
                            stateTransitionSyncPoint = renderDevice_->executeCommandList(depthToSubmit.StateTransitionCmdList.Get());
                        }

                        if ((depthIdx + 1) == this->depths_.size()) {
                            this->bIsFirstExecutionAfterCompile_ = false;
                        }

                        //< Submit the Current Depth's Graphics Workload
                        depthToSubmit.GraphicsSyncPoint = renderDevice_->executeCommandLists(
                            depthToSubmit.GraphicsCmdListsToSubmit.data(),
                            depthToSubmit.GraphicsCmdListsToSubmit.size(),
                            nvrhi::CommandQueue::Graphics);

                        //< Submit the Current Depth's Async Compute Workload (wait for the state transitions on graphics queue)
                        if (depthInfoToSubmit.ShouldAsyncComputeWaitStateTransitions()) {
                            M3_ASSERT(!depthToSubmit.AsyncComputeCmdLists.empty());
                            M3_ASSERT(stateTransitionSyncPoint != internal::RGDepth::InvalidSyncPoint);
                            renderDevice_->queueWaitForCommandList(
                                nvrhi::CommandQueue::Compute,
                                nvrhi::CommandQueue::Graphics,
                                stateTransitionSyncPoint);
                            depthToSubmit.AsyncComputeSyncPoint = renderDevice_->executeCommandLists(
                                depthToSubmit.AsyncComputeCmdListsToSubmit.data(),
                                depthToSubmit.AsyncComputeCmdListsToSubmit.size(),
                                nvrhi::CommandQueue::Compute);
                        }
                    });

            depthSubmissionTask.depends_on(depthExecutionSystem);

            depthSubmissionTasks_.emplace_back(depthSubmissionTask);
        }

        bIsCompiled_ = true;
        return ERenderGraphCompileResult::Success;
    }

    void RenderGraph::Clear() {
        passes_.clear();
        ClearCompiledData();
        bIsCompiled_ = false;
    }

    void RenderGraph::ClearCompiledData() {
        // @todo 마지막 실행 지점을 기다려야하는가?

        // @todo 2026-03-31 텍스처나 버퍼의 경우엔 필요에 따라 캐싱해두고 다시 사용 할 수 있도록 하기
        textures_.clear();
        buffers_.clear();

        // @todo Depth의 경우 내부 리스트만 clear 해두고 재활용하는 방식 고려
        depths_.clear();

        for (flecs::entity& depthEntity: depthComponents_) {
            depthEntity.destruct();
        }
        depthComponents_.clear();

        for (flecs::system& depthExecutionSystem: depthExecutionSystems_) {
            depthExecutionSystem.destruct();
        }
        depthExecutionSystems_.clear();

        for (flecs::system& depthSubmissionTask: depthSubmissionTasks_) {
            depthSubmissionTask.destruct();
        }
        depthSubmissionTasks_.clear();

        for (flecs::entity passEntity: passEntities_) {
            passEntity.destruct();
        }
        passEntities_.clear();

        scheduler_.Clear();

        renderDevice_->runGarbageCollection();
    }
}
