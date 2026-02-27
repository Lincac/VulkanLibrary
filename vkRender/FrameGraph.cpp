#include "FrameGraph.h"

#include <algorithm>
#include <limits>
#include <queue>
#include <stdexcept>
#include <unordered_map>

namespace {
uint64_t estimateFormatBytesPerPixel(VkFormat format) {
    switch (format) {
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8_SRGB:
        return 1;
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8_SRGB:
        return 2;
    case VK_FORMAT_R8G8B8_UNORM:
    case VK_FORMAT_B8G8R8_UNORM:
    case VK_FORMAT_R8G8B8_SRGB:
    case VK_FORMAT_B8G8R8_SRGB:
        return 3;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return 8;
    default:
        return 4;
    }
}
} // namespace

FrameGraph::ResourceId FrameGraph::createImage(const std::string& name, const ImageDesc& desc) {
    ResourceNode node{};
    node.name = name;
    node.type = ResourceType::Image;
    node.image = desc;
    resources.push_back(node);
    RuntimeResource runtime{};
    runtime.imageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    runtime.imageRange.baseMipLevel = 0;
    runtime.imageRange.levelCount = VK_REMAINING_MIP_LEVELS;
    runtime.imageRange.baseArrayLayer = 0;
    runtime.imageRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    runtimeResources.push_back(runtime);
    return static_cast<ResourceId>(resources.size() - 1);
}

FrameGraph::ResourceId FrameGraph::createBuffer(const std::string& name, const BufferDesc& desc) {
    ResourceNode node{};
    node.name = name;
    node.type = ResourceType::Buffer;
    node.buffer = desc;
    resources.push_back(node);
    runtimeResources.push_back(RuntimeResource{});
    return static_cast<ResourceId>(resources.size() - 1);
}

FrameGraph::PassId FrameGraph::addPass(const std::string& name, bool sideEffect) {
    PassBuilder pass{};
    pass.pass = static_cast<PassId>(passes.size());
    pass.name = name;
    pass.sideEffect = sideEffect;
    passes.push_back(pass);
    return pass.pass;
}

void FrameGraph::setPassEnabled(PassId pass, bool enabled) {
    if (pass >= passes.size()) {
        throw std::runtime_error("FrameGraph::setPassEnabled: invalid pass id");
    }
    passes[pass].enabled = enabled;
}

void FrameGraph::setPassCallback(PassId pass, std::function<void(VkCommandBuffer)> callback) {
    if (pass >= passes.size()) {
        throw std::runtime_error("FrameGraph::setPassCallback: invalid pass id");
    }
    passes[pass].callback = std::move(callback);
}

void FrameGraph::readResource(PassId pass, const ResourceUsage& usage) {
    if (pass >= passes.size()) {
        throw std::runtime_error("FrameGraph::readResource: invalid pass id");
    }
    if (usage.resource >= resources.size()) {
        throw std::runtime_error("FrameGraph::readResource: invalid resource id");
    }
    passes[pass].reads.push_back(usage);
}

void FrameGraph::writeResource(PassId pass, const ResourceUsage& usage) {
    if (pass >= passes.size()) {
        throw std::runtime_error("FrameGraph::writeResource: invalid pass id");
    }
    if (usage.resource >= resources.size()) {
        throw std::runtime_error("FrameGraph::writeResource: invalid resource id");
    }
    passes[pass].writes.push_back(usage);
}

void FrameGraph::markResourceExported(ResourceId resource, bool exported) {
    if (resource >= resources.size()) {
        throw std::runtime_error("FrameGraph::markResourceExported: invalid resource id");
    }
    if (resources[resource].type == ResourceType::Image) {
        resources[resource].image.exported = exported;
    } else {
        resources[resource].buffer.exported = exported;
    }
}

void FrameGraph::markResourceImported(ResourceId resource, bool imported) {
    if (resource >= resources.size()) {
        throw std::runtime_error("FrameGraph::markResourceImported: invalid resource id");
    }
    if (resources[resource].type == ResourceType::Image) {
        resources[resource].image.imported = imported;
    } else {
        resources[resource].buffer.imported = imported;
    }
}

