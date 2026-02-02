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

#include "memory_block.h"

#include <stdexcept>

memory_block::~memory_block()
{
  destroy(device_handle_);
}

memory_block::memory_block(memory_block &&other) noexcept
{
  // Steal handles and state
  logical_memory_block_handle = other.logical_memory_block_handle;
  physical_memory_block_handle = other.physical_memory_block_handle;
  device_size = other.device_size;

  device_handle_ = other.device_handle_;
  allocation_size_ = other.allocation_size_;
  owns_buffer_ = other.owns_buffer_;
  owns_memory_ = other.owns_memory_;

  // Invalidate the source object to avoid double-free on its destruction.
  other.logical_memory_block_handle = VK_NULL_HANDLE;
  other.physical_memory_block_handle = VK_NULL_HANDLE;
  other.device_size = 0;
  other.device_handle_ = VK_NULL_HANDLE;
  other.allocation_size_ = 0;
  other.owns_buffer_ = false;
  other.owns_memory_ = false;
}

memory_block &memory_block::operator=(memory_block &&other) noexcept
{
  if (this != &other) {
    // Release current resources, then steal
    destroy(device_handle_);

    logical_memory_block_handle = other.logical_memory_block_handle;
    physical_memory_block_handle = other.physical_memory_block_handle;
    device_size = other.device_size;

    device_handle_ = other.device_handle_;
    allocation_size_ = other.allocation_size_;
    owns_buffer_ = other.owns_buffer_;
    owns_memory_ = other.owns_memory_;

    // Invalidate other
    other.logical_memory_block_handle = VK_NULL_HANDLE;
    other.physical_memory_block_handle = VK_NULL_HANDLE;
    other.device_size = 0;
    other.device_handle_ = VK_NULL_HANDLE;
    other.allocation_size_ = 0;
    other.owns_buffer_ = false;
    other.owns_memory_ = false;
  }

  return *this;
}

void memory_block::create(VkDevice logical_device,
                          VkPhysicalDevice physical_device, VkDeviceSize size,
                          VkBufferUsageFlags buffer_usage_flags,
                          VkMemoryPropertyFlags memory_property_flags)
{
  // store the requested logical size (what callers expect) and the device to
  // be used by subsequent operations.
  device_size = size;
  device_handle_ = logical_device;

  VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  buffer_info.size = device_size;
  buffer_info.usage = buffer_usage_flags;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device_handle_, &buffer_info, nullptr, &logical_memory_block_handle) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create memory_block logical buffer handle");
  }
  owns_buffer_ = true;

  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements(device_handle_, logical_memory_block_handle, &memory_requirements);

  VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  alloc_info.allocationSize = memory_requirements.size;
  alloc_info.memoryTypeIndex = find_memory_type(physical_device, memory_requirements.memoryTypeBits, memory_property_flags);

  allocation_size_ = alloc_info.allocationSize;

  if (vkAllocateMemory(device_handle_, &alloc_info, nullptr, &physical_memory_block_handle) != VK_SUCCESS) {
    // cleanup previously created buffer
    if (owns_buffer_) {
      vkDestroyBuffer(device_handle_, logical_memory_block_handle, nullptr);
      logical_memory_block_handle = VK_NULL_HANDLE;
      owns_buffer_ = false;
    }
    throw std::runtime_error("Failed to allocate memory for memory_block");
  }
  owns_memory_ = true;

  if (vkBindBufferMemory(device_handle_, logical_memory_block_handle, physical_memory_block_handle, 0) != VK_SUCCESS) {
    // cleanup on failure to bind
    if (owns_memory_) {
      vkFreeMemory(device_handle_, physical_memory_block_handle, nullptr);
      physical_memory_block_handle = VK_NULL_HANDLE;
      owns_memory_ = false;
    }
    if (owns_buffer_) {
      vkDestroyBuffer(device_handle_, logical_memory_block_handle, nullptr);
      logical_memory_block_handle = VK_NULL_HANDLE;
      owns_buffer_ = false;
    }
    throw std::runtime_error("Failed to bind buffer memory for memory_block");
  }
}

void *memory_block::map(VkDevice /*logical_device*/)
{
  // For compatibility the device param is accepted but ignored; prefer the
  // stored device from create(). Mapping the allocation lets the host access
  // the memory. This keeps the original behavior of mapping the entire
  // allocation; if you prefer smaller mappings an overload can be added.
  if (device_handle_ == VK_NULL_HANDLE || physical_memory_block_handle == VK_NULL_HANDLE) {
    throw std::runtime_error("memory_block not initialized for mapping");
  }

  void *data = nullptr;
  VkDeviceSize map_size = allocation_size_ == 0 ? device_size : allocation_size_;
  VkResult res = vkMapMemory(device_handle_, physical_memory_block_handle, 0, map_size, 0, &data);
  if (res != VK_SUCCESS) {
    throw std::runtime_error("vkMapMemory failed for memory_block");
  }
  return data;
}

