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
#include "latency_test.h"
#include <stdexcept>

void memory_block::create(VkDevice logical_device,
                          VkPhysicalDevice physical_device, VkDeviceSize size,
                          VkBufferUsageFlags buffer_usage_flags,
                          VkMemoryPropertyFlags memory_property_flags) {
  device_size = size;
  VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  buffer_info.size = device_size;
  buffer_info.usage = buffer_usage_flags;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(logical_device, &buffer_info, nullptr,
                     &logical_memory_block_handle) != VK_SUCCESS) {
    throw std::runtime_error(
        "Failed to create memory_block logical_memory_block_handle!");
  }

  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements(logical_device, logical_memory_block_handle,
                                &memory_requirements);

  VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocInfo.allocationSize = memory_requirements.size;
  allocInfo.memoryTypeIndex =
      find_memory_type(physical_device, memory_requirements.memoryTypeBits,
                       memory_property_flags);

  if (vkAllocateMemory(logical_device, &allocInfo, nullptr,
                       &physical_memory_block_handle) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate MemoryBlock silicon!");
  }

  vkBindBufferMemory(logical_device, logical_memory_block_handle,
                     physical_memory_block_handle, 0);
}

void *memory_block::map(VkDevice logical_device) {
  void *data;
  vkMapMemory(logical_device, physical_memory_block_handle, 0, device_size, 0,
              &data);
  return data;
}

void memory_block::unmap(VkDevice logical_device) {
  vkUnmapMemory(logical_device, physical_memory_block_handle);
}

void memory_block::destroy(VkDevice logical_device) {
  if (logical_memory_block_handle != VK_NULL_HANDLE) {
    vkDestroyBuffer(logical_device, logical_memory_block_handle, nullptr);
    logical_memory_block_handle = VK_NULL_HANDLE;
  }
  if (physical_memory_block_handle != VK_NULL_HANDLE) {
    vkFreeMemory(logical_device, physical_memory_block_handle, nullptr);
    physical_memory_block_handle = VK_NULL_HANDLE;
  }
}

uint32_t
memory_block::find_memory_type(VkPhysicalDevice physical_device,
                               uint32_t type_filter,
                               VkMemoryPropertyFlags memory_property_flags) {
  // 1. Query the physical logical_device for its physical_memory_block_handle
  // heaps and types
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
    // 2. Check if this physical_memory_block_handle type index is allowed by
    // the 'type_filter' bitmask
    bool is_type_allowed = type_filter & (1 << i);

    // 3. Check if this physical_memory_block_handle type has all the requested
    // property flags
    bool has_required_properties =
        (mem_properties.memoryTypes[i].propertyFlags & memory_property_flags) ==
        memory_property_flags;

    if (is_type_allowed && has_required_properties) {
      return i;
    }
  }

  throw std::runtime_error(
      "Failed to find suitable physical_memory_block_handle type!");
}

void memory_block::sync_to_gpu(VkDevice logical_device) {
  VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
  range.memory = physical_memory_block_handle;
  range.offset = 0;
  range.size = device_size;
  // This is not necessary in Mac like M4 as the memory is unified and
  // by default would be HOST_COHERENT
  vkFlushMappedMemoryRanges(logical_device, 1, &range);
}

void memory_block::sync_from_gpu(VkDevice logical_device) {
  VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
  range.memory = physical_memory_block_handle;
  range.offset = 0;
  range.size = device_size;
  // This is not necessary in Mac like M4 as the memory is unified and
  // by default would be HOST_COHERENT
  vkInvalidateMappedMemoryRanges(logical_device, 1, &range);
}