void FrameGraph::bindImage(ResourceId resource, VkImage image, VkImageSubresourceRange subresourceRange) {
    if (resource >= resources.size()) {
        throw std::runtime_error("FrameGraph::bindImage: invalid resource id");
    }
    if (resources[resource].type != ResourceType::Image) {
        throw std::runtime_error("FrameGraph::bindImage: resource is not image");
    }
    if (resource >= runtimeResources.size()) {
        throw std::runtime_error("FrameGraph::bindImage: runtime resource table mismatch");
    }

    runtimeResources[resource].image = image;
    runtimeResources[resource].imageRange = subresourceRange;
}

void FrameGraph::bindBuffer(
    ResourceId resource,
    VkBuffer buffer,
    VkDeviceSize offset,
    VkDeviceSize size) {
    if (resource >= resources.size()) {
        throw std::runtime_error("FrameGraph::bindBuffer: invalid resource id");
    }
    if (resources[resource].type != ResourceType::Buffer) {
        throw std::runtime_error("FrameGraph::bindBuffer: resource is not buffer");
    }
    if (resource >= runtimeResources.size()) {
        throw std::runtime_error("FrameGraph::bindBuffer: runtime resource table mismatch");
    }

    runtimeResources[resource].buffer = buffer;
    runtimeResources[resource].bufferOffset = offset;
    runtimeResources[resource].bufferSize = size;
}

