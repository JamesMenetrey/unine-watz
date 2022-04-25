#ifndef _TZ_STRING_H
#define _TZ_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

size_t strcspn(const char *s1, const char *s2);
char * strerror (int n);
size_t strspn(const char *s, const char *accept);

#ifdef __cplusplus
}
#endif

#endif /* end of _TZ_STRING_H */