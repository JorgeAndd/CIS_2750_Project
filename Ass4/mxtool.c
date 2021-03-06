/*****
mxtool.c -- source file
Last update:  1-Nov-2014, Version 2

Jorge Luiz Andrade
#0906139
*****/

#include "mxtool.h"
#include <errno.h>

/*
    Struct that holds both a pointer to an XmElem and to a char
    Used on sortRecs
*/
typedef struct
{
    unsigned long index;
    char *key;
} Combined;

/*  Concatenate up to four strings(char*)
    Allocate the necessary memory

    Arguments:
    part1, part2, part3, part4: strings to be concatenated. Pass "" if some string is null or empty

    Return value:
    pointer(char*) to the final string
*/
char* concatStrings(const char *part1, const char *part2,
                   const char *part3, const char *part4);

/*  Remove additional spaces and periods at end of a string

    Arguments:
    string: address of string to be trimmed
*/
void trimString(char *string);


/* Compares the keys of two Combined elements

    Arguments:
    elem1, elem2: elements to be compared

    Return value:
    -1: elem1.key < elem2.key
     0: elem1.key == elem2.key
     1: elem1.key > elem2.key
*/
int compCombined(const void *elem1, const void *elem2);

int libFormat(const XmElem *top, FILE *outfile)
{
    BibData *bdata;

    unsigned long nElem = top->nsubs;

    bdata = malloc(sizeof(BibData) * nElem);
    assert(bdata);

    for(unsigned long i = 0; i < nElem; i++)
        marc2bib((*(top->subelem))[i], bdata[i]);

    char **keys = malloc(sizeof(char*) * nElem);
    assert(keys);

    for(unsigned long i = 0; i < nElem; i++)
        keys[i] = bdata[i][3];
	
    sortRecs((XmElem*) top, (const char**)keys);

    for(unsigned long i = 0; i < nElem; i++)
        free(keys[i]);

    for(unsigned long i = 0; i < nElem; i++)
        marc2bib((*(top->subelem))[i], bdata[i]);

    for(unsigned long i = 0; i < nElem; i++)
    {
        fprintf(outfile, "%s. %s %s. %s\n\n",
                bdata[i][3], bdata[i][0], bdata[i][1], bdata[i][2]);

        free(bdata[i][0]);
        free(bdata[i][1]);
        free(bdata[i][2]);
        free(bdata[i][3]);
    }

    return EXIT_SUCCESS;
}

int bibFormat(const XmElem *top, FILE *outfile)
{
    BibData *bdata;

    unsigned long nElem = top->nsubs;

    bdata = malloc(sizeof(BibData) * nElem);
    assert(bdata);

    for(unsigned long i = 0; i < nElem; i++)
        marc2bib((*(top->subelem))[i], bdata[i]);

    char *(*keys) = malloc(sizeof(char*) * nElem);
    assert(keys);

    for(unsigned long i = 0; i < nElem; i++)
        keys[i] = bdata[i][0];

    sortRecs((XmElem*) top, (const char**)keys);

    for(unsigned long i = 0; i < nElem; i++)
        marc2bib((*(top->subelem))[i], bdata[i]);

    for(unsigned long i = 0; i < nElem; i++)
    {
        trimString(bdata[i][0]);
        trimString(bdata[i][1]);
        trimString(bdata[i][2]);

        fprintf(outfile, "%s. %s. %s.\n\n",
                bdata[i][0], bdata[i][1], bdata[i][2]);

        free(bdata[i][0]);
        free(bdata[i][1]);
        free(bdata[i][2]);
        free(bdata[i][3]);
    }
    return EXIT_SUCCESS;
}

int concat(const XmElem *top1, const XmElem *top2, FILE *outfile)
{	
    XmElem *concatElem = malloc(sizeof(*top1));
    assert(concatElem);
    memcpy(concatElem, top1, sizeof(*top1));

    unsigned long extraElem = top2->nsubs;
    unsigned long lastElem = top1->nsubs;

    concatElem->nsubs += extraElem;

    concatElem->subelem = realloc(concatElem->subelem,
                                  sizeof(XmElem*)*(concatElem->nsubs));
    assert(concatElem->subelem);

    for(int i = 0; i < extraElem; i++)
    {
        XmElem *copy = (*(top2->subelem))[i];
        (*(concatElem->subelem))[lastElem+i] = copy;
    }
    
    int result = mxWriteFile(concatElem, outfile);

    if(result == concatElem->nsubs)
    {
        free(concatElem);
        return EXIT_SUCCESS;
    }
    else
    {
        free(concatElem);
        return EXIT_FAILURE;
    }
}

