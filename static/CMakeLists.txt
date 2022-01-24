cmake_minimum_required(VERSION 3.4.3)
project(static-libs)

set(CMAKE_CXX_STANDARD 11)

if(NOT DEFINED ENV{LLVMPREFIX})
  message(FATAL_ERROR "$LLVMPREFIX is not defined")
else ()
  set(ENV{LLVM_DIR} $ENV{LLVMPREFIX}/lib/cmake/llvm)
endif()

SET (CMAKE_C_COMPILER             $ENV{LLVMPREFIX}/bin/clang)
SET (CMAKE_CXX_COMPILER           $ENV{LLVMPREFIX}/bin/clang++)

# kernel's:
#SET (CMAKE_C_FLAGS "-nostdinc -isystem $ENV{ROOT}/kdfsan-llvm-project/build/lib/clang/11.1.0/include -I$ENV{KERNEL}/arch/x86/include -I$ENV{KERNEL}/arch/x86/include/generated -I$ENV{KERNEL}/include -I$ENV{KERNEL}/arch/x86/include/uapi -I$ENV{KERNEL}/arch/x86/include/generated/uapi -I$ENV{KERNEL}/include/uapi -I$ENV{KERNEL}/include/generated/uapi -include $ENV{KERNEL}/include/linux/kconfig.h -include $ENV{KERNEL}/include/linux/compiler_types.h -D__KERNEL__ -Qunused-arguments -fmacro-prefix-map=./= -Wall -Wundef -Werror=strict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -fno-PIE -Werror=implicit-function-declaration -Werror=implicit-int -Werror=return-type -Wno-format-security -std=gnu89 -Werror=unknown-warning-option -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -mno-avx -fcf-protection=none -m64 -mno-80387 -mstack-alignment=8 -mtune=generic -mno-red-zone -mcmodel=kernel -Wno-sign-compare -fno-asynchronous-unwind-tables -mretpoline-external-thunk -fno-delete-null-pointer-checks -Wno-frame-address -Wno-address-of-packed-member -O2 -Wframe-larger-than=2048 -fstack-protector-strong -Wno-format-invalid-specifier -Wno-gnu -mno-global-merge -Wno-unused-const-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -g -fno-var-tracking -fno-lto -flto=thin -fsplit-lto-unit -fvisibility=hidden -Wdeclaration-after-statement -Wvla -Wno-pointer-sign -Wno-array-bounds -fno-strict-overflow -fno-stack-check -Werror=date-time -Werror=incompatible-pointer-types -Wno-initializer-overrides -Wno-format -Wno-sign-compare -Wno-format-zero-length -Wno-pointer-to-enum-cast -Wno-tautological-constant-out-of-range-compare")

SET (CMAKE_C_FLAGS " -nostdinc -isystem $ENV{ROOT}/kdfsan-llvm-project/build/lib/clang/11.1.0/include -I$ENV{KERNEL}/arch/x86/include -I$ENV{KERNEL}/arch/x86/include/generated -I$ENV{KERNEL}/include -I$ENV{KERNEL}/arch/x86/include/uapi -I$ENV{KERNEL}/arch/x86/include/generated/uapi -I$ENV{KERNEL}/include/uapi -I$ENV{KERNEL}/include/generated/uapi -include $ENV{KERNEL}/include/linux/kconfig.h -include $ENV{KERNEL}/include/linux/compiler_types.h -D__KERNEL__ -Qunused-arguments -fmacro-prefix-map=./= -Wall -Wundef -Werror=strict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -fno-PIE -Werror=implicit-function-declaration -Werror=implicit-int -Werror=return-type -Wno-format-security -std=gnu89 -Werror=unknown-warning-option -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -mno-avx -fcf-protection=none -m64 -mno-80387 -mstack-alignment=8 -mtune=generic -mno-red-zone -mcmodel=kernel -Wno-sign-compare -fno-asynchronous-unwind-tables -mretpoline-external-thunk -fno-delete-null-pointer-checks -Wno-frame-address -Wno-address-of-packed-member -O2 -Wframe-larger-than=2048 -fstack-protector-strong -Wno-format-invalid-specifier -Wno-gnu -mno-global-merge -Wno-unused-const-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -g -fno-var-tracking -fno-lto -flto=thin -fsplit-lto-unit -fvisibility=hidden -Wdeclaration-after-statement -Wvla -Wno-pointer-sign -Wno-array-bounds -fno-strict-overflow -fno-stack-check -Werror=date-time -Werror=incompatible-pointer-types -Wno-initializer-overrides -Wno-format -Wno-sign-compare -Wno-format-zero-length -Wno-pointer-to-enum-cast -Wno-tautological-constant-out-of-range-compare")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mno-omit-leaf-frame-pointer -fno-stack-protector")

include_directories($ENV{KERNEL}/arch/x86/include)
include_directories($ENV{KERNEL}/arch/x86/include/generated)
include_directories($ENV{KERNEL}/include)
include_directories($ENV{KERNEL}/arch/x86/include/uapi)
include_directories($ENV{KERNEL}/arch/x86/include/uapi/generated)
include_directories($ENV{KERNEL}/include/uapi)
include_directories($ENV{KERNEL}/include/generated/uapi)

SET (ENABLE_KSPECEM $ENV{ENABLE_KSPECEM})
if (ENABLE_KSPECEM)
  add_subdirectory(kspecem)
endif()
