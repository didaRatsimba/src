/*
 * Copyright 1987 the Board of Trustees of the Leland Stanford Junior
 * University. Official permission to use this software is included in
 * the documentation. It authorizes you to use this file for any
 * non-commercial purpose, provided that this copyright notice is not
 * removed and that any modifications made to this file are commented
 * and dated in the style of my example below.
 */

/*
 *
 *  source file:   ./filters/vplib/vpattr.c
 *
 * Joe Dellinger (SEP), Dec 19 1987
 *	Inserted this sample edit history entry.
 *	Please log any further modifications made to this file:
 * Joe Dellinger (AMOCO), Nov 19 1994
 *	Keep track of what color table entries have been used.
 *	Don't bother to re-write out color table entries if
 *	doing so would have no effect.
 */

#include <stdio.h>

#include <rsfplot.h>

#include "../include/attrcom.h"
#include "../include/params.h"
#include "../include/extern.h"
#include "../include/round.h"
#include "../include/enum.h"
#include "../include/pat.h"
#include "../include/closestat.h"
#include "../include/err.h"
#include "../include/erasecom.h"
#include "../include/mesgcom.h"

#include "../genlib/genpen.h"
#include "../utilities/util.h"

#include "dovplot.h"
#include "init_vplot.h"

#include "_vp.h"
#include "vpdoc.h"

#include "vppen.h"

extern int      allow_pipe;
extern int      first_time;

bool             vpbig = true;
bool             vpdumb = false;
int             vpstat = NO;
int             vpfit = NO;
float           xsize = 0., ysize = 0.;
int             vpalign = NO;
bool             vpstyle = true;
bool             vpblast = true;
int             vpbit = 0;
char            *vpaligns;
int             vparray[2] = {0, 0};
int             vpasize[2] = {0, 0};
int             vpframe = -1;

int             vpsetflag;

int             lost = YES;

char            name[] = "vppen";

struct device dev = {
    /* control routines */
    vpopen,		/* open */
    vpreset,		/* reset */
    vpmessage,		/* message */
    vperase,		/* erase */
    nullclose,            /* close */
    
    /* high level output */
    vpvector,		/* vector */
    vpmarker,		/* marker */
    vptext,		/* text */
    genarea,		/* area */
    vpraster,		/* raster */
    genpoint,		/* point */
    vpattributes,		/* attributes */
    
    /* input */
    vp_do_dovplot,               /* reader */
    nullgetpoint,		/* getpoint */
    nullinteract,		/* interact */
    
    /* low level output */
    vpplot,		/* plot */
    vpstartpoly,		/* startpoly */
    vpmidpoly,		/* midpoly */
    vpendpoly		/* endpoly */
};

int             vpcolor = VP_WHITE;
int             vpfat = 0;

int		vpscoltabinfo[VPPEN_NUM_COL][4];
int		vpsetcoltabanyway = NO;

