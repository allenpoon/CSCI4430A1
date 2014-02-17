void signal_handle(int sig){
	puts("");
	print_req_in();
	fflush(stdin);
	fflush(stdout);
}

void signal_init(){
	signal(SIGINT, signal_handle);
	signal(SIGTERM, signal_handle);
	signal(SIGQUIT, signal_handle);
	signal(SIGTSTP, signal_handle);
}

void signal_reset(){
	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);
}
