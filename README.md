# tkremap
change keybindings of terminal applications

tkremap is a C program built upon libtermkey which allows you to change the keybindings of a terminal application.

It supports multiple keymodes like, similar to vi's input-/insert mode.

Usage: **./tkremap** [_OPTIONS_] PROGRAM [ARGUMENTS...]  
  
OPTIONS  
  
 **-C** _STRING_	Read config string  
 **-c** _FILE_	Read config file (see -h load)  
 **-m** _MODE_	Switch mode  
 **-b** _KEY_ _CMD_	Alias for 'bind _KEY_ _CMD_'  
 **-k** _IN_ _OUT_	Alias for 'bind _IN_ key _OUT_'  
 **-u** _KEY_	Alias for 'unbind _KEY_  
 **-v**		Load vi config  
  
For more help:  
 **-h** _COMMAND_  
 **-h** commands  
 **-h** config  
 **-h** keys  
  
_Configuration keywords_  
  
**mode** _MODE_  
 Start configuring MODE  
  
**bind** _KEY_... _COMMAND_...  
 Bind _KEY_ to _COMMAND_  
 Multiple commands can be specified, they have to be seperated by '\;'.  
 Keys can be chained  
  
**unbind** _KEY_... _COMMAND_...  
 Unbind key to commands  
  
**unbound** _TYPE_... _ACTION_  
 What to do on unbound keys  
   
 _TYPE_  
  **char**     characters  
  **sym**      symbolic keys or modified key  
  **function** function keys (F1..FN)  
  **mouse**    mouse events  
  **all**      char|sym|function  
   
 _ACTION_  
  **ignore**   discard key  
  **pass**     write key to program  
  **reeval**   re-evaluate (leave key chain, rehandle key again)  
  
**load** _FILE_  
 Load configuration file  
 See command load  
  
**repeat** _on|off_  
 Enable repetition mode  
  
  
_Available commands_  
  
**key** [-r N] _KEY_...  
 Send key to program  
  
 **-r** _N_  Repeat the key N times  
  
**goto** _MODE_  
 Switch the current mode  
  
**ignore**  
 Do nothing  
 This command can be used to ignore keypresses.  
  
**write** [-r N] _STRING_...  
 Write string to program  
  
 **-r** _N_  Repeat the string N times  
  
**mask**  
 Do not interprete the next keypress as a keybinding  
  
**pass**  
 Write the pressed key to the program  
  
**load** _FILE_  
 Load configuration file  
 If file is a sole filename it will be  
 searched in the following places:  
  - $PWD  
  - $XDG_CONFIG_HOME/.tkremap  
  - $HOME/.config/tkremap  
  - $HOME/.tkremap  
  
**signal** _SIGNAL_  
 Send signal to program  
  
**readline** [-nCR] [-p PROMPT] [-i INIT] [-x X] [-y Y] [-k KEY] [-P TEXT] [-A TEXT]  
 Write to program using readline  
   
 (1) The program's window content is refreshed by resizing its pty.  
 You may use **-R** in conjunction with **-k** to send a key (e.g. **C-l**) for refreshing  
 the screen instead. (This may be faster)  
  
 **-p** _PROMPT_  Set prompt text  
 **-i** _INIT_    Set pre fill buffer with text  
 **-x** _X_       Set x cursor position (starting from left - use negative value to count from right)  
 **-y** _Y_       Set y cursor position (starting from top - use negative value to count from bottom)  
 **-n**         Do not write a tralining newline  
 **-k** _KEY_     Send _KEY_ after writing line  
 **-C**         Do not clear the cursor line  
 **-R**         Do not refresh the window (1)  
 **-P** _TEXT_    Prepend _TEXT_ to result  
 **-A** _TEXT_    Append _TEXT_ to result  
  
  
_Keys_  
  
 **Symbolic keys**  
  Up/Down/Left/Right, PageUp/PageDown, Home/End,  
  Insert/Delete, Space, Enter, Tab, Backspace, F1 .. F12  
  
 **Modifiers**  
  **Control**: Control-key, Ctrl-key, C-key, ^key  
  **Alt**:     Alt-key, A-key, Meta-key, M-key  
  **Shift**:   Shift-key, S-key  
  