void vpattributes (int command, int value, int v1, int v2, int v3)
/*< attributes >*/
{
static float    vpdash[MAXDASH];
static float    vpgap[MAXDASH];
static int      vpndash;
static int     *vpattrarray;
static int      vpxwmin, vpywmin, vpxwmax, vpywmax;
static int      vpfont1, vpfont2, vpfont3;
static int      vpjust1, vpjust2;
static int      vpcolt1, vpcolt2, vpcolt3, vpcolt4;
static int      vpovly;
int             ii, jj;

    lost = YES;

    if (!vpdumb)
    {
	switch (command)
	{
	case SET_COLOR:
	    if (vpsetflag & F_COL)
	    {
		if (vpcolor == value)
		    break;
	    }
	    vp_color (value);
	    vpcolor = value;
	    vpsetflag |= F_COL;
	    break;

	case SET_COLOR_TABLE:
	    if (vpsetflag & F_COLT)
	    {
		if (vpcolt1 == value &&
		    vpcolt2 == v1 &&
		    vpcolt3 == v2 &&
		    vpcolt4 == v3)
		    break;
	    }
/*
 * The only global attribute in vplot that stays set across
 * erases is the color table. If we're about to re-set this
 * color to the same thing it's already set to, then don't
 * bother! (Unless we've been told to do it anyway.)
 */
	    if (vpsetcoltabanyway ||
		vpscoltabinfo[value][ISITSET] != YES ||
	        vpscoltabinfo[value][1] != v1 ||
	        vpscoltabinfo[value][2] != v2 ||
	        vpscoltabinfo[value][3] != v3)
	    {
	        vp_coltab (value,
		       (float) v1 / (float) MAX_GUN,
		       (float) v2 / (float) MAX_GUN,
		       (float) v3 / (float) MAX_GUN);
/*
 * A new one! Save it.
 */
		vpscoltabinfo[value][ISITSET] = YES;
		vpscoltabinfo[value][1] = v1;
		vpscoltabinfo[value][2] = v2;
		vpscoltabinfo[value][3] = v3;
	    }

	    vpcolt1 = value;
	    vpcolt2 = v1;
	    vpcolt3 = v2;
	    vpcolt4 = v3;
	    vpsetflag |= F_COLT;

	    break;

	case SET_WINDOW:

	    if (vpsetflag & F_CLIP)
	    {
		if (value == vpxwmin &&
		    v1 == vpywmin &&
		    v2 == vpxwmax &&
		    v3 == vpywmax)
		    break;
	    }

	    vp_clip ((float) (value) / RPERIN, (float) (v1) / RPERIN,
		     (float) (v2) / RPERIN, (float) (v3) / RPERIN);
	    vpxwmin = value;
	    vpywmin = v1;
	    vpxwmax = v2;
	    vpywmax = v3;
	    vpsetflag |= F_CLIP;
	    break;

	case NEW_DASH:
	    if (vpsetflag & F_DASH)
	    {
		if (value == vpndash)
		{
		    jj = YES;
		    for (ii = 0; ii < value; ii++)
		    {
			if (vpdash[ii] != dashes[2 * ii] ||
			    vpgap[ii] != dashes[2 * ii + 1])
			    jj = NO;
		    }
		    if (jj)
			break;
		}
	    }

	    for (ii = 0; ii < value; ii++)
	    {
		vpdash[ii] = dashes[2 * ii];
		vpgap[ii] = dashes[2 * ii + 1];
	    }
	    vp_setdash (vpdash, vpgap, value);
	    vpndash = value;
	    vpsetflag |= F_DASH;
	    break;

	case NEW_PAT:
	    vpattrarray = (int *) malloc ((unsigned)
			(pat[value].xdim * pat[value].ydim * sizeof (int)));

	    if (vpattrarray != NULL)
	    {
		for (ii = 0; ii < pat[value].xdim * pat[value].ydim; ii++)
		    vpattrarray[ii] = pat[value].patbits[ii];

		vp_patload ((int) RPERIN,
			    pat[value].xdim, pat[value].ydim,
			    value - 1, vpattrarray);

		free ((char *) vpattrarray);
	    }
	    break;

	case NEW_FONT:
	    if (value == -1)
		value = vpfont1;
	    if (v1 == -1)
		v1 = vpfont2;
	    if (v2 == -1)
		v2 = vpfont3;

	    if (vpsetflag & F_FONT)
	    {
		if (vpfont1 == value &&
		    vpfont2 == v1 &&
		    vpfont3 == v2)
		    break;
	    }

	    vp_tfont (value, v1, v2);
	    vpfont1 = value;
	    vpfont2 = v1;
	    vpfont3 = v2;
	    vpsetflag |= F_FONT;
	    break;

	case NEW_OVERLAY:
	    if (vpsetflag & F_OVLY)
	    {
		if (vpovly == value)
		    break;
	    }
/*
 * Another libvplot command that doesn't exist but should.
 * XXXXXX
 *		vp_overlay(value);
 */
	    vpsetflag |= F_OVLY;
	    vpovly = value;
	    break;

	case NEW_ALIGN:
	    if (vpsetflag & F_JUST)
	    {
		if (vpjust1 == value &&
		    vpjust2 == v1)
		    break;
	    }
	    vp_tjust (value, v1);
	    vpjust1 = value;
	    vpjust2 = v1;
	    vpsetflag |= F_JUST;
	    break;

	case NEW_FAT:
	    if (vpsetflag & F_FAT)
	    {
		if (vpfat == value)
		    break;
	    }

	    vp_fat (ROUND (value * FATPERIN / RPERIN));
	    vpfat = value;
	    vpsetflag |= F_FAT;
	    break;

	case BEGIN_GROUP:
	    if (value > 0)
		vp_bgroup (group_name);
	    break;

	case END_GROUP:
	    if (value > 0)
		vp_egroup ();
	    break;

	default:
	    break;
	}
    }
    else
    {
	switch (command)
	{
	case SET_COLOR:
	    if (vpsetflag & F_COL)
	    {
		if (vpcolor == value)
		    break;
	    }
	    vp_color (value);
	    vpcolor = value;
	    vpsetflag |= F_COL;
	    break;

	default:
	    break;
	}
    }
}

