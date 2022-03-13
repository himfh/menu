/*
======================================================================
PROGRAM C Editor - An editor with top-down menus.
@author : Velorek
@version : 1.0
Last modified : 17/04/2021 - Added real MilliSecond timer (tm.c)
                 Added home/end keys
                 Clean -wExtra warnings
                 Added signedchar to makefile
======================================================================*/

/*====================================================================*/
/* COMPILER DIRECTIVES AND INCLUDES                                   */
/*====================================================================*/

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "rterm.h"
#include "listc.h"
#include "scbuf.h"
#include "opfile.h"
#include "uintf.h"
#include "fileb.h"
#include "keyb.h"
#include "tm.h"
/*====================================================================*/
/* CONSTANT VALUES                                                    */
/*====================================================================*/

//GOODBYE MSG - ASCII ART
#define cedit_ascii_1 "  _____      ______    _ _ _   \n"
#define cedit_ascii_2 " / ____|    |  ____|  | (_) |  \n"
#define cedit_ascii_3 "| |   ______| |__   __| |_| |_ \n"
#define cedit_ascii_4 "| |  |______|  __| / _` | | __|\n"
#define cedit_ascii_5 "| |____     | |___| (_| | | |_ \n"
#define cedit_ascii_6 " \\_____|    |______\\__,_|_|\\__|\n"

//ABOUT - ASCII ART
#define ABOUT_ASC_1 "        _   __       \n"
#define ABOUT_ASC_2 "       /  _|__ _|.___\n"
#define ABOUT_ASC_3 "       \\_  |__(_]| |  v0.1 \n       Coded   by  V3l0rek\n "
#define ANIMATION "||//--\\\\"

//USER-DEFINED MESSAGES
#define UNKNOWN "UNTITLED"
#define WINDOWMSG "DISPLAY IS TOO SMALL. PLEASE, RESIZE WINDOW"
#define STATUS_BAR_MSG1  " [C-Edit] | F2,CTRL+L: MENU | F1: HELP"
#define STATUS_BAR_MSG2 " [C-Edit] Press ESC to exit menu.             "
#define STATUS_BAR_MSG3 " ENTER: SELECT | <- -> ARROW KEYS             "
#define WLEAVE_MSG "\n       Are you sure\n    you want to quit?"
#define WSAVE_MSG ":\nFile saved successfully!"
#define WSAVELABEL_MSG "[-] File:"
#define WINFO_NOPEN "Error:\nThere isn't any file open!"
#define WINFO_SIZE "-File size: "
#define WINFO_SIZE2 "\n-No. of lines: "
#define WINFO_SIZE3 "\n-File name: "
#define WCHECKFILE2_MSG " File does not exist!  \n\n A new buffer will be created. \n"
#define WCHECKFILE_MSG " This file isn't a  \n text file. Program \n may crash. Open anyway?"
#define WINFONOTYET_MSG "Not implemented yet!"
#define WCHECKLINES_MSG " File longer than 3000 \n lines. You'll view those \n lines as read Mode! "
#define WMODIFIED_MSG " File has been modified\n Save current buffer?"
#define WFILEEXISTS_MSG " File exists. \n Overwrite?"
#define WFILEINREADMODE_MSG " File is on read mode. \n"

//MISC. CONSTANTS
#define EXIT_FLAG -1
#define TAB_DISTANCE 8      //How many spaces TAB key will send.
#define START_CURSOR_X 2
#define START_CURSOR_Y 3
#define ROWS_FAILSAFE 25
#define COLUMNS_FAILSAFE 80
#define K_LEFTMENU -1       //Left arrow key pressed while in menu
#define K_RIGHTMENU -2      //Right arrow key pressed while in menu
#define MAX_TEXT 150
#define MAX_PATH 1024

//MENU CONSTANTS
#define HOR_MENU -1
#define FILE_MENU 0
#define OPT_MENU 1
#define HELP_MENU 2
#define YESNO_MENU 3
#define OK_MENU 4
#define MAX_FILENAME 100

//DROP-DOWN MENUS
#define OPTION_1 0
#define OPTION_2 1
#define OPTION_3 2
#define OPTION_4 3
#define OPTION_5 4
#define OPTION_NIL -1       //Reset option
#define CONFIRMATION 1

