//-----------------------------------
// Windows GUI  crap


#ifdef WIN32GUI


//#include <conio.h> 
//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "resource.h"
#include "qcc.h"
#include "hash.h"

#include <time.h>

// this is getting downright ridiculous
char frikqccdir[1024];
extern char progssrc[1024];
// many k of extra junk, geeze...
extern char errorguide[256]; // where!
extern char editor[256];
void summary_print (char *format, ...)
{
	va_list         argptr;
	static char             string[1024];
	
	va_start (argptr, format);
	vsprintf (string, format,argptr);
	va_end (argptr);

	strcat(summary_buf, string);

}

HWND hList;
HWND main_hWnd;
HINSTANCE hInst;

void SwitchToList (void);
static HICON	hIcon;

#define MAX_ERROR_SPACE 256

// FIX ME, make safe
// FIXME make struct??

char *ED_Name[MAX_ERROR_SPACE];
int ED_IDNumber[MAX_ERROR_SPACE];
int ED_LineNumber[MAX_ERROR_SPACE];
int ED_EnumID[MAX_ERROR_SPACE];
int ED_Ofs[MAX_ERROR_SPACE];
int ED_Len[MAX_ERROR_SPACE];

int ED_count;
extern char	includedir[1024];
char *pt = "-+/";

void Sys_Quit (void)
{
	FILE *h;
	char filename[1024];
	SwitchToList();

	sprintf(filename, "%s\\frikqcc.cfg", frikqccdir);

	h = fopen(filename, "w");
	if (h)
	{
		fprintf(h, "// FrikQCC configuration\n// Lines with + or / are enabled options, lines with - are off\n");
		if (includedir[0] != 0)
			fprintf(h, "+i %s\n", includedir);
		fprintf(h, "%csummary %cnolog %cnopause\n", pt[summary], pt[!logging], pt[!pr_pause]);
		fprintf(h, "%cOt %cOi %cOp %cOc %cOd %cOs %cOl %cOn %cOf %cOu %cOo %cOr %cOa\n", pt[pr_optimize_eliminate_temps],
			pt[pr_optimize_shorten_ifs], pt[pr_optimize_nonvec_parms], pt[pr_optimize_constant_names], pt[pr_optimize_defs],
			pt[pr_optimize_hash_strings], pt[pr_optimize_locals], pt[pr_optimize_function_names], pt[pr_optimize_filenames],
			pt[pr_optimize_unreferenced], pt[pr_optimize_logicops], pt[pr_optimize_recycle], pt[pr_optimize_constant_arithmetic],
			pt[pr_optimize_constant_names]);
		fprintf(h, "+warn %i %ccos\n", warninglevel, pt[compile_on_start]);
		fprintf(h, "/errorguide \"%s\"\n", errorguide);
		fprintf(h, "/editor \"%s\"\n", editor);
		fclose(h);
	}
	EndDialog(main_hWnd, 1);
	

	exit(1);
}

// special version of WinPrint that keeps track of error info too
extern char *pr_line_start, *pr_start, *pr_file_p; /// keep the hacks comin' boy!

void ErrorPrint(char *fname, int line, int ernum, char *format, ...) 
{
	int ret;
	va_list         argptr;
	static char             string[1024];

	va_start (argptr, format);
	vsprintf (string, format,argptr);
	va_end (argptr);

	ED_count++;
 	ret = SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)string);
	if (ED_count >= MAX_ERROR_SPACE)
	{
		WinPrint("Ran out of room, increase MAX_ERROR_SPACE and recompile frikqcc");
		ED_count = 0;
	}
	ED_Ofs[ED_count] = pr_line_start - pr_start; // HACK CITY
	ED_Len[ED_count] = pr_file_p - pr_line_start; // NEW HACK CITY
	ED_LineNumber[ED_count] = line;
	ED_Name[ED_count] = fname;
	ED_IDNumber[ED_count] = ret;
	ED_EnumID[ED_count] = ernum;
	SendMessage(hList, LB_SETITEMDATA, ret, ED_count);
	SendMessage(hList, LB_SETCURSEL, ret, 0);

}
char open_filename[1024];
void WinSize(void);

