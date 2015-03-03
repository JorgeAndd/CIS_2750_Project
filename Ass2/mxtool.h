/*****
mxtool.h -- header file for mxtool.c
Last update:  9-Oct-2014, Version 1

Jorge Luiz Andrade
#0906139
*****/

#ifndef MXTOOL_H
#define MXTOOL_H
#include <stdlib.h>
#include <regex.h>
#include <ctype.h>

#include "mxutil.h"

enum SELECTOR { KEEP, DISCARD };

enum BIBFIELD { AUTHOR=0, TITLE, PUBINFO, CALLNUM };

typedef char *BibData[4];

int review(const XmElem *top, FILE *outfile);

int concat(const XmElem *top1, const XmElem *top2, FILE *outfile);

int selects(const XmElem *top, const enum SELECTOR sel,
            const char *pattern, FILE *outfile);

int libFormat(const XmElem *top, FILE *outfile);
int bibFormat(const XmElem *top, FILE *outfile);

//Helper functions
int match(const char *data, const char *regex);
void marc2bib(const XmElem *mrec, BibData bdata);
void sortRecs(XmElem *collection, const char *keys[]);

#endif
