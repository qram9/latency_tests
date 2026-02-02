/*
 * ----------------------------------------------------------------------------
 * PUBLIC DOMAIN AND DISCLAIMER NOTICE
 * ----------------------------------------------------------------------------
 * This software is released into the public domain using the Creative Commons
 * Zero (CC0) designation. To the extent possible under law, the author(s)
 * have waived all copyright and related or neighboring rights to this work.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * ----------------------------------------------------------------------------
 */

#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>

// memory_block: a minimal, entry-level Vulkan buffer+memory helper.
//
// This class is intended to be an approachable entry point for developers who
// are learning Vulkan memory management. It preserves the original public
// names and method signatures so existing call sites in the repo require
// minimal changes, while improving safety and clarity.
//
// Short, focused explanations (API names start each bullet when present):
// - VkBuffer: a logical buffer object used in command recording and in
//   descriptor sets.
// - VkDeviceMemory: the GPU memory allocation that backs a VkBuffer.
// - vkBindBufferMemory: bind a VkDeviceMemory allocation to a VkBuffer before
//   the buffer can be used by commands.
// - vkMapMemory / vkUnmapMemory: map device memory into host address space for
//   CPU reads/writes, and unmap when finished.
// - vkFlushMappedMemoryRanges / vkInvalidateMappedMemoryRanges:
//     - vkFlushMappedMemoryRanges: make host writes visible to the device when
//       the memory is not host-coherent.
//     - vkInvalidateMappedMemoryRanges: make device writes visible to the
//       host when the memory is not host-coherent.
//
// Other important concepts:
// - Requested size vs allocation size:
//     The driver may allocate more memory than requested (alignment/granularity).
//     device_size stores the user-requested logical size; allocation_size_
//     (internal) stores the actual size returned by the driver.
// - Mapping behavior:
//     Mapping the allocation gives the CPU access to the memory. For simple
//     usage this class maps the whole allocation; if you need sub-range maps an
//     overload can be added.
// - Design goals:
//     - RAII: destructor frees owned resources; explicit destroy() is still
//       available for parity with the original API.
//     - Move-only: copy is disabled to avoid accidental double-free; move
//       semantics transfer ownership safely.
//     - Minimal breakage: public handles and method signatures preserved so
//       other files in the repo require minimal or no changes.
//
// Note: the device parameter is kept in map/unmap/destroy/sync_* for API
// compatibility, but the implementation prefers the VkDevice stored at create().
class memory_block {
public:
  // Public handles retained for backward compatibility.
  VkBuffer logical_memory_block_handle = VK_NULL_HANDLE;
  VkDeviceMemory physical_memory_block_handle = VK_NULL_HANDLE;

  // Logical size requested by the caller (kept for compatibility).
  VkDeviceSize device_size = 0;

  // Construction / destruction
  memory_block() = default;
  ~memory_block();

  // Non-copyable, moveable
  memory_block(const memory_block &) = delete;
  memory_block &operator=(const memory_block &) = delete;
  memory_block(memory_block &&other) noexcept;
  memory_block &operator=(memory_block &&other) noexcept;

  // Create a buffer + allocate memory. Kept signature to minimize changes.
  void create(VkDevice logical_device,
              VkPhysicalDevice physical_device,
              VkDeviceSize size,
              VkBufferUsageFlags usage,
              VkMemoryPropertyFlags memory_property_flags);

  // Map / unmap for host access. Device param is accepted for compatibility.
  void *map(VkDevice logical_device);
  void unmap(VkDevice logical_device);

  // Destroy releases owned Vulkan resources. Device param kept for compatibility.
  void destroy(VkDevice logical_device);

  // Flush / invalidate helpers for non-coherent memory.
  void sync_to_gpu(VkDevice logical_device);
  void sync_from_gpu(VkDevice logical_device);

private:
  // Internal state uses snake_case to avoid confusion with Vulkan names.
  VkDevice device_handle_ = VK_NULL_HANDLE;     // device used to create handles
  VkDeviceSize allocation_size_ = 0;            // actual allocation size returned by driver
  bool owns_buffer_ = false;                    // whether this object owns the VkBuffer
  bool owns_memory_ = false;                    // whether this object owns the VkDeviceMemory

  // Helper to pick a memory type index that satisfies the requested properties.
  uint32_t find_memory_type(VkPhysicalDevice physical_device,
                            uint32_t type_filter,
                            VkMemoryPropertyFlags memory_property_flags);
};