// DISPLAY CONSTANTS
#define FILL_CHAR 32

//Temporary implementation- Static Buffer
//EDIT AREA CONSTANTS
//Edit buffer is currently 3000 lines x 500 chars

#define MAX_CHARS 500       // 500 chars per line
#define MAX_LINES 3000      // 3000 lines per page (max mem 32750)

#define CHAR_NIL '\0'
#define END_LINE_CHAR 0x0A  // $0A
#define VSCROLL_ON 1
#define VSCROLL_OFF 0
#define DIRECTION_DOWN 1
#define DIRECTION_UP 0

//FILE CONSTANTS
#define FILE_MODIFIED 1
#define FILE_UNMODIFIED 0
#define FILE_READMODE 2

/*====================================================================*/
/* TYPEDEF DEFINITIONS                                                */
/*====================================================================*/

typedef struct _charint {
  char    ch;
  char     specialChar; //to account for chars with accents in Unix
} CHARINT;

typedef struct _editbuffer {
  CHARINT charBuf[MAX_LINES];
} EDITBUFFER;

typedef struct _editScroll {
  long totalLines; //Maximum no. of lines in File or Buffer
  long displayLength; //number of current display Lines
  float scrollRatio; //used for scroll indicator
  long scrollLimit; //limit of scroll
  long scrollPointer; //last pointed/shown line
  long pagePointer; //last page displayed
  char scrollActiveV; //whether there are lines to scroll or not.
  long bufferX; //coordinates within the buffer
  long bufferY; //coordinates within the buffer
} EDITSCROLL;


/*====================================================================*/
/* GLOBAL VARIABLES */
/*====================================================================*/

LISTCHOICE *mylist, data; //menus handler
SCREENCELL *my_screen; //display handler
SCROLLDATA openFileData; //openFile dialog
EDITBUFFER editBuffer[MAX_LINES]; //edit buffer
EDITSCROLL editScroll; //scroll Values

FILE   *filePtr;

int     rows = 0, columns = 0, old_rows = 0, old_columns = 0;   // Terminal dimensions
long    cursorX = START_CURSOR_X, cursorY = START_CURSOR_Y; //Cursor position
long    oldX = START_CURSOR_X, oldY = START_CURSOR_Y; //Cursor position blink
int     timerCursor = 0;    // Timer
char    kglobal = 0;        // Global variable for menu animation
char    currentFile[MAX_TEXT];
long    limitRow = 0, limitCol = 0; // These variables account for the current limits of the buffer.
int     c_animation = 0;    //Counter for animation
long    maxLineFile = 1;        //Last line of file
char    currentPath[MAX_PATH];
//FLAGS
int     exitp = 0;      // Exit flag for main loop
int     fileModified = FILE_UNMODIFIED; //Have we modified the buffer?
char    timerOnOFF = 1;
int forceBufferUpdate = 0; //To allow a smoother scroll animation.
NTIMER  _timer1; //Timer for animations and screen update
/*====================================================================*/
/* PROTOTYPES OF FUNCTIONS                                            */
/*====================================================================*/

//Display prototypes
void    credits();
int     main_screen();
int     refresh_screen(int force_refresh);
void    flush_editarea();
void    cleanStatusBar();

//Timers
void    draw_cursor(long whereX, long whereY, long oldX, long oldY);
//int     timer_1(int *timer1, long whereX, long whereY, char onOff);
int     _tick();
void    update_indicators();

//Dialogs & menus
void    filemenu();
void    optionsmenu();
void    helpmenu();
int     confirmation();
int     about_info();
int     help_info();
void    drop_down(char *kglobal);
char    horizontal_menu();
int     newDialog(char fileName[MAX_TEXT]);
int     saveDialog(char fileName[MAX_TEXT]);
int     saveasDialog(char fileName[MAX_TEXT]);
int     openFileHandler();
int     fileInfoDialog();

//Keyhandling and input prototypes
int     process_input(EDITBUFFER editBuffer[MAX_LINES], long *whereX,
              long *whereY, char ch);