extern int      first_time;
extern int      style;
extern int      default_style;

int             vpxmax, vpxmin, vpymax, vpymin;
static int      vpxmaxs, vpxmins, vpymaxs, vpymins;

void vp_do_dovplot (int nn, FILE **inpltin, char *innames[])
/*< do vplot >*/
{
int             ii;
int             save_wantras;
int             save_shade;
char            string[80];
static int      it_got_clipped;
float           hh, ww;
float           rescale_x, rescale_y;
char            format_string[80];

    if (nn == 0)
	return;

/*
 * If they want statistics, make one "dummy" pass through first
 * before you really do it.
 * The align and fit options need these statistics to do their work.
 */
    if (vpalign || vpfit)
    {
	/*
	 * Turn on automatic processing 
	 */
	smart_clip = false;
	smart_raster = false;

	/*
	 * Just outline polygons and raster with vectors 
	 */
	save_wantras = wantras;
	save_shade = shade;

	wantras = false;
	shade = NO;

	/*
	 * Turn off any actual output 
	 */
	dev.reset = nulldev;
	dev.message = vplogmessage;
	message = dev.message;
	dev.erase = vpderase;
	dev.close = nullclose;
	dev.vector = vplogvector;
	dev.marker = genmarker;
	dev.text = gentext;
	dev.area = nullarea;
	dev.raster = nullraster;
	dev.point = genpoint;
	dev.attributes = nullattributes;


/*
 * Now do the trial pass
 */

	vpxmaxs = dev_xmin;
	vpxmins = dev_xmax;
	vpymaxs = dev_ymin;
	vpymins = dev_ymax;

	it_got_clipped = NO;

	if (vpstat)
	{
	    if (vpstat == YES)
	    {
		strcpy (format_string,
		"%17s: h=%6.2f w=%6.2f; x=(%6.2f,%6.2f) y=(%6.2f,%6.2f)\n");
	    }
	    else
	    {
		strcpy (format_string,
			"%17s: h= %6.2f w= %6.2f ;  x=( %6.2f , %6.2f ) y=( %6.2f , %6.2f ) ");
	    }
	}

	for (ii = 0; ii < nn; ii++)
	{
	    vpxmax = dev_xmin;
	    vpxmin = dev_xmax;
	    vpymax = dev_ymin;
	    vpymin = dev_ymax;

	    pltin = inpltin[ii];
	    strcpy (pltname, innames[ii]);
	    dovplot ();
	    rewind (pltin);

	    if (vpxmaxs < vpxmax)
		vpxmaxs = vpxmax;
	    if (vpymaxs < vpymax)
		vpymaxs = vpymax;
	    if (vpxmins > vpxmin)
		vpxmins = vpxmin;
	    if (vpymins > vpymin)
		vpymins = vpymin;

/*
 * For vpstat=y, write out the statistics. Also note if any
 * parts of the plot got clipped.
 */
	    if (vpstat)
	    {
		hh = (float) (vpymax - vpymin) / RPERIN;
		ww = (float) (vpxmax - vpxmin) / RPERIN;

		if (hh < 0. || ww < 0.)
		{
		    printf ("%17s: clipped away. ",
			     innames[ii]);
		}
		else
		{
		    printf (format_string,
			    innames[ii],
			    hh, ww,
			    (float) vpxmin / RPERIN,
			    (float) vpxmax / RPERIN,
			    (float) vpymin / RPERIN,
			    (float) vpymax / RPERIN);
		}

		if (vpxmax == dev_xmin || vpxmax == dev_xmax ||
		    vpxmin == dev_xmax || vpxmin == dev_xmin ||
		    vpymax == dev_ymin || vpymax == dev_ymax ||
		    vpymin == dev_ymax || vpymin == dev_ymin)
		{
		    printf ("*\n");
		    it_got_clipped = YES;
		}
		else
		{
		    printf ("\n");
		}
	    }
	}

	if (vpstat && nn > 1)
	{
	    sprintf (string, "All %d", nn);
	    printf (format_string,
		    string,
		    (float) (vpymaxs - vpymins) / RPERIN,
		    (float) (vpxmaxs - vpxmins) / RPERIN,
		    (float) vpxmins / RPERIN,
		    (float) vpxmaxs / RPERIN,
		    (float) vpymins / RPERIN,
		    (float) vpymaxs / RPERIN);
	}

	if (vpstat)
	{
	    if (vpframecount == 0)
	        printf(
		    "            Total %d plot frame.\n", vpframecount + 1);
	    else
	        printf(
		       "            Total %d plot frames.\n", vpframecount + 1);

	    if (it_got_clipped)
	    {
		if (vpbig)
		{
		    printf (
			"\nA * indicates a plot that has been clipped.\n");
		    printf (
			"Remember rotated style or relative size plots go to the top\n");
		    printf (
			"of the \"screen\", which is infinitely far away if big=y.\n");
		}
		else
		{
		    printf (
			"\nA * indicates a plot that has been clipped at the\n");
		    printf (
			"virtual screen boundaries. You may not want this.\n");
		    printf (
			"This clipping can be disabled by the big=y option.\n");
		}
	    }

	    for (ii = 0; ii < nn; ii++)
	    {
		pltin = inpltin[ii];
		fclose (pltin);
	    }
/*
 * Statistics get changed by re-aligning anyways,
 * So might as well just exit if we're doing vpstat=y.
 */
	    return;
	}

/*
 * Compute scale factors for vpfit option.
 * These are passed to dovplot as default scales.
 * The user may have specified both xsize and ysize, or just one.
 * Specifying one but not both is a signal that vppen should choose
 * the other scale factor to preserve the plot's aspect ratio.
 */
	if (vpfit)
	{
	    if (xsize == 0.)
	    {
		xsize = ysize * (vpxmaxs - vpxmins) / (vpymaxs - vpymins);
	    }
	    if (ysize == 0.)
	    {
		ysize = xsize * (vpymaxs - vpymins) / (vpxmaxs - vpxmins);
	    }

	    rescale_x = xsize * RPERIN / (vpxmaxs - vpxmins);
	    vpxmaxs *= rescale_x;
	    vpxmins *= rescale_x;
	    default_xscale *= rescale_x;

	    rescale_y = ysize * RPERIN / (vpymaxs - vpymins);
	    vpymaxs *= rescale_y;
	    vpymins *= rescale_y;
	    default_yscale *= rescale_y;

/*
 * Scale fatness according to whichever dimension is more compressed
 */
	    if (rescale_x > rescale_y)
		fatmult_orig *= rescale_y;
	    else
		fatmult_orig *= rescale_x;

	}
/*
 * Compute shifts for vpalign and vpfit options.
 * These are passed to dovplot as the default shifts.
 */
	switch (vpaligns[0])
	    /*
	     * horizontal 
	     */
	{
	case 'l':
	    default_hshift += (0 - vpxmins);
	    break;
	case 'r':
	    default_hshift += (0 - vpxmaxs);
	    break;
	case 'c':
	    default_hshift += (0 - ((vpxmaxs + vpxmins) / 2));
	    break;
	case 'u':
	    break;
	default:
	    ERR (WARN, name, "Unknown left-right alignment type %c.",
		 vpaligns[0]);
	    break;
	}


	switch (vpaligns[1])
	    /*
	     * vertical 
	     */
	{
	case 'b':
	    default_vshift += (0 - vpymins);
	    break;
	case 't':
	    default_vshift += (0 - vpymaxs);
	    break;
	case 'c':
	    default_vshift += (0 - ((vpymaxs + vpymins) / 2));
	    break;
	case 'u':
	    break;
	default:
	    ERR (WARN, name, "Unknown top-bottom alignment type %c.",
		 vpaligns[1]);
	    break;
	}

	style = default_style;

	reset_parameters ();

	/*
	 * Lie to dovplot, tell it to start from scratch again 
	 */
	first_time = YES;

	/*
	 * Undo the damage from the first pass 
	 */
	wantras = save_wantras;
	shade = save_shade;

	dev.reset = vpreset;
	dev.message = vpmessage;
	message = dev.message;
	dev.erase = vperase;
	dev.close = nullclose;
	dev.vector = vpvector;
	dev.marker = vpmarker;
	dev.text = vptext;
	dev.area = genarea;
	dev.raster = vpraster;
	dev.point = genpoint;
	dev.attributes = vpattributes;
    }

/*
 *********************************************************************
 * "Real" pass
 *********************************************************************
 */

    if (vpdumb)
    {
	dev.message = genmessage;
	dev.vector = genvector;
	dev.marker = genmarker;
	dev.text = gentext;
	dev.area = vecarea;
	dev.raster = genraster;
	smart_clip = false;
	smart_raster = false;
    }
    else
    {
	smart_clip = true;
	smart_raster = true;
    }

/* Second (or first) pass */
    for (ii = 0; ii < nn; ii++)
    {
	pltin = inpltin[ii];
	strcpy (pltname, innames[ii]);
	dovplot ();
	fclose (pltin);
    }
}

