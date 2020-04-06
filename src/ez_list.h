#ifndef EZ_LIST_H
#define EZ_LIST_H

struct _ez_list {
    char *data;
    struct _ez_list *next;
};
typedef struct _ez_list ez_list;

void ez_free_list(ez_list *list);

#endif
