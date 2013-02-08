/* 3-D angle-domain Kirchhoff migration based on escape tables. */
/*
  Copyright (C) 2012 University of Texas at Austin

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <rsf.h>

#include "cram_gather3.h"
#include "cram_point3.h"
#include "esc_point3.h"

int main (int argc, char* argv[]) {
    int iz, ix, iy, nz, nx, ny, nb, na, nt;
    float dt, db, da, t0, b0, a0, dbx, dby, dxm, dym;
    float dz, z0, dx, x0, dy, y0, zd, z, x, y, zf, vconst = 1.5;
    float oazmin = 180.0, oazmax = 180.0, dazmin = 180.0, dazmax = 180.0;
    float xbmin, xbmax, ybmin, ybmax, zbmin, zbmax;
    float oaz, daz, armin, armax;
    float ***esc;
    sf_file esct, data = NULL, survey = NULL, vz = NULL;
    sf_file imag = NULL, oimag = NULL, dimag = NULL, osmap = NULL,
            dsmap = NULL, oimap = NULL, dimap = NULL;

    bool amp, mute, outaz;
    sf_cram_data2 cram_data;
    sf_cram_survey3 cram_survey;
    sf_cram_slowness3 cram_slowness;
    sf_cram_rbranch3 cram_rbranch;
    sf_cram_point3 cram_point;
    sf_cram_gather3 cram_gather;

    sf_init (argc, argv);

    esct = sf_input ("in");
    /* Escape tables */

    /* Phase space dimensions (also, imaging dimensions) */
    if (!sf_histint (esct, "n1", &na)) sf_error ("No n1= in input");
    if (na != ESC3_NUM) sf_error ("Need n1=%d in input", ESC3_NUM);
    if (!sf_histint (esct, "n2", &nb)) sf_error ("No n2= in input");
    if (!sf_histint (esct, "n3", &na)) sf_error ("No n3= in input");
    if (!sf_histint (esct, "n4", &nz)) sf_error ("No n4= in input");
    if (!sf_histint (esct, "n5", &nx)) sf_error ("No n5= in input");
    if (!sf_histint (esct, "n6", &ny)) sf_error ("No n6= in input");
    if (!sf_histfloat (esct, "d2", &db)) sf_error ("No d2= in input");
    if (!sf_histfloat (esct, "o2", &b0)) sf_error ("No o2= in input");
    if (!sf_histfloat (esct, "d3", &da)) sf_error ("No d3= in input");
    if (!sf_histfloat (esct, "o3", &a0)) sf_error ("No o3= in input");
    if (!sf_histfloat (esct, "d4", &dz)) sf_error ("No d4= in input");
    if (!sf_histfloat (esct, "o4", &z0)) sf_error ("No o4= in input");
    if (!sf_histfloat (esct, "d5", &dx)) sf_error ("No d5= in input");
    if (!sf_histfloat (esct, "o5", &x0)) sf_error ("No o5= in input");
    if (!sf_histfloat (esct, "d6", &dy)) sf_error ("No d6= in input");
    if (!sf_histfloat (esct, "o6", &y0)) sf_error ("No o6= in input");

    if (!sf_histfloat (esct, "Xmin", &xbmin)) sf_error ("No Xmin= in input");
    if (!sf_histfloat (esct, "Xmax", &xbmax)) sf_error ("No Xmax= in input");
    if (!sf_histfloat (esct, "Ymin", &ybmin)) sf_error ("No Ymin= in input");
    if (!sf_histfloat (esct, "Ymax", &ybmax)) sf_error ("No Ymax= in input");
    if (!sf_histfloat (esct, "Zmin", &zbmin)) sf_error ("No Zmin= in input");
    if (!sf_histfloat (esct, "Zmax", &zbmax)) sf_error ("No Zmax= in input");

    /* Surface depth */
    zd = zbmin + 0.25*dz;

    if (!sf_getbool ("amp", &amp)) amp = true;
    /* n - do not apply amplitude correction weights */
    if (!sf_getbool ("mute", &mute)) mute = false;
    /* y - mute signal in constant z plane before stacking */
    if (!sf_getbool ("outaz", &outaz)) outaz = true;
    /* n - stack azimuth direction before output */

    if (mute) {
        if (!sf_getfloat ("oazmin", &oazmin)) oazmin = 180.0;
        /* Maximum allowed opening angle at z min */
        if (!sf_getfloat ("oazmax", &oazmax)) oazmax = 180.0;
        /* Maximum allowed opening angle at z max */
        if (!sf_getfloat ("dazmin", &dazmin)) dazmin = 180.0;
        /* Maximum allowed opening dip angle at z min */
        if (!sf_getfloat ("dazmax", &dazmax)) dazmax = 180.0;
        /* Maximum allowed opening dip angle at z max */
        if (oazmin < 0.0) oazmin = 0.0;
        if (oazmax < 0.0) oazmax = 0.0;
        if (dazmin < 0.0) dazmin = 0.0;
        if (dazmax < 0.0) dazmax = 0.0;
    }

    if (!sf_getfloat ("dbx", &dbx)) dbx = 10.0*dx;
    /* Size of search bins in x */
    if (!sf_getfloat ("dby", &dby)) dby = 10.0*dy;
    /* Size of search bins in y */

    if (!sf_getfloat ("dxm", &dxm)) dxm = 5.0*dx;
    /* Taper length in x */
    if (!sf_getfloat ("dym", &dym)) dym = 5.0*dy;
    /* Taper length in y */

    if (!sf_getfloat ("armin", &armin)) armin = 0.01*dy*dx;
    /* Minimum allowed area for an exit ray branch */
    if (!sf_getfloat ("armax", &armax)) armax = 100.0*dy*dx;
    /* Maximum allowed area for an exit ray branch */

    esc = sf_floatalloc3 (ESC3_NUM, nb, na);

    if (sf_getstring ("data")) {
        /* Processed prestack data */
        data = sf_input ("data");
    } else {
        sf_error ("Need data=");
    }

    if (sf_getstring ("survey")) {
        /* Survey info for input data */
        survey = sf_input ("survey");
    } else {
        sf_error ("Need survey=");
    }

    /* Data dimensions */
    if (!sf_histint (data, "n1", &nt)) sf_error ("No n1= in data");
    if (!sf_histfloat (data, "d1", &dt)) sf_error ("No d1= in data");
    if (!sf_histfloat (data, "o1", &t0)) sf_error ("No o1= in data");

    if (sf_getstring ("vz")) {
        /* Velocity model for amplitude weights */
        vz = sf_input ("vz");
    } else {
        if (!sf_getfloat ("vconst", &vconst)) vconst = 1.5;
        /* Constant velocity, if vz= is not used */
    }

    /* Data object */
    cram_data = sf_cram_data2_init (data);
    /* Survey object */
    cram_survey = sf_cram_survey3_init (survey, true);
    sf_fileclose (survey);
    /* Slowness object */
    cram_slowness = sf_cram_slowness3_init (vz, vconst);
    /* Exit ray branches object */
    cram_rbranch = sf_cram_rbranch3_init (nb, na, zd, t0 + (nt - 1)*dt,
                                          xbmin, xbmax, ybmin, ybmax,
                                          dbx, dby, cram_slowness);
    sf_cram_rbranch3_set_arminmax (cram_rbranch, armin, armax);
    /* Subsurface point image object */
    cram_point = sf_cram_point3_init (nb, b0, db, na, a0, da, cram_data, cram_survey,
                                      cram_slowness, cram_rbranch);
    sf_cram_point3_set_amp (cram_point, amp);
    sf_cram_point3_set_taper (cram_point, dxm, dym);
    /* Image and gathers accumulator object */
    cram_gather = sf_cram_gather3_init (nb, na, nz, b0, db, a0, da,
                                        oazmax, dazmax, outaz);

    imag = sf_output ("out");
    sf_cram_gather3_setup_image_output (cram_gather, esct, imag);

    if (sf_getstring ("agath")) {
        /* Opening angle gathers (angle, azimuth, z, x, y) */
        oimag = sf_output ("agath");
        sf_cram_gather3_setup_oangle_output (cram_gather, esct, oimag);
        if (sf_getstring ("imap")) {
            /* Opening gathers illumination (angle, azimuth, z, x, y) */
            oimap = sf_output ("imap");
            sf_cram_gather3_setup_oangle_output (cram_gather, esct, oimap);
        }
        if (sf_getstring ("smap")) {
            /* Opening gathers energy (angle, azimuth, z, x, y) */
            osmap = sf_output ("smap");
            sf_cram_gather3_setup_oangle_output (cram_gather, esct, osmap);
        }
        sf_cram_point3_set_compute_agath (cram_point, true);
    }

    if (sf_getstring ("dipagath")) {
        /* Dip angle gathers (angle, azimuth, z, x, y) */
        dimag = sf_output ("dipagath");
        sf_cram_gather3_setup_dangle_output (cram_gather, esct, dimag);
        if (sf_getstring ("dipimap")) {
            /* Dip gathers illumination (angle, azimuth, z, x, y) */
            dimap = sf_output ("dipimap");
            sf_cram_gather3_setup_dangle_output (cram_gather, esct, dimap);
        }
        if (sf_getstring ("dipsmap")) {
            /* Dip gathers energy (angle, azimuth, z, x, y) */
            dsmap = sf_output ("dipsmap");
            sf_cram_gather3_setup_dangle_output (cram_gather, esct, dsmap);
        }
        sf_cram_point3_set_compute_dipgath (cram_point, true);
    }

    for (iy = 0; iy < ny; iy++) { /* Loop over image y */
        y = y0 + iy*dy;
        for (ix = 0; ix < nx; ix++) { /* Loop over image x */
            x = x0 + ix*dx;
            sf_warning ("Lateral %d of %d (x=%g, y=%g)", iy*nx + ix + 1, nx*ny, x, y);
            for (iz = 0; iz < nz; iz++) { /* Loop over image z */
                z = z0 + iz*dz;
                if (1 == nx && 1 == ny)
                    sf_warning ("z=%g;", z);
                /* Escape variables for this (z, x, y) location */
                sf_floatread (esc[0][0], ESC3_NUM*na*nb, esct);
                if (mute) {
                    zf = (z - zbmin)/(zbmax - zbmin);
                    /* Maximum opening angle for this z */
                    oaz = oazmax - zf*(oazmax - oazmin);
                    /* Maximum dip angle for this z */
                    daz = dazmax - zf*(dazmax - dazmin);
                    sf_cram_point3_set_mute (cram_point, oaz, daz);
                }
                /* Image for one (z, x, y) location */
                if (z > zd)
                    sf_cram_point3_compute (cram_point, z, x, y, esc);
                sf_cram_gather3_image_output (cram_gather, cram_point, imag);
                /* Add to the angle gathers */
                if (oimag)
                    sf_cram_gather3_oangle_output (cram_gather, cram_point,
                                                   oimag, osmap, oimap);
                if (dimag)
                    sf_cram_gather3_dangle_output (cram_gather, cram_point,
                                                   dimag, dsmap, dimap);
            } /* iz */
        } /* ix */
    } /* iy */

    sf_cram_gather3_close (cram_gather);
    sf_cram_point3_close (cram_point);
    sf_cram_rbranch3_close (cram_rbranch);
    sf_cram_slowness3_close (cram_slowness);
    sf_cram_survey3_close (cram_survey);
    sf_cram_data2_close (cram_data);

    sf_fileclose (data);
    if (vz)
        sf_fileclose (vz);
    if (dimag)
        sf_fileclose (dimag);
    if (osmap)
        sf_fileclose (osmap);
    if (dsmap)
        sf_fileclose (dsmap);
    if (oimap)
        sf_fileclose (oimap);
    if (dimap)
        sf_fileclose (dimap);

    free (esc[0][0]);
    free (esc[0]);
    free (esc);

    return 0;
}