int      vpframecount = -1;

void vperase (int command)
/*< erase >*/
{
int ii;
extern int vpsetcoltabanyway;


    if (vparray[0] == 0)
    {
	switch (command)
	{
	case ERASE_START:
	    vpframecount = 0;
	    break;
	case ERASE_MIDDLE:
	    vpframecount++;
	    vp_erase ();
	    if (!vpdumb && vpstyle)
	    {
		vp_style (ABSOLUTE);
	    }
	    lost = YES;
	    vpsetflag = NO;

	    if (!vpdumb)
	    {
/*
 * If this is a new output file, then explicitly set the entire
 * color table to its current state.
 */
		vpsetcoltabanyway = YES;
		for (ii=0; ii < VPPEN_NUM_COL; ii++)
		{
		    if (vpscoltabinfo[ii][ISITSET])
		    {
			vpattributes (SET_COLOR_TABLE, ii,
			    vpscoltabinfo[ii][1],
			    vpscoltabinfo[ii][2],
			    vpscoltabinfo[ii][3]);
		    }
		}
		vpsetcoltabanyway = NO;

		lost = YES;
		vpsetflag = NO;
            }

	    break;
	case ERASE_BREAK:
	    vp_break ();
	    if (!vpdumb && vpstyle)
	    {
		vp_style (ABSOLUTE);
	    }
	    lost = YES;
	    vpsetflag = NO;
	    break;
	default:
	    break;
	}
    }
    else
    {
	switch (command)
	{
	case ERASE_START:
	    vpframecount = 0;
	    dev_ymin = VP_STANDARD_HEIGHT * RPERIN;
	case ERASE_MIDDLE:
	    if (vpframecount < 0)
		ERR (FATAL, name, "Must have initial erase with gridnum");
	    if ((vpframecount % vparray[0]) == 0)
	    {
		dev_xmin = 0;
		dev_ymin -= vpasize[1];
	    }
	    else
	    {
		dev_xmin += vpasize[0];
	    }
	    dev_xmax = dev_xmin + vpasize[0];
	    dev_ymax = dev_ymin + vpasize[1];

	    if (command == ERASE_MIDDLE)
		vp_break ();

	    lost = YES;
	    vpsetflag = NO;
	    reset_parameters ();
	    vpframecount++;

	    if (vpframe >= 0)
	    {
		vp_color (VP_WHITE);
		vp_fat (vpframe);

		vp_move ((float) dev_xmin / RPERIN, (float) dev_ymin / RPERIN);
		vp_draw ((float) dev_xmax / RPERIN, (float) dev_ymin / RPERIN);
		vp_draw ((float) dev_xmax / RPERIN, (float) dev_ymax / RPERIN);
		vp_draw ((float) dev_xmin / RPERIN, (float) dev_ymax / RPERIN);
		vp_draw ((float) dev_xmin / RPERIN, (float) dev_ymin / RPERIN);

		vp_color (vpcolor);
		vp_fat (ROUND (vpfat * FATPERIN / RPERIN));
	    }
	    break;
	case ERASE_BREAK:
	    break;
	default:
	    break;
	}
    }
}

