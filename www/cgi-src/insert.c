#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"

#define PAGE_HEADER \
    "<html>" \
        "<head>" \
            "<title>Insert</title>" \
        "</head>" \
        "<body>"

#define PAGE_FOOTER \
        "</body>" \
    "</html>"

int main(void)
{
    char *buf, *arg_begin, *arg_end;
    char *key, *value;
    int length, arg_len;

    if (strcmp(getenv("REQUEST_METHOD"), "POST")) {
        printf("Content-type: text/html\r\n");
        printf("\r\n");

        printf(PAGE_HEADER);
        printf("<h1>INSERT</h1><br>");
        printf("You should check insert.html first ;)<br>");
        printf(PAGE_FOOTER);
        return 0;
    }

    length = atoi(getenv("CONTENT_LENGTH")) + 1;
    buf = malloc(length);
    memset(buf, 0, length);
    fgets(buf, length, stdin);

    /* parse post argument */
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

    if ((arg_begin = strstr(buf, "value=")) == NULL)
        goto ERROR;

    arg_begin += 6;
    arg_end = arg_begin;
    while (*arg_end != '\0' && *arg_end != '&') 
        ++arg_end;
    arg_len = arg_end - arg_begin;
    value = malloc(arg_len + 1);
    strncpy(value, arg_begin, arg_len);
    value[arg_len] = '\0';

    /* don't allow key to be empty */
    if (strlen(key) == 0)
        goto KEY_ERROR;

    /* insert */
    if (insert(key, value) != 0)
        goto INSERT_ERROR;

    /* output */
    printf("Content-type: text/html\r\n");
    printf("\r\n");

    printf(PAGE_HEADER);
    printf("<h1>INSERT</h1><br>");
    printf("key  : %s<br>", key);
    printf("value: %s<br>", value);
    printf("<p>Insert OK!</p>");
    printf(PAGE_FOOTER);

    return 0;

INSERT_ERROR:
    /* output on error */
    printf("Content-type: text/html\r\n");
    printf("\r\n");

    printf(PAGE_HEADER);
    printf("<h1>INSERT</h1><br>");
    printf("INSERT error :(<br>");
    printf("maybe key has already existed<br>");
    printf(PAGE_FOOTER);
    return -1;

KEY_ERROR:
    /* output on error */
    printf("Content-type: text/html\r\n");
    printf("\r\n");

    printf(PAGE_HEADER);
    printf("<h1>INSERT</h1><br>");
    printf("'key' should not be empty!<br>");
    printf(PAGE_FOOTER);
    return -1;

ERROR:
    /* output on error */
    printf("Content-type: text/html\r\n");
    printf("\r\n");

    printf(PAGE_HEADER);
    printf("<h1>INSERT</h1><br>");
    printf("No 'key' or 'value'!<br>");
    printf(PAGE_FOOTER);
    return -1;
}
