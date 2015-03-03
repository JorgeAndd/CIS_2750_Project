#include "mxutil.h"

//copyString declaration
void copyString(char *source, char **destination);

xmlSchemaPtr mxInit(const char *xsdfile)
{
    xmlSchemaParserCtxtPtr schemaContext;
    xmlSchemaPtr schema;

    LIBXML_TEST_VERSION;
    xmlLineNumbersDefault(1);

    //Create an XML Schemas parse context for that file/resource expected to contain an XML Schemas file.
    schemaContext = xmlSchemaNewParserCtxt(xsdfile);

    //parse a schema definition resource and build an internal XML Shema struture which can be used to validate instances.
    schema = xmlSchemaParse(schemaContext);

    //Free the resources associated to the schema parser context
    xmlSchemaFreeParserCtxt(schemaContext);

    return schema;
}

//Parse XML file
//returns: 0 success, 1 parse error, 2 schema not match
int mxReadFile(FILE *marcxmlfp, xmlSchemaPtr sp, XmElem **top)
{
    //parse an XML from a file descriptor and build a tree
    xmlDocPtr doc = xmlReadFd(fileno(marcxmlfp), "", NULL, 0);
    if(doc == NULL)
        return 1;

    //Create an XML Schemas validation context based on the given schema.
    xmlSchemaValidCtxtPtr validationContext = xmlSchemaNewValidCtxt(sp);
    if(validationContext == NULL)
        return 1;

    //Validate the document based on the provided schema
    int valid = xmlSchemaValidateDoc(validationContext, doc);

    //Check if the document is valid and return function when needed
    if(valid > 0)
        return 2;
    else if(valid == 1)
        return 1;

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if(root == NULL)
        return 1;

    //Build XmElem tree
    *top = mxMakeElem(doc, root);
    if(*top == NULL)
        return 1;

    //Frees DOM document resources
    xmlSchemaFreeValidCtxt(validationContext);
    xmlFreeDoc(doc);

    return 0;
}

//Build XmElem tree recursively
//Returns the root node of the tree
XmElem *mxMakeElem(xmlDocPtr doc, xmlNodePtr node )
{
    //Check if node is a comment
    while(node->type == XML_COMMENT_NODE)
    {
        //Go to the next node
        node = node->next;
    }
    if(node == NULL)
        return NULL;
    else
    {
        XmElem *newNode = malloc(sizeof(XmElem));

        //Copy xml tag to node name
        copyString((char*)node->name, &(newNode->tag));

        //Get note text
        xmlChar *nodeText = xmlNodeListGetString(doc, node->children, 1);

        //Check if the text is just whitespace(or tab or newline)
        int justWhite = 1;
        char *c = (char*)nodeText;

        while(*c)
        {
            if(*c != '\n' && *c != '\t' && *c != ' ')
            {
                justWhite = 0;
                break;
            }

            c++;
        }

        if(xmlFirstElementChild(node) == NULL)
            newNode->text = NULL;
        else
            newNode->text = (char*)nodeText;

        if(justWhite== 1)
            newNode->isBlank = 1;
        else
            newNode->isBlank = 0;

        //Process node attributes
        int attrCounter = 0;
        xmlAttr *attrIt = node->properties;

        //Count number of attribute
        while(attrIt != NULL)
        {
                attrCounter++;
                attrIt = (attrIt->next);
        }

        newNode->nattribs = attrCounter;
        if(attrCounter == 0)
            newNode->attrib = NULL;
        else
        {
            //Allocates attributes array memory
            newNode->attrib = malloc(sizeof(char*) * 2 * attrCounter);

            xmlAttrPtr attrIt = node->properties;

            //Iterates over attributes, copying name and text to newNode
            for(int i = 0; i < attrCounter; i++)
            {
                copyString((char*)(attrIt->name), &(*(newNode->attrib))[i][0]);
                copyString((char*)(attrIt->children->content), &((*(newNode->attrib))[i][1]));

                attrIt = attrIt->next;
            }
        }

        //Process node subelements
        int nchild = (newNode->nsubs = xmlChildElementCount(node));
        if(nchild == 0)
        {
            newNode->subelem = NULL;
            return newNode;
        }
        else
        {
            //Allocates subelements array memory
            newNode->subelem = malloc(sizeof(XmElem*) * nchild);

            xmlNodePtr curChild = xmlFirstElementChild(node);

            //Iterates over sublelements of the node
            int childIt = 0;
            do
            {
                XmElem *auxElem = mxMakeElem(doc, curChild);
                (*(newNode->subelem))[childIt] = auxElem;

                curChild = xmlNextElementSibling(curChild);
                childIt++;
            }while(curChild != NULL);
            return newNode;
        }

    }
}