void SwitchToList (void)
{
	int i;
	char *buf;
	HWND hEdit = GetDlgItem(main_hWnd, IDC_TEXT);
	if (open_filename[0])
	{
		i = SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0);
		buf = malloc(i+1);
		SendMessage(hEdit, WM_GETTEXT, (WPARAM)(i+1),  (LPARAM)buf);
		SaveFile(open_filename, buf, i);
		free(buf);
	}
	open_filename[0] = 0;
	SetWindowText(main_hWnd, "FrikQCC - [Results]");

	WinSize();
};

void SwitchToEdit (char *file, int pos, int len, int line)
{
	char	*buf;

	HWND hEdit = GetDlgItem(main_hWnd, IDC_TEXT);

	strcpy(open_filename, file);
	LoadFile(open_filename, (void **)&buf);

	SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)buf);
	SendMessage(hEdit, EM_SETSEL, pos, pos+len);
	WinSize();
	SendMessage(hEdit, EM_LINESCROLL, 0, line > 3 ? line - 3 : 0);
	SetWindowText(main_hWnd, va("FrikQCC - %s", file));

	SetFocus(hEdit);

}
void WinPrint(char *format, ...) 
{
	int ret;
	va_list         argptr;
	static char             string[1024];

	va_start (argptr, format);
	vsprintf (string, format,argptr);
	va_end (argptr);

 	ret = SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)string);
	SendMessage(hList, LB_SETITEMDATA, ret, -1);
	SendMessage(hList, LB_SETCURSEL, ret, 0);
}
void PR_Clear (void);
void HelpButton (void)
{
	int ret1, ret2;
	HINSTANCE hInstance;
	ret1 = SendMessage(hList, LB_GETCURSEL, 0, 0);
	ret2 = SendMessage(hList, LB_GETITEMDATA, ret1, 0);
	if (ret2 == -1)
		return; // Nope, sorry...
	if (ED_IDNumber[ret2] != ret1)
		return; 	// that's an old compile
	hInstance = ShellExecute(NULL, "open", va(errorguide, ED_EnumID[ret2]), NULL, NULL, 1);  
	if (hInstance <= (HINSTANCE) 32)
		WinPrint("Failed to run \"%s\", Error: %d", va(errorguide, ED_EnumID[ret2]), GetLastError());

}
void EditButton (void)
// for now...
{
	int ret1, ret2, i, len;
	char *parms = "%s";
	HINSTANCE hInstance;
	ret1 = SendMessage(hList, LB_GETCURSEL, 0, 0);
	ret2 = SendMessage(hList, LB_GETITEMDATA, ret1, 0);
	if (ret2 == -1)
		return; // Nope, sorry...
	if (ED_IDNumber[ret2] != ret1)
		return; // that's an old compile
	len = strlen(editor);
	if (len)
	{
		for (i = 0; i <= len; i++)
		{
			if (editor[i] == ' ')
			{
				editor[i] = 0;
				parms = editor+i+1;
				break;
			}
		}
		hInstance = ShellExecute(NULL, "open", editor, va(parms, ED_Name[ret2], ED_LineNumber[ret2]), NULL, 1);
		if (i < len)
			editor[i] = ' ';
	}
	else
		SwitchToEdit(ED_Name[ret2], ED_Ofs[ret2], ED_Len[ret2], ED_LineNumber[ret2]);

		//hInstance = ShellExecute(NULL, "edit", ED_Name[ret2], NULL, NULL, 1);  
	if (hInstance <= (HINSTANCE) 32)
		WinPrint("Failed to edit \"%s\", Error: %d", ED_Name[ret2], GetLastError());
}
time_t StartTime;

