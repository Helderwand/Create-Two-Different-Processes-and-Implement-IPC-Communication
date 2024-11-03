#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#define FIFO1 "fifo1"
#define FIFO2 "fifo2"
#define MAX_CHILDREN 2

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("Usage: %s <integer>\n", argv[0]);
    exit(1);
  }
  int child_counter = 0;  // conunter for terminated children
  int total_children = 0; // conunter for total child processes created
  int num = atoi(argv[1]);
  int random_numbers[num];

  // create fifo
  mkfifo(FIFO1, 0666);
  mkfifo(FIFO2, 0666);

  // random numbers
  srand(time(NULL));
  for (int i = 0; i < num; i++)
  {
    random_numbers[i] = rand() % 10;
    printf("Random number %d: %d\n", i + 1, random_numbers[i]);
  }

  // set up sigaction for SIGCHLD
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART; // restart interrupted system calls
  if (sigaction(SIGCHLD, &sa, NULL) == -1)
  {
    perror("sigaction");
    exit(1);
  }
  printf("Signal handler registered successfully.\n");

  // create child process
  for (int i = 0; i < MAX_CHILDREN; i++)
  {
    total_children++; // increment total child process counter
    printf("Total Childen %d\n", total_children);
    pid_t pid = fork();
    if (pid == -1)
    {
      perror("fork");
      exit(1);
    }
    else if (pid == 0)
    {            // child process
      sleep(10); // sleep for 10 seconds (in the pdf given 10 secs)
      if (i == 0)
      { // 1. child
        int fd;
        int sum = 0;

        // open fif 1 for reading
        fd = open(FIFO1, O_RDONLY);
        if (fd == -1)
        {
          perror("open (FIFO1)");
          exit(1);
        }

        // read random numbers from fifo 1
        for (int j = 0; j < num; j++)
        {
          int n;
          if (read(fd, &n, sizeof(int)) == -1)
          {
            perror("read (FIFO1)");
            exit(1);
          }
          sum += n;
        }
        close(fd); // close the read olny fifo for 1.child
        printf("Child 1: Sum of random numbers = %d\n", sum);
      }
      else
      { // 2. child
        int fd;
        char command[10];
        int result = 1;

        // open fifo2 for read command from parent
        fd = open(FIFO2, O_RDONLY);
        if (fd == -1)
        {
          perror("open (FIFO2)");
          exit(1);
        }

        // rerd command from fifo 2
        if (read(fd, command, sizeof(command)) == -1)
        {
          perror("read (FIFO2)");
          exit(1);
        }
        close(fd); // close the read-only fifo for 2. child

        // check command is multiplyy
        if (strcmp(command, "multiply") == 0)
        {
          // open fifo 1 for reading
          fd = open(FIFO1, O_RDONLY);
          if (fd == -1)
          {
            perror("open (FIFO1)");
            exit(1);
          }

          // read random numbers from fifo 1 and after that multiplication
          for (int j = 0; j < num; j++)
          {
            int n;
            if (read(fd, &n, sizeof(int)) == -1)
            {
              perror("read (FIFO1)");
              exit(1);
            }
            result *= n;
          }
          close(fd); // close read only fifo for 2. child

          // print the multiplication result
          printf("Child 2: Multiplication of random numbers = %d\n", result);
        }
        else
        {
          printf("Child 2: Invalid command received\n");
        }
        exit(0); // 2. child  exits
      }
    }
    else
    { // parent process
      if (i == 0)
      { // for the first child
        // write random numbers to fifo 1
        int fd = open(FIFO1, O_WRONLY);
        if (fd == -1)
        {
          perror("open (FIFO1)");
          exit(1);
        }
        for (int i = 0; i < num; i++)
        {
          if (write(fd, &random_numbers[i], sizeof(int)) == -1)
          {
            perror("write (FIFO1)");
            exit(1);
          }
        }
        close(fd);
      }
      else
      { // for the second child
        // write random numbers to fifo 1

        int fd = open(FIFO1, O_WRONLY);
        if (fd == -1)
        {
          perror("open (FIFO1)");
          exit(1);
        }
        for (int i = 0; i < num; i++)
        {
          if (write(fd, &random_numbers[i], sizeof(int)) == -1)
          {
            perror("write (FIFO1)");
            exit(1);
          }
        }
        close(fd);

        // write "multiply" command to fifo 2
        fd = open(FIFO2, O_WRONLY);
        if (fd == -1)
        {
          perror("open (FIFO2)");
          exit(1);
        }
        if (write(fd, "multiply", sizeof("multiply")) == -1)
        {
          perror("write (FIFO2)");
          exit(1);
        }
        close(fd);
      }
      // sa.sa_handler = handle_child; // ı used to us it for handle child func but ı did it after the process because of the sleep 2 for proceeding .
    }
  }

  while (1)
  {
    printf("Proceeding...child counter(terminated) %d \t Total child %d \n", child_counter, total_children);
    sleep(2); // delay
    if (child_counter < total_children)
    {
      // wait child process to terminate
      pid_t terminated_pid = wait(NULL);

      if (terminated_pid > 0)
      {
        // printf("child process pid %d terminated with child count %d\n", terminated_pid, child_counter);
        child_counter++;
        printf("Proceeding...child counter(terminated) %d \t Total child %d \n", child_counter, total_children);
        sleep(2); // delay
      }
      else if (terminated_pid == 0)
      {
        printf("No child processes have terminated.\n");
      }
      else
      {
        if (terminated_pid == -1 && errno == ECHILD)
        {
          break;
        }
        else
        {
          printf("Error waitpid()\n");
          perror("waitpid");
          exit(1);
        }
      }
      child_counter++;
    }
    else
    {
      break;
    }
  }
  // printf("childcounter %d", child_counter);
  printf("All child processes have terminated.\n");

  // remove unlink fifos
  unlink(FIFO1);
  unlink(FIFO2);

  return 0;
}