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

// I know. "It is all about memory, stupid". Now, to access memory
// we can't just pull a pointer or malloc- tho I wish we cud...
// We need a physical memory block handle and a logical
// memory block handle. These two block handles would first
// need to be instantiated, memory-allocated and then
// be bound to the logical_device.
// All of that is done by the create function.
// We can specify the kind of memory, for instance, DEVICE_LOCAL
// or HOST_COHERENT etc. in memory_property_flags.
// Availability of the requested kinds of memory would be
// platform specific - so there is a helper routine
// find_memory_type.
// The logical memory block handle is used to set-up
// a device memory buffer. Given that buffer memories
// may require target specific alignments - the create
// function would request for 'memory requirements'
// from the driver, with the user requested size.
// The returned requirements would match the
// requested size to an actual size that is aligned
// to target-requirements. This is the size that
// is allocated. However, logically, the buffer size would
// still be what the user wanted.
//
// CPU would be inputing to GPU by writing to this memory,
// but we first need to map first - i.e. CPU needs to get
// access first.
// Once the CPU is done writing - we must remove that
// permission or 'unmap'. This is also true for CPU reads.
// Thus, always call map before and unmap after.
//
// When the memory block is not coherent, data has to be
// pushed and pulled from the GPU manually. The two
// functions sync_to_gpu and sync_from_gpu are for that.
// In unified memory architectures like Apple-M4, these steps
// are redundant - the host and device use the same main
// memory and the data is coherent by default.
// But in a discrete GPU setup we may not be able to avoid
// calling these sync functions.
class memory_block {
public:
  VkBuffer logical_memory_block_handle = VK_NULL_HANDLE;
  VkDeviceMemory physical_memory_block_handle = VK_NULL_HANDLE;
  VkDeviceSize device_size = 0;

  // Creates the block on the GPU
  void create(VkDevice logical_device, VkPhysicalDevice physical_device,
              VkDeviceSize size, VkBufferUsageFlags usage,
              VkMemoryPropertyFlags memory_property_flags);

  // Opens the block so the CPU can write to it (Unified
  // physical_memory_block_handle)
  void *map(VkDevice logical_device);

  // Closes the block after writing
  void unmap(VkDevice logical_device);

  // Wipes the block from the GPU
  void destroy(VkDevice logical_device);

  void sync_from_gpu(VkDevice logical_device);
  void sync_to_gpu(VkDevice logical_device);

private:
  uint32_t find_memory_type(VkPhysicalDevice physical_device,
                            uint32_t type_filter,
                            VkMemoryPropertyFlags memory_property_flags);
};