void vpderase (int command)
/*< Dummy erase command; does nothing but count frames. >*/
{
	switch (command)
	{
	case ERASE_START:
	    vpframecount = 0;
	    break;
	case ERASE_MIDDLE:
	    vpframecount++;
	    break;
	case ERASE_BREAK:
	    break;
	default:
	    break;
	}
}

static int      saveitlog;

void vplogmessage (int command, char *string)
/*< logged message >*/ 
{
    switch (command)
    {
    case MESG_READY:
	saveitlog = YES;
	break;
    case MESG_MESSAGE:
	saveitlog = NO;
	break;
    case MESG_TEXT:
	if (saveitlog)
	    fprintf (stderr, "%s", string);
	break;
    default:
	break;
    }
}

extern int      vpxmax, vpxmin, vpymax, vpymin;

void vplogvector (int x1, int y1, int x2, int y2, int nfat, int vpdashon)
/*< log vector >*/
{
    if (clip (&x1, &y1, &x2, &y2))
	return;

    if (x1 > vpxmax)
	vpxmax = x1;
    if (x1 < vpxmin)
	vpxmin = x1;

    if (y1 > vpymax)
	vpymax = y1;
    if (y1 < vpymin)
	vpymin = y1;

    if (x2 > vpxmax)
	vpxmax = x2;
    if (x2 < vpxmin)
	vpxmin = x2;

    if (y2 > vpymax)
	vpymax = y2;
    if (y2 < vpymin)
	vpymin = y2;
}

