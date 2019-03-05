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
CC_FLAGS += -DREADME
endif

CMDS = bind unbind unbound core key load readline signal write
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
	sed -n '/^Usage:/q;p' README.md > README.new
	./$(PROGNAME) -h all | sed 's/^ /\&nbsp;\&nbsp;\&nbsp;\&nbsp;/g; s/$$/  /g' >> README.new
	mv README.new README.md

$(PROGNAME).1: .force
	cp README.md $(PROGNAME).1.ronn
	ronn --roff $(PROGNAME).1.ronn
	rm -f $(PROGNAME).1.ronn

vi_conf.h: .force
	./tools/stripconf.py -c -v VI_CONF confs/vi.conf > vi_conf.h

clean:
	rm -f $(PROGNAME)
	rm -rf objs

nanotest:
	./$(PROGRAM) -v nano vi_conf.h

.force:
	@true

