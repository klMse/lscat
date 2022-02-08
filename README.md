# lscat

## Does:
It is intend to combine both `ls` and `cat` dependent on the arguments passed to it. It's further behaviour is controlled by the mode `lscat` run under.

## Modes:
### Legend
| Variable Name | Meaning                                               |
| ------------- | ----------------------------------------------------- |
| PROGRAM       | The program choosen either from config or default     |
| CONFIG_ARGS   | The arguments provided by the config for this PROGRAM |
| ARGS          | The arguments passed to `lscat`                       |

The `PROGRAM` is called like this:

`PROGRAM CONFIG_ARGS ARGS`

### File mode
This mode is choosen, if all the arguments passed to `lscat` are files and `lscat` is called as `lscat-file`.
Sets PROGRAM to specified in the config by _FILE_EXEC_ or the default program `cat`.
If arguments starting with '-' are passed they are added to the PROGRAM call.

### Dir mode
This mode is choosen, if all the arguments passed to `lscat` are directories and `lscat` is called as `lscat-dir`.
Sets PROGRAM to specified in the config by _DIR_EXEC_ or the default program `ls`.
If arguments starting with '-' are passed they are added to the PROGRAM call.

### Generic mode
This mode is choosen, if `lscat` is called as `lscat` and/or the arguments passed to it are a combination of files and directories.
Sets PROGRAM to specified in the config by _DIR_EXEC_ or the default program `ls` for all directories and sets PROGRAM to specified in the config
by _FILE_EXEC_ or the default program `cat`. Then both PROGRAM's are called if needed.
If arguments starting with '-' are passed and they are not a file or directory they are ignored.

### Notes
If no arguments are passed `lscat` assumes dir mode.
Supports multiple arguments and mixed files and directories in arguments.

## Config
The default `config` is when installed via AUR under `/usr/share/doc/lscat/example` and should be copied to `~/.config/lscat`

### Options
- _FILE_EXEC_: What program to run if encountering a file
- _DIR_EXEC_: What program to run if encountering a directory
- _FILE_ARGS_: What arguments to pass to the file program
- _DIR_ARGS_: What arguments to pass to the directory program

# TODO
- Make the whole execution process, especially the assembly of the args array better