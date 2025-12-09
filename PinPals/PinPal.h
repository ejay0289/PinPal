#pragma once

#include <windows.h>
#include "Resource.h"
#include "sqlite3.h"

#define MAX_NOTE_TEXT_LEN 1024



struct Note {
    RECT rect;
    char title[50];
    char text[MAX_NOTE_TEXT_LEN];
    int textLen;
    int id;
};

int getNoteCount(sqlite3* db);
void deleteNoteFromDatabase(int noteId);
int addToDatabase(struct Note* note);
char* getNoteContent(int noteId);
char* getNoteTitle(int noteId);
void RecalculateNotePositions(HWND hwnd);
int OpenDatabase(void);
void updateDatabaseEntry(int noteId, const char* noteContent, const char* noteTitle);

