//---------------------------------------------------------------------------
// EdgeNoise
// PBA 23/11/2011
// Suppresses noise around edges
//---------------------------------------------------------------------------

// TODO :

#include <windows.h>
#include <stdio.h>
#include <math.h>

#include "include/ScriptInterpreter.h"
#include "include/ScriptError.h"
#include "include/ScriptValue.h"

#include "resource.h"
#include "include/filter.h"

#define SQRT2PI 2.5066282746310	// sqrt(2*PI)
#define MATRIXSIZE	16

///////////////////////////////////////////////////////////////////////////

int	InitProc(FilterActivation *fa, const FilterFunctions *ff);
void	deInitProc(FilterActivation *fa, const FilterFunctions *ff);
int	RunProc(const FilterActivation *fa, const FilterFunctions *ff);
int	StartProc(FilterActivation *fa, const FilterFunctions *ff);
int	EndProc(FilterActivation *fa, const FilterFunctions *ff);
long	ParamProc(FilterActivation *fa, const FilterFunctions *ff);
int	ConfigProc(FilterActivation *fa, const FilterFunctions *ff, HWND hwnd);
void	StringProc(const FilterActivation *fa, const FilterFunctions *ff, char *str);
void	ScriptConfig(IScriptInterpreter *isi, void *lpVoid, CScriptValue *argv, int argc);
bool	FssProc(FilterActivation *fa, const FilterFunctions *ff, char *buf, int buflen);

///////////////////////////////////////////////////////////////////////////

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char byte;

typedef struct FilterData
{
	long	EdgeWidth;
	long	BlurWidth;
	long	PosStrength;
	long	NegStrength;
	long	EdgeOffset;
	long	BlurOffset;
	bool	ShowEdges;
	bool	ShowBlur;
} FilterData;

ScriptFunctionDef tutorial_func_defs[]={
    { (ScriptFunctionPtr)ScriptConfig, "Config", "0iiiiii" },
    { NULL },
};

CScriptObject tutorial_obj={
    NULL, tutorial_func_defs
};

struct FilterDefinition FilterDef =
{
	NULL,NULL,NULL,			// next, prev, module
	"EdgeNoise",					// name
	"Suppress noise around edges", // description
	"PBA",
	NULL,							// private_data
	sizeof(FilterData),		// inst_data_size
	InitProc,					// initProc
	NULL,							// deinitProc
	RunProc,
	ParamProc,
	ConfigProc,
	StringProc,
	StartProc,
	EndProc,
	&tutorial_obj,
	FssProc,
};

///////////////////////////////////////////////////////////////////////////

extern "C" int __declspec(dllexport) __cdecl VirtualdubFilterModuleInit2(FilterModule *fm, const FilterFunctions *ff, int& vdfd_ver, int& vdfd_compat);
extern "C" void __declspec(dllexport) __cdecl VirtualdubFilterModuleDeinit(FilterModule *fm, const FilterFunctions *ff);

static FilterDefinition *fd_tutorial;

///////////////////////////////////////////////////////////////////////////

int __declspec(dllexport) __cdecl VirtualdubFilterModuleInit2(FilterModule *fm, const FilterFunctions *ff, int& vdfd_ver, int& vdfd_compat)
{
	if (!(fd_tutorial = ff->addFilter(fm, &FilterDef, sizeof(FilterDefinition))))
		return 1;

	vdfd_ver=VIRTUALDUB_FILTERDEF_VERSION;
	vdfd_compat=VIRTUALDUB_FILTERDEF_COMPATIBLE;

	return(0);
}

void __declspec(dllexport) __cdecl VirtualdubFilterModuleDeinit(FilterModule *fm, const FilterFunctions *ff)
{
	ff->removeFilter(fd_tutorial);
}

///////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
// InitProc
// Called one time during the filter initialisation
// PBA 11/10/2006
//---------------------------------------------------------------------------

int InitProc(FilterActivation *fa, const FilterFunctions *ff)
{
	FilterData *mfd=(FilterData*)fa->filter_data;

	mfd->EdgeWidth=5;
	mfd->BlurWidth=5;
	mfd->PosStrength=32;
	mfd->NegStrength=32;
	mfd->EdgeOffset=0;
	mfd->BlurOffset=0;
	mfd->ShowEdges=0;
	mfd->ShowBlur=0;
	return(0);
}

//---------------------------------------------------------------------------
// deInitProc
// Called one time during the filter initialisation
// PBA 22/05/2008
//---------------------------------------------------------------------------

void deInitProc(FilterActivation *fa, const FilterFunctions *ff)
{
//	FilterData *mfd=(FilterData*)fa->filter_data;
}

//---------------------------------------------------------------------------
// StartProc
// Called before processing for allocating buffers and initialising data
// PBA 16/05/2008
//---------------------------------------------------------------------------

int StartProc(FilterActivation *fa, const FilterFunctions *ff)
{
//	FilterData *mfd=(FilterData*)fa->filter_data;
	return(0);
}

