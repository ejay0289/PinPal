#pragma once
#define UNICODE
#define _UNICODE
#include <windows.h>
#include "Resource.h"
#include "sqlite3.h"
#include <stdint.h>

#define MAX_NOTE_TEXT_LEN 2056


struct Note {
    RECT rect;
    wchar_t title[50];
    wchar_t text[MAX_NOTE_TEXT_LEN];
    int textLen;
    int id;
    uint32_t noteColor;
};

int getNoteCount(sqlite3* db);
uint32_t getNoteColor(int noteId);
void deleteNoteFromDatabase(int noteId);
int addToDatabase(struct Note* note);
char* getNoteContent(int noteId);
char* getNoteTitle(int noteId);
void RecalculateNotePositions(HWND hwnd);
void calculateColorRectPosition(HWND hwnd);
int OpenDatabase(void);
void updateDatabaseEntry(int noteId, const wchar_t* noteContent, const wchar_t* noteTitle,uint32_t noteColor);
wchar_t* Utf8ToWide(const char* utf8);

