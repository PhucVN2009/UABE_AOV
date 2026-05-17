#pragma once

#include "KittyMemory/KittyMemory.h"
#include "TuanMeta/IL2CppSDKGenerator/Il2Cpp.h"

static uintptr_t g_il2cpp_base = 0;

inline void InitUnityResolve() {
    while (g_il2cpp_base == 0) {
        g_il2cpp_base = KittyMemory::getLibraryBaseMap("libil2cpp.so").startAddress;
        if (g_il2cpp_base == 0) sleep(1);
    }
    Il2Cpp::Attach("libil2cpp.so");
}

inline void *GetMethodOffset(const char *image, const char *namespaze, const char *clazz, const char *method, int args) {
    return Il2Cpp::GetMethodOffset(image, namespaze, clazz, method, args);
}

inline uintptr_t GetFieldOffset(const char *image, const char *namespaze, const char *clazz, const char *field) {
    return Il2Cpp::GetFieldOffset(image, namespaze, clazz, field);
}
