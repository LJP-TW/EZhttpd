#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"

#define PAGE_HEADER \
    "<html>" \
        "<head>" \
            "<title>View</title>" \
        "</head>" \
        "<body>"

#define PAGE_FOOTER \
        "</body>" \
    "</html>"
int main(void)
{
    FILE *fp;
    long fsize;
    char *buf;
    unsigned char ret;

    fp = fopen(DB_PATH, "rb");
    if (fp == NULL)
        goto DB_NOT_FOUND_ERROR;

    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buf = malloc(fsize);

    /* output */
    printf("Content-type: text/html\r\n");
    printf("\r\n");

    printf(PAGE_HEADER);
    printf("<h1>VIEW</h1><br>");

    while (fgets(buf, fsize, fp) != NULL) {
        printf("<a href=\"#\">%s</a><br>", buf);
    }

    printf("<p>View OK!</p>");
    printf(PAGE_FOOTER);

    return 0;

DB_NOT_FOUND_ERROR:
    /* output on error */
    printf("Content-type: text/html\r\n");
    printf("\r\n");

    printf(PAGE_HEADER);
    printf("<h1>VIEW</h1><br>");
    printf("Database not found :(<br>");
    printf(PAGE_FOOTER);
    return -1;
}
