#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

void demo_dup_offset(){
  int fd1 = open("test_dup_file.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd1 == -1) {
    perror("open fd1 failed");
    return;
  }
  printf("fd1 = %d\n", fd1);

  // 使用 dup 复制文件描述符
  int fd2 = dup(fd1);
  if (fd2 == -1) {
    perror("dup failed");
    close(fd1);
    return;
  }
  printf("fd2 (dup of fd1) = %d\n", fd2);

  // 通过两个文件描述符写入数据
  const char *msg1 = "This is written through fd1\n";
  const char *msg2 = "This is written through fd2\n";

  write(fd1, msg1, strlen(msg1));
  write(fd2, msg2, strlen(msg2));

  // 关闭文件描述符
  close(fd1);
  close(fd2);

  printf("Data written to test_file.txt. Check the file contents.\n");
}

void demo_fork_offset() {
  int fd = open("test_fork_file.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  pid_t pid = fork();
  if (pid < 0) {
    perror("Failed to fork");
    close(fd);
  } else if (pid == 0) {
    // Child process
    write(fd, "D", 1);
    exit(EXIT_SUCCESS);
  } else {
    // Parent process
    wait(NULL); // Wait for the child
    write(fd, "C", 1);
    close(fd);
  }
}

int main() {
  demo_dup_offset();
  return 0;
}

