#pragma once
#include "memory_block.h"
#include "timer.h"
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

// The "Conductor" of the GPU work.
// It loads the shader program (SPIR-V) and manages the pipeline
// through which the memory_blocks will be processed.
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

#include <vulkan/vulkan.h>
class shader_pipeline {
public:
  VkDescriptorSetLayout descriptor_layout = VK_NULL_HANDLE;
  VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
  VkPipeline pipeline_handle = VK_NULL_HANDLE;
  VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
  VkDescriptorSet descriptor_set = VK_NULL_HANDLE;

  // 1. Loads the shader and sets up the "blueprint" for the GPU
  void prepare(VkDevice logical_device, const std::string &shader_path);

  // 2. Plumbs the specific memory_blocks into the shader bindings
  void bind_blocks(VkDevice logical_device,
                   const std::vector<memory_block *> &blocks);

  // 3. Tells the GPU to execute the task and records the time
  void run(VkDevice logical_device, VkQueue queue, uint32_t queue_idx,
           timer &stopwatch);

  // 4. Tears down the pipeline logic
  void destroy(VkDevice logical_device);
};
