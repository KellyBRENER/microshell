#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

// Function to write an error message to stderr
int err(char *str)
{
    // Loop through each character in the string and write it to stderr
    while (*str)
        write(2, str++, 1);
    return 1;
}

// Function to change the current working directory
int cd(char **argv, int i)
{
    // If the number of arguments is not 2, return an error
    if (i != 2)
        return err("error: cd: bad arguments\n");
    // If changing the directory fails, return an error
    if (chdir(argv[1]) == -1)
        return err("error: cd: cannot change directory to "), err(argv[1]), err("\n");
    return 0;
}

// Function to execute a command
int exec(char **argv, int i, char **envp)
{
    int fd[2];
    int status;
    int has_pipe = argv[i] && !strcmp(argv[i], "|");//=TRUE si | (1), FALSE sinon (0);

    if (!has_pipe && !strcmp(*argv, "cd"))//si pas de pipe et argv[0] = cd
        return cd(argv, i);//exec cd
    if (has_pipe && pipe(fd) == -1)//si pipe, et que la creation de pipe echoue, retourne err
        return err("error: fatal\n");
    int pid = fork();//creation de fork (meme si pas de pipe pour ne pas sortir du main)
    if (!pid)//si on dans l'enfant
    {
        argv[i] = 0;//pour terminer le tableau par une string null
		/*si pipe, on dup2*/
        if (has_pipe && (dup2(fd[1], 1) == -1 || close(fd[0]) == -1 || close(fd[1]) == -1))
            exit(err("error: fatal\n"));
        /*si cd, erreur (cd doit etre donne sans pipe ni avant ni apres)*/
        if (!strcmp(*argv, "cd"))
            exit(cd(argv, i));
        execve(*argv, argv, envp);//execute la commande
        err("error: cannot execute ");
		err(*argv);
		exit(err("\n"));//sinon erreur
    }
    waitpid(pid, &status, 0);
    // If the command includes a pipe and setting up the pipe fails, return an error
    if (has_pipe && (dup2(fd[0], 0) == -1 || close(fd[0]) == -1 || close(fd[1]) == -1))
        return err("error: fatal\n");
    // Return the exit status of the child process
    return WIFEXITED(status) && WEXITSTATUS(status);
}

int main(int argc, char **argv, char **envp)
{
    int    i = 0;
    int    status = 0;

    if (argc > 1)
    {
        // Loop through each argument
        while (argv[i] && argv[++i])
        {
            // Move the pointer to the next argument
            argv += i;
            i = 0;
            // Loop through each argument until a pipe or semicolon is found
            while (argv[i] && strcmp(argv[i], "|") && strcmp(argv[i], ";"))
                i++;
            // If there are arguments, execute them
            if (i)
                status = exec(argv, i, envp);
        }
    }
    return status;
}
