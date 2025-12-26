// Weak fallback for __dso_handle to satisfy app-only link step.
// The full firmware link will prefer the libcxx definition.
void* __attribute__((weak)) __dso_handle;

