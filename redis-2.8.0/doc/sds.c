/*************************************************************************
	> File Name: sds.c
	> Author: 
	> Mail: 
	> Created Time: Tue 02 Jan 2018 12:14:59 AM PST
 ************************************************************************/

#include<stdio.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define SDS_MAX_PERALLOC 1024 * 1024

typedef char *sds;

struct sdshdr {
    int len;
    int free;
    char buf[];
};

size_t sdslen(const sds s) {
    struct sdshdr *sh = (void *)(s - (sizeof(struct sdshdr)));
    return sh->len;
}

sds sdsnewlen(size_t initlen) {
    struct sdshdr *sh;

    sh = malloc(sizeof(struct sdshdr)+initlen+1);
    if (sh == NULL) return NULL;
    sh->len = initlen;
    sh->free = 0;
    sh->buf[initlen] = '\0';
    return (char*)sh->buf;
}

size_t sdsavail(sds s) {
    struct sdshdr *sh = (void *)(s - (sizeof(struct sdshdr)));
    return sh->free;
}

static sds sdsMakeRoomFor(sds s, size_t addlen) {
    struct sdshdr *sh, *newsh;
    size_t free = sdsavail(s);
    size_t len, newlen;

    if(free >= addlen) return s;
    len = sdslen(s);
    sh = (void *)(s - (sizeof(struct sdshdr)));
    newlen = (len + addlen);
    printf("old_len = %ld, newlen = %ld\n", len, newlen);
    if(newlen < SDS_MAX_PERALLOC)
        newlen *= 2;
    else
        newlen = SDS_MAX_PERALLOC;

    printf("newlen = %ld\n", newlen);
    newsh = realloc(sh, sizeof(struct sdshdr) + newlen + 1);
    if(newsh == NULL) return NULL;
    newsh->free = newlen - len;
    return newsh->buf;
}

sds sdscatlen(sds s, const void *t, size_t len) {
    struct sdshdr *sh;
    size_t curlen = sdslen(s);

    s = sdsMakeRoomFor(s,len);
    if (s == NULL) return NULL;
    printf("sdscatlen %ld %s\n", curlen, t);
    sh = (void*) (s-(sizeof(struct sdshdr)));
    memcpy(s+curlen, t, len);
    sh->len = curlen+len;
    sh->free = sh->free-len;
    s[curlen+len] = '\0';
    return s;

}

int main() {
    sds s = sdsnewlen(0);
    s = sdscatlen(s, "hell", 4);
    size_t len = sdslen(s);
    size_t free = sdsavail(s);
    printf("len = %p, %ld, %ld\n", s, len, free);

    s = sdscatlen(s, "abcdefgabc", 10);
    len = sdslen(s);
    free = sdsavail(s);
    printf("len = %s, %ld, %ld\n", s, len, free);
}