void FrameGraph::compile() {
    compileResult = {};

    const size_t passCount = passes.size();
    const size_t resourceCount = resources.size();
    if (passCount == 0) {
        compileResult.valid = true;
        return;
    }

    std::vector<std::vector<PassId>> deps(passCount);
    std::vector<std::vector<PassId>> reverseDeps(passCount);

    struct UseRef {
        PassId pass = 0;
        bool isWrite = false;
        ResourceUsage usage{};
    };

    std::vector<std::vector<UseRef>> resourceUses(resourceCount);
    for (const PassBuilder& pass : passes) {
        if (!pass.enabled) {
            continue;
        }
        for (const ResourceUsage& read : pass.reads) {
            resourceUses[read.resource].push_back({pass.pass, false, read});
        }
        for (const ResourceUsage& write : pass.writes) {
            resourceUses[write.resource].push_back({pass.pass, true, write});
        }
    }

    for (std::vector<UseRef>& uses : resourceUses) {
        std::sort(uses.begin(), uses.end(), [](const UseRef& a, const UseRef& b) {
            if (a.pass != b.pass) {
                return a.pass < b.pass;
            }
            return static_cast<uint8_t>(a.isWrite) < static_cast<uint8_t>(b.isWrite);
        });

        for (size_t i = 0; i < uses.size(); ++i) {
            for (size_t j = i + 1; j < uses.size(); ++j) {
                if (!hasWriteHazard(uses[i].usage.access, uses[j].usage.access)) {
                    continue;
                }
                const PassId src = uses[i].pass;
                const PassId dst = uses[j].pass;
                if (src == dst) {
                    continue;
                }
                deps[src].push_back(dst);
                reverseDeps[dst].push_back(src);
            }
        }
    }

    for (size_t i = 0; i < passCount; ++i) {
        auto& out = deps[i];
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());

        auto& in = reverseDeps[i];
        std::sort(in.begin(), in.end());
        in.erase(std::unique(in.begin(), in.end()), in.end());
    }

    std::vector<bool> required(passCount, false);
    std::queue<PassId> backtrackQueue;

    for (const PassBuilder& pass : passes) {
        if (pass.enabled && pass.sideEffect) {
            required[pass.pass] = true;
            backtrackQueue.push(pass.pass);
        }
    }

    for (ResourceId rid = 0; rid < resourceCount; ++rid) {
        bool exported = resources[rid].type == ResourceType::Image
            ? resources[rid].image.exported
            : resources[rid].buffer.exported;
        if (!exported) {
            continue;
        }

        PassId lastWriter = std::numeric_limits<PassId>::max();
        for (const UseRef& use : resourceUses[rid]) {
            if (use.isWrite && passes[use.pass].enabled) {
                lastWriter = use.pass;
            }
        }
        if (lastWriter != std::numeric_limits<PassId>::max() && !required[lastWriter]) {
            required[lastWriter] = true;
            backtrackQueue.push(lastWriter);
        }
    }

    while (!backtrackQueue.empty()) {
        const PassId current = backtrackQueue.front();
        backtrackQueue.pop();
        for (PassId pred : reverseDeps[current]) {
            if (!passes[pred].enabled) {
                continue;
            }
            if (!required[pred]) {
                required[pred] = true;
                backtrackQueue.push(pred);
            }
        }
    }

    std::vector<uint32_t> indegree(passCount, 0);
    for (PassId p = 0; p < passCount; ++p) {
        if (!passes[p].enabled || !required[p]) {
            continue;
        }
        for (PassId out : deps[p]) {
            if (passes[out].enabled && required[out]) {
                ++indegree[out];
            }
        }
    }

    std::queue<PassId> ready;
    for (PassId p = 0; p < passCount; ++p) {
        if (passes[p].enabled && required[p] && indegree[p] == 0) {
            ready.push(p);
        }
    }

    while (!ready.empty()) {
        const PassId p = ready.front();
        ready.pop();
        compileResult.executionOrder.push_back(p);
        for (PassId out : deps[p]) {
            if (!passes[out].enabled || !required[out]) {
                continue;
            }
            if (--indegree[out] == 0) {
                ready.push(out);
            }
        }
    }

    size_t requiredCount = 0;
    for (PassId p = 0; p < passCount; ++p) {
        if (passes[p].enabled && required[p]) {
            ++requiredCount;
        } else {
            compileResult.culledPasses.push_back(p);
        }
    }

    if (compileResult.executionOrder.size() != requiredCount) {
        throw std::runtime_error("FrameGraph compile failed: dependency cycle detected");
    }

    std::unordered_map<PassId, int32_t> orderIndex;
    orderIndex.reserve(compileResult.executionOrder.size());
    for (int32_t i = 0; i < static_cast<int32_t>(compileResult.executionOrder.size()); ++i) {
        orderIndex[compileResult.executionOrder[static_cast<size_t>(i)]] = i;
    }

    std::vector<ResourceUsage> lastUsage(resourceCount);
    std::vector<PassId> lastUsagePass(resourceCount, std::numeric_limits<PassId>::max());
    std::vector<bool> hasLastUsage(resourceCount, false);

    for (PassId passId : compileResult.executionOrder) {
        const PassBuilder& pass = passes[passId];

        for (const ResourceUsage& usage : pass.reads) {
            const ResourceId rid = usage.resource;
            if (hasLastUsage[rid] && hasWriteHazard(lastUsage[rid].access, usage.access)) {
                Barrier barrier{};
                barrier.resource = rid;
                barrier.srcPass = lastUsagePass[rid];
                barrier.dstPass = passId;
                barrier.srcStageMask = lastUsage[rid].stageMask;
                barrier.dstStageMask = usage.stageMask;
                barrier.srcAccessMask = lastUsage[rid].accessMask;
                barrier.dstAccessMask = usage.accessMask;
                if (isImageType(resources[rid].type)) {
                    barrier.oldLayout = lastUsage[rid].imageLayout;
                    barrier.newLayout = usage.imageLayout;
                }
                compileResult.barriers.push_back(barrier);
            }
            lastUsage[rid] = usage;
            lastUsagePass[rid] = passId;
            hasLastUsage[rid] = true;
        }

        for (const ResourceUsage& usage : pass.writes) {
            const ResourceId rid = usage.resource;
            if (hasLastUsage[rid] && hasWriteHazard(lastUsage[rid].access, usage.access)) {
                Barrier barrier{};
                barrier.resource = rid;
                barrier.srcPass = lastUsagePass[rid];
                barrier.dstPass = passId;
                barrier.srcStageMask = lastUsage[rid].stageMask;
                barrier.dstStageMask = usage.stageMask;
                barrier.srcAccessMask = lastUsage[rid].accessMask;
                barrier.dstAccessMask = usage.accessMask;
                if (isImageType(resources[rid].type)) {
                    barrier.oldLayout = lastUsage[rid].imageLayout;
                    barrier.newLayout = usage.imageLayout;
                }
                compileResult.barriers.push_back(barrier);
            }
            lastUsage[rid] = usage;
            lastUsagePass[rid] = passId;
            hasLastUsage[rid] = true;
        }
    }

    compileResult.lifetimes.resize(resourceCount);
    for (ResourceId rid = 0; rid < resourceCount; ++rid) {
        ResourceLifetime life{};
        life.resource = rid;
        life.firstUse = std::numeric_limits<int32_t>::max();
        life.lastUse = -1;
        life.estimatedBytes = estimateResourceBytes(resources[rid]);

        for (PassId passId : compileResult.executionOrder) {
            const int32_t idx = orderIndex[passId];
            const PassBuilder& pass = passes[passId];
            bool used = false;
            for (const ResourceUsage& r : pass.reads) {
                if (r.resource == rid) {
                    used = true;
                    break;
                }
            }
            if (!used) {
                for (const ResourceUsage& w : pass.writes) {
                    if (w.resource == rid) {
                        used = true;
                        break;
                    }
                }
            }
            if (used) {
                life.firstUse = std::min(life.firstUse, idx);
                life.lastUse = std::max(life.lastUse, idx);
            }
        }

        if (life.firstUse == std::numeric_limits<int32_t>::max()) {
            life.firstUse = -1;
        }
        compileResult.lifetimes[rid] = life;
    }

    struct SlotState {
        ResourceType type = ResourceType::Image;
        uint64_t sizeBytes = 0;
        int32_t lastUse = -1;
    };
    std::vector<SlotState> slots;

    std::vector<ResourceId> allocCandidates;
    allocCandidates.reserve(resourceCount);
    for (ResourceId rid = 0; rid < resourceCount; ++rid) {
        const ResourceLifetime& life = compileResult.lifetimes[rid];
        if (life.firstUse < 0 || life.lastUse < 0) {
            continue;
        }
        bool imported = resources[rid].type == ResourceType::Image
            ? resources[rid].image.imported
            : resources[rid].buffer.imported;
        if (!imported) {
            allocCandidates.push_back(rid);
        }
    }

    std::sort(allocCandidates.begin(), allocCandidates.end(), [&](ResourceId a, ResourceId b) {
        return compileResult.lifetimes[a].firstUse < compileResult.lifetimes[b].firstUse;
    });

    for (ResourceId rid : allocCandidates) {
        const ResourceNode& res = resources[rid];
        const ResourceLifetime& life = compileResult.lifetimes[rid];
        const uint64_t needed = std::max<uint64_t>(life.estimatedBytes, 1);

        int32_t bestSlot = -1;
        for (int32_t i = 0; i < static_cast<int32_t>(slots.size()); ++i) {
            const SlotState& slot = slots[static_cast<size_t>(i)];
            if (slot.type != res.type) {
                continue;
            }
            if (slot.lastUse >= life.firstUse) {
                continue;
            }
            if (slot.sizeBytes < needed) {
                continue;
            }
            bestSlot = i;
            break;
        }

        if (bestSlot < 0) {
            SlotState slot{};
            slot.type = res.type;
            slot.sizeBytes = needed;
            slot.lastUse = life.lastUse;
            slots.push_back(slot);
            bestSlot = static_cast<int32_t>(slots.size() - 1);
        } else {
            SlotState& slot = slots[static_cast<size_t>(bestSlot)];
            slot.lastUse = life.lastUse;
        }

        AliasAllocation alloc{};
        alloc.resource = rid;
        alloc.slot = static_cast<uint32_t>(bestSlot);
        alloc.slotSizeBytes = slots[static_cast<size_t>(bestSlot)].sizeBytes;
        compileResult.aliasAllocations.push_back(alloc);
    }

    compileResult.valid = true;
}

