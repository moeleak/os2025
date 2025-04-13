#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main() {
    int pipefd[2];  // pipefd[0] - read, pipefd[1] - write
    pid_t pid;
    char buf[256];

    // 创建管道
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // 创建子进程
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {  // 子进程
        close(pipefd[1]);  // 关闭写端

        // 读取父进程发送的消息
        ssize_t n = read(pipefd[0], buf, sizeof(buf));
        if (n > 0) {
            printf("[%d] Read: \"%s\"\n", getpid(), buf);
        }

        close(pipefd[0]);
        exit(EXIT_SUCCESS);
    } else {  // 父进程
        close(pipefd[0]);  // 关闭读端

        char *msg = "Hello, World!";
        printf("[%d] Write: \"%s\"\n", getpid(), msg);
        write(pipefd[1], msg, strlen(msg) + 1);

        close(pipefd[1]);
        wait(NULL);  // 等待子进程结束
        exit(EXIT_SUCCESS);
    }

    return 0;
}

