#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_HEADER \
    "<html>" \
        "<head>" \
            "<title>post-example</title>" \
        "</head>" \
        "<body>"

#define PAGE_FOOTER \
        "</body>" \
    "</html>"

int main(void)
{
    int length;
    char *buf;

    if (strcmp(getenv("REQUEST_METHOD"), "POST")) {
        printf("Content-type: text/html\r\n");
        printf("\r\n");

        printf(PAGE_HEADER);
        printf("<h1>POST DATA DETAIL</h1><br>");
        printf("You should check post-example.html first ;)<br>");
        printf(PAGE_FOOTER);
        return 0;
    }

    length = atoi(getenv("CONTENT_LENGTH")) + 1;
    buf = malloc(length);
    memset(buf, 0, length);
    fgets(buf, length, stdin);

    printf("Content-type: text/html\r\n");
    printf("\r\n");

    printf(PAGE_HEADER);
    printf("<h1>POST DATA DETAIL</h1><br>");
    printf("POST length: %d<br>", length);
    printf("POST data  : %s<br>", buf);
    printf(PAGE_FOOTER);

    return 0;
}