void FrameGraph::execute(VkCommandBuffer commandBuffer) const {
    if (!compileResult.valid) {
        throw std::runtime_error("FrameGraph::execute: graph is not compiled");
    }

    for (PassId passId : compileResult.executionOrder) {
        emitBarriersForPass(commandBuffer, passId);
        const PassBuilder& pass = passes[passId];
        if (pass.callback) {
            pass.callback(commandBuffer);
        }
    }
}

void FrameGraph::reset() {
    resources.clear();
    runtimeResources.clear();
    passes.clear();
    compileResult = {};
}

uint64_t FrameGraph::estimateResourceBytes(const ResourceNode& resource) const {
    if (resource.type == ResourceType::Buffer) {
        return static_cast<uint64_t>(resource.buffer.size);
    }

    const uint64_t bpp = estimateFormatBytesPerPixel(resource.image.format);
    const uint64_t width = std::max<uint64_t>(resource.image.width, 1);
    const uint64_t height = std::max<uint64_t>(resource.image.height, 1);
    const uint64_t depth = std::max<uint64_t>(resource.image.depth, 1);
    const uint64_t layers = std::max<uint64_t>(resource.image.layers, 1);
    const uint64_t mips = std::max<uint64_t>(resource.image.mipLevels, 1);
    return width * height * depth * layers * bpp * mips;
}