int     special_keys(long *whereX, long *whereY, char ch);

//Edit prototypes
void    cleanBuffer(EDITBUFFER editBuffer[MAX_LINES]);
int     writetoBuffer(EDITBUFFER editBuffer[MAX_LINES], long whereX,
              long whereY, char ch);
int     findEndline(EDITBUFFER editBuffer[MAX_LINES], long line);

//Scroll prototypes
short   checkScrollValues(long lines);

//File-handling prototypes
int     writeBuffertoFile(FILE * filePtr,
              EDITBUFFER editBuffer[MAX_LINES]);
int     writeBuffertoDisplay(EDITBUFFER editBuffer[MAX_LINES]);
int     writeBuffertoDisplayRaw(EDITBUFFER editBuffer[MAX_LINES]);
int     filetoBuffer(FILE * filePtr, EDITBUFFER editBuffer[MAX_LINES]);
int     handleopenFile(FILE ** filePtr, char *fileName, char *oldFileName);
int     createnewFile(FILE ** filePtr, char *fileName, int checkFile);


/*-------------------------*/
/* Display File menu       */
/*-------------------------*/

void filemenu() {
  cleanStatusBar();
  data.index = OPTION_NIL;
  write_str(1, rows, STATUS_BAR_MSG2, STATUSBAR, STATUSMSG);
  loadmenus(mylist, FILE_MENU);
  write_str(1, 1, "File", MENU_SELECTOR, MENU_FOREGROUND1);
  draw_window(1, 2, 13, 8, MENU_PANEL, MENU_FOREGROUND0,0, 1,0);
  kglobal = start_vmenu(&data);
  close_window();
  write_str(1, 1, "File  Options  Help", MENU_PANEL, MENU_FOREGROUND0);
  write_str(1, 1, "F", MENU_PANEL, F_RED);
  write_str(8, 1, "p", MENU_PANEL, F_RED);
  write_str(16, 1, "H", MENU_PANEL, F_RED);
  update_screen();
  free_list(mylist);

  if(data.index == OPTION_1) {
    //New file option
    newDialog(currentFile);
    //Update new global file name
    refresh_screen(-1);
  }
  if(data.index == OPTION_2) {
    //External Module - Open file dialog.
    openFileHandler();
    //flush_editarea();
  }
  if(data.index == OPTION_3) {
    //Save option
    if (fileModified != FILE_READMODE){
      if(strcmp(currentFile, UNKNOWN) == 0)
        saveasDialog(currentFile);
      else {
        saveDialog(currentFile);
      }
    }
    else{
      //Error: We are on readmode
      infoWindow(mylist, WFILEINREADMODE_MSG, READMODEWTITLE);
    }
    //Update new global file name
    refresh_screen(-1);
  }
  if(data.index == OPTION_4) {
    //Save as option
      saveasDialog(currentFile);
      refresh_screen(-1);
  }

  if(data.index == OPTION_5) {
    //Exit option
    if(fileModified == 1)
      exitp = confirmation();   //Shall we exit? Global variable!
    else
      exitp = EXIT_FLAG;
  }
  data.index = OPTION_NIL;
  cleanStatusBar();
   //Restore message in status bar
   write_str(1, rows, STATUS_BAR_MSG1, STATUSBAR, STATUSMSG);
}

/*--------------------------*/
/* Display Options menu     */
/*--------------------------*/

void optionsmenu() {
  int  setColor;
  data.index = OPTION_NIL;
  cleanStatusBar();
  write_str(1, rows, STATUS_BAR_MSG2, STATUSBAR, STATUSMSG);
  loadmenus(mylist, OPT_MENU);
  write_str(7, 1, "Options", MENU_SELECTOR, MENU_FOREGROUND1);
  draw_window(7, 2, 20, 6, MENU_PANEL, MENU_FOREGROUND0,0, 1,0);
  kglobal = start_vmenu(&data);
  close_window();
  write_str(1, 1, "File  Options  Help", MENU_PANEL, MENU_FOREGROUND0);
  write_str(1, 1, "F", MENU_PANEL, F_RED);
  write_str(8, 1, "p", MENU_PANEL, F_RED);
  write_str(16, 1, "H", MENU_PANEL, F_RED);
  update_screen();

  free_list(mylist);
  if(data.index == OPTION_1) {
    //File Info
    fileInfoDialog();
  }
  if(data.index == OPTION_3) {
    //Set Colors
    setColor = colorsWindow(mylist,COLORSWTITLE);
    setColorScheme(setColor);
    checkConfigFile(setColor);  //save new configuration in config file
    refresh_screen(1);
  }
  data.index = OPTION_NIL;
  //Restore message in status bar
  cleanStatusBar();
  write_str(1, rows, STATUS_BAR_MSG1, STATUSBAR, STATUSMSG);

}

