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

#define EXPORT_NAME "payload5"
#define FUNC_NAME   payload5
#include "unroll.c"
#undef EXPORT_NAME
#undef FUNC_NAME

#define EXPORT_NAME "payload6"
#define FUNC_NAME   payload6
#include "unroll.c"
#undef EXPORT_NAME
#undef FUNC_NAME

#define EXPORT_NAME "payload7"
#define FUNC_NAME   payload7
#include "unroll.c"
#undef EXPORT_NAME
#undef FUNC_NAME

#define EXPORT_NAME "payload8"
#define FUNC_NAME   payload8
#include "unroll.c"
#undef EXPORT_NAME
#undef FUNC_NAME

#define EXPORT_NAME "payload9"
#define FUNC_NAME   payload9
#include "unroll.c"
#undef EXPORT_NAME
#undef FUNC_NAME

#define EXPORT_NAME "payload10"
#define FUNC_NAME   payload10
#include "unroll.c"
#undef EXPORT_NAME
#undef FUNC_NAME

#define EXPORT_NAME "payload11"
#define FUNC_NAME   payload11
#include "unroll.c"
#undef EXPORT_NAME
#undef FUNC_NAME
