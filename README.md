change keybindings of terminal applications

tkremap is a C program built upon libtermkey which allows you to change the keybindings of a terminal application.

It supports multiple keymodes like, similar to vi's input-/insert mode.

See [VI Config](confs/vi.conf) for a configuration sample.

Usage: **./tkremap** [_OPTIONS_] PROGRAM [_ARGUMENTS_...]  
  
_OPTIONS_  
  
&nbsp;&nbsp;&nbsp;&nbsp;**-C** _STRING_	Read config string  
&nbsp;&nbsp;&nbsp;&nbsp;**-c** _FILE_	Read config file (see -h load)  
&nbsp;&nbsp;&nbsp;&nbsp;**-m** _MODE_	Switch to _MODE_  
&nbsp;&nbsp;&nbsp;&nbsp;**-b** _KEY_ _CMD_	Alias for 'bind _KEY_ _CMD_'  
&nbsp;&nbsp;&nbsp;&nbsp;**-k** _IN_ _OUT_	Alias for 'bind _IN_ key _OUT_'  
&nbsp;&nbsp;&nbsp;&nbsp;**-u** _KEY_		Alias for 'unbind _KEY_  
&nbsp;&nbsp;&nbsp;&nbsp;**-v**		Load builtin vi config  
  
For more help:  
&nbsp;&nbsp;&nbsp;&nbsp;**-h** _COMMAND_  
&nbsp;&nbsp;&nbsp;&nbsp;**-h** commands  
&nbsp;&nbsp;&nbsp;&nbsp;**-h** keys  
&nbsp;&nbsp;&nbsp;&nbsp;**-h** all  
  
_Commands_  
  
**bind** _KEY_... _COMMAND_...  
&nbsp;&nbsp;&nbsp;&nbsp;Bind _KEY_ to _COMMAND_  
&nbsp;&nbsp;&nbsp;&nbsp;Multiple commands can be specified, they have to be seperated by '\;'.  
&nbsp;&nbsp;&nbsp;&nbsp;Keys can be chained  
  
**ignore**  
&nbsp;&nbsp;&nbsp;&nbsp;Completely ignore the current pressed key  
  
**key** [-r N] _KEY_...  
&nbsp;&nbsp;&nbsp;&nbsp;Send key to program  
  
&nbsp;&nbsp;&nbsp;&nbsp;**-r** _N_  Repeat the key N times  
  
**load** _FILE_  
&nbsp;&nbsp;&nbsp;&nbsp;Load configuration file  
&nbsp;&nbsp;&nbsp;&nbsp;If file is a sole filename it will be  
&nbsp;&nbsp;&nbsp;&nbsp;searched in the following places:  
&nbsp;&nbsp;&nbsp;&nbsp; - $PWD  
&nbsp;&nbsp;&nbsp;&nbsp; - $XDG_CONFIG_HOME/tkremap  
&nbsp;&nbsp;&nbsp;&nbsp; - $HOME/.config/tkremap  
&nbsp;&nbsp;&nbsp;&nbsp; - $HOME/.tkremap  
  
**mask**  
&nbsp;&nbsp;&nbsp;&nbsp;Do not interprete the next keypress as a keybinding  
  
**mode** _MODE|-p_  
&nbsp;&nbsp;&nbsp;&nbsp;Switch to _MODE_  
&nbsp;&nbsp;&nbsp;&nbsp;  
&nbsp;&nbsp;&nbsp;&nbsp;Pass **-p** for previous mode  
  
**pass**  
&nbsp;&nbsp;&nbsp;&nbsp;Send the pressed key to the program  
  
**readline** [-nCR] [-p PROMPT] [-i INIT] [-x X] [-y Y] [-k KEY] [-P TEXT] [-A TEXT]  
&nbsp;&nbsp;&nbsp;&nbsp;Write to program using readline  
&nbsp;&nbsp;&nbsp;&nbsp;  
&nbsp;&nbsp;&nbsp;&nbsp;(1) The program's window content is refreshed by resizing its PTY.  
&nbsp;&nbsp;&nbsp;&nbsp;You may use **-R** in conjunction with **-k** to send a key (e.g. **C-l**) for refreshing  
&nbsp;&nbsp;&nbsp;&nbsp;the screen instead (this may be more appropriate).  
  