void vpmarker (int npts, int type, int size, int *pvec)
/*< marker >*/
{
float          *xp;
float          *yp;
int             ii;

    vpsetflag = NO;
    lost = YES;

    xp = (float *) malloc ((unsigned) (npts * sizeof (float)));
    yp = (float *) malloc ((unsigned) (npts * sizeof (float)));

    for (ii = 0; ii < npts; ii++)
    {
	xp[ii] = (float) pvec[2 * ii] / RPERIN;
	yp[ii] = (float) pvec[2 * ii + 1] / RPERIN;
    }

    size = ROUND (size * TXPERIN / RPERIN);

    vp_pmark (npts, type, size, xp, yp);

    free ((char *) xp);
    free ((char *) yp);

}

static int      saveit;
static char     savestring[80 * 24];

void vpmessage (int command, char *string)
/*< message >*/
{
    switch (command)
    {
    case MESG_READY:
	saveit = NO;
	break;
    case MESG_MESSAGE:
	saveit = YES;
	strcpy (savestring, "");
	break;
    case MESG_DONE:
	if (saveit && !vpdumb)
	{
	    vp_message (savestring);
	}
	break;
    case MESG_TEXT:

	if (saveit)
	{
	    if (strcmp (string, CRLF) != 0)
		(void) strcat (savestring, string);
	}
	else
	{
	    fprintf (stderr, "%s", string);
	}

	break;
    default:
	break;
    }
}

void vpopen (int argc, char* argv[])
/*< open >*/
{
    float           atemp[2];
    char            *vpstat_string;
    int		ii;
    
    first_time = YES;
    
    
/*
 * Reset the saved global color table information array.
 */
    for (ii=0; ii < VPPEN_NUM_COL; ii++)
    {
	vpscoltabinfo[ii][ISITSET] = NO;
	vpscoltabinfo[ii][1] = 0;
	vpscoltabinfo[ii][2] = 0;
	vpscoltabinfo[ii][3] = 0;
    }

/*
 * Special options
 */
    if (!sf_getbool ("dumb", &vpdumb)) vpdumb=false;
    if (!sf_getbool ("blast", &vpblast)) vpblast=true;
    if (!sf_getint ("bit", &vpbit)) vpbit=0;
    if (!sf_getint ("grid", &vpframe)) vpframe=-1;

/*
 * stat=L means to insert extra spaces into stat output.
 */
    if (NULL != (vpstat_string = sf_getstring ("stat")))
    {
	if (vpstat_string[0] == 'y' || vpstat_string[0] == 'Y' ||
	    vpstat_string[0] == '1')
	    vpstat = YES;
	else
	    if (vpstat_string[0] == 'l' || vpstat_string[0] == 'L')
		vpstat = 2;
	    else
		vpstat = NO;
    }

    if (NULL == (vpaligns = sf_getstring ("align"))) vpaligns="uu";
    if (!sf_getfloat ("xsize", &xsize)) xsize=0.;
    if (!sf_getfloat ("ysize", &ysize)) ysize=0.;
    if (xsize != 0. || ysize != 0.)
	vpfit = YES;
    
    if (vpstat || strcmp (vpaligns, "uu") != 0 || vpfit)
    {
	vpalign = YES;
	allow_pipe = NO;
    }

    if (!sf_getints ("gridnum", vparray, 2))
	vparray[0] = vparray[1] = 0;
    
    if (vparray[1] == 0)
	vparray[1] = vparray[0];
    
    if (vparray[0] != 0)
    {
	vpbig = false;
	vpstyle = false;
    }

    sf_getbool ("big", &vpbig);
    sf_getbool ("vpstyle", &vpstyle);

    if (vparray[0] != 0)
    {

/* Let it override.

  if (vpbig || vpalign)
  ERR (FATAL, name, "Incompatible option with gridnum");
*/

	if (!sf_getfloats ("gridsize", atemp, 2)) {
	    atemp[0] = (float) (VP_STANDARD_HEIGHT / VP_SCREEN_RATIO) / vparray[0];
	    atemp[1] = (float) (VP_STANDARD_HEIGHT) / vparray[1];	 
	}   

	vpasize[0] = atemp[0] * RPERIN;
	vpasize[1] = atemp[1] * RPERIN;
    }


/*
 * We want to go through the input files ourselves
 */

    dev.reader = vp_do_dovplot;

/*
 * device capabilities
 */

    if (vpbig)
    {
	dev_xmax = VP_MAX * RPERIN;
	dev_ymax = VP_MAX * RPERIN * VP_SCREEN_RATIO;
	dev_xmin = -dev_xmax;
	dev_ymin = -dev_ymax;
	default_hshift = -dev_xmin;
	default_vshift = -dev_ymin;
    }
    else
    {
	dev_xmax = VP_STANDARD_HEIGHT * RPERIN / VP_SCREEN_RATIO;
	dev_ymax = VP_STANDARD_HEIGHT * RPERIN;
	dev_xmin = 0;
	dev_ymin = 0;
	default_hshift = 0;
	default_vshift = 0;
    }

    pixels_per_inch = RPERIN;
    aspect_ratio = 1.;
    num_col = VPPEN_NUM_COL;
    if (vparray[0] == 0)
	size = ABSOLUTE;


/*
 * Since font gets hard-wired in after first pass,
 * make it a nice one if they don't specify it.
 */
    txfont = DEFAULT_HARDCOPY_FONT;
    txprec = DEFAULT_HARDCOPY_PREC;

/*
 * Make vplib routines more useful to be included in other programs
 * besides just vppen
 */
    dev.open = vpopen;
    dev.reset = vpreset;
    dev.message = vpmessage;
    dev.erase = vperase;
    dev.close = nullclose;
    dev.vector = vpvector;
    dev.marker = vpmarker;
    dev.text = vptext;
    dev.area = genarea;
    dev.raster = vpraster;
    dev.point = genpoint;
    dev.attributes = vpattributes;
    dev.getpoint = nullgetpoint;
    dev.interact = nullinteract;
    dev.plot = vpplot;
    dev.startpoly = vpstartpoly;
    dev.midpoly = vpmidpoly;
    dev.endpoly = vpendpoly;

/*
 * To keep messages from being lost
 */
    message = dev.message;

    cachepipe = true;
}

