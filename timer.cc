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
#include "timer.h"

void timer::create(VkDevice logical_device, VkPhysicalDevice physical_device) {
  // 1. Get the hardware's tick-to-nanosecond conversion rate
  VkPhysicalDeviceProperties props;
  vkGetPhysicalDeviceProperties(physical_device, &props);
  timestamp_period = props.limits.timestampPeriod;

  // 2. Create a pool to hold 2 timestamps (Start and Stop)
  VkQueryPoolCreateInfo info{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
  info.queryType = VK_QUERY_TYPE_TIMESTAMP;
  info.queryCount = 2;

  vkCreateQueryPool(logical_device, &info, nullptr, &query_pool_handle);
}

void timer::start(VkCommandBuffer cb) {
  vkCmdResetQueryPool(cb, query_pool_handle, 0, 2);
  // Write timestamp at the very beginning of the GPU pipe
  vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, query_pool_handle,
                      0);
}

void timer::stop(VkCommandBuffer cb) {
  // Write timestamp at the very end of the GPU pipe
  vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                      query_pool_handle, 1);
}

double timer::get_nanoseconds(VkDevice logical_device) {
  uint64_t timestamps[2];

  // Pull the 2 timestamps (Start at index 0, Stop at index 1)
  vkGetQueryPoolResults(logical_device, query_pool_handle, 0,
                        2,                  // Start at index 0, take 2 queries
                        sizeof(timestamps), // Data size
                        timestamps,         // Where to put it
                        sizeof(uint64_t),   // Stride between results
                        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

  // Calculate delta and convert to nanoseconds
  // (Result is Stop - Start * Period)
  return static_cast<double>(timestamps[1] - timestamps[0]) * timestamp_period;
}

void timer::destroy(VkDevice logical_device) {
  if (query_pool_handle != VK_NULL_HANDLE) {
    vkDestroyQueryPool(logical_device, query_pool_handle, nullptr);
    query_pool_handle = VK_NULL_HANDLE;
  }
}
