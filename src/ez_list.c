#include <stdlib.h>

#include "ez_list.h"

void ez_free_list(ez_list *list)
{
    ez_list *tmp;

    while (list) {
        tmp = list->next;
        free(list->data);
        free(list);
        list = tmp;
    }
}
