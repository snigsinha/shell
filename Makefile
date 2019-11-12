CFLAGS = -g3 -Wall -Wextra -Wconversion -Wcast-qual -Wcast-align -g
CFLAGS += -Winline -Wfloat-equal -Wnested-externs
CFLAGS += -pedantic -std=gnu99
CFLAGS += -D_GNU_SOURCE 
CC = gcc
33 = 33sh
33NO = 33noprompt
OBJECTS = sh.c jobs.c

PROMPT = -DPROMPT

EXECS = 33sh 33noprompt

.PHONY = clean all

all: $(EXECS)

$(33): $(OBJECTS) jobs.h
	$(CC) $(CFLAGS) -o $(33) -DPROMPT $(OBJECTS)
	
$(33NO): $(OBJECTS) jobs.h
	$(CC) $(CFLAGS) -o $(33NO) $(OBJECTS)

clean:
	rm -f $(EXECS)

