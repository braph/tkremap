#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

void sighandler(int sig) {
  signal(sig, SIG_DFL);

  #define case break;case
  switch (sig) {
    case SIGINT:  printf("SIGINT\n");
    case SIGHUP:  printf("SIGHUP\n");
    case SIGTERM: printf("SIGTERM\n");
    case SIGUSR1: printf("SIGUSR1\n");
    case SIGUSR2: printf("SIGUSR2\n");
  }

  exit(0);
}

int main(int argc, char *argv[]) {
  signal(SIGINT,  sighandler);
  signal(SIGHUP,  sighandler);
  signal(SIGTERM, sighandler);
  signal(SIGUSR1, sighandler);
  signal(SIGUSR2, sighandler);
  pause();
}
