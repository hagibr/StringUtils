#include <stdio.h>
#include "str_utils.h"


int main(int argc, char * argv[])
{
    char buffer[1024];

    while(true)
    {
        size_t line_size = sizeof(buffer);
        char * line = buffer;
        
        getline(&line, &line_size, stdin);
        StringView sv = sv_from_cstr(buffer);
        sv = sv_trim(sv);
        int intVal;
        if( sv_to_int32(sv, &intVal) )
        {
            printf("intVal: %d\r\n",intVal);
        }
        else
        {
            printf("Invalid value: %s\r\n", buffer);
        }
    }
    return 0;
}