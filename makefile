
all:
	gcc -pthread server.c -o server.out
	gcc -pthread server.c -o client.out
	git add -i .
	git commit -a -m "Compiled all files. Timestamp: $(shell date --iso=seconds)"
	git push
server:
	gcc -pthread server.c -o server.out
	echo $(shell date --iso=seconds)
	git add -i  .
	git commit -a -m "Compiled server files. Timestamp: $(shell date --iso=seconds)\nlinebreak TEST"
	git push
client:
	gcc -pthread client.c -o client.out
	git add -i  .
	git commit -a -m "Compiled client files. Timestamp: $(shell date --iso=seconds)"
	git push
clean:
	rm -rf *.out
	git add -i  .
	git commit -a -m "Cleared existing executable. Timestamp: $(shell date --iso=seconds)"