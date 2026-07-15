#ifndef NAMED_LOOKUP_H_
#define NAMED_LOOKUP_H_

// Find an element by name in arrays of named structs or pointers to named structs.
// Requirements: the struct must have `const char *name` as its first field.
//               Macros must be used on actual arrays, not pointers.

#include <string.h>
#include <stddef.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

const void *find_by_name    (const void       *array,     size_t count, size_t elem_size, const char *name);
const void *find_by_name_ptr(const void *const *ptr_array, size_t count,                  const char *name);

// Search in an array of structs where .name is the first field.
#define FIND_BY_NAME(arr, str) \
    ((__typeof__(&(arr)[0]))find_by_name((arr), ARRAY_SIZE(arr), sizeof((arr)[0]), (str)))

// Search in an array of pointers to structs where .name is the first field.
#define FIND_BY_NAME_PTR(arr, str) \
    ((__typeof__((arr)[0]))find_by_name_ptr((const void *const *)(arr), ARRAY_SIZE(arr), (str)))

#endif // NAMED_LOOKUP_H_

// ---- Implementation ---------------------------------------------------------
#ifdef NAMED_LOOKUP_IMPLEMENTATION
#ifndef NAMED_LOOKUP_IMPLEMENTATION_DONE_
#define NAMED_LOOKUP_IMPLEMENTATION_DONE_

const void *find_by_name(const void *array, size_t count, size_t elem_size, const char *name)
{
    const char *p = (const char *)array;
    for (size_t i = 0; i < count; i++) {
        if (strcmp(*(const char **)p, name) == 0)
            return p;
        p += elem_size;
    }
    return NULL;
}

const void *find_by_name_ptr(const void *const *ptr_array, size_t count, const char *name)
{
    for (size_t i = 0; i < count; i++) {
        if (strcmp(*(const char **)ptr_array[i], name) == 0)
            return ptr_array[i];
    }
    return NULL;
}

#endif // NAMED_LOOKUP_IMPLEMENTATION_DONE_
#endif // NAMED_LOOKUP_IMPLEMENTATION
