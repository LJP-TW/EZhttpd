insert:
	gcc -c cgi-src/db.c -o cgi-bin/db.o
	gcc -c cgi-src/insert.c -o cgi-bin/insert.o
	gcc cgi-bin/db.o cgi-bin/insert.o -o cgi-bin/insert.cgi
	rm cgi-bin/db.o
	rm cgi-bin/insert.o

search:
	gcc -c cgi-src/db.c -o cgi-bin/db.o
	gcc -c cgi-src/search.c -o cgi-bin/search.o
	gcc cgi-bin/db.o cgi-bin/search.o -o cgi-bin/search.cgi
	rm cgi-bin/db.o
	rm cgi-bin/search.o

view:
	gcc -c cgi-src/db.c -o cgi-bin/db.o
	gcc -c cgi-src/view.c -o cgi-bin/view.o
	gcc cgi-bin/db.o cgi-bin/view.o -o cgi-bin/view.cgi
	rm cgi-bin/db.o
	rm cgi-bin/view.o