void mxCleanElem(XmElem *top)
{
    //Check every XmElem sub to free
    int val = top->nsubs;
    while(val > 0)
    {
        mxCleanElem((*top->subelem)[--val]);
    }

    //When elelement doesn't have any more sub-elements, free others fields
    free(top->subelem);

    //Free tag
    free(top->tag);

    //Free text
    free(top->text);

    //Free attributes
    val = top->nattribs;
    while(val > 0)
    {
        free((*top->attrib)[--val][0]);
        free((*top->attrib)[val][1]);
    }
    free(top->attrib);

    free(top);
    return;
}

void mxTerm(xmlSchemaPtr sp)
{
    xmlSchemaFree(sp);

    xmlSchemaCleanupTypes();

    xmlCleanupParser();
}

//Copy a string(char*) from source to destination, allocating the corresponding memory
void copyString(char *source, char **destination)
{
    int textSize = 0;

    //Check string size
    while(*(source+textSize) != '\0')
        textSize++;

    //Allocate memory
    *destination = malloc(sizeof(char) * (textSize+1));

    strcpy(*destination, source);

}

int mxFindField(const XmElem *mrecp, int tag)
{
    int nElem = mrecp->nsubs;
    int count = 0;

    for(int i = 0; i < nElem; i++)
    {
        XmElem *subElem = (*(mrecp->subelem))[i];

        if(subElem->nattribs > 0)
        {
            char *value = (*(subElem->attrib))[0][1] ;
            int charValue;

            sscanf(value, "%d", &charValue);
            if(charValue == tag)
                count++;
        }
    }

    return count;
}

int mxFindSubfield( const XmElem *mrecp, int tag, int tnum, char sub )
{
    int nElem = mrecp->nsubs;
    int count = 0;
    XmElem *subElem;

    //Get the nth field with the tag, specified in tnum

    for(int i = 0; i < nElem; i++)
    {
        subElem = (*(mrecp->subelem))[i];

        if(subElem->nattribs > 0)
        {
            char *value = (*(subElem->attrib))[0][1] ;
            int charValue;

            charValue = atoi(value);
            if(charValue == tag)
            {
                count++;
                if(count == tnum)
                    break;
            }
        }else
            return 0;
    }

    //Get the nth subfield with tag, specified in snum

    if(tnum > count || count == 0)
        return 0;
    else
    {
        nElem = subElem->nsubs;
        int subFieldCount = 0;

        for(int i = 0; i < nElem; i++)
        {
            XmElem *subField = (*(subElem->subelem))[i];
            char *code = ((*(subField->attrib))[0][1]);

            if(code[0] == sub)
                subFieldCount++;
        }

        return subFieldCount;
    }

}

const char *mxGetData( const XmElem *mrecp, int tag, int tnum, char sub, int snum )
{
    int nElem = mrecp->nsubs;
    int count = 0;
    XmElem *subElem;

    //Get the nth field with the tag, specified in tnum

    for(int i = 0; i < nElem; i++)
    {
        subElem = (*(mrecp->subelem))[i];

        if(subElem->nattribs > 0)
        {
            char *value = (*(subElem->attrib))[0][1] ;
            int charValue;

            charValue = atoi(value);
            if(charValue == tag)
            {
                count++;
                if(count == tnum)
                    break;
            }
        }else
            return NULL;
    }

    //Get the nth subfield with tag, specified in snum

    if(tnum > count || count == 0)
        return NULL;
    else
    {
        //Special case where tag is between 000 and 009
        if(tag >= 0 && tag <= 9)
            return subElem->text;

        nElem = subElem->nsubs;
        int subFieldCount = 0;

        for(int i = 0; i < nElem; i++)
        {
            XmElem *subField = (*(subElem->subelem))[i];

            //Get the first element from *char to convert to char
            char code = ((*(subField->attrib))[0][1])[0];

            if(code == sub)
            {
                subFieldCount++;
                if(subFieldCount == snum)
                    return subField->text;
            }
        }
    }
    return NULL;
}

