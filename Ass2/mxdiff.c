/* mxdiff.c
Bill Gardner, 4-Feb-12

"Diff" two MARCXML files using mxutil utilities
   
Usage: mxdiff a.xml b.xml

The comparing stops when a *structural* difference is detected, meaning that it
descends the tree until the elements being compared have differing numbers of
subelements.  Until then, it prints differences in tags, text, isBlank, and attributes.

- If two elements are "isBlank", their texts are not compared.
- If two attributes have different names, their values are not compared.

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mxutil.h"

// plug in the path to your copy of the official MARCXML schema
#define XSDFILE "MARC21slim.xsd"

// stack of tags for identifying location of diffs
#define NTAGS 10
static const char *tagstack[NTAGS];
static int toptag = -1;		// -1 = tag stack empty

static int diffcount = 0;	// no. of differences detected & reported

/* These 3 functions manage the tag stack:

pushtag(tag):	push the ID onto the stack (unless would overflow)
poptag:		pop off the top ID
printags:	print the tags from the bottom up on one line
*/
static void pushtag( const char *tag )
{
	++toptag;
	if ( toptag < NTAGS ) tagstack[toptag] = tag;
}

static void poptag()
{
	--toptag;
}

static void printags()
{
	for ( int i=0; i<=toptag; i++ ) {
		printf( "<%s> ", (i < NTAGS ? tagstack[i] : "?") );
	}
}

/* diffText:
what:	label for texts being compared
texta, textb:	texts to compare

Returns true if texta and textb differ, otherwise false.
*/
static int diffText( const char *what, const char *texta, const char *textb )
{
	const char *a= texta ? texta : "NULL";
	const char *b= textb ? textb : "NULL";
	
	if ( strcmp( a, b ) == 0 ) return 0;
	
	printags();
	printf( "Different %s: '%s' <> '%s'\n", what, a, b );
	++diffcount;
	return 1;
}

/* diffNum:
   1) what:	label for numbers being compared
   2-3) numa, numb:	numbers to compare
   Returns true if texta and textb differ, otherwise false.
*/
static void diffNum( const char *what, int numa, int numb )
{
	if ( numa == numb ) return;
	
	printags();
	printf( "Different %s: %d <> %d\n", what, numa, numb );
	++diffcount;
}

/* diffElems: Compare elements' members, and recursively its subelements
   1) ela: first XmElem
   2) elb: second XmElem
*/
void diffElems( XmElem *ela, XmElem *elb )
{
	pushtag( ela->tag ? ela->tag : "UNK" );		// record tag for tracing diff location
	
	diffText( "tags", ela->tag, elb->tag );
	
	char *isba, *isbb;
	isba = ela->isBlank ? "T" : "F";
	isbb = elb->isBlank ? "T" : "F";
	
	diffText( "isBlank", isba, isbb );
	
	// only compare texts if not blank
	if ( ! (ela->isBlank && elb->isBlank) )
		diffText( "text", ela->text, elb->text );

	diffNum( "nattribs", ela->nattribs, elb->nattribs );
	
	int minattribs = ela->nattribs < elb->nattribs ? ela->nattribs : elb->nattribs;
	
	for ( int i=0; i<minattribs; i++ ) {
		if ( ! diffText( "attrib", (*ela->attrib)[i][0], (*elb->attrib)[i][0] ) ) {
			diffText( "attrib value", (*ela->attrib)[i][1], (*elb->attrib)[i][1] );
		}
	}
	
	diffNum( "nsubs", ela->nsubs, elb->nsubs );
	
	// descend to next level only if both quantities of subelements are the same
	if ( ela->nsubs == elb->nsubs ) {
		for ( int i=0; i<ela->nsubs; i++ ) {
			diffElems( (*ela->subelem)[i], (*elb->subelem)[i] );
		}
	}

	poptag();
	return;
}

int main( int argc, char *argv[] )
{
	if ( argc != 3 ) {
		printf( "Usage:  mxdiff a.xml b.xml\n" );
		return EXIT_FAILURE;
	}
	
	FILE *filea = fopen( argv[1], "r" );
	if ( !filea ) {
		printf( "Can't open %s\n", argv[1] );
		return EXIT_FAILURE;
	}
		
	FILE *fileb = fopen( argv[2], "r" );
	if ( !fileb ) {
		printf( "Can't open %s\n", argv[2] );
		return EXIT_FAILURE;
	}

	XmElem *mxa, *mxb;

	xmlSchemaPtr sp = mxInit( XSDFILE );
	if ( ! sp ) {
		printf( "Can't parse schema %s\n", XSDFILE );
		return EXIT_FAILURE;
	}

	if ( mxReadFile( filea, sp, &mxa ) ) {
		printf( "Can't read %s as MARCXML\n", argv[1] );
		return EXIT_FAILURE;
	}
	if ( mxReadFile( fileb, sp, &mxb ) ) {
		printf( "Can't read %s as MARCXML\n", argv[2] );
		return EXIT_FAILURE;
	}

	fclose( filea );
	fclose( fileb );
	
	diffElems( mxa, mxb );
	if ( diffcount )
  		printf( "\nMARCXML files differ in %d place%s!\n", diffcount, (diffcount>1 ? "s": "") );
	else
		printf( "\nMARCXML files are the same!\n");
	
	mxCleanElem( mxa );
	mxCleanElem( mxb );
	mxTerm( sp );
	
	if ( diffcount ) return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
