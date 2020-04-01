#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "db.h"

int _search(char *_key, char *buf, int write_buf)
{
    FILE *fp;
    char *key, *val;
    char *content, *loc;
    int key_size;
    long fsize;
    char c;

    if (_key == NULL)
        return -2;

    if (write_buf && buf == NULL)
        return -2;

    if ((fp = fopen(DB_PATH, "rb")) == NULL)
        return -2;

    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* read file */
    content = malloc(fsize);
    fread(content, fsize, 1, fp);
    fclose(fp);
    
    /* append '=' to key */
    key_size = strlen(_key);
    key = malloc(key_size + 2);
    strcpy(key, _key);
    key[key_size] = '=';
    key[key_size + 1] = '\0';

    /* find key 
     * if a value of key is 'abc=blablabalb'
     * searching for key abc may misrecognize this value as a record 
     * so here we need to do some work for this */
    loc = content;
    while (1) {
        if ((loc = strstr(loc, key)) == NULL)
            return -1;

        /* if loc is the position of first record, it must be key */
        if (loc == content)
            break;

        /* or loc is a begin position of a line, it must be key too */
        if (*(loc - 1) == '\n')
            break;

        /* otherwise, loc points to a misrecognized value */
        ++loc;
    }

    val = loc + strlen(key);

    if (write_buf) {
        /* write value to buf */
        while ((c = *val) != '\n' && c != '\0') {
            *buf = c;
            ++buf;
            ++val;
        }
    }

    free(key);
    free(content);
    return 0;
}

int search(char *key, char *buf)
{
    return _search(key, buf, 1);
}

int insert(char *key, char *val)
{
    FILE *fp;
    char c[2];
    int count = VAL_LEN_MAX;

    /* check is there a duplicated key */
    if (_search(key, NULL, 0) != -1) {
        return -1;        
    }

    /* open file */
    if ((fp = fopen(DB_PATH, "a")) == NULL)
        return -1;

    /* write file */
    fwrite(key, strlen(key), 1, fp);
    fwrite("=", 1, 1, fp);
    c[1] = '\0';
    while ((c[0] = *val) != '\n' && c[0] != '\0' && count) {
        fwrite(c, 1, 1, fp);
        ++val;
        --count;
    }
    fwrite("\n", 1, 1, fp);

    fclose(fp);
    return 0;
}