void vpplot (int x, int y, int draw)
/*< plot >*/
    {
    vpsetflag = NO;

    if (draw)
	vp_draw ((float) x / RPERIN, (float) y / RPERIN);
    else
	vp_move ((float) x / RPERIN, (float) y / RPERIN);

    lost = NO;
}

static float   *xp;
static float   *yp;
static int      vnpts;

void vpstartpoly (int npts)
/*< start polygon >*/
{
    vpsetflag = NO;
    lost = YES;
    vnpts = 0;

    xp = (float *) malloc ((unsigned) (npts * sizeof (float)));
    yp = (float *) malloc ((unsigned) (npts * sizeof (float)));
}

void vpmidpoly (int x, int y)
/*< middle polygon >*/
{
    xp[vnpts] = (float) (x) / RPERIN;
    yp[vnpts] = (float) (y) / RPERIN;
    vnpts++;
}

void vpendpoly (int last)
/*< end polygon >*/
{
    if (ipat == 0)
    {
	vp_area (xp, yp, vnpts, -1, pat[ipat].xdim_orig, pat[ipat].ydim_orig);
    }
    else
    {

	if (ipat - 1 != vpcolor)
	    vp_color (ipat - 1);

	vp_fill (xp, yp, vnpts);

	if (ipat - 1 != vpcolor)
	    vp_color (vpcolor);

    }

    free ((char *) xp);
    free ((char *) yp);
}

void vpraster (int xpix, int ypix, int xmin, int ymin, int xmax, int ymax, 
	       unsigned char **raster_block, int orient, int dither_it)
/*< raster >*/
{
    vpsetflag = NO;
    lost = YES;

    vp_raster (raster_block, (vpbit > 0), vpbit, 
	       xpix, ypix, 
	       (float) xmin / RPERIN, (float) ymin / RPERIN,
	       (float) xmax / RPERIN, (float) ymax / RPERIN, orient);
}

extern int      erase;

