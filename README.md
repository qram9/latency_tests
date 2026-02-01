# latency_tests
### measure latencies on gpus

This is almost entirely done with the free AI available at google.com  - hit AI mode 

You would need Molten-VK & vulkan for this

brew search vulkan
==> Formulae
vulkan-extensionlayer ✔               vulkan-loader ✔                       vulkan-tools ✔                        vulkan-validationlayers ✔
vulkan-headers ✔                      vulkan-profiles ✔                     vulkan-utility-libraries ✔            vulkan-volk

Install the ones with the tickmarks - not all of them are needed but they are useful

These as well

brew search spirv 
==> Formulae
spirv-cross ✔                         spirv-llvm-translator                 spr                                   spim
spirv-headers ✔                       spirv-tools ✔                         spin

Dont forget xcode!

Once these are in, you may try build.sh.

This is what I saw


./m4_profiler 
64.00 KB | Latency: 53.9551 ns/hop

4.00 MB | Latency: 144.657 ns/hop

1.00 GB | Latency: 466.031 ns/hop

