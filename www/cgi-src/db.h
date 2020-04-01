#ifndef DB_H
#define DB_H

#define DB_PATH "/tmp/tmp_db" 
#define VAL_LEN_MAX 0x0fff

#include <stdio.h>

/* search db for @key
 * if find it, store value at @buf
 *
 * if find it, return 0
 * if key's not found, return -1 
 * return -2 on other error */
extern int search(char *key, char *buf);

/* insert db
 * this function will search for duplicated @key first
 * @val is terminated by null byte or by '\n'
 * the max length of @val is VAL_LEN_MAX
 *
 * if ok, return 0
 * else return -1*/
extern int insert(char *key, char *val);

#endif