/*--------------------------*/
/* Display Help menu        */
/*--------------------------*/

void helpmenu() {
  cleanStatusBar();
  data.index = OPTION_NIL;
  write_str(1, rows, STATUS_BAR_MSG2, STATUSBAR, STATUSMSG);
  loadmenus(mylist, HELP_MENU);
  write_str(16, 1, "Help", MENU_SELECTOR, MENU_FOREGROUND1);
  draw_window(16, 2, 26, 5, MENU_PANEL, MENU_FOREGROUND0, 0,1,0);
  kglobal = start_vmenu(&data);
  close_window();
  write_str(1, 1, "File  Options  Help", MENU_PANEL, MENU_FOREGROUND0);
  write_str(1, 1, "F", MENU_PANEL, F_RED);
  write_str(8, 1, "p", MENU_PANEL, F_RED);
  write_str(16, 1, "H", MENU_PANEL, F_RED);
  update_screen();
  free_list(mylist);
  if(data.index == OPTION_1) {
    //About info
    help_info();
  }
  if(data.index == OPTION_2) {
    //About info
    about_info();
  }
  data.index = -1;
  //Restore message in status bar
  cleanStatusBar();
  write_str(1, rows, STATUS_BAR_MSG1, STATUSBAR, STATUSMSG);
}

/*-------------------------------*/
/* Display Confirmation Dialog   */
/*-------------------------------*/

/* Displays a window to asks user for confirmation */
int confirmation() {
  int     ok = 0;
   if (forceBufferUpdate == 1 ) {
      writeBuffertoDisplay(editBuffer);
      forceBufferUpdate = 0;
   }
  if(fileModified == 1) {
    ok = yesnoWindow(mylist, WMODIFIED_MSG, CONFIRMWTITLE);
    data.index = OPTION_NIL;
    //save file if modified?
    if(ok == 1) {
      if(strcmp(currentFile, UNKNOWN) == 0)
    saveasDialog(currentFile);
      else {
    saveDialog(currentFile);
      }
      ok = EXIT_FLAG;
    } else {
      if (ok != 2) ok = EXIT_FLAG;
    }
  } else {
    ok = -1;            //Exit without asking.
  }
  return ok;
}

/*--------------------------*/
/* Display About Dialog     */
/*--------------------------*/

int about_info() {
  int     ok = 0;
  char    msg[105];
  if (forceBufferUpdate == 1) {
    writeBuffertoDisplay(editBuffer);
    forceBufferUpdate = 0;
  }
  msg[0] = '\0';
  strcat(msg, ABOUT_ASC_1);
  strcat(msg, ABOUT_ASC_2);
  strcat(msg, ABOUT_ASC_3);
  strcat(msg, "\0");
  alertWindow(mylist, msg,ABOUTWTITLE);
  return ok;
}

/*--------------------------*/
/* Display About Dialog     */
/*--------------------------*/

int help_info() {
  int     ok = 0;
  char    msg[500];
  if (forceBufferUpdate == 1) {
    writeBuffertoDisplay(editBuffer);
    forceBufferUpdate = 0;
  }
 //writeBuffertoDisplay(editBuffer);
  msg[0] = '\0';
  strcat(msg, HELP1);       //located in user_inter.h
  strcat(msg, HELP2);       //located in user_inter.h
  strcat(msg, HELP3);       //located in user_inter.h
  strcat(msg, HELP4);       //located in user_inter.h
  strcat(msg, HELP5);       //located in user_inter.h
  strcat(msg, HELP6);       //located in user_inter.h
  strcat(msg, HELP7);       //located in user_inter.h
  strcat(msg, HELP8);       //located in user_inter.h
  strcat(msg, HELP9);       //located in user_inter.h
  strcat(msg, HELP10);      //located in user_inter.h
  strcat(msg, "\0");
  helpWindow(mylist, msg, HELPWTITLE);
  refresh_screen(0);
  return ok;
}

