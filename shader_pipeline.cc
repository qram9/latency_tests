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

#include "shader_pipeline.h"
#include "utils.h"
#include <iostream>

void shader_pipeline::prepare(VkDevice logical_device,
                              const std::string &shader_path) {
  // 1. Describe the "Blueprint" (Descriptor Set Layout).
  // This is the buffer to slot-binding step. Slots are
  // how the shader accesses buffers.
  // Buffers get bound to a slot.
  // We expect two buffers: nodes bound at slot 0, and result bound at slot 1.
  // -  This is called a descriptor-set.
  VkDescriptorSetLayoutBinding bindings[2] = {};
  for (int i = 0; i < 2; i++) {
    bindings[i].binding = i;
    bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[i].descriptorCount = 1;
    bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  }

  // Enclose descriptor-set in a layout, which indicates for ex., how many
  // bindings are in the set.
  VkDescriptorSetLayoutCreateInfo layout_info{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  layout_info.bindingCount = 2;
  layout_info.pBindings = bindings;
  VK_CHECK(vkCreateDescriptorSetLayout(logical_device, &layout_info, nullptr,
                                       &descriptor_layout));

  // 2. Create the Pipeline Layout (The connection between shader and
  // descriptor-set)
  VkPipelineLayoutCreateInfo pipe_layout_info{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipe_layout_info.setLayoutCount = 1;
  pipe_layout_info.pSetLayouts = &descriptor_layout;
  // Now add the pipeline-layout to logical_device
  VK_CHECK(vkCreatePipelineLayout(logical_device, &pipe_layout_info, nullptr,
                                  &pipeline_layout));

  // 3. Load the SPIR-V
  auto shader_code = readBinaryFile(shader_path);
  if (shader_code.empty()) {
    throw std::runtime_error(
        "Shader_pipeline: SPIR-V file is empty or missing: " + shader_path);
  }

  // The binary is added to a Shader-module.
  VkShaderModuleCreateInfo mod_info{
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  mod_info.codeSize = shader_code.size();
  mod_info.pCode = reinterpret_cast<const uint32_t *>(shader_code.data());

  VkShaderModule shader_module;
  VK_CHECK(
      vkCreateShaderModule(logical_device, &mod_info, nullptr, &shader_module));

  // 4. Create the Compute Pipeline
  VkComputePipelineCreateInfo pipe_info{
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
  pipe_info.layout = pipeline_layout;
  pipe_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pipe_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  pipe_info.stage.module = shader_module;
  pipe_info.stage.pName = "main"; // Entry point in your .comp shader

  // On M4 Max, this will trigger the MoltenVK/Metal compilation
  VK_CHECK(vkCreateComputePipelines(logical_device, VK_NULL_HANDLE, 1,
                                    &pipe_info, nullptr, &pipeline_handle));

  // 5. Cleanup the temporary module (the pipeline handle now owns the binary)
  vkDestroyShaderModule(logical_device, shader_module, nullptr);
}

void shader_pipeline::bind_blocks(VkDevice logical_device,
                                  const std::vector<memory_block *> &blocks) {
  // 1. Create a Pool to hold our Descriptor Set
  VkDescriptorPoolSize pool_size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                 (uint32_t)blocks.size()};

  VkDescriptorPoolCreateInfo pool_info{
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  pool_info.maxSets = 1;
  pool_info.poolSizeCount = 1;
  pool_info.pPoolSizes = &pool_size;

  VK_CHECK(vkCreateDescriptorPool(logical_device, &pool_info, nullptr,
                                  &descriptor_pool));

  // 2. Allocate the set from the pool using the layout we made in 'prepare'
  VkDescriptorSetAllocateInfo alloc_info{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  alloc_info.descriptorPool = descriptor_pool;
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts = &descriptor_layout;

  VK_CHECK(
      vkAllocateDescriptorSets(logical_device, &alloc_info, &descriptor_set));

  // 3. Connect the physical memory blocks to the shader bindings
  std::vector<VkDescriptorBufferInfo> buffer_infos;
  std::vector<VkWriteDescriptorSet> writes;

  // CRITICAL: Prevent reallocations so pointers remain valid
  buffer_infos.reserve(blocks.size());
  writes.reserve(blocks.size());

  for (uint32_t i = 0; i < blocks.size(); i++) {
    VkDescriptorBufferInfo b_info{};
    b_info.buffer = blocks[i]->logical_memory_block_handle;
    b_info.offset = 0;
    b_info.range = VK_WHOLE_SIZE;
    buffer_infos.push_back(b_info);

    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = descriptor_set;
    write.dstBinding = i;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.descriptorCount = 1;
    // Now this pointer is safe because we reserved space!
    write.pBufferInfo = &buffer_infos.back();
    writes.push_back(write);
  }

  vkUpdateDescriptorSets(logical_device, (uint32_t)writes.size(), writes.data(),
                         0, nullptr);
}

void shader_pipeline::run(VkDevice logical_device, VkQueue queue,
                          uint32_t queue_idx, timer &stopwatch) {
  // 1. Create a temporary Command Pool for this run
  VkCommandPool pool;
  VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  pool_info.queueFamilyIndex = queue_idx;
  VK_CHECK(vkCreateCommandPool(logical_device, &pool_info, nullptr, &pool));

  // 2. Allocate the Command Buffer
  VkCommandBuffer cb;
  VkCommandBufferAllocateInfo cb_info{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  cb_info.commandPool = pool;
  cb_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cb_info.commandBufferCount = 1;
  VK_CHECK(vkAllocateCommandBuffers(logical_device, &cb_info, &cb));

  // 3. Record the "Story" for the GPU
  VkCommandBufferBeginInfo begin_info{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  vkBeginCommandBuffer(cb, &begin_info);

  // Start the stopwatch
  stopwatch.start(cb);

  // Bind the tools and the data
  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_handle);
  vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout,
                          0, 1, &descriptor_set, 0, nullptr);

  // Go! (Dispatch 1 thread for latency)
  vkCmdDispatch(cb, 1, 1, 1);

  // Stop the stopwatch
  stopwatch.stop(cb);

  vkEndCommandBuffer(cb);

  // 4. Submit to the M4 Max and wait
  VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cb;

  VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
  vkQueueWaitIdle(queue);

  // 5. Cleanup temporary command objects
  vkDestroyCommandPool(logical_device, pool, nullptr);
}

void shader_pipeline::destroy(VkDevice logical_device) {
  if (pipeline_handle != VK_NULL_HANDLE)
    vkDestroyPipeline(logical_device, pipeline_handle, nullptr);
  if (pipeline_layout != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(logical_device, pipeline_layout, nullptr);
  if (descriptor_layout != VK_NULL_HANDLE)
    vkDestroyDescriptorSetLayout(logical_device, descriptor_layout, nullptr);
  if (descriptor_pool != VK_NULL_HANDLE)
    vkDestroyDescriptorPool(logical_device, descriptor_pool, nullptr);

  // Reset handles
  pipeline_handle = VK_NULL_HANDLE;
  pipeline_layout = VK_NULL_HANDLE;
  descriptor_layout = VK_NULL_HANDLE;
  descriptor_pool = VK_NULL_HANDLE;
  descriptor_set = VK_NULL_HANDLE;
}

