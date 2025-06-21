import json

import methods


def run(target, source, env):
    buffer = methods.get_buffer(str(source[0]))
    data = json.loads(buffer)

    with methods.generated_wrapper(str(target[0])) as file:
        file.write("""\
#ifndef __cplusplus
#include <stddef.h>
#include <stdint.h>

typedef uint32_t char32_t;
typedef uint16_t char16_t;
#else
#include <cstddef>
#include <cstdint>

extern "C" {
#endif
""")

        pass

        file.write("""\
#ifdef __cplusplus
}
#endif
""")
