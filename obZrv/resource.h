#pragma once

/* Resource ID layout
 *
 * 100-199:   icons and other top-level visual resources.
 * 200-299:   menus and accelerator tables.
 * 300-399:   toolbar image strips and binary toolbar resources.
 * 400-499:   dialogs.
 * 500-999:   dialog controls.
 * 1000-1999: fixed command IDs. These IDs are also used directly as
 *             command string IDs in obZrv.rc. Command strings may contain
 *             "long\nshort" text, where long is used for status/menu hints
 *             and short is used for toolbar tooltips.
 * 2000-2099: dynamic Recent Files command IDs. The registry keeps 20 items,
 *             but the command range intentionally reserves 100 IDs.
 *
 * Keep fixed commands below 2000. Dynamic command IDs do not have resource
 * strings and should provide display text at runtime.
 */

#define IDC_STATIC          -1

/* Icons */
#define IDI_APPICON        101

/* Menus */
#define IDR_MAINMENU       201

/* Toolbar PNG strips (RCDATA) -- one per DPI tier */
#define IDR_TOOLBAR_96     301
#define IDR_TOOLBAR_120    302
#define IDR_TOOLBAR_144    303
#define IDR_TOOLBAR_192    304

/* Fixed commands - also used as command string IDs for hints/tooltips. */
#define ID_FILE_EXIT       1001
#define ID_VIEW_FILELIST   1002
#define ID_HELP_ABOUT      1003
#define ID_FILE_OPEN       1004
#define ID_FILE_SAVE_AS    1015
#define ID_FILE_PREV       1005
#define ID_FILE_NEXT       1006
#define ID_VIEW_ZOOMIN     1007
#define ID_VIEW_ZOOMOUT    1008
#define ID_VIEW_ZOOMMODE   1009  /* toolbar button: opens zoom-mode popup */

/* Zoom mode sub-commands (contiguous for CheckMenuRadioItem) */
#define ID_ZOOMMODE_W2I_ZOOMOUT  1010  /* Window Fit Image, ZoomOut if Large */
#define ID_ZOOMMODE_W2I          1011  /* Window Fit Image, No Zoom              */
#define ID_ZOOMMODE_I2W_ZOOMOUT  1012  /* Image Fit Window, ZoomOut Only         */
#define ID_ZOOMMODE_I2W          1013  /* Image Fit Window                       */
#define ID_ZOOMMODE_NOFIT        1014  /* No Fit                                 */

/* Stub commands (not yet implemented, always disabled) */
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
#define ID_VIEW_SLIDESHOW    1028

/* Recent files submenu */
#define ID_FILE_RECENT_EMPTY 1027
#define ID_FILE_RECENT_FIRST 2000
#define ID_FILE_RECENT_LAST  2099

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

/* Dialog: Slide show */
#define IDD_SLIDESHOW              403
#define IDC_SS_SOURCE_CURRENT      510
#define IDC_SS_SOURCE_CUSTOM       511
#define IDC_SS_INTERVAL            512
#define IDC_SS_ORDER_NORMAL        513
#define IDC_SS_ORDER_REVERSE       514
#define IDC_SS_ORDER_RANDOM        515
#define IDC_SS_REPEAT_FOREVER      516
#define IDC_SS_REPEAT_COUNT        517
#define IDC_SS_ROUNDS              518
#define IDC_SS_ZOOM                519
#define IDC_SS_CUSTOM_GROUP        520
#define IDC_SS_FILELIST            521
#define IDC_SS_ADD_FILES           522
#define IDC_SS_ADD_FOLDER          523
#define IDC_SS_MOVE_UP             524
#define IDC_SS_MOVE_DOWN           525
#define IDC_SS_FULLSCREEN          526
#define IDC_SS_WINDOWED            527

/* MainWnd private messages */
#define WM_APP_SLIDESHOW_RESET_TIMER (WM_APP + 2)
#define WM_APP_SLIDESHOW_STOP        (WM_APP + 3)
