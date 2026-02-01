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
#include <vector>
#include <vulkan/vulkan.h>

// The "Stopwatch" for the GPU.
// It uses hardware timestamp queries to measure exactly how long
// the silicon spent on a task, bypassing any OS/driver noise.
class timer {
public:
  VkQueryPool query_pool_handle = VK_NULL_HANDLE;
  float timestamp_period = 0.0f;

  // Sets up the hardware stopwatch
  void create(VkDevice logical_device, VkPhysicalDevice physical_device);

  // Records the "Start" tick in the command stream
  void start(VkCommandBuffer command_buffer);

  // Records the "Stop" tick in the command stream
  void stop(VkCommandBuffer command_buffer);

  // Retrieves the result in nanoseconds
  double get_nanoseconds(VkDevice logical_device);

  // Wipes the stopwatch from the GPU
  void destroy(VkDevice logical_device);
};
