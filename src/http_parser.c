#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "http_parser.h"

const char *http_methods[] = {"GET", "POST"};
const char *http_protos[] = {"HTTP/1.1", "HTTP/2", "HTTP/3"};

int http_parser(char *req_buf, http_request_rec *hrr)
{    
    const char *now_str;
    char *tmp, *tmp2;
    /* GET parameter */
    char *get_prm;
    char *header_last;
    char *header_cur;
    char c;
    /* is GET parameter parsed */
    char get_parsed = 0;
    /* is there a file extension */
    char ext_parsed;
    int i, len;
    /* length of GET parameter */
    int get_len;
    /* header counter and amount */
    int header_cnt, header_num;

    /* parse method */
#ifdef DEBUG
    printf("[@] parsing method\n");
#endif
    for (i = 0; i < METHOD_AMOUNT; ++i) {
        now_str = http_methods[i];
        len = strlen(now_str);
        if (strncmp(req_buf, now_str, len) == 0) {
            hrr->method = i;
            req_buf += len;
            if (*req_buf == ' ') {
                req_buf += 1;
            } else {
                goto PARSE_ERROR;
            }
            break;
        }
    }

    if (i == METHOD_AMOUNT)
        goto PARSE_ERROR;

    /* parse url and GET parameter */
#ifdef DEBUG
    printf("[@] parsing url\n");
#endif
    tmp = req_buf;
    c = *tmp;
    while (c != ' ') {
        if (c == '\0') {
            goto PARSE_ERROR;
        } else if (c == '?' && !get_parsed) {
            tmp2 = tmp;
            get_parsed = 1;
        }
        
        ++tmp;
        c = *tmp;
    }

    if (get_parsed) {
        len = tmp2 - req_buf;
        /* exclude '?' */
        ++tmp2;
        get_len = tmp - tmp2;
        get_prm = hrr->get_data = malloc(get_len + 1);
        strncpy(get_prm, tmp2, get_len);
#ifdef DEBUG
        printf("[@] get_prm: %s\n", get_prm);
#endif
    } else {
        len = tmp - req_buf;
        hrr->get_data = NULL;
    }

    hrr->req_path = malloc(len + 1);
    strncpy(hrr->req_path, req_buf, len);
    hrr->req_path[len] = '\0';

    req_buf = tmp;
    ++req_buf;

    /* parse protocol */
#ifdef DEBUG
    printf("[@] parsing protocol\n");
#endif
    for (i = 0; i < PROTO_AMOUNT; ++i) {
        now_str = http_protos[i];
        len = strlen(now_str);
        if (strncmp(req_buf, now_str, len) == 0) {
            hrr->proto = i;
            req_buf += len;
            if (strncmp(req_buf, "\r\n", 2) == 0) {
                req_buf += 2;
            } else {
                goto PARSE_PROTO_ERROR;
            }
            break;
        }
    }

    if (i == PROTO_AMOUNT)
        goto PARSE_PROTO_ERROR;
    
    if (!(header_last = strstr(req_buf, "\r\n\r\n")))
        goto PARSE_PROTO_ERROR;

    /* parse headers */ 
#ifdef DEBUG
    printf("[@] parsing headers\n");
#endif
    header_num = 1;
    tmp = req_buf;
    while ((tmp = strstr(tmp, "\r\n")) != header_last) {
        header_num += 1;
        tmp += 2;
    }
    
#ifdef DEBUG
    printf("[@] header_num: %d\n", header_num);
#endif
    hrr->headers = malloc(sizeof(char *) * (header_num + 1));
    header_cnt = 0;
    header_cur = req_buf;
    do {
        char *str;

        tmp = strstr(header_cur, ": ");
        tmp2 = strstr(header_cur, "\r\n");
        len = tmp2 - header_cur;

        if (tmp == NULL && tmp > tmp2)
            goto PARSE_HEADER_ERROR;

        str = hrr->headers[header_cnt] = malloc(sizeof(char) * len);
        strncpy(str, header_cur, tmp - header_cur);
        strncat(str, "=", 1);
        strncat(str, tmp + 2, tmp2 - (tmp + 2));
        str[len - 1] = '\0';

        header_cnt += 1;
        header_cur = tmp2 + 2;
    } while (tmp2 != header_last);
    hrr->headers[header_num] = NULL;

    /* parse file extension */
#ifdef DEBUG
    printf("[@] parse file extension\n");
#endif
    tmp = hrr->req_path + strlen(hrr->req_path) - 1;
    c = *tmp;
    len = 0;
    if (c == '/') {
        hrr->req_ext = (char *) - 1;
    } else {
        ext_parsed = 1;
        while(c != '.') {
            if (c == '/') {
                ext_parsed = 0;
                break;
            }
            
            len += 1;
            --tmp;
            c = *tmp;
        }
        if (ext_parsed) {
            hrr->req_ext = malloc(len + 1);
            strncpy(hrr->req_ext, tmp + 1, len);
            hrr->req_ext[len] = '\0';
        } else {
            hrr->req_ext = NULL;
        }
    }

    /* parse post data */ 
#ifdef DEBUG
    printf("[@] parse post data\n");
#endif
    req_buf = header_last + 4;
    if (*req_buf != '\0')
        hrr->post_data = strdup(req_buf);
    else
        hrr->post_data = NULL;

    return 0;

PARSE_HEADER_ERROR:
PARSE_PROTO_ERROR:
PARSE_ERROR:
    http_destroy_request(hrr);
    fprintf(stderr, "request format error\n");
    return -1;
}

int http_search_header(http_request_rec *hrr, const char *header_name, char *buf)
{
    int i = 0;
    int len = strlen(header_name);
    char *curr, *value;
    char found = -1;

    while ((curr = hrr->headers[i++]) != NULL) {
        if (strncasecmp(curr, header_name, len) == 0) {
            value = strchr(curr, '=');
            strcpy(buf, value + 1);
            found = 0;
            break;
        }
    }

    return found;
}
