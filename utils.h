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
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#define VK_CHECK(X)                                                            \
  {                                                                            \
    VkResult r = X;                                                            \
    if (r != VK_SUCCESS) {                                                     \
      std::cerr << "VK ERROR: " << r << " AT " << __LINE__ << std::endl;       \
      exit(1);                                                                 \
    }                                                                          \
  }
// Mainly to read spir-v shader
std::vector<uint8_t> readBinaryFile(const std::string &filePath);
// a simple formatter than returns number of Bytes, KB, MB etc. to TB for a
// given bytesize
std::string formatBytes(uint64_t bytesize);
// Set up numElmts shuffled data but make sure to call vkMapMemory to initialize
// input array, dataPtr, first. 0..numElmts-1 will have been shuffled in the
// array.
void initialize_and_shuffle_indices(uint32_t *dataPtr, uint32_t numElmts);
