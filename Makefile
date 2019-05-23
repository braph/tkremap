PROGNAME = tkremap
LIBS 		 = -lutil -ltermkey -lcurses -lreadline
PREFIX   = /usr
STRIP    = strip
override CFLAGS += -Wall -z muldef

ifeq ($(DEBUG), 1)
	CFLAGS 		+= -O0 -g -DFREE_MEMORY=1 -DDEBUG=1
	STRIP      = true
else
	CFLAGS 		+= -O3
endif

ifeq ($(FREE_MEMORY), 1)
	CFLAGS 		+= -DFREE_MEMORY=1
endif

ifeq ($(README), 1)
	CFLAGS 		+= -DREADME
endif

COMMANDS = bind exec group unbind unbound core key load readline redraw_method signal suspend write
COMMANDS_BAS := $(addprefix cmd_, $(COMMANDS))
COMMANDS_SRC := $(addsuffix .c, $(COMMANDS_BAS))
COMMANDS_OBJ := $(addsuffix .o, $(COMMANDS_BAS))

OBJS = aliases termkeystuff tkremap conf common lexer options commands help errormsg
OBJS_SRC := $(addsuffix .c, $(OBJS))
OBJS_OBJ := $(addsuffix .o, $(OBJS))

#COMMAND_STRUCTS = $(shell sed -n -r 's/command_t (command_[a-z]+).*/\1/p' $(COMMANDS_SRC) | sort)

build:
	$(CC) $(CFLAGS) $(LIBS) $(OBJS_SRC) $(COMMANDS_SRC) main.c -o $(PROGNAME)
	$(STRIP) $(PROGNAME)

build2: $(COMMANDS_OBJ) $(OBJS_OBJ)
	$(CC) $(CFLAGS) $(LIBS) objs/*.o main.c -o $(PROGNAME)
	$(STRIP) $(PROGNAME)

%.o:
	@mkdir -p objs
	$(CC) $(CFLAGS) -c $*.c -o objs/$*.o

install:
	install -m 0755 $(PROGNAME) $(PREFIX)/bin/$(PROGNAME)

README.md: .force
	sed -n '/^Usage:/q;p' README.md > README.new
	./$(PROGNAME) -h all \
		| sed 's/^ /\&nbsp;\&nbsp;\&nbsp;\&nbsp;/g; s/$$/  /g' >> README.new
	mv README.new README.md

$(PROGNAME).1: .force
	sed -n '/^Usage:/q;p' README.md > $(PROGNAME).1.ronn
	./$(PROGNAME) -h all \
		| sed 's/$$/  /g' \
		| sed 's/^_OPTIONS_/## OPTIONS/g' \
		| sed 's/^_Commands_/## COMMANDS/g' \
		| sed 's/^_Keys_/## KEYS/g' \
		>> $(PROGNAME).1.ronn
	ronn --roff $(PROGNAME).1.ronn
	rm -f $(PROGNAME).1.ronn

vi_conf.h: .force
	./tools/stripconf.py -c -v VI_CONF confs/vi.conf > vi_conf.h

copy_confs_to_home:
	mkdir -p ~/.config/$(PROGNAME)
	cp confs/*.conf ~/.config/$(PROGNAME)

clean:
	rm -f $(PROGNAME)
	rm -rf objs

nanotest:
	./$(PROGRAM) -v nano vi_conf.h

.force:
	@true

