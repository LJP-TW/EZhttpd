#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#define BUF_MAX (VAL_LEN_MAX + 1)

#define PAGE_HEADER \
    "<html>" \
        "<head>" \
            "<title>Search</title>" \
        "</head>" \
        "<body>"

#define PAGE_FOOTER \
        "</body>" \
    "</html>"
int main(void)
{
    char *buf, *arg_begin, *arg_end;
    char *key;
    char val[BUF_MAX];
    int length, arg_len, ret;

    if (strcmp(getenv("REQUEST_METHOD"), "GET")) {
        printf("Content-type: text/html\r\n");
        printf("\r\n");

        printf(PAGE_HEADER);
        printf("<h1>SEARCH</h1><br>");
        printf("You should check search.html first ;)<br>");
        printf(PAGE_FOOTER);
        return 0;
    }

    buf = strdup(getenv("QUERY_STRING"));

    /* parse get argument */
    if ((arg_begin = strstr(buf, "key=")) == NULL)
        goto ERROR;

    arg_begin += 4;
    arg_end = arg_begin;
    while (*arg_end != '\0' && *arg_end != '&') 
        ++arg_end;
    arg_len = arg_end - arg_begin;
    key = malloc(arg_len + 1);
    strncpy(key, arg_begin, arg_len);
    key[arg_len] = '\0';

    /* don't allow key to be empty */
    if (strlen(key) == 0)
        goto KEY_ERROR;

    /* search */
    ret = search(key, val);
    switch (ret) {
    case -1:
        goto SEARCH_404;
    case -2:
        goto SEARCH_ERROR;
    }

    /* output */
    printf("Content-type: text/html\r\n");
    printf("\r\n");

    printf(PAGE_HEADER);
    printf("<h1>SEARCH</h1><br>");
    printf("key  : %s<br>", key);
    printf("value: %s<br>", val);
    printf("<p>Search OK!</p>");
    printf(PAGE_FOOTER);

    return 0;

SEARCH_404:
    /* output on error */
    printf("Content-type: text/html\r\n");
    printf("\r\n");

    printf(PAGE_HEADER);
    printf("<h1>SEARCH</h1><br>");
    printf("key not found :(<br>");
    printf(PAGE_FOOTER);
    return -1;

SEARCH_ERROR:
    /* output on error */
    printf("Content-type: text/html\r\n");
    printf("\r\n");

    printf(PAGE_HEADER);
    printf("<h1>SEARCH</h1><br>");
    printf("SEARCH error :(<br>");
    printf("unexpected error occurs<br>");
    printf(PAGE_FOOTER);
    return -1;

KEY_ERROR:
    /* output on error */
    printf("Content-type: text/html\r\n");
    printf("\r\n");

    printf(PAGE_HEADER);
    printf("<h1>SEARCH</h1><br>");
    printf("'key' should not be empty!<br>");
    printf(PAGE_FOOTER);
    return -1;

ERROR:
    /* output on error */
    printf("Content-type: text/html\r\n");
    printf("\r\n");

    printf(PAGE_HEADER);
    printf("<h1>SEARCH</h1><br>");
    printf("No 'key'!<br>");
    printf(PAGE_FOOTER);
    return -1;
}
