#pragma once

/* Icons */
#define IDI_APPICON        101

/* Menus */
#define IDR_MAINMENU       201

/* Toolbar PNG strips (RCDATA) -- one per DPI tier */
#define IDR_TOOLBAR_96     301
#define IDR_TOOLBAR_120    302
#define IDR_TOOLBAR_144    303
#define IDR_TOOLBAR_192    304

/* Commands - also used as string table IDs for status bar hints.
 * Adding STRING_TIP_OFFSET gives the short tooltip string ID. */
#define ID_FILE_EXIT       1001
#define ID_VIEW_FILELIST   1002
#define ID_HELP_ABOUT      1003
#define ID_FILE_OPEN       1004
#define ID_FILE_PREV       1005
#define ID_FILE_NEXT       1006
#define ID_VIEW_ZOOMIN     1007
#define ID_VIEW_ZOOMOUT    1008
#define ID_VIEW_ZOOMMODE   1009  /* toolbar button: opens zoom-mode popup */

/* Zoom mode sub-commands (contiguous for CheckMenuRadioItem) */
#define ID_ZOOMMODE_W2I_ZOOMOUT  1010  /* Window Fit Image, ZoomOut if too Large */
#define ID_ZOOMMODE_W2I          1011  /* Window Fit Image, No Zoom              */
#define ID_ZOOMMODE_I2W_ZOOMOUT  1012  /* Image Fit Window, ZoomOut Only         */
#define ID_ZOOMMODE_I2W          1013  /* Image Fit Window                       */
#define ID_ZOOMMODE_NOFIT        1014  /* No Fit                                 */

/* Stub commands (not yet implemented, always disabled) */
#define ID_FILE_SAVE_AS    1015
#define ID_EDIT_UNDO       1016
#define ID_EDIT_CUT        1017
#define ID_EDIT_COPY       1018
#define ID_EDIT_PASTE      1019
#define ID_VIEW_ZOOMTO     1020
#define ID_VIEW_ZOOMREM    1021
#define ID_VIEW_ZOOM       1022

/* Remote open command */
#define ID_FILE_OPEN_REMOTE  1023

/* Delete commands */
#define ID_FILE_DELETE       1024   /* Del:       move to recycle bin (local) / confirm+delete (remote) */
#define ID_FILE_DELETE_PERM  1025   /* Shift+Del: permanent delete (local) / direct delete (remote) */

/* Refresh: rescan directory; if current file gone, behave like delete */
#define ID_FILE_REFRESH      1026

/* Offset: command ID + STRING_TIP_OFFSET = short tooltip string */
#define STRING_TIP_OFFSET  100

/* Custom message: LPARAM = const wchar_t * info text */
#define WM_APP_SETINFO     (WM_APP + 1)

/* Dialog: Connect to remote server */
#define IDD_CONNECT        401
#define IDC_HOST           501

/* Dialog: Remote file browser */
#define IDD_REMOTE_BROWSER 402
#define IDC_PATHBAR        503
#define IDC_FILELIST       504
#define IDC_BTN_UP         505
