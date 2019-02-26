PROGNAME   = tkremap
LIBS 		  = -lutil -ltermkey -lcurses -lpthread -lreadline
PREFIX     = /usr
STRIP      = strip
CC_FLAGS   = -Wall

ifeq ($(DEBUG), 1)
	CC_FLAGS += -g -DFREE_MEMORY=1 -DDEBUG=1
	STRIP     = true
else
	CC_FLAGS += -O3
endif

ifeq ($(README), 1)
CC_FLAGS += -DBOLD='"**"' -DBOLD_END='"**"' -DITALIC='"_"' -DITALIC_END='"_"'
endif

CMDS = goto ignore key load mask pass readline signal write
CMDS := $(addprefix cmd_, $(CMDS))

OBJS  = $(CMDS) termkeystuff bind_parse tkremap conf lexer options commands help errormsg
OBJS := $(addsuffix .o, $(OBJS))

build: $(OBJS)
	$(CC) $(CC_FLAGS) $(LIBS) objs/*.o main.c -o $(PROGNAME)
	$(STRIP) $(PROGNAME)

%.o:
	@mkdir -p objs
	$(CC) $(CC_FLAGS) $(LIBS) -c $*.c -o objs/$*.o

install:
	install -m 0755 $(PROGNAME) $(PREFIX)/bin/$(PROGNAME)

README.md: .force
	./$(PROGNAME) -h all | sed 's/$$/  /g' >> README.md

doc: .force
	./tools/genhelp.py > $(PROGNAME).md
	go-md2man -in $(PROGNAME).md -out $(PROGNAME).1

vi_conf.h: .force
	./tools/stripconf.py -c -v VI_CONF confs/vi.conf > vi_conf.h

clean:
	rm -f $(PROGNAME)
	rm -rf objs

nanotest:
	./$(PROGRAM) -v nano vi_conf.h

.force:
	@true