void vpreset (void)
/*<  Reset everything we can think of.
 * Ignore initial erases, and instead look at the command line
 * value of "erase" to decide whether to have an initial erase
 * or not. >*/
{

/*
 * vpsetflag is used to squeeze out redundant attribute-setting commands.
 */
    vpsetflag = NO;

    if (erase & FORCE_INITIAL)
	vp_erase ();

    if (!vpdumb && vpstyle)
    {
	vp_style (ABSOLUTE);
    }

    if (!vpdumb)
    {
	dev.attributes (SET_WINDOW, dev_xmin, dev_ymin, dev_xmax, dev_ymax);
	dev.attributes (SET_COLOR, VP_WHITE, 0, 0, 0);
	dev.attributes (NEW_FAT, 0, 0, 0, 0);
	dev.attributes (NEW_DASH, 0, 0, 0, 0);
	dev.attributes (NEW_FONT, txfont, txprec, txovly, 0);
	dev.attributes (NEW_ALIGN, txalign.hor, txalign.ver, 0, 0);
	dev.attributes (NEW_OVERLAY, overlay, 0, 0, 0);
    }
}

void vptext (char *string, float pathx, float pathy, float upx, float upy)
/*< text >*/
{
    void (*savevector)(int x1, int y1, int x2, int y2, int nfat, int vpdashon);
    void (*saveattributes)(int command, int value, int v1, int v2, int v3);
    void (*savearea)(int npts, struct vertex  *head);

    vpsetflag = NO;
    lost = YES;

    if (*string == '\0')
	return;

    vp_gtext ((float) xold / RPERIN, (float) yold / RPERIN,
	      pathx / RPERIN, pathy / RPERIN,
	      upx / RPERIN, upy / RPERIN,
	      string);

/*
 *   Now reset the pen position to the end of the text.
 *   Do a dummy run through (if this indeed a gentext font)
 */
    if (txfont < NUMGENFONT)
    {
	savevector = dev.vector;
	saveattributes = dev.attributes;
	savearea = dev.area;

/*
 *   Disconnect everything except error messages
 */
	dev.vector = nullvector;
	dev.attributes = nullattributes;
	dev.area = nullarea;

	gentext (string, pathx, pathy, upx, upy);

	dev.vector = savevector;
	dev.attributes = saveattributes;
	dev.area = savearea;

/*
 * Jon note that this shows you how to find the size of the text.
 */
	vp_move ((float) xold / RPERIN, (float) yold / RPERIN);
    }
}

#define MOVE 0
#define DRAW 1

extern int      lost;

void vpvector (int x1, int y1, int x2, int y2, int nfat, int vpdashon)
/*< vector >*/
{
static int      xlst, ylst;
int             d1, d2;

    if (nfat < 0)
	return;

    /*
     * Important special case: Zero-length vector at the end of what you've
     * already plotted. Don't need to do anything. 
     */
    if (x1 == x2 && y1 == y2 && !lost && x1 == xlst && y1 == ylst)
    {
	return;
    }

/*
 * As stated in the documentation, dev.vector must be
 * ready to accept changes in fatness and linestyle without
 * warning at any time.
 */

    if (nfat != fat)
    {
	vp_fat (ROUND ((float) nfat * FATPERIN / RPERIN));
	lost = YES;
    }

    if (vpdashon != dashon)
    {
	dev.attributes (NEW_DASH, vpdashon, 0, 0, 0);
    }

    /*
     * Minimize movement of "pen" Don't turn around dashed lines, since order
     * of drawing matters. 
     */
    if (!lost && !vpdashon)
    {
	d1 = abs (x1 - xlst) + abs (y1 - ylst);
	d2 = abs (x2 - xlst) + abs (y2 - ylst);
	if (d2 < d1)
	{
	    d1 = x1;
	    d2 = y1;
	    x1 = x2;
	    y1 = y2;
	    x2 = d1;
	    y2 = d2;
	}
    }

    if ((x1 != xlst) || (y1 != ylst) || lost)
    {
	/* Make sure it is a move, not a draw */
	dev.plot (x1, y1, MOVE);
    }
    dev.plot (x2, y2, DRAW);
    xlst = x2;
    ylst = y2;

/*
 * Restore fat and dash stuff if we changed it.
 */
    if (nfat != fat)
    {
	vp_fat (ROUND ((float) fat * FATPERIN / RPERIN));
	lost = YES;
    }

    if (vpdashon != dashon)
    {
	dev.attributes (NEW_DASH, dashon, 0, 0, 0);
    }
/*
 * Above can be inefficient, but that's a rare case and it's hard
 * to get around. (Very hard.) This works!
 */
}
