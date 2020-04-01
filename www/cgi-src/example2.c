#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[], char *envp[])
{
    int i = 0;

    printf("Content-type: text/html\r\n");
    printf("\r\n");
    printf("Hello CGI<br>");
    printf("<table border=\"1\" cellspacing=\"0\">");
    while (envp[i] != NULL) {
        printf("<tr><td>%s</td></tr>", envp[i++]);
    }
    printf("</table>");
    printf("Byebye CGI<br>");
}
