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
    memory_block nodes, result;

    // Setup memory
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

    // Prep data
    uint32_t *ptr = (uint32_t *)nodes.map(m4.logical_device_handle);
    initialize_and_shuffle_indices(ptr, count);
    nodes.unmap(m4.logical_device_handle);

    // Run
    latency_bench.bind_blocks(m4.logical_device_handle, {&nodes, &result});
    latency_bench.run(m4.logical_device_handle, m4.compute_queue_handle,
                      m4.compute_queue_family_index, stopwatch);

    // Result
    double ns = stopwatch.get_nanoseconds(m4.logical_device_handle) / 1000000.0;
    std::cout << formatBytes(size) << " | Latency: " << ns << " ns/hop"
              << std::endl;

    nodes.destroy(m4.logical_device_handle);
    result.destroy(m4.logical_device_handle);
  }

  m4.shutdown();
  return 0;
}