void CompileIt ()
{
	char	*src;
	char	*src2;
	char	filename[1024];
	int		i, p, crc, handle;
	StartTime = I_FloatTime();
	PR_Clear(); // hacky fun for the whole family!
	ED_count = 0;

	if (logging)
		remove("error.log");
	InitData ();
	pr_dumpasm = false;

	if (!progssrc[0])
		strcpy(progssrc, "progs.src");

	sprintf (filename, "%s%s", sourcedir,progssrc);
	LoadFile (filename, (void **)&src);

	if(STRCMP(progssrc, "preprogs.src"))
	{
		src = COM_Parse (src);
		if (!src)
		{
			Sys_Error ("Q606: No destination filename.\n");
			return;
		}
		strcpy (destfile, com_token);

	// compile all the files
		do
		{
			src = COM_Parse(src);
			if (!src)
				break;
			sprintf (filename, "%s%s", sourcedir, com_token);
	//		printf ("%s\n", filename);
			LoadFile (filename, (void **)&src2);

			if (!PR_CompileFile (src2, com_token) )
			{
				EndProgram(false);
				return;
			}
				
		} while (1);
	}
	else
	{
		if (!PR_CompileFile (src, progssrc) )
		{
			EndProgram(false);
			return;
		}
		if(!destfile)
			strcpy(destfile, "progs.dat");

	}

	if (!PR_FinishCompilation ())
	{
		EndProgram(false);
		return;
		//Sys_Error ("compilation errors");
	}

// write progdefs.h
	crc = PR_WriteProgdefs ("progdefs.h");
	
	
// write data file
	WriteData (crc);

// report / copy the data files

	EndProgram(true);
}


