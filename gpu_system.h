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
// create a vulkan context
struct gpu_system {
  VkInstance instance_handle =
      VK_NULL_HANDLE; // says going to use vulkan
                      // this is the physical logical_device or gpu/cpu
                      // device extensions attached to the physical device
  VkPhysicalDevice physical_device_handle = VK_NULL_HANDLE;
  // Logical logical_device with a device queue,
  // extensions are some features like VK_KHR_portability_subset
  VkDevice logical_device_handle = VK_NULL_HANDLE;
  // logical_device queue would be a compute, fragment or vertex queue
  VkQueue compute_queue_handle = VK_NULL_HANDLE;
  // This will be an int marker to compute queue for now
  uint32_t compute_queue_family_index = 0;
  uint32_t timestamp_valid_bits = 0; // bits supported by the clock

  void initialize();
  void shutdown(); // Cleanup
};
