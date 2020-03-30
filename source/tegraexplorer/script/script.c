#include <string.h>
#include "../../mem/heap.h"
#include "../gfx/gfxutils.h"
#include "../emmc/emmc.h"
#include "../../utils/types.h"
#include "../../libs/fatfs/ff.h"
#include "../../utils/sprintf.h"
#include "../../utils/btn.h"
#include "../../gfx/gfx.h"
#include "../../utils/util.h"
#include "../../storage/emummc.h"
#include "script.h"
#include "../common/common.h"
#include "../fs/fsactions.h"


u32 countchars(char* in, char target) {
    u32 len = strlen(in);
    u32 count = 0;
    for (u32 i = 0; i < len; i++) {
        if (in[i] == target)
            count++;
    }
    return count;
}

char **argv = NULL;
u32 argc;
u32 splitargs(char* in) {
    // arg like '5, "6", @arg7'
    u32 i, current = 0, count = countchars(in, ',') + 1, len = strlen(in), curcount = 0;

    if (argv != NULL) {
        for (i = 0; argv[i] != NULL; i++)
            free(argv[i]);
        free(argv);
    }
        
    argv = calloc(count + 1, sizeof(char*));

    for (i = 0; i < count; i++)
        argv[i] = calloc(128, sizeof(char));
    

    for (i = 0; i < len && curcount < count; i++) {
        if (in[i] == ',') {
            curcount++;
            current = 0;
        }
        else if (in[i] == '@' || in[i] == '$') {
            while (in[i] != ',' && in[i] != ' ' && in[i] != ')' && i < len) {
                argv[curcount][current++] = in[i++];
            }
            i--;
        }
        else if (in[i] >= '0' && in[i] <= '9')
            argv[curcount][current++] = in[i];
        else if (in[i] == '"') {
            i++;
            while (in[i] != '"') {
                argv[curcount][current++] = in[i++];
            }
        }
    }
    return count;
}

FIL scriptin;
UINT endByte = 0;
int forceExit = false;
char currentchar = 0;

char getnextchar(){
    f_read(&scriptin, &currentchar, sizeof(char), &endByte);
    
    if (sizeof(char) != endByte)
        forceExit = true;

    //gfx_printf("|%c|", currentchar);
    return currentchar;
}

void getfollowingchar(char end){
    while (currentchar != end){
        if (currentchar == '"'){
            while (getnextchar() != '"');
        }
        getnextchar();
    }
}

void getnextvalidchar(){
    while ((!((currentchar >= '0' && currentchar <= 'Z') || (currentchar >= 'a' && currentchar <= 'z') || currentchar == '#' || currentchar == '@') && !f_eof(&scriptin)) || currentchar == ';')
        getnextchar();
}

void functionparser(){
    char *unsplitargs;
    FSIZE_t fileoffset;
    u32 argsize = 0;

    //gfx_printf("getting func %c\n", currentchar);
    char *funcbuff = calloc(20, sizeof(char));
    for (int i = 0; i < 19 && currentchar != '(' && currentchar != ' '; i++){
        funcbuff[i] = currentchar;
        getnextchar();
    }
    getfollowingchar('(');
    getnextchar();

    //gfx_printf("getting arg len %c\n", currentchar);

    fileoffset = f_tell(&scriptin);
    getfollowingchar(')');
    argsize = f_tell(&scriptin) - fileoffset;
    f_lseek(&scriptin, fileoffset - 1);

    getnextchar();

    //gfx_printf("getting args %c\n", currentchar);

    unsplitargs = calloc(argsize + 1, sizeof(char));
    for (int i = 0; i < argsize; i++){
        unsplitargs[i] = currentchar;
        getnextchar();
    }

    //gfx_printf("parsing args %c\n", currentchar);
    argc = splitargs(unsplitargs);
    getnextchar();

    gfx_printf("\n\nFunc: %s\n", funcbuff, currentchar);
    gfx_printf("Split: %s\n", unsplitargs);

    for (int i = 0; i < argc; i++)
        gfx_printf("%d | %s\n", i, argv[i]);

    free(unsplitargs);
    free(funcbuff); 
}

char *gettargetvar(){
    char *variable = NULL;
    FSIZE_t fileoffset;
    u32 varsize = 0;

    fileoffset = f_tell(&scriptin);
    getfollowingchar('=');
    varsize = f_tell(&scriptin) - fileoffset;
    f_lseek(&scriptin, fileoffset - 1);

    variable = calloc(varsize + 1, sizeof(char));

    getnextchar();
    for (int i = 0; i < varsize; i++){
        if (currentchar == ' ')
            break;

        variable[i] = currentchar;
        getnextchar();
    }

    getfollowingchar('=');
    getnextchar();

    return variable;
}

void mainparser(){
    char *variable = NULL;
    FSIZE_t fileoffset;
    u32 varsize = 0;

    getnextvalidchar();

    if (f_eof(&scriptin))
        return;

    if (currentchar == '#'){
        getfollowingchar('\n');
        return;
    }

    if (currentchar == '@'){
        variable = gettargetvar();
        getnextvalidchar();
    }

    functionparser();
    if (variable != NULL)
        gfx_printf("target: %s", variable);

    free(variable);
}

void tester(char *path){
    int res;

    gfx_clearscreen();

    res = f_open(&scriptin, path, FA_READ | FA_OPEN_EXISTING);
    if (res != FR_OK){
        gfx_errDisplay("ParseScript", res, 1);
        return;
    }
    

    printerrors = false;

    while (!f_eof(&scriptin)){
        mainparser();
    }

    f_close(&scriptin);
    btn_wait();
}