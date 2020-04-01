#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG_REQUEST_INFO \
    printf("[>] req method: %d\n", request.method); \
    printf("[>] req path  : %s\n", request.req_path); \
    printf("[>] req proto : %d\n", request.proto); \
    if (request.req_ext == (char *)-1) \
        printf("[>] req ext   : dir\n"); \
    else if (request.req_ext == NULL) \
        printf("[>] req ext   : None\n"); \
    else \
        printf("[>] req ext   : %s\n", request.req_ext); \
    if (request.post_data) \
        printf("[>] req post_data : %s\n", request.post_data); \
    int debug_i = 0; \
    while (request.headers[debug_i] != NULL) { \
        printf("[D] %s\n", request.headers[debug_i++]); \
    } 

#endif
