#include "mxutil.h"

#include <stdio.h>

void main()
{
    const char *path = "teste.xsd";
    xmlSchemaPtr schema = mxInit("teste.xsd");

    FILE *testfile = fopen("trellis.xml", "r");

    XmElem *top;
    int mf = mxReadFile(testfile, schema, &top);
    if(mf == 0)
        printElement(top);

    fclose(testfile);

    XmElem *rec = (*top->subelem)[0];
    // rec->tag should be "record"; other members:
    // strlen(text)=123, isBlank=true, nattribs=0, attrib=NULL, nsubs=24

    int nfields = mxFindField( rec, 3 );
    // nfields should be 1

    const char *data = mxGetData( rec, 3, 1, 'x', 0 );
    // 'x' and 0 are ignored; data should be "DLC"

    data = mxGetData( rec, 3, 0, 'x', 0 );
    // tnum is out of range; data should be NULL

    nfields = mxFindField( rec, 650 );
    // nfields should be 5

    int nsf = mxFindSubfield( rec, 650, 1, 'x' );
    // nsf should be 1

    data = mxGetData( rec, 650, 1, 'x', 1 );
    // data should be "Juvenile poetry."

    nsf = mxFindSubfield( rec, 50, 1, 'c' ) ;
    // nsf should be 0
    mxCleanElem(top);
    mxTerm(schema);

}
