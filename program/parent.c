#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    int pipe1[2] = {-1, -1};
    int pipe2[2] = {-1, -1};
    int pipe3[2] = {-1, -1};
    struct sigaction action = {0};
    action.sa_handler = SIG_IGN;
    if (sigemptyset(&action.sa_mask) == -1 || sigaction(SIGPIPE, &action, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1 || pipe(pipe3) == -1) {
        perror("pipe");
        for (int i = 0; i < 2; i++) {
            if (pipe1[i] != -1) {
                close(pipe1[i]);
            }
            if (pipe2[i] != -1) {
                close(pipe2[i]);
            }
            if (pipe3[i] != -1) {
                close(pipe3[i]);
            }
        }
        return 1;
    }

    pid_t child1_pid = fork();
    if (child1_pid == -1) {
        perror("fork child1");
        for (int i = 0; i < 2; i++) {
            close(pipe1[i]);
            close(pipe2[i]);
            close(pipe3[i]);
        }
        return 1;
    }
    if (child1_pid == 0) {
        if (dup2(pipe1[0], STDIN_FILENO) == -1 || dup2(pipe2[1], STDOUT_FILENO) == -1) {
            perror("dup2 child1");
            _exit(1);
        }

        int close_error = 0;
        for (int i = 0; i < 2; i++) {
            if (close(pipe1[i]) == -1) {
                close_error = 1;
            }
            if (close(pipe2[i]) == -1) {
                close_error = 1;
            }
            if (close(pipe3[i]) == -1) {
                close_error = 1;
            }
        }
        if (close_error) {
            perror("close");
            _exit(1);
        }

        execl("./child1", "child1", NULL);
        perror("execl child1");
        _exit(1);
    }

    pid_t child2_pid = fork();
    if (child2_pid == -1) {
        perror("fork child2");
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        close(pipe3[0]);
        close(pipe3[1]);
        waitpid(child1_pid, NULL, 0);
        return 1;
    }
    if (child2_pid == 0) {
        if (dup2(pipe2[0], STDIN_FILENO) == -1 || dup2(pipe3[1], STDOUT_FILENO) == -1) {
            perror("dup2 child2");
            _exit(1);
        }

        int close_error = 0;
        for (int i = 0; i < 2; i++) {
            if (close(pipe1[i]) == -1) {
                close_error = 1;
            }
            if (close(pipe2[i]) == -1) {
                close_error = 1;
            }
            if (close(pipe3[i]) == -1) {
                close_error = 1;
            }
        }
        if (close_error) {
            perror("close");
            _exit(1);
        }

        execl("./child2", "child2", NULL);
        perror("execl child2");
        _exit(1);
    }

    int error = 0;
    if (close(pipe1[0]) == -1) {
        error = 1;
    }
    if (close(pipe2[0]) == -1) {
        error = 1;
    }
    if (close(pipe2[1]) == -1) {
        error = 1;
    }
    if (close(pipe3[1]) == -1) {
        error = 1;
    }
    if (error) {
        perror("close");
    }

    FILE *pipe3_input = fdopen(pipe3[0], "r");
    if (pipe3_input == NULL) {
        perror("fdopen");
        close(pipe1[1]);
        close(pipe3[0]);
        waitpid(child1_pid, NULL, 0);
        waitpid(child2_pid, NULL, 0);
        return 1;
    }

    char *line = NULL;
    char *result = NULL;
    size_t line_capacity = 0;
    size_t result_capacity = 0;
    ssize_t line_length;
    int pipe1_open = 1;

    while (!error && (line_length = getline(&line, &line_capacity, stdin)) != -1) {
        size_t written = 0;

        while (written < (size_t)line_length) {
            ssize_t count = write(pipe1[1], line + written, (size_t)line_length - written);
            if (count == -1 && errno == EINTR) {
                continue;
            }
            if (count <= 0) {
                perror("write");
                error = 1;
                break;
            }
            written += (size_t)count;
        }
        if (error) {
            break;
        }

        if (line[line_length - 1] != '\n') {
            if (close(pipe1[1]) == -1) {
                perror("close pipe1");
                error = 1;
            }
            pipe1_open = 0;
        }

        ssize_t result_length = getline(&result, &result_capacity, pipe3_input);
        if (result_length == -1) {
            fprintf(stderr, "Cannot read result from child2\n");
            error = 1;
            break;
        }
        if (fwrite(result, 1, (size_t)result_length, stdout) != (size_t)result_length || fflush(stdout) == EOF) {
            perror("stdout");
            error = 1;
            break;
        }
    }

    if (!error && !feof(stdin)) {
        perror("stdin");
        error = 1;
    }

    free(line);
    free(result);
    if (pipe1_open && close(pipe1[1]) == -1) {
        perror("close pipe1");
        error = 1;
    }
    if (fclose(pipe3_input) == EOF) {
        perror("fclose pipe3");
        error = 1;
    }

    pid_t children[2] = {child1_pid, child2_pid};
    for (int i = 0; i < 2; i++) {
        int status;
        pid_t waited;
        do {
            waited = waitpid(children[i], &status, 0);
        } while (waited == -1 && errno == EINTR);

        if (waited == -1) {
            perror("waitpid");
            error = 1;
        } else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "child%d failed\n", i + 1);
            error = 1;
        }
    }

    return error;
}
