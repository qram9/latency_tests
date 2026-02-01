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
#include <stdexcept>
#include <vector>

void gpu_system::initialize() {
  // 1. Instance Setup (The Mac "Secret Sauce")
  std::vector<const char *> extensions = {
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};

  VkInstanceCreateInfo inst_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  inst_info.flags |=
      VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR; // Critical for Mac
  inst_info.enabledExtensionCount = (uint32_t)extensions.size();
  inst_info.ppEnabledExtensionNames = extensions.data();

  if (vkCreateInstance(&inst_info, nullptr, &instance_handle) != VK_SUCCESS) {
    throw std::runtime_error(
        "GpuSystem: Failed to create Instance. Is MoltenVK installed?");
  }

  // 2. Pick the M4 Max
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance_handle, &device_count, nullptr);
  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(instance_handle, &device_count, devices.data());

  if (devices.empty())
    throw std::runtime_error("GpuSystem: No GPUs found!");
  physical_device_handle = devices[0];

  // 3. Find the Compute Queue
  uint32_t q_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device_handle, &q_count,
                                           nullptr);
  std::vector<VkQueueFamilyProperties> q_props(q_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device_handle, &q_count,
                                           q_props.data());

  bool found = false;
  for (uint32_t i = 0; i < q_count; i++) {
    if (q_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
      compute_queue_family_index = i;
      found = true;
      // --- NEW: Check for Timestamp Support ---
      timestamp_valid_bits = q_props[i].timestampValidBits;

      if (timestamp_valid_bits == 0) {
        throw std::runtime_error(
            "GpuSystem: Selected queue does not support timestamps!");
      }

      break;
    }
  }
  if (!found)
    throw std::runtime_error("GpuSystem: No Compute Queue found!");

  // 4. Logical Device (The Subset requirement)
  float priority = 1.0f;
  VkDeviceQueueCreateInfo q_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  q_info.queueFamilyIndex = compute_queue_family_index;
  q_info.queueCount = 1;
  q_info.pQueuePriorities = &priority;

  // This extension is the "buddy" to the Instance portability flag
  std::vector<const char *> dev_ext = {"VK_KHR_portability_subset"};

  VkDeviceCreateInfo dev_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  dev_info.queueCreateInfoCount = 1;
  dev_info.pQueueCreateInfos = &q_info;
  dev_info.enabledExtensionCount = (uint32_t)dev_ext.size();
  dev_info.ppEnabledExtensionNames = dev_ext.data();

  if (vkCreateDevice(physical_device_handle, &dev_info, nullptr,
                     &logical_device_handle) != VK_SUCCESS) {
    throw std::runtime_error("GpuSystem: Failed to create Logical Device!");
  }

  vkGetDeviceQueue(logical_device_handle, compute_queue_family_index, 0,
                   &compute_queue_handle);
}

void gpu_system::shutdown() {
  if (logical_device_handle != VK_NULL_HANDLE)
    vkDestroyDevice(logical_device_handle, nullptr);
  if (instance_handle != VK_NULL_HANDLE)
    vkDestroyInstance(instance_handle, nullptr);
}
