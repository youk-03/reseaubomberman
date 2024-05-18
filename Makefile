CC=gcc
CFLAGS=-g -DMAC -Wall -lncurses
DEPS= format_messages.h serveur/joueur.h serveur/partie.h client/auxiliaire.h game/game.h game/myncurses.h
EXEC=serveur/serveur client/client

all : serveur/serveur client/client


client/client : client/client.o client/auxiliaire.o game/ncurses.o game/game.o
	$(CC) -o $@ $^ $(LIBS) -lncurses 
	
client/%.o: %.c $(DEPS)
	$(CC) -o $@ -c $< $(CFLAGS)

serveur/serveur : serveur/serveur.o serveur/partie.o serveur/joueur.o game/ncurses.o game/game.o client/auxiliaire.o
	$(CC) -o $@ $^ $(LIBS) -lncurses

serveur/%.o: %.c $(DEPS)
	$(CC) -o $@ -c $< $(CFLAGS)

game/%.o: %.c $(DEPS)
	$(CC) -o $@ -c $< $(CFLAGS)

clean :
	rm -rf $(EXEC) */*.o