# lscat

## Does:
Behaves by default like ls if argument is a directory and like cat if argument is a file. Supports multiple arguments
and mixed files and directories in arguments.

## Config
The default `config` is when installed via AUR under `/usr/share/doc/lscat/example` and should be copied to `~/.config/lscat`

### Options
- _FILE_EXEC_: What program to run if encountering a file
- _DIR_EXEC_: What program to run if encountering a directory
- _FILE_ARGS_: What arguments to pass to the file program
- _DIR_ARGS_: What arguments to pass to the directory program
- _MULTIPLE_CALLS_: Should all files and all directories be grouped together into one call to the specified program or not. True means only one argument is passed to the specific program

# TODO
- Make the whole execution process, especially the assembly of the args array better
- Add feature: If first argument starts with `-` use last argument to determine file or directory behaviour and pass all the arguments to the specific program