/*----------------------*/
/* Drop_Down Menu Loop  */
/*----------------------*/

void drop_down(char *kglobal) {
/*
   Drop_down loop animation.
   K_LEFTMENU/K_RIGHTMENU -1 is used when right/left arrow keys are used
   so as to break vertical menu and start the adjacent menu
   kglobal is changed by the menu functions.
*/
  if (forceBufferUpdate == 1) {
    writeBuffertoDisplay(editBuffer);
    forceBufferUpdate = 0;
  }
 //  writeBuffertoDisplay(editBuffer);
  do {
    if(*kglobal == K_ESCAPE) {
      //Exit drop-down menu with ESC
      *kglobal = 0;
      break;
    }
    if(data.index == FILE_MENU) {
      filemenu();
      if(*kglobal == K_LEFTMENU) {
    data.index = OPT_MENU;
      }
      if(*kglobal == K_RIGHTMENU) {
    data.index = HELP_MENU;
      }
    }
    if(data.index == OPT_MENU) {
      optionsmenu();
      if(*kglobal == K_LEFTMENU) {
    data.index = HELP_MENU;
      }
      if(*kglobal == K_RIGHTMENU) {
    data.index = FILE_MENU;
      }
    }
    if(data.index == HELP_MENU) {
      helpmenu();
      if(*kglobal == K_LEFTMENU) {
    data.index = FILE_MENU;
      }
      if(*kglobal == K_RIGHTMENU) {
    data.index = OPT_MENU;
      }
    }
  } while(*kglobal != K_ENTER);
}

/*---------*/
/* Credits */
/*---------*/
void credits() {
/* Frees memory and displays goodbye message */
  //Free selected path item/path from opfiledialog
  size_t i; //to be compatible with strlen
  char auth[27] ="Coded by v3l0r3k 2019-2021";
if (openFileData.itemIndex != 0) {
    free(openFileData.item);
    free(openFileData.path);
  }
  free_buffer();
  resetTerm();          //restore terminal settings from failsafe
  showcursor();
  resetAnsi(0);
  screencol(0);
  outputcolor(7, 0);
  printf(cedit_ascii_1);
  printf(cedit_ascii_2);
  printf(cedit_ascii_3);
  printf(cedit_ascii_4);
  printf(cedit_ascii_5);
  printf(cedit_ascii_6);

  outputcolor(0, 90);
  printf("\n%s",auth);
  outputcolor(0, 37);
  _timer1.ms=10;
  _timer1.ticks=0;
  i=0;

  outputcolor(0, 97);
  do{
    if (timerC(&_timer1) == 1) {
       gotoxy(i,8);
       if (i==strlen(auth)) outputcolor(0,93);
       if (i<10 || i>16)
    if (i<=strlen(auth)) printf("%c", auth[i-1]);
       printf("\n");
       i++;
     }
    } while (_timer1.ticks <30);

  printf("\n");
  outputcolor(7, 0);
}

/*=================*/
/*BUFFER functions */
/*=================*/

/*----------------------------------*/
/* Initialize and clean edit buffer */
/*----------------------------------*/

void cleanBuffer(EDITBUFFER editBuffer[MAX_LINES]) {
//Clean and initialize the edit buffer
  long     i, j;

  for(j = 0; j < MAX_LINES; j++)
    for(i = 0; i < MAX_CHARS; i++) {
      editBuffer[j].charBuf[i].specialChar = 0;
      editBuffer[j].charBuf[i].ch = CHAR_NIL;
    }
}

