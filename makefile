
all:
	gcc -pthread server.c -o server.out
	gcc -pthread client.c -o client.out
	git add -i .
	@echo "Please enter your commit message. (CTRL+D to end)"
	@make git-all
	
server:
	gcc -pthread server.c -o server.out
	git add -i  .
	@echo "Please enter your commit message. (CTRL+D to end)"
	@make git-server
client:
	gcc -pthread client.c -o client.out
	git add -i  .
	@echo "Please enter your commit message. (CTRL+D to end)"
	@make git-client
testall:
	gcc -pthread server.c -o server.out
	gcc -pthread client.c -o client.out
	
testserver:
	gcc -pthread server.c -o server.out
testclient:
	gcc -pthread client.c -o client.out
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