bool FrameGraph::hasWriteHazard(AccessType lhs, AccessType rhs) {
    const bool lhsWrites = lhs == AccessType::Write || lhs == AccessType::ReadWrite;
    const bool rhsWrites = rhs == AccessType::Write || rhs == AccessType::ReadWrite;
    return lhsWrites || rhsWrites;
}

bool FrameGraph::isImageType(ResourceType type) {
    return type == ResourceType::Image;
}

void FrameGraph::emitBarriersForPass(VkCommandBuffer commandBuffer, PassId pass) const {
    std::vector<VkMemoryBarrier2> memoryBarriers;
    std::vector<VkImageMemoryBarrier2> imageBarriers;
    std::vector<VkBufferMemoryBarrier2> bufferBarriers;

    for (const Barrier& barrier : compileResult.barriers) {
        if (barrier.dstPass != pass) {
            continue;
        }
        if (barrier.resource >= resources.size() || barrier.resource >= runtimeResources.size()) {
            continue;
        }

        const ResourceNode& resource = resources[barrier.resource];
        const RuntimeResource& runtime = runtimeResources[barrier.resource];

        if (resource.type == ResourceType::Image && runtime.image != VK_NULL_HANDLE) {
            VkImageMemoryBarrier2 imageBarrier{};
            imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            imageBarrier.srcStageMask = barrier.srcStageMask;
            imageBarrier.srcAccessMask = barrier.srcAccessMask;
            imageBarrier.dstStageMask = barrier.dstStageMask;
            imageBarrier.dstAccessMask = barrier.dstAccessMask;
            imageBarrier.oldLayout = barrier.oldLayout;
            imageBarrier.newLayout = barrier.newLayout;
            imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.image = runtime.image;
            imageBarrier.subresourceRange = runtime.imageRange;
            imageBarriers.push_back(imageBarrier);
            continue;
        }

        if (resource.type == ResourceType::Buffer && runtime.buffer != VK_NULL_HANDLE) {
            VkBufferMemoryBarrier2 bufferBarrier{};
            bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
            bufferBarrier.srcStageMask = barrier.srcStageMask;
            bufferBarrier.srcAccessMask = barrier.srcAccessMask;
            bufferBarrier.dstStageMask = barrier.dstStageMask;
            bufferBarrier.dstAccessMask = barrier.dstAccessMask;
            bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferBarrier.buffer = runtime.buffer;
            bufferBarrier.offset = runtime.bufferOffset;
            bufferBarrier.size = runtime.bufferSize;
            bufferBarriers.push_back(bufferBarrier);
            continue;
        }

        // Fallback when resource is not bound to a concrete handle yet.
        VkMemoryBarrier2 memoryBarrier{};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        memoryBarrier.srcStageMask = barrier.srcStageMask;
        memoryBarrier.srcAccessMask = barrier.srcAccessMask;
        memoryBarrier.dstStageMask = barrier.dstStageMask;
        memoryBarrier.dstAccessMask = barrier.dstAccessMask;
        memoryBarriers.push_back(memoryBarrier);
    }

    if (memoryBarriers.empty() && imageBarriers.empty() && bufferBarriers.empty()) {
        return;
    }

    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.memoryBarrierCount = static_cast<uint32_t>(memoryBarriers.size());
    dependencyInfo.pMemoryBarriers = memoryBarriers.empty() ? nullptr : memoryBarriers.data();
    dependencyInfo.bufferMemoryBarrierCount = static_cast<uint32_t>(bufferBarriers.size());
    dependencyInfo.pBufferMemoryBarriers = bufferBarriers.empty() ? nullptr : bufferBarriers.data();
    dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(imageBarriers.size());
    dependencyInfo.pImageMemoryBarriers = imageBarriers.empty() ? nullptr : imageBarriers.data();

    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}