int selects(const XmElem *top, const enum SELECTOR sel,
            const char *pattern, FILE *outfile)
{
    if((pattern[0] != 'a' && pattern[0] != 't' &&
       pattern[0] != 'p') || pattern[1] != '=')
       {
            fprintf(stderr, "\"%s\" is not a valid pattern", pattern);
            return EXIT_FAILURE;
       }

    int field;
    switch(pattern[0])
    {
    case 'a':
        field = 0;
        break;
    case 't':
        field = 1;
        break;
    case 'p':
        field = 2;
        break;
    }

    pattern += 2;

    //Print headers
    fprintf(outfile, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(outfile, "<!-- Output by mxutil library"
                    " Jorge Luiz Andrade #0906139 -->\n");

    fprintf(outfile, "<marc:collection xmlns:"
        "marc=\"http://www.loc.gov/MARC21/slim\" "
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
        "xsi:schemaLocation=\"http://www.loc.gov/MARC21/slim\n"
        "http://www.loc.gov/standards/marcxml/schema/MARC21slim.xsd\">\n");

    unsigned long nElem = top->nsubs;
    BibData bdata;

    if(sel == KEEP)
        for(unsigned long i = 0; i < nElem; i++)
        {
            marc2bib((*(top->subelem))[i], bdata);

            int result = match(bdata[field], pattern);

            free(bdata[AUTHOR]);
            free(bdata[TITLE]);
            free(bdata[PUBINFO]);
            free(bdata[CALLNUM]);

            if(result == 1)
            {
                result = mxWriteFile((*(top->subelem))[i], outfile);
                if(result == -1)
                    return EXIT_FAILURE;
                fprintf(outfile, "\n");
            }
        }
    else
        for(unsigned long i = 0; i < nElem; i++)
        {
            marc2bib((*(top->subelem))[i], bdata);

            int result = match(bdata[field], pattern);

            free(bdata[AUTHOR]);
            free(bdata[TITLE]);
            free(bdata[PUBINFO]);
            free(bdata[CALLNUM]);

            if(result == 0)
            {
                result = mxWriteFile((*(top->subelem))[i], outfile);
                if(result == -1)
                    return EXIT_FAILURE;
                fprintf(outfile, "\n");
            }
        }

    fprintf(outfile, "</marc:collection>");

    return EXIT_SUCCESS;
}

int review(const XmElem *top, FILE *outfile)
{
    FILE *consoleOut = fopen("/dev/tty", "w");
    FILE *consoleIn = fopen("/dev/tty", "r");

    int input = 0;
    unsigned long nSubs = top->nsubs;

    //Print headers
    fprintf(outfile, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(outfile, "<!-- Output by mxutil library"
                    " Jorge Luiz Andrade #0906139 -->\n");

    fprintf(outfile, "<marc:collection xmlns:"
        "marc=\"http://www.loc.gov/MARC21/slim\" "
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
        "xsi:schemaLocation=\"http://www.loc.gov/MARC21/slim\n"
        "http://www.loc.gov/standards/marcxml/schema/MARC21slim.xsd\">\n");

    for(unsigned long i = 0; i < nSubs; i++)
    {
        BibData bibdata;

        marc2bib((*(top->subelem))[i], bibdata);
        int result;

        if(input == 'k')
        {
            result = mxWriteFile((*(top->subelem))[i], outfile);
            if(result == -1)
                return EXIT_FAILURE;
            fprintf(outfile, "\n");
            continue;
        }

        do{
            fprintf(consoleOut, "%lu. %s. %s. %s\n",
                i+1, bibdata[AUTHOR], bibdata[TITLE], bibdata[PUBINFO]);

            input = fgetc(consoleIn);
            switch(input)
            {
            case 10:
                result = mxWriteFile((*(top->subelem))[i], outfile);
                if(result == -1)
                    return EXIT_FAILURE;
                fprintf(outfile, "\n");
                break;
            case 32:
                fgetc(consoleIn);
                break;
            case 'k':
                fgetc(consoleIn);
                result = mxWriteFile((*(top->subelem))[i], outfile);
                if(result == -1)
                    return EXIT_FAILURE;
                fprintf(outfile, "\n");
                break;
            case 'd':
                fgetc(consoleIn);
                result = mxWriteFile((*(top->subelem))[i], outfile);
                if(result == -1)
                    return EXIT_FAILURE;
                fprintf(outfile, "\n");
                fprintf(outfile, "</marc:collection>");
                return EXIT_SUCCESS;
            default:
                fgetc(consoleIn);
                fprintf(consoleOut, "Enter: record is copied to stdout.\n"
                                "Spacebar: record is skipped.\n"
                                "'k' (keep): remaining records are copied"
                                "and the program exits.\n"
                                "'d' (discard): remaining records are skipped"
                                " and the program exits.\n");
                break;
            }

        }while(input != 10 && input != 32 && input != 'k' && input != 'd');

    }
    fprintf(outfile, "</marc:collection>");
    return EXIT_SUCCESS;
}

int match(const char *data, const char *regex)
{
    regex_t regexComp;
    int compileResult;

    compileResult = regcomp(&regexComp, regex, 0);
    if(compileResult != 0)
        return 0;

    int matchResult = regexec(&regexComp, data, 0, NULL, 0);
    regfree(&regexComp);
    if(matchResult == 0)
        return 1;
    else
        return 0;
}

void marc2bib(const XmElem *mrec, BibData bdata)
{
    unsigned stringSize;

    int fieldCount, subfieldCount;

    //Author
    const char *author;

    //Check if record has an author
    fieldCount = mxFindField(mrec, 100);

    if(fieldCount != 0)
    {
        //Author on field 100

        subfieldCount = mxFindSubfield(mrec, 100, 1, 'a');

        if(subfieldCount != 0)
            author = mxGetData(mrec, 100, 1, 'a', 1);

    }else //Have not found field 100
    {
        fieldCount = mxFindField(mrec, 130);

        if(fieldCount != 0)
        {
            //Author on field 130

            subfieldCount = mxFindSubfield(mrec, 130, 1, 'a');

            if(subfieldCount != 0)
                author = mxGetData(mrec, 130, 1, 'a', 1);
        }
    }

    //Check if have found an author(field and subfield)
    if(fieldCount == 0)
    {
        bdata[AUTHOR] = malloc(sizeof("na"));
        assert(bdata[AUTHOR]);
        strcpy(bdata[AUTHOR], "na");
    }
    else if(subfieldCount == 0)
    {
        bdata[AUTHOR] = malloc(sizeof(""));
        assert(bdata[AUTHOR]);
        strcpy(bdata[AUTHOR], "");
    }else
    {
        stringSize = strlen(author);

        bdata[AUTHOR] = malloc(stringSize + 1);
        assert(bdata[AUTHOR]);
        strcpy(bdata[AUTHOR], author);
    }

    //Title
    //Check if record has an title
    fieldCount = mxFindField(mrec, 245);

    if(fieldCount != 0)
    {
        const char *part1, *part2, *part3;

        //Get each subfield(a, p and b) and assign to one char*
        //If the subfield doensn't exist,
        //assign "" to the corresponding pointer

        subfieldCount = mxFindSubfield(mrec, 245, 1, 'a');
        if(subfieldCount != 0)
            part1 = mxGetData(mrec, 245, 1, 'a', 1);
        else
            part1 = "";

        subfieldCount = mxFindSubfield(mrec, 245, 1, 'p');
        if(subfieldCount != 0)
            part2 = mxGetData(mrec, 245, 1, 'p', 1);
        else
            part2 = "";

        subfieldCount = mxFindSubfield(mrec, 245, 1, 'b');
        if(subfieldCount != 0)
            part3 = mxGetData(mrec, 245, 1, 'b', 1);
        else
            part3 = "";

        //Allocate bdata[TITLE] memory and copy data obtained before
        bdata[TITLE] = concatStrings(part1, part2, part3, "");
    }else
    {
        bdata[TITLE] = malloc(sizeof("na"));
        assert(bdata[TITLE]);
        strcpy(bdata[TITLE], "na");
    }

    //Publication info
    fieldCount = mxFindField(mrec, 260);

    if(fieldCount != 0)
    {
        const char *part1, *part2, *part3, *part4;

        subfieldCount = mxFindSubfield(mrec, 260, 1, 'a');
        if(subfieldCount != 0)
            part1 = mxGetData(mrec, 260, 1, 'a', 1);
        else
            part1 = "";

        subfieldCount = mxFindSubfield(mrec, 260, 1, 'b');
        if(subfieldCount != 0)
            part2 = mxGetData(mrec, 260, 1, 'b', 1);
        else
            part2 = "";

        subfieldCount = mxFindSubfield(mrec, 260, 1, 'c');
        if(subfieldCount != 0)
            part3 = mxGetData(mrec, 260, 1, 'c', 1);
        else
            part3 = "";

        fieldCount = mxFindField(mrec, 250);
        if(fieldCount != 0)
        {
            subfieldCount = mxFindSubfield(mrec, 250, 1, 'a');
            if(subfieldCount != 0)
                part4 = mxGetData(mrec, 250, 1, 'a', 1);
            else
                part4 = "";
        }else
            part4 = "";

        bdata[PUBINFO] = concatStrings(part1, part2, part3, part4);
    }else
    {
        fieldCount = mxFindField(mrec, 250);
        if(fieldCount != 0)
        {
            subfieldCount = mxFindSubfield(mrec, 250, 1, 'a');
            if(subfieldCount != 0)
            {
            	const char *pubinfo = mxGetData(mrec, 250, 1, 'a', 1);
                bdata[PUBINFO] = malloc(sizeof(*pubinfo));
				assert(bdata[PUBINFO]);
				strcpy(bdata[PUBINFO], pubinfo);
            }
            else
            {
                bdata[PUBINFO] = malloc(sizeof("na"));
                assert(bdata[PUBINFO]);
                strcpy(bdata[PUBINFO], "na");
            }
        }else
        {
            bdata[PUBINFO] = malloc(sizeof("na"));
            assert(bdata[PUBINFO]);
            strcpy(bdata[PUBINFO], "na");
        }
    }

    //Call number
    fieldCount = mxFindField(mrec, 90);

    if(fieldCount != 0)
    {
        const char *part1, *part2;

        subfieldCount = mxFindSubfield(mrec, 90, 1, 'a');
        if(subfieldCount != 0)
            part1 = mxGetData(mrec, 90, 1, 'a', 1);
        else
            part1 = "";

        subfieldCount = mxFindSubfield(mrec, 90, 1, 'b');
        if(subfieldCount != 0)
            part2 = mxGetData(mrec, 90, 1, 'b', 1);
        else
            part2 = "";

        bdata[CALLNUM] = concatStrings(part1, part2, "", "");
    }else
    {
        fieldCount = mxFindField(mrec, 50);

        if(fieldCount != 0)
        {
            const char *part1, *part2;

            subfieldCount = mxFindSubfield(mrec, 50, 1, 'a');
            if(subfieldCount != 0)
                part1 = mxGetData(mrec, 50, 1, 'a', 1);
            else
                part1 = "";

            subfieldCount = mxFindSubfield(mrec, 50, 1, 'b');
            if(subfieldCount != 0)
                part2 = mxGetData(mrec, 50, 1, 'b', 1);
            else
                part2 = "";

            bdata[CALLNUM] = concatStrings(part1, part2, "", "");
        }else
        {
            bdata[CALLNUM] = malloc(sizeof("na"));
            assert(bdata[CALLNUM]);
            strcpy(bdata[CALLNUM], "na");
        }
    }
}

void sortRecs(XmElem *collection, const char *keys[])
{
    unsigned long nElem = collection->nsubs;
    //Special cases, list is already ordered
    if(nElem <= 1)
        return;

    Combined *combined = malloc(sizeof(Combined) * nElem);
    assert(combined);

    char **copyKeys = malloc(sizeof(*keys) * nElem);
    assert(copyKeys);
    memcpy(copyKeys, keys, sizeof(*keys) * nElem);

    for(unsigned long i = 0; i < nElem; i++)
    {
        combined[i].index = i;
        combined[i].key = copyKeys[i];
    }
    free(copyKeys);

    qsort(combined, nElem, sizeof(Combined), compCombined);

    //Reorder collection array
    XmElem *aux;
    unsigned long index;
    for(unsigned long i = 0; i < nElem - 1; i++)
    {
        index = combined[i].index;
        aux = (*(collection->subelem))[i];
        (*(collection->subelem))[i] = (*(collection->subelem))[index];
        (*(collection->subelem))[index] = aux;
    }
    free(combined);

}

int compCombined(const void *elem1, const void *elem2)
{
    char *key1 = (*((Combined*)elem1)).key;
    char *key2 = (*((Combined*)elem2)).key;

    while(key1 != '\0' && key2 != '\0')
    {
        int diff = tolower(*key1) - tolower(*key2);
        if(diff != 0)
            return diff;

        key1++;
        key2++;
    }

    if(key1 == '\0')
    {
        if(key2 == '\0')
            return 0;
        else
            return 1;
    }
    else
        return -1;

}

char* concatStrings(const char *part1, const char *part2,
                   const char *part3, const char *part4)
{
    unsigned stringSize = strlen(part1) + strlen(part2) +
                            strlen(part3) + strlen(part4);
    char *resultString;

    resultString = malloc(stringSize + 1);
    assert(resultString);

    strcpy(resultString, part1);
    strcat(resultString, part2);
    strcat(resultString, part3);
    strcat(resultString, part4);

    return resultString;
}

void trimString(char *string)
{
    int last = strlen(string) - 1;

    while((string)[last] == '.' || (string)[last] == ' ')
        last--;

    (string)[last + 1] = '\0';
}

int main(int argc, char *argv[])
{
    char *schemaPath = getenv("MXTOOL_XSD");

    //FILE *consoleOut = fopen("/dev/tty", "w");
    //FILE *consoleIn = fopen("/dev/tty", "r");

    if(argc == 2 && strcmp(argv[1], "-review") == 0)
    {
        xmlSchemaPtr schema = mxInit(schemaPath);

        XmElem *top;
        mxReadFile(stdin, schema, &top);

        review(top, stdout);
        int result = review(top, stdout);

        mxCleanElem(top);
        mxTerm(schema);

        return result;
    }else if(argc > 1 && strcmp(argv[1], "-cat") == 0)
    {
        if(argc != 3)
        {
            fprintf(stderr, "Invalid command. Use:\n"
                "  mxtool -review\n"
                "  mxtool -cat xmlfile\n"
                "  mxtool -keep pattern\n"
                "  mxtool -discard pattern\n"
                "  mxtool -lib\n"
                "  mxtool -bib\n");

            return EXIT_FAILURE;
        }

        FILE *input1 = fopen(argv[2], "r");

        if(input1 == NULL)
            return EXIT_FAILURE;

        xmlSchemaPtr schema = mxInit(schemaPath);

        XmElem *top1;
        mxReadFile(input1, schema, &top1);

        XmElem *top2;
        mxReadFile(stdin, schema, &top2);

        fclose(input1);

        int result = concat(top1, top2, stdout);
        mxCleanElem(top1);
        mxCleanElem(top2);
        mxTerm(schema);

        return result;
    }else if(argc > 1 && strcmp(argv[1], "-keep") == 0)
    {
        if(argc != 3)
        {
            fprintf(stderr, "Invalid command. Use:\n"
                "  mxtool -review\n"
                "  mxtool -cat xmlfile\n"
                "  mxtool -keep pattern\n"
                "  mxtool -discard pattern\n"
                "  mxtool -lib\n"
                "  mxtool -bib\n");

            return EXIT_FAILURE;
        }

        xmlSchemaPtr schema = mxInit(schemaPath);

        XmElem *top;
        mxReadFile(stdin, schema, &top);

        int result = selects(top, KEEP, argv[2], stdout);

        mxCleanElem(top);
        mxTerm(schema);

        return result;
    }else if(argc > 1 && strcmp(argv[1], "-discard") == 0)
    {
        if(argc != 3)
        {
            fprintf(stderr, "Invalid command. Use:\n"
                "  mxtool -review\n"
                "  mxtool -cat xmlfile\n"
                "  mxtool -keep pattern\n"
                "  mxtool -discard pattern\n"
                "  mxtool -lib\n"
                "  mxtool -bib\n");

            return EXIT_FAILURE;
        }

        xmlSchemaPtr schema = mxInit(schemaPath);

        XmElem *top;
        mxReadFile(stdin, schema, &top);

        int result = selects(top, DISCARD, argv[2], stdout);

        mxCleanElem(top);
        mxTerm(schema);

        return result;
    }else if(argc > 1 && strcmp(argv[1], "-lib") == 0)
    {
        xmlSchemaPtr schema = mxInit(schemaPath);

        XmElem *top;
        mxReadFile(stdin, schema, &top);

        int result = libFormat(top, stdout);

        mxCleanElem(top);
        mxTerm(schema);

        return result;
    }else if(argc > 1 && strcmp(argv[1], "-bib") == 0)
    {
        xmlSchemaPtr schema = mxInit(schemaPath);

        XmElem *top;
        mxReadFile(stdin, schema, &top);

        int result = bibFormat(top, stdout);

        mxCleanElem(top);
        mxTerm(schema);

        return result;
    }else
    {
        fprintf(stderr, "Invalid command. Use:\n"
                "  mxtool -review\n"
                "  mxtool -cat xmlfile\n"
                "  mxtool -keep pattern\n"
                "  mxtool -discard pattern\n"
                "  mxtool -lib\n"
                "  mxtool -bib\n");

        return EXIT_FAILURE;
    }

    return EXIT_FAILURE;
}