&nbsp;&nbsp;&nbsp;&nbsp;**-p** _PROMPT_  Set prompt text  
&nbsp;&nbsp;&nbsp;&nbsp;**-i** _INIT_    Set pre fill buffer with text  
&nbsp;&nbsp;&nbsp;&nbsp;**-x** _X_       Set x cursor position (starting from left - use negative value to count from right)  
&nbsp;&nbsp;&nbsp;&nbsp;**-y** _Y_       Set y cursor position (starting from top - use negative value to count from bottom)  
&nbsp;&nbsp;&nbsp;&nbsp;**-n**         Do not write a tralining newline  
&nbsp;&nbsp;&nbsp;&nbsp;**-k** _KEY_     Send _KEY_ after writing line  
&nbsp;&nbsp;&nbsp;&nbsp;**-C**         Do not clear the cursor line  
&nbsp;&nbsp;&nbsp;&nbsp;**-R**         Do not refresh the window (1)  
&nbsp;&nbsp;&nbsp;&nbsp;**-P** _TEXT_    Prepend _TEXT_ to result  
&nbsp;&nbsp;&nbsp;&nbsp;**-A** _TEXT_    Append _TEXT_ to result  
  
**rehandle**  
&nbsp;&nbsp;&nbsp;&nbsp;Rehandle the current key  
  
**repeat** _on|off_  
&nbsp;&nbsp;&nbsp;&nbsp;Enable repetition mode  
  
**signal** _SIGNAL_  
&nbsp;&nbsp;&nbsp;&nbsp;Send signal to program  
  
**unbind** _KEY_... _COMMAND_...  
&nbsp;&nbsp;&nbsp;&nbsp;Unbind key to commands  
  
**unbound** [_TYPE_...] _COMMAND_...  
&nbsp;&nbsp;&nbsp;&nbsp;Specify action for unbound keys  
&nbsp;&nbsp;&nbsp;&nbsp;  
&nbsp;&nbsp;&nbsp;&nbsp;_TYPE_  
&nbsp;&nbsp;&nbsp;&nbsp; **char**     characters  
&nbsp;&nbsp;&nbsp;&nbsp; **sym**      symbolic keys or modified key  
&nbsp;&nbsp;&nbsp;&nbsp; **function** function keys  
&nbsp;&nbsp;&nbsp;&nbsp; **mouse**    mouse events  
&nbsp;&nbsp;&nbsp;&nbsp; **all**      char|sym|function [**default**]  
&nbsp;&nbsp;&nbsp;&nbsp;  
&nbsp;&nbsp;&nbsp;&nbsp;_COMMAND_  
&nbsp;&nbsp;&nbsp;&nbsp; Most of the time you want to use the following commands:  
&nbsp;&nbsp;&nbsp;&nbsp;  
&nbsp;&nbsp;&nbsp;&nbsp; **pass**     - for passing the key as is to the program  
&nbsp;&nbsp;&nbsp;&nbsp; **ignore**   - for completely ignoring the key  
&nbsp;&nbsp;&nbsp;&nbsp; **rehandle** - for rehandling the key (in conjunction with preceding **mode** command)  
  
**write** [-r N] _STRING_...  
&nbsp;&nbsp;&nbsp;&nbsp;Send string to program  
  
&nbsp;&nbsp;&nbsp;&nbsp;**-r** _N_  Repeat the string N times  
  
  
_Keys_  
  
&nbsp;&nbsp;&nbsp;&nbsp;**Symbolic keys**  
&nbsp;&nbsp;&nbsp;&nbsp; Up/Down/Left/Right, PageUp/PageDown, Home/End, Insert/Delete,  
&nbsp;&nbsp;&nbsp;&nbsp; Escape, Space, Enter, Tab, Backspace, F1 .. F12  
  
&nbsp;&nbsp;&nbsp;&nbsp;**Modifiers**  
&nbsp;&nbsp;&nbsp;&nbsp; **Control**: Control-key, Ctrl-key, C-key, ^key  
&nbsp;&nbsp;&nbsp;&nbsp; **Alt**:     Alt-key, A-key, Meta-key, M-key  
&nbsp;&nbsp;&nbsp;&nbsp; **Shift**:   Shift-key, S-key  
  
&nbsp;&nbsp;&nbsp;&nbsp;**Special**  
  
