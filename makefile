
all:
	gcc -lpthread server.c -o server.out
	gcc -lpthread client.c -o client.out
	git add -i .
	@echo "Please enter your commit message. (CTRL+D to end)"
	@make git-all
	
server:
	gcc -lpthread server.c -o server.out
	git add -i  .
	@echo "Please enter your commit message. (CTRL+D to end)"
	@make git-server
client:
	gcc -lpthread client.c -o client.out
	git add -i  .
	@echo "Please enter your commit message. (CTRL+D to end)"
	@make git-client
testall:
	gcc -lpthread server.c -o server.out
	gcc -lpthread client.c -o client.out
testalls:
	gcc server.c -o spserver.out -lnsl -lsocket -lpthread -lresolv
	gcc client.c -o spclient.out -lnsl -lsocket -lpthread -lresolv
	
testserver:
	gcc -lpthread server.c -o server.out
testclient:
	gcc -lpthread client.c -o client.out
clean:
	rm -rf *.out
	git add -i  .
	@echo "Please enter your commit message. (CTRL+D to end)"
	@make git-server
	

git-server:
	git commit -a -m "Compiled server files. Timestamp: $(shell date --iso=seconds)" -m "$(shell cat)"
	git push
git-client:
	git commit -a -m "Compiled client files. Timestamp: $(shell date --iso=seconds)" -m "$(shell cat)"
	git push
git-all:
	git commit -a -m "Compiled all files. Timestamp: $(shell date --iso=seconds)" -m "$(shell cat)"
	git push
git-clean:
	git commit -a -m "Cleared existing executable. Timestamp: $(shell date --iso=seconds)" -m "$(shell cat)"