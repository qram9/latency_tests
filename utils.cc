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
#include "utils.h"
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

std::vector<uint8_t> readBinaryFile(const std::string &filePath) {
  // 1. Open with binary mode and move to the end
  std::ifstream file(filePath, std::ios::binary | std::ios::ate);

  if (!file) {
    // Use standard error for portable logging
    std::cerr << "Error: Could not open " << filePath << std::endl;
    return {};
  }

  // 2. Use 64-bit size for files larger than 4GB
  std::streamsize size = file.tellg();
  if (size <= 0)
    return {};

  // 3. Reset to beginning
  file.seekg(0, std::ios::beg);

  // 4. Pre-allocate exactly what we need
  std::vector<uint8_t> buffer(static_cast<size_t>(size));

  // 5. Direct read into the vector's memory
  if (!file.read(reinterpret_cast<char *>(buffer.data()), size)) {
    std::cerr << "Error: Failed to read data from " << filePath << std::endl;
    return {};
  }

  return buffer;
}

std::string formatBytes(uint64_t bytesize) {
  const char *units[] = {"B", "KB", "MB", "GB", "TB"};
  int i = 0;
  double size = static_cast<double>(bytesize);

  while (size >= 1024 && i < 4) {
    size /= 1024;
    i++;
  }

  std::stringstream ss;
  ss << std::fixed << std::setprecision(2) << size << " " << units[i];
  return ss.str();
}

/*
 * TODO:
 * For the 1GB test (256 million elements), std::vector<uint32_t> indices will
 allocate another 1GB of CPU memory. On a 36GB M4 Max, this is fine, but on an
 eventually push for a 16GB test, we might hit a memory wall.

    Future Refinement: we could shuffle the dataPtr directly in-place using a
 Fisher-Yates algorithm to save that extra 1GB of CPU-side allocation.
 * */

void initialize_and_shuffle_indices(uint32_t *dataPtr, uint32_t numElmts) {
  assert(numElmts > 0 && "num elements must be non-zero");
  std::vector<uint32_t> indices(numElmts);
  std::iota(indices.begin(), indices.end(), 0);
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(indices.begin(), indices.end(), g);

  for (uint32_t i = 0; i < numElmts - 1; ++i) {
    dataPtr[indices[i]] = indices[i + 1];
  }
  dataPtr[indices[numElmts - 1]] = indices[0];
}