int writetoBuffer(EDITBUFFER editBuffer[MAX_LINES], long whereX, long whereY,
          char ch) {
  int     oldValue;
  oldValue = editBuffer[whereY].charBuf[whereX].specialChar;
  if(ch == SPECIAL_CHARS_SET1) {
    editBuffer[whereY].charBuf[whereX].specialChar = SPECIAL_CHARS_SET1;
    editBuffer[whereY].charBuf[whereX].ch = ch;
  } else if(ch == SPECIAL_CHARS_SET2) {
    editBuffer[whereY].charBuf[whereX].specialChar = SPECIAL_CHARS_SET2;
    editBuffer[whereY].charBuf[whereX].ch = ch;
  } else {
    //Special char example -61 -95 : รก
    if(oldValue == SPECIAL_CHARS_SET1 || oldValue == SPECIAL_CHARS_SET2)
      editBuffer[whereY].charBuf[whereX].specialChar = oldValue;
    else
      editBuffer[whereY].charBuf[whereX].specialChar = 0;

    editBuffer[whereY].charBuf[whereX].ch = ch;
  }
  return 1;
}

/*========================*/
/*FILE handling functions */
/*========================*/

/*--------------------------*/
/* Write EditBuffer to File */
/*--------------------------*/

int writeBuffertoFile(FILE * filePtr, EDITBUFFER editBuffer[MAX_LINES]) {
  char    tempChar=0, oldChar=0;
  long     lineCounter = 0;
  long     inlineChar = 0;
  int     specialChar = 0;

  //Check if pointer is valid
  if(filePtr != NULL) {
    rewind(filePtr);        //Make sure we are at the beginning
    lineCounter = 0;
    inlineChar = 0;
    do {
      oldChar = tempChar;
      tempChar = editBuffer[lineCounter].charBuf[inlineChar].ch;
      specialChar =
      editBuffer[lineCounter].charBuf[inlineChar].specialChar;
      inlineChar++;
      if(tempChar == END_LINE_CHAR) {
    inlineChar = 0;
    lineCounter++;
      }
      if(tempChar > 0) {
    fprintf(filePtr, "%c", tempChar);
      } else {
    //Special accents
    fprintf(filePtr, "%c%c", specialChar, tempChar);
      }
    } while(tempChar != CHAR_NIL);
    //Check if last line finishes in 0x0A - oldChar = 0x0A
    if(oldChar != END_LINE_CHAR)
      fprintf(filePtr, "%c", END_LINE_CHAR);
  }
  return 1;
}

/*------------------*/
/* Save File Dialog */
/*------------------*/

int saveDialog(char fileName[MAX_TEXT]) {
  int     ok = 0;
  char    tempMsg[MAX_TEXT];
  data.index = OPTION_NIL;
  openFile(&filePtr, fileName, "w");
  writeBuffertoFile(filePtr, editBuffer);
  strcpy(tempMsg, fileName);
  strcat(tempMsg, WSAVE_MSG);
  ok = infoWindow(mylist, tempMsg, FILESAVEDWTITLE);
  fileModified = 0;     //reset modified flag

  return ok;
}

int saveasDialog(char fileName[MAX_TEXT]) {
  int     ok=0, count=0;
  char    tempFile[MAX_TEXT], tempMsg[MAX_TEXT];
  data.index = OPTION_NIL;

  clearString(tempFile, MAX_TEXT);

  count = inputWindow(WSAVELABEL_MSG, tempFile, SAVEWTITLE);

  if(count > 0) {
    //Save file
    data.index = OPTION_NIL;
    clearString(fileName, MAX_TEXT);
    strcpy(fileName, tempFile);

    //Check whether file exists.
    if(file_exists(fileName)) {
      ok = yesnoWindow(mylist, WFILEEXISTS_MSG, EXISTSWTITLE);
      if(ok == CONFIRMATION) {
    //Save anyway.
    closeFile(filePtr);
    openFile(&filePtr, fileName, "w");
    writeBuffertoFile(filePtr, editBuffer);
    strcpy(tempMsg, fileName);
    strcat(tempMsg, WSAVE_MSG);
    ok = infoWindow(mylist, tempMsg, FILESAVEDWTITLE);
    fileModified = 0;
      } else {
    //Do nothing.
      }
    } else {
      closeFile(filePtr);
      openFile(&filePtr, fileName, "w");
      writeBuffertoFile(filePtr, editBuffer);
      strcpy(tempMsg, fileName);
      strcat(tempMsg, WSAVE_MSG);
      ok = infoWindow(mylist, tempMsg, FILESAVEDWTITLE);
      fileModified = 0;
    }
  }
  return ok;
}

