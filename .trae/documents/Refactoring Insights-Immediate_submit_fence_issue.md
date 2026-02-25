# 1. Problems

The immediate command path in LibRenderer uses an uninitialized fence (`m_imFence`) inside `Engine::immediateSubmit`. This can cause invalid handle usage and undefined synchronization during staging buffer copies (e.g., in `uploadMesh`). Additionally, the fence wait timeout uses `UINT16_MAX` instead of `UINT64_MAX`.

## 1.1. **Uninitialized immediate fence**
- Location:
  - `LibRenderer/include/Engine.hpp` line ~65: `vk::raii::Fence m_imFence = nullptr;`
  - `LibRenderer/src/Engine.cpp` lines ~550-565: `immediateSubmit` dereferences and uses `m_imFence`.
  - `LibRenderer/src/Engine.cpp` lines ~173-206: `initFrameDatas` creates command pools and command buffers but never creates `m_imFence`.
- Why it is a problem:
  - Dereferencing a `vk::raii::Fence` that wraps a null handle leads to undefined behavior or crashes.
  - Immediate operations (copy from staging buffers, quick barriers) rely on proper synchronization to ensure data visibility.
- Representative code (current):
```cpp
// Engine.cpp (current)
void Engine::immediateSubmit(std::function<void(vk::CommandBuffer cmd)>&& function) {
    m_device.resetFences(*m_imFence);
    m_imCommandBuffer.reset();

    vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    m_imCommandBuffer.begin(beginInfo);

    function(m_imCommandBuffer);

    m_imCommandBuffer.end();
    vk::CommandBufferSubmitInfo cmdSubmitInfo{.commandBuffer = m_imCommandBuffer};
    const auto submitInfo = vkStructsUtils::makeSubmitInfo(&cmdSubmitInfo, nullptr, nullptr);
    m_graphicsQueue.submit2(submitInfo, m_imFence);
    m_device.waitForFences(*m_imFence, vk::True, UINT16_MAX);
}
```
And in initialization:
```cpp
// Engine.cpp (current) initFrameDatas
m_imCommandPool = vk::raii::CommandPool(m_device, poolInfo);
allocInfo.commandPool = m_imCommandPool;
allocInfo.commandBufferCount = 1;
m_imCommandBuffer = std::move(m_device.allocateCommandBuffers(allocInfo).front());
// NOTE: m_imFence is never created
```

## 1.2. **Incorrect fence wait timeout**
- Location: `LibRenderer/src/Engine.cpp` line ~564 inside `immediateSubmit`.
- Why it is a problem:
  - `vk::Device::waitForFences` expects a 64-bit nanosecond timeout. Using `UINT16_MAX` (~65 microseconds) can cause spurious timeouts on normal workloads.
  - Inconsistent with the regular frame path where `UINT64_MAX` is used.
- Representative code (current):
```cpp
m_device.waitForFences(*m_imFence, vk::True, UINT16_MAX); // too small; wrong type
```

# 2. Benefits

Fixing initialization and timeout yields the most direct benefit: reliable and well-defined synchronization for immediate GPU work, preventing null-handle crashes and spurious timeouts.

## 2.1. **Stability and correctness**
- Eliminates null-handle dereference risk in immediate submissions.
- Ensures staging copies complete before CPU proceeds.

## 2.2. **Consistent synchronization semantics**
- Aligns immediate path timeout with the main rendering path by using **UINT64_MAX**.
- Reduces chances of intermittent failures under load.

## 2.3. **Clearer initialization responsibilities**
- Centralizes creation of immediate synchronization objects alongside immediate command pool/buffer.
- Improves maintainability and reduces future mistakes.

# 3. Solutions

Create and reuse a dedicated fence for the immediate path during initialization, and correct the fence wait timeout. This is a local, safe change with minimal blast radius.

## 3.1. **Initialize and reuse m_imFence: To Solve "Uninitialized immediate fence"**

- Solution overview:
  - Create `m_imFence` once in `initFrameDatas` (similar to frame fences), preferably starting signaled so the first reset is valid.
  - Reuse it for each immediate submission: reset -> submit with fence -> wait.

- Implementation steps:
  - In `initFrameDatas`, after setting up `m_imCommandPool` and `m_imCommandBuffer`, create `m_imFence`.
  - Keep `immediateSubmit` flow but ensure fence is initialized.

- Code before modification:
```cpp
// Engine.cpp (current) initFrameDatas
m_imCommandPool = vk::raii::CommandPool(m_device, poolInfo);
allocInfo.commandPool = m_imCommandPool;
allocInfo.commandBufferCount = 1;
m_imCommandBuffer = std::move(m_device.allocateCommandBuffers(allocInfo).front());
// m_imFence not created
```

- Code after modification:
```cpp
// Engine.cpp (proposed) initFrameDatas
m_imCommandPool = vk::raii::CommandPool(m_device, poolInfo);
allocInfo.commandPool = m_imCommandPool;
allocInfo.commandBufferCount = 1;
m_imCommandBuffer = std::move(m_device.allocateCommandBuffers(allocInfo).front());

vk::FenceCreateInfo imFenceInfo{ .flags = vk::FenceCreateFlagBits::eSignaled };
m_imFence = vk::raii::Fence(m_device, imFenceInfo);
```

- Explanation:
  - Starting signaled allows the initial `resetFences` call to be meaningful. Subsequent cycles follow the reset-submit-wait pattern.

## 3.2. **Fix timeout: To Solve "Incorrect fence wait timeout"**

- Solution overview:
  - Use a 64-bit maximum timeout consistent with the draw path.

- Implementation steps:
  - Replace `UINT16_MAX` with `UINT64_MAX` in `waitForFences`.

- Code before modification:
```cpp
m_device.waitForFences(*m_imFence, vk::True, UINT16_MAX);
```

- Code after modification:
```cpp
m_device.waitForFences(*m_imFence, vk::True, UINT64_MAX);
```

- Explanation:
  - Prevents unexpected timeouts on normal workloads. Matches the standard waiting behavior used elsewhere in the engine.

# 4. Regression testing scope

This change affects immediate GPU submission used for staging buffer copies and small one-shot operations. Testing should focus on end-to-end data upload and synchronization correctness.

## 4.1. Main Scenarios
- Upload mesh data via `uploadMesh`:
  - Preconditions: valid `vertices` and `indices` arrays; device initialized.
  - Steps: call `uploadMesh`; ensure immediate copy completes; draw using uploaded buffers.
  - Expected: geometry renders correctly on the next frame; no device errors; no CPU access before GPU copy completes.

- Repeated immediate submissions:
  - Steps: call `immediateSubmit` multiple times back-to-back for buffer copies.
  - Expected: fence reuse works; no spurious timeouts; no crashes.

## 4.2. Edge Cases
- High load conditions:
  - Trigger: large buffer sizes or multiple uploads.
  - Expected: `waitForFences` does not prematurely timeout; submission completes as expected.

- Device-lost or driver errors (simulated if feasible):
  - Trigger: force an error during submission.
  - Expected: error paths are reported; no null-handle dereference.