INT CALLBACK OptionsDlgProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	int saveandquit = 0;
	char temp[256];

	switch (iMsg)
	{

		case WM_INITDIALOG:
		{
			SendMessage(GetDlgItem(hWnd, IDC_SUMMARY), BM_SETCHECK, summary ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_PAUSE), BM_SETCHECK, !pr_pause ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_COS), BM_SETCHECK, compile_on_start ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_LOG), BM_SETCHECK, logging ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_OT), BM_SETCHECK, pr_optimize_eliminate_temps ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_OI), BM_SETCHECK, pr_optimize_shorten_ifs ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_OP), BM_SETCHECK, pr_optimize_nonvec_parms ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_OC), BM_SETCHECK, pr_optimize_constant_names ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_OD), BM_SETCHECK, pr_optimize_defs ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_OS), BM_SETCHECK, pr_optimize_hash_strings ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_OL), BM_SETCHECK, pr_optimize_locals ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_ON), BM_SETCHECK, pr_optimize_function_names ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_OF), BM_SETCHECK, pr_optimize_filenames ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_OU), BM_SETCHECK, pr_optimize_unreferenced ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_OO), BM_SETCHECK, pr_optimize_logicops ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_OR), BM_SETCHECK, pr_optimize_recycle ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_OA), BM_SETCHECK, pr_optimize_constant_arithmetic ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_HELPLOC), WM_SETTEXT, 0,(LPARAM)&errorguide);
			SendMessage(GetDlgItem(hWnd, IDC_EDITOR), WM_SETTEXT, 0,(LPARAM)&editor);
			sprintf(temp, "%i", warninglevel);
			SendMessage(GetDlgItem(hWnd, IDC_WARN), WM_SETTEXT, 0, (LPARAM)&temp);



		} break;

		case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDOK)
				saveandquit = 1;
			else if (LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hWnd, 0);
			}
					
		} break;

		case WM_CLOSE:
			saveandquit = 1;

			break;
		//default:
		//	return DefWindowProc(hWnd, iMsg, wParam, lParam);
	}
	if (!saveandquit)
		return 0;
	summary = SendMessage(GetDlgItem(hWnd, IDC_SUMMARY), BM_GETSTATE, 0, 0) == BST_CHECKED;
	pr_pause = !SendMessage(GetDlgItem(hWnd, IDC_PAUSE), BM_GETSTATE, 0, 0) == BST_CHECKED;
	compile_on_start = SendMessage(GetDlgItem(hWnd, IDC_COS), BM_GETSTATE, 0, 0) == BST_CHECKED;
	logging = SendMessage(GetDlgItem(hWnd, IDC_LOG), BM_GETSTATE, 0, 0) == BST_CHECKED;
	pr_optimize_eliminate_temps = SendMessage(GetDlgItem(hWnd, IDC_OT), BM_GETSTATE, 0, 0) == BST_CHECKED;
	pr_optimize_shorten_ifs = SendMessage(GetDlgItem(hWnd, IDC_OI), BM_GETSTATE, 0, 0) == BST_CHECKED;
	pr_optimize_nonvec_parms = SendMessage(GetDlgItem(hWnd, IDC_OP), BM_GETSTATE, 0, 0) == BST_CHECKED;
	pr_optimize_constant_names = SendMessage(GetDlgItem(hWnd, IDC_OC), BM_GETSTATE, 0, 0) == BST_CHECKED;
	pr_optimize_defs = SendMessage(GetDlgItem(hWnd, IDC_OD), BM_GETSTATE, 0, 0) == BST_CHECKED;
	pr_optimize_hash_strings = SendMessage(GetDlgItem(hWnd, IDC_OS), BM_GETSTATE, 0, 0) == BST_CHECKED;
	pr_optimize_locals = SendMessage(GetDlgItem(hWnd, IDC_OL), BM_GETSTATE, 0, 0) == BST_CHECKED;
	pr_optimize_function_names = SendMessage(GetDlgItem(hWnd, IDC_ON), BM_GETSTATE, 0, 0) == BST_CHECKED;
	pr_optimize_filenames = SendMessage(GetDlgItem(hWnd, IDC_OF), BM_GETSTATE, 0, 0) == BST_CHECKED;
	pr_optimize_unreferenced = SendMessage(GetDlgItem(hWnd, IDC_OU), BM_GETSTATE, 0, 0) == BST_CHECKED;
	pr_optimize_logicops = SendMessage(GetDlgItem(hWnd, IDC_OO), BM_GETSTATE, 0, 0) == BST_CHECKED;
	pr_optimize_recycle = SendMessage(GetDlgItem(hWnd, IDC_OR), BM_GETSTATE, 0, 0) == BST_CHECKED;
	pr_optimize_constant_arithmetic = SendMessage(GetDlgItem(hWnd, IDC_OA), BM_GETSTATE, 0, 0) == BST_CHECKED;
	SendMessage(GetDlgItem(hWnd, IDC_HELPLOC), WM_GETTEXT, 255, (LPARAM)&errorguide);
	SendMessage(GetDlgItem(hWnd, IDC_EDITOR), WM_GETTEXT, 255, (LPARAM)&editor);
	SendMessage(GetDlgItem(hWnd, IDC_WARN), WM_GETTEXT, 255, (LPARAM)&temp);
	warninglevel = atoi(temp);

	EndDialog(hWnd, 0);
	return 0;

}
void WinSize()
{
	RECT rcClient;
	int ret;

	GetClientRect(main_hWnd, &rcClient);

	if (open_filename[0])
	{
		SetWindowPos(hList, NULL, 0, 0, rcClient.right, rcClient.bottom - 30, SWP_NOZORDER | SWP_HIDEWINDOW);
		SetWindowPos(GetDlgItem(main_hWnd, IDC_TEXT), NULL, 0, 0, rcClient.right, rcClient.bottom - 30, SWP_NOZORDER | SWP_SHOWWINDOW);
	}
	else
	{
		SetWindowPos(hList, NULL, 0, 0, rcClient.right, rcClient.bottom - 30, SWP_NOZORDER | SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(main_hWnd, IDC_TEXT), NULL, 0, 0, rcClient.right, rcClient.bottom - 30, SWP_NOZORDER | SWP_HIDEWINDOW);

	}
	ret = rcClient.right / 5;
	SetWindowPos(GetDlgItem(main_hWnd, IDC_HELPBUTTON), NULL, 0, rcClient.bottom - 28, ret, 28,  SWP_NOZORDER);
	SetWindowPos(GetDlgItem(main_hWnd, IDC_OPTIONS), NULL, ret, rcClient.bottom - 28, ret, 28, SWP_NOZORDER);
	SetWindowPos(GetDlgItem(main_hWnd, IDC_EDITBUTTON), NULL, ret*2, rcClient.bottom - 28, ret,  28, SWP_NOZORDER);
	SetWindowPos(GetDlgItem(main_hWnd, ID_OK), NULL, ret*3, rcClient.bottom - 28, ret,  28, SWP_NOZORDER);
	SetWindowPos(GetDlgItem(main_hWnd, ID_CANCEL), NULL, ret*4, rcClient.bottom - 28, ret, 28, SWP_NOZORDER);
}

