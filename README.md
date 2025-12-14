# 📌 PinPal

**PinPal** is a lightweight, native Windows sticky-notes application built in **pure C using the Win32 API**.  
It’s designed to stay out of your way — until you need it — then instantly surface your notes right where you want them.

No frameworks. No bloat. Just fast, direct Windows programming.

---

## Features

- **Sticky Notes**  
  Create lightweight note windows with titles and content, rendered using custom Win32 painting.

- **Pin to Foreground**  
  Pin notes so they stay **always on top**, perfect for reminders, tasks, or reference material.

- **Color-Coded Notes**  
  Assign background colors to notes for quick visual organization.

- **Persistent Storage (SQLite)**  
  Notes are saved and restored automatically using a local SQLite database.

- **Native Win32 Windows**  
  Each note is a real window — resizable, movable, and integrated naturally with Windows.

- **Manual Memory Management**  
  Notes are stored in a dynamically managed array (`realloc` + `memmove`) for precise control and performance.

- **Fast Startup & Low Overhead**  
  No runtime dependencies beyond what Windows already provides.

---

## To do:
- Add checklist
- Keyboard shortcuts for quick note creation
- System tray integration for quick access
- Review design
- Search / Filter Notes
- Reminders and notifications. Optional pop-up reminders for specific notes.
- Custom Fonts & Text Styles
- Transparent effect, because why not
- Pinning important notes in the pinboard
- Tagging / Categories

## Built With

- **C (ISO C / MSVC)**
- **Win32 API**
- **SQLite3**
- **GDI** for custom drawing
- **Visual Studio** (Debug & Release builds)

---

## How It Works (High Level)

- Each note is represented by a `struct Note` stored in a dynamically resized array.
- Notes are inserted at the front of the array for quick access and rendering.
- Window state (color, pin status, content) is synced between:
  - The Win32 window
  - The in-memory note array
  - The SQLite database
- Custom messages (`WM_APP_*`) are used to coordinate updates cleanly and safely.
- Pinned notes use window styles to stay above other windows without stealing focus.

---

## Design Goals

- Stay **simple and native**
- Avoid unnecessary abstractions
- Learn and leverage **real Win32 patterns**
- Build something useful while understanding *exactly* how it works

---

## Status

PinPal is actively developed and evolving.  
Current focus areas:

- UI polish
- Improved note management
- Additional quality-of-life features

---

## Screenshots

<img width="375" height="403" alt="image" src="https://github.com/user-attachments/assets/896d9146-44fc-40d1-9d49-0421e7ce53da" />

<img width="1910" height="1034" alt="image" src="https://github.com/user-attachments/assets/11bbcc74-4f24-42c4-81fb-e5653ec73291" />

<img width="1917" height="1037" alt="image" src="https://github.com/user-attachments/assets/9340b854-5d8d-4f1c-9547-ed136d0cd792" />



---

## License

This project is currently for personal and educational use.

---

## Author Notes

PinPal is part of a larger journey into low-level Windows development, memory management, and system programming — built intentionally without C++ or external UI frameworks to understand what’s really happening under the hood.