void memory_block::unmap(VkDevice /*logical_device*/)
{
  if (device_handle_ == VK_NULL_HANDLE || physical_memory_block_handle == VK_NULL_HANDLE) {
    return;
  }
  vkUnmapMemory(device_handle_, physical_memory_block_handle);
}

void memory_block::destroy(VkDevice /*logical_device*/)
{
  // Use the internally stored device when available; if it's not set the
  // object either never created resources or has already been destroyed.
  VkDevice dev = device_handle_;
  if (dev == VK_NULL_HANDLE) {
    return;
  }

  if (logical_memory_block_handle != VK_NULL_HANDLE && owns_buffer_) {
    vkDestroyBuffer(dev, logical_memory_block_handle, nullptr);
    logical_memory_block_handle = VK_NULL_HANDLE;
    owns_buffer_ = false;
  }

  if (physical_memory_block_handle != VK_NULL_HANDLE && owns_memory_) {
    vkFreeMemory(dev, physical_memory_block_handle, nullptr);
    physical_memory_block_handle = VK_NULL_HANDLE;
    owns_memory_ = false;
  }

  // Reset stored device and sizes
  device_handle_ = VK_NULL_HANDLE;
  device_size = 0;
  allocation_size_ = 0;
}

uint32_t memory_block::find_memory_type(VkPhysicalDevice physical_device,
                                       uint32_t type_filter,
                                       VkMemoryPropertyFlags memory_property_flags)
{
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i) {
    bool is_type_allowed = (type_filter & (1 << i)) != 0;
    bool has_required_properties = (mem_properties.memoryTypes[i].propertyFlags & memory_property_flags) == memory_property_flags;

    if (is_type_allowed && has_required_properties) {
      return i;
    }
  }

  throw std::runtime_error("Failed to find suitable memory type for memory_block");
}

void memory_block::sync_to_gpu(VkDevice /*logical_device*/)
{
  // Flush host writes so device sees them
  if (device_handle_ == VK_NULL_HANDLE || physical_memory_block_handle == VK_NULL_HANDLE) {
    return;
  }

  VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
  range.memory = physical_memory_block_handle;
  range.offset = 0;
  range.size = allocation_size_ == 0 ? device_size : allocation_size_;

  vkFlushMappedMemoryRanges(device_handle_, 1, &range);
}

void memory_block::sync_from_gpu(VkDevice /*logical_device*/)
{
  // Invalidate host cache to see device writes
  if (device_handle_ == VK_NULL_HANDLE || physical_memory_block_handle == VK_NULL_HANDLE) {
    return;
  }

  VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
  range.memory = physical_memory_block_handle;
  range.offset = 0;
  range.size = allocation_size_ == 0 ? device_size : allocation_size_;

  vkInvalidateMappedMemoryRanges(device_handle_, 1, &range);
}

#ifdef MEMORY_BLOCK_UNIT_TEST

// A tiny unit test that doesn't require a Vulkan device. The test checks
// basic behavior for default objects, exception behavior from map(), and
// move-construction semantics for public handles.
//
// To build and run this test locally:
//
//   clang++ -std=c++17 -DMEMORY_BLOCK_UNIT_TEST memory_block.cc -o mb_test
//   ./mb_test
//
// The test uses only public members and exception behavior, so it is valid
// to run without a Vulkan device or runtime.
#include <cassert>
#include <iostream>

int main()
{
  // Default object: destroy() should be safe (no-op).
  memory_block a;
  a.destroy(VK_NULL_HANDLE);

  // map() on an uninitialized object should throw; verify that behavior.
  bool threw = false;
  try {
    (void)a.map(VK_NULL_HANDLE);
  } catch (const std::runtime_error &) {
    threw = true;
  }
  assert(threw && "map() must throw when object is not initialized");

  // Test move-construction transfers public handles and resets source.
  memory_block src;
  src.logical_memory_block_handle = reinterpret_cast<VkBuffer>(0x1);
  src.physical_memory_block_handle = reinterpret_cast<VkDeviceMemory>(0x2);
  src.device_size = 128;

  memory_block dst = std::move(src);

  assert(dst.logical_memory_block_handle == reinterpret_cast<VkBuffer>(0x1));
  assert(dst.physical_memory_block_handle == reinterpret_cast<VkDeviceMemory>(0x2));
  assert(dst.device_size == 128);

  // Source should have been cleared.
  assert(src.logical_memory_block_handle == VK_NULL_HANDLE);
  assert(src.physical_memory_block_handle == VK_NULL_HANDLE);
  assert(src.device_size == 0);

  std::cout << "memory_block unit test passed\n";
  return 0;
}

#endif // MEMORY_BLOCK_UNIT_TEST
