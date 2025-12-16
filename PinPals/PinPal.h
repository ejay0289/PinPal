#pragma once
#define UNICODE
#define _UNICODE
#include <windows.h>
#include "Resource.h"
#include "./SQLite/sqlite3.h"
#include <stdint.h>

#define MAX_NOTE_TEXT_LEN 2056
#define TOTAL_NUMBER_OF_TAGS 3



struct Note {
    RECT rect;
    wchar_t title[50];
    wchar_t text[MAX_NOTE_TEXT_LEN];
    uint32_t noteColor;
    HWND noteHandle;
    int textLen;
    int id;
    int tmpId;
    int tags[TOTAL_NUMBER_OF_TAGS];
    int noteWidth;//TODO: Persist note width and height in DB entry and restore to the last known state
    int noteHeight;
};

enum noteTags {
//TODO: Add meaningful tags for note filtering
};


int getNoteCount(sqlite3* db);
uint32_t getNoteColor(int noteId);
void deleteNoteFromDatabase(int noteId);
int addToDatabase(struct Note* note);
char* getNoteContent(int noteId);
char* getNoteTitle(int noteId);
void RecalculateNotePositions(HWND hwnd);
void calculateColorRectPosition(HWND hwnd);
void initNotes();
int OpenDatabase(void);
void updateDatabaseEntry(int noteId, const char* noteContent, const char* noteTitle, uint32_t noteColor);
wchar_t* Utf8ToWide(const char* utf8);
void searchDatabase(const char* searchText);


