#include "main.c"

// Increase artificially the payload of the WASM
#define EXPORT_NAME "payload1"
#define FUNC_NAME   payload1
#include "unroll.c"
#undef EXPORT_NAME
#undef FUNC_NAME

#define EXPORT_NAME "payload2"
#define FUNC_NAME   payload2
#include "unroll.c"
#undef EXPORT_NAME
#undef FUNC_NAME

#define EXPORT_NAME "payload3"
#define FUNC_NAME   payload3
#include "unroll.c"
#undef EXPORT_NAME
#undef FUNC_NAME

#define EXPORT_NAME "payload4"
#define FUNC_NAME   payload4
#include "unroll.c"
#undef EXPORT_NAME
#undef FUNC_NAME