//---------------------------------------------------------------------------
// RunProc
// Main processing routine
// PBA
//---------------------------------------------------------------------------

int RunProc(const FilterActivation *fa,const FilterFunctions *ff)
{
long	R,G,B,Y,Y2,D,DD,RR,GG,BB,R2,G2,B2;
long	i,x;
PixDim w,h,width,height;
bool	ShowEdges,ShowBlur;
PixOffset pitch,modulo;
long	PosStrength,EdgeWidth,EdgeOffset,NegStrength,BlurWidth,BlurOffset;
Pixel32	Pix;
Pixel32	*Src,*Dst;
long	DeltaTable[4096];
long	*DeltaPtr;

// Initializations
	FilterData *mfd=(FilterData*)fa->filter_data;
	width=fa->src.w;
	height=fa->src.h;
	pitch=fa->src.pitch;
	modulo=fa->src.modulo;

// Parameters from the user interface
	PosStrength=mfd->PosStrength;
	EdgeWidth=mfd->EdgeWidth;
	EdgeOffset=mfd->EdgeOffset;
	NegStrength=mfd->NegStrength;
	BlurWidth=mfd->BlurWidth;
	BlurOffset=mfd->BlurOffset;
	ShowEdges=mfd->ShowEdges;
	ShowBlur=mfd->ShowBlur;

	if((BlurWidth<1)||(EdgeWidth<1)) return(1);

// Main loop

	Src=(Pixel32*)fa->src.data;
	Dst=(Pixel32*)fa->dst.data;
	for(h=height;--h>=0;)
	{
		Y2=0;
		RR=GG=BB=0;
		for(x=0,w=width;--w>=0;x++)
		{
			// Blur computing
			if((x-BlurOffset>=0)&&(x-BlurOffset<width))
				Pix=Src[x-BlurOffset];
			else
				Pix=0;
			R=(short)((Pix>>16)&0xFF);
			G=(short)((Pix>>8)&0xFF);
			B=(short)(Pix&0xFF);
			RR+=R;
			GG+=G;
			BB+=B;

			if((x-BlurWidth>=0)&&(x-BlurOffset-BlurWidth>=0)&&(x-BlurOffset-BlurWidth<width))
				Pix=Src[x-BlurOffset-BlurWidth];
			else
				Pix=0;
			R2=(short)((Pix>>16)&0xFF);
			G2=(short)((Pix>>8)&0xFF);
			B2=(short)(Pix&0xFF);
			RR-=R2;
			GG-=G2;
			BB-=B2;

			// Edge computing
			if((x-EdgeOffset>=0)&&(x-EdgeOffset<width))
				Pix=Src[x-EdgeOffset];
			else
				Pix=0;
			R=(short)((Pix>>16)&0xFF);
			G=(short)((Pix>>8)&0xFF);
			B=(short)(Pix&0xFF);
			Y=B+(R<<1)+(G<<2);	// Quick RGB to Y conversion
			D=Y-Y2;
			DeltaPtr=&DeltaTable[x];
			*DeltaPtr=D;
			DD=D;

			i=EdgeWidth;
			if(i>x) i=x;
			while(--i>=0)
				DD+=*DeltaPtr--;
			if(DD>0)
				DD=((DD*PosStrength)>>5);
			else
				DD=((DD*NegStrength)>>5);

			if(ShowEdges)
			{
				// Preview showing rising edges in blue and falling edges in red
				G=Y/7;
				if(DD<0)
				{
					R=-DD;
					if(R>255) R=255;
					B=0;
				}
				else if(DD>0)
				{
					B=DD;
					if(B>255) B=255;
					R=0;
				}
				else
					R=B=0;
			}
			else if(ShowBlur)
			{
				// Preview showing only the blurred picture
				R=RR/BlurWidth;
				G=GG/BlurWidth;
				B=BB/BlurWidth;
			}
			else
			{
				// The real picture operation
				if(DD<0)
				{
					DD=-DD;
					if(DD>256) DD=256;
					R=(R*(256-DD)+(RR*DD/BlurWidth))>>8;
					G=(G*(256-DD)+(GG*DD/BlurWidth))>>8;
					B=(B*(256-DD)+(BB*DD/BlurWidth))>>8;
				}
				else if(DD>0)
				{
					if(DD>256) DD=256;
					R=(R*(256-DD)+(RR*DD/BlurWidth))>>8;
					G=(G*(256-DD)+(GG*DD/BlurWidth))>>8;
					B=(B*(256-DD)+(BB*DD/BlurWidth))>>8;
				}
			}
			*Dst++=(R<<16)|(G<<8)|B;
			Y2=Y;
		}
		Src=(Pixel32*)((char*)(Src+width)+modulo);
		Dst=(Pixel32*)((char*)(Dst)+modulo);
	}

	return(0);
}

//---------------------------------------------------------------------------
// EndProc
// Called after processing for freeing buffers
// PBA 11/10/2006
//---------------------------------------------------------------------------

