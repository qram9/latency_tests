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

#include "gpu_system.h"
#include "memory_block.h"
#include "shader_pipeline.h"
#include "timer.h"
#include "utils.h"
#include <iostream>

// Updated to use the safer memory_block API:
// - rely on RAII: explicit destroy() calls removed (destructors free resources)
// - map/unmap calls prefer the device stored by memory_block::create(); we
//   pass VK_NULL_HANDLE for clarity to show the caller no longer needs to
//   forward the device handle repeatedly.
int main() {
  gpu_system m4;
  m4.initialize();

  shader_pipeline latency_bench;
  latency_bench.prepare(m4.logical_device_handle, "lat_comp.spv");

  timer stopwatch;
  stopwatch.create(m4.logical_device_handle, m4.physical_device_handle);

  // The Sweep
  for (uint32_t count : {16 * 1024, 1024 * 1024, 256 * 1024 * 1024}) {
    VkDeviceSize size = count * sizeof(uint32_t);

    // memory_block now stores the device internally when create() is called.
    // We can rely on RAII to free resources when the objects go out of scope.
    memory_block nodes, result;

    // Setup memory (store device internally during create)
    nodes.create(m4.logical_device_handle, m4.physical_device_handle, size,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    result.create(m4.logical_device_handle, m4.physical_device_handle, size,
                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Prep data: pass VK_NULL_HANDLE to map() to indicate we rely on the
    // stored device inside memory_block. The implementation prefers the
    // stored device and ignores the passed parameter for compatibility.
    uint32_t *ptr = reinterpret_cast<uint32_t *>(nodes.map(VK_NULL_HANDLE));
    initialize_and_shuffle_indices(ptr, count);
    nodes.unmap(VK_NULL_HANDLE);

    // Run
    latency_bench.bind_blocks(m4.logical_device_handle, {&nodes, &result});
    latency_bench.run(m4.logical_device_handle, m4.compute_queue_handle,
                      m4.compute_queue_family_index, stopwatch);

    // Result
    double ns = stopwatch.get_nanoseconds(m4.logical_device_handle) / 1000000.0;
    std::cout << formatBytes(size) << " | Latency: " << ns << " ns/hop"
              << std::endl;

    // No explicit destroy() needed: nodes and result will be destroyed when
    // they go out of scope, freeing their Vulkan resources in their
    // destructors.
  }

  m4.shutdown();
  return 0;
}