INT CALLBACK MainDlgProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	int ret;
	hList = GetDlgItem(hWnd, IDC_LIST1);
	main_hWnd = hWnd;

	switch (iMsg)
	{
		case WM_INITDIALOG:
		{
			HFONT	hFont;
			SendMessage (hWnd, WM_SETICON, (WPARAM)TRUE, (LPARAM)hIcon);
			SendMessage (hWnd, WM_SETICON, (WPARAM)FALSE, (LPARAM)hIcon);
			hFont = CreateFont(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, &"fixedsys");
			SendMessage (GetDlgItem(hWnd, IDC_TEXT), WM_SETFONT, (WPARAM)hFont, 0);

			WinSize();
			if (compile_on_start)
				CompileIt();
			else if (progssrc[0])
				SwitchToEdit(progssrc, 0,0, 0);
		} break;
		case WM_SIZE:
		{
			WinSize();
		}
		break;

		case WM_COMMAND:
		{
			// this should be a switch statement
			if (LOWORD(wParam) == ID_OK)
			{
				SwitchToList();
				CompileIt ();
			}
			else if (LOWORD(wParam) == ID_FILE_EXIT)
				Sys_Quit();
			else if (LOWORD(wParam) == ID_CANCEL)
				Sys_Quit();
			else if (LOWORD(wParam) == IDC_EDITBUTTON)
				EditButton();
			else if (LOWORD(wParam) == IDC_OPTIONS)
				DialogBox(hInst, MAKEINTRESOURCE(IDD_OPDIALOG), NULL, &OptionsDlgProc);
			else if (LOWORD(wParam) == IDC_HELPBUTTON)
				HelpButton();
			else if (LOWORD(wParam) == ID_EDIT_PROGSSRC)
				SwitchToEdit(progssrc, 0, 0, 0);
			else if (LOWORD(wParam) == ID_EDIT_VIEWOUTPUT)
				SwitchToList();

		} break;

		case WM_CLOSE:
			Sys_Quit();

			break;
		//default:
		//	return DefWindowProc(hWnd, iMsg, wParam, lParam);
	}

	return 0;
}


static char	*empty_string = "";


int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG				msg;

	double			time, oldtime, newtime;
	MEMORYSTATUS	lpBuffer;
	int				t;
	WNDCLASS		wc;
	HDC				hdc;
	int				i, handle;
	RECT			rect;
	char filename[1024];
	char	*src2;

    if (hPrevInstance)
        return 0;
	hInst = hInstance;


	if (!GetCurrentDirectory (sizeof(sourcedir), sourcedir))
		Sys_Error ("Q605: Couldn't determine current directory");
	strcat(sourcedir, "\\");
	warninglevel = 2;
	logging = 1;
	pr_pause = 1;
	strcpy(errorguide, "http://wiki.quakesrc.org/index.php/Q%i");
	GetModuleFileName(NULL, frikqccdir, sizeof(frikqccdir));
	StripFilename(frikqccdir);

	sprintf (filename, "%s\\frikqcc.cfg", frikqccdir);
	handle = open(filename,O_RDONLY | O_BINARY);
	if (handle != -1)
	{
		close(handle);
		LoadFile (filename, (void **)&src2);
		ParseParms(src2);
	}

	ParseParms(lpCmdLine); // should work...
	hIcon = LoadIcon (hInstance, MAKEINTRESOURCE (IDR_MAINFRAME));
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAINWIN), NULL, &MainDlgProc);
    return TRUE;
}



#endif