int EndProc(FilterActivation *fa,const FilterFunctions *ff)
{
//	FilterData *mfd=(FilterData*)fa->filter_data;
	return(0);
}

//---------------------------------------------------------------------------
// ParamProc
// Filter parameters
// PBA 11/10/2006
//---------------------------------------------------------------------------

long ParamProc(FilterActivation *fa,const FilterFunctions *ff)
{
//	FilterData *mfd=(FilterData*)fa->filter_data;
	return(FILTERPARAM_SWAP_BUFFERS);
}

//---------------------------------------------------------------------------
// ConfigDlgProc
// Configuration dialog box input / output
//---------------------------------------------------------------------------

BOOL CALLBACK ConfigDlgProc(HWND hdlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
	FilterData *mfd=(FilterData*)GetWindowLong(hdlg,DWL_USER);

	switch(msg)
	{
	case WM_INITDIALOG:
		SetWindowLong(hdlg,DWL_USER,lParam);
		mfd=(FilterData*)lParam;
		if(mfd)
		{
			SetDlgItemInt(hdlg,IDC_EW,mfd->EdgeWidth,0);
			SetDlgItemInt(hdlg,IDC_BW,mfd->BlurWidth,0);
			SetDlgItemInt(hdlg,IDC_PES,mfd->PosStrength,0);
			SetDlgItemInt(hdlg,IDC_NES,mfd->NegStrength,0);
			SetDlgItemInt(hdlg,IDC_EO,mfd->EdgeOffset,1);
			SetDlgItemInt(hdlg,IDC_BO,mfd->BlurOffset,1);
			CheckDlgButton(hdlg,IDC_SE,mfd->ShowEdges);
			CheckDlgButton(hdlg,IDC_SB,mfd->ShowBlur);
		}
		return(true);
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			if(mfd)
			{
				mfd->EdgeWidth=GetDlgItemInt(hdlg,IDC_EW,0,0);
				mfd->BlurWidth=GetDlgItemInt(hdlg,IDC_BW,0,0);
				mfd->PosStrength=GetDlgItemInt(hdlg,IDC_PES,0,0);
				mfd->NegStrength=GetDlgItemInt(hdlg,IDC_NES,0,0);
				mfd->EdgeOffset=GetDlgItemInt(hdlg,IDC_EO,0,1);
				mfd->BlurOffset=GetDlgItemInt(hdlg,IDC_BO,0,1);
				mfd->ShowEdges=!!IsDlgButtonChecked(hdlg,IDC_SE);
				mfd->ShowBlur=!!IsDlgButtonChecked(hdlg,IDC_SB);
			}
			EndDialog(hdlg,0);
			return(true);
		case IDCANCEL:
			EndDialog(hdlg,1);
			return(false);
		}
		break;
	}
	return(false);
}

//---------------------------------------------------------------------------
// ConfigProc
// Access to the configuration dialog box
//---------------------------------------------------------------------------

int ConfigProc(FilterActivation *fa,const FilterFunctions *ff,HWND hwnd)
{
	return(DialogBoxParam(fa->filter->module->hInstModule,MAKEINTRESOURCE(IDD_FILTER_TUTORIAL),hwnd,ConfigDlgProc,(LPARAM)fa->filter_data));
}

//---------------------------------------------------------------------------
// StringProc
// Display info in the filter list
//---------------------------------------------------------------------------

void StringProc(const FilterActivation *fa, const FilterFunctions *ff, char *str)
{
	FilterData *mfd=(FilterData*)fa->filter_data;
	_snprintf(str,80," (<%d/%d>[+%ld-%ld]{%ld %ld})",mfd->EdgeWidth,mfd->EdgeOffset,mfd->PosStrength,mfd->NegStrength,mfd->BlurWidth,mfd->BlurOffset);
}

//---------------------------------------------------------------------------
// ScriptConfig
// Initialisation of filter parameters
//---------------------------------------------------------------------------

void ScriptConfig(IScriptInterpreter *isi, void *lpVoid, CScriptValue *argv, int argc)
{
	FilterActivation *fa=(FilterActivation*)lpVoid;
	FilterData *mfd=(FilterData*)fa->filter_data;
	mfd->EdgeWidth=argv[0].asInt();
	mfd->BlurWidth=argv[1].asInt();
	mfd->PosStrength=argv[2].asInt();
	mfd->NegStrength=argv[3].asInt();
	mfd->EdgeOffset=argv[4].asInt();
	mfd->BlurOffset=argv[5].asInt();
}

//---------------------------------------------------------------------------
// FssProc
// Backup of filter parameters
//---------------------------------------------------------------------------

bool FssProc(FilterActivation *fa, const FilterFunctions *ff, char *buf, int buflen)
{
	FilterData *mfd=(FilterData *)fa->filter_data;
	_snprintf(buf,buflen,"Config(%d,%d,%d,%d,%d,%d)",mfd->EdgeWidth,mfd->BlurWidth,mfd->PosStrength,mfd->NegStrength,mfd->EdgeOffset,mfd->BlurOffset);
	return(true);
}