/*-----------------*/
/* New File Dialog */
/*-----------------*/

int newDialog(char fileName[MAX_TEXT]) {
  char    tempFile[MAX_TEXT];
  int     ok, count;

  clearString(tempFile, MAX_TEXT);

  data.index = OPTION_NIL;

  count = inputWindow(WSAVELABEL_MSG, tempFile, NEWFILEWTITLE);
  if(count > 0) {
    //Check whether file exists and create file.
    ok = createnewFile(&filePtr, tempFile, 1);
    if(ok == 1) {
      cleanBuffer(editBuffer);
      strcpy(fileName, tempFile);
    editScroll.scrollActiveV = VSCROLL_OFF; //Scroll is inactive.
    editScroll.totalLines = 1; //There is just one line active in buffer
    editScroll.bufferX = 1;
    editScroll.bufferY = 1;
    cursorX=START_CURSOR_X;
        cursorY=START_CURSOR_Y;
    }
    ok = 1;
  }
  return ok;
}

/*--------------------------*/
/* OPEN File Dialog HANDLER */
/*--------------------------*/

int openFileHandler() {
  char    oldFile[MAX_TEXT];

  int ok=chdir(currentPath);        //Go back to original path
  ok++; //to avoid warning
  //Update screen if not scrolling
  if (forceBufferUpdate == 1) {
    writeBuffertoDisplay(editBuffer);
    forceBufferUpdate = 0;
  }
  //free current item if it exists
  if (openFileData.itemIndex != 0){
        free(openFileData.item);
        free(openFileData.path);
  }
  openFileDialog(&openFileData);
 //Update new global file name
  //refresh_screen(1);
  flush_editarea();
  if(openFileData.itemIndex != 0) {
    //check if file exists
    if (!file_exists(openFileData.path)) {
    alertWindow(mylist, WCHECKFILE2_MSG, INFOWTITLE);
    }
    //Change current File Name
    //if the index is different than CLOSE_WINDOW
    clearString(oldFile, MAX_TEXT);
    strcpy(oldFile, currentFile);   //save previous file to go back
    clearString(currentFile, MAX_TEXT);
    strcpy(currentFile, openFileData.path);
    cleanBuffer(editBuffer);
    //Open file and dump first page to buffer - temporary
    if(filePtr != NULL)
      closeFile(filePtr);
    handleopenFile(&filePtr, currentFile, oldFile);
  }

  //Write current buffer
  writeBuffertoDisplay(editBuffer);
  return 1;
}

/*------------------*/
/* File Info Dialog */
/*------------------*/

int fileInfoDialog() {
  long    size = 0, lines = 0;
  int     i;
  char    sizeStr[20];
  char    linesStr[20];
  char    tempMsg[150];
  char    pathtxt[60];
  if(filePtr != NULL) {
    closeFile(filePtr);
    openFile(&filePtr, currentFile, "r");
    size = getfileSize(filePtr);
    lines = countLinesFile(filePtr);
    if(size <= 0)
      size = 0;
    if(lines <= 0)
      lines = 0;
    sprintf(sizeStr, "%ld", size);
    sprintf(linesStr, "%ld", lines);
    strcpy(tempMsg, WINFO_SIZE);
    strcat(tempMsg, sizeStr);
    strcat(tempMsg, " bytes.");
    strcat(tempMsg, WINFO_SIZE2);
    strcat(tempMsg, linesStr);
    strcat(tempMsg, " lines.\n");
    for (i=0;i<60;i++){
        if (i!=31) pathtxt[i] = openFileData.fullPath[i];
        else pathtxt[31] = '\n';
    }
    pathtxt[59] = CHAR_NIL;
    strcat(tempMsg, pathtxt);
    alertWindow(mylist, tempMsg, INFOWTITLE);
  } else {
    infoWindow(mylist, WINFO_NOPEN, NOFILEOPENWTITLE);
  }
  return 